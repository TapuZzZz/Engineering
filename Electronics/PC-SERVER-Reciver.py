# =================================================================
#  PC SERVER - Part 1: Receive frames from ESP32-CAM over TCP
# =================================================================

import socket   # Built-in Python library for raw network communication
import struct   # Helps convert between raw bytes and Python numbers

# -----------------------------------------------------------------
#  Configuration - must match the ESP32 code exactly
# -----------------------------------------------------------------
LISTEN_IP   = "0.0.0.0"   # "0.0.0.0" means: accept connections on any network interface of this Mac
LISTEN_PORT = 8000          # Must match PC_PORT in the Arduino code


# -----------------------------------------------------------------
#  Function: receive_exact
#  What it does: TCP doesn't guarantee that one call to recv()
#  returns exactly the number of bytes you asked for - it might
#  return less. This function keeps calling recv() in a loop until
#  it has collected exactly num_bytes, no more, no less.
# -----------------------------------------------------------------
def receive_exact(connection, num_bytes):
    data = b""   # b"" = an empty sequence of bytes (as opposed to "" which is empty text)

    while len(data) < num_bytes:
        chunk = connection.recv(num_bytes - len(data))   # Ask for only what's still missing

        if not chunk:
            # An empty chunk means the other side closed the connection mid-frame
            raise ConnectionError("Connection closed while receiving data.")

        data += chunk   # Append what we got to what we already have

    return data


# -----------------------------------------------------------------
#  Function: receive_one_frame
#  What it does: reads exactly one frame from the connection, using
#  the same [4 bytes size][JPEG bytes] format the ESP32 sends.
#  Returns the JPEG bytes.
# -----------------------------------------------------------------
def receive_one_frame(connection):
    size_bytes = receive_exact(connection, 4)          # Step 1: read exactly 4 raw bytes
    frame_size = struct.unpack("<I", size_bytes)[0]     # Step 2: interpret those 4 bytes as a number

    jpeg_bytes = receive_exact(connection, frame_size)  # Step 3: read exactly that many more bytes

    return jpeg_bytes


# =================================================================
#  MAIN PROGRAM
# =================================================================

def main():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # AF_INET     = we're using regular IPv4 addresses (like 192.168.0.173)
    # SOCK_STREAM = we want TCP (a reliable, ordered byte stream) - not UDP

    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # Without this, restarting the script quickly after stopping it can fail
    # with "port already in use" for about a minute - this avoids that.

    server_socket.bind((LISTEN_IP, LISTEN_PORT))   # Claim this IP+port combination
    server_socket.listen(1)                          # Start accepting connections, 1 waiting at a time

    print(f"Listening on port {LISTEN_PORT}, waiting for ESP32-CAM to connect...")

    connection, address = server_socket.accept()
    # This line BLOCKS (pauses) here until the ESP32-CAM actually connects.
    # connection = a new socket specifically for talking to that one device
    # address    = a tuple like ('192.168.0.176', some_port) - who connected

    print(f"ESP32-CAM connected from {address}")

    frame_count = 0

    while True:
        try:
            jpeg_bytes = receive_one_frame(connection)
            frame_count += 1
            print(f"Received frame #{frame_count}, size in bytes: {len(jpeg_bytes)}")

        except ConnectionError:
            print("ESP32-CAM disconnected.")
            break   # Exit the loop cleanly if the connection drops

    connection.close()
    server_socket.close()


if __name__ == "__main__":
    # This check means: only run main() if this file is executed directly,
    # not if it gets imported into another script later.
    main()