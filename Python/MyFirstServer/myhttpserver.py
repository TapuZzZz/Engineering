import socket
import uuid

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_socket.bind(('0.0.0.0', 8080))
server_socket.listen(5)
print("Complete Chat & Secure Auth Server is listening on port 8080...")

# 1. בסיס נתונים מורחב הכולל תפקידים (Roles)
USER_DB = {
    "admin": {"password": "1234", "role": "administrator"},
    "user2": {"password": "5678", "role": "regular_user"}
}

# 2. ניהול סשנים בזיכרון { session_id: {"username": x, "role": y} }
ACTIVE_SESSIONS = {}

# 3. רשימת הודעות גלובלית לצ'אט
CHAT_MESSAGES = [
    {"from": "System", "text": "Welcome to the global chat room!"}
]

# פונקציית עזר לחילוץ ה-Session ID מתוך הבקשה
def get_session_id(request_text):
    if "Cookie:" in request_text:
        for line in request_text.split("\r\n"):
            if line.startswith("Cookie:") and "session_id=" in line:
                return line.split("session_id=")[1].split(";")[0].strip()
    return None

while True:
    client_socket, addr = server_socket.accept()
    try:
        request = client_socket.recv(2048).decode('utf-8')
        if not request:
            continue
            
        first_line = request.split("\r\n")[0]
        parts = first_line.split(" ")
        method = parts[0]
        path = parts[1]
        
        cookie_value = get_session_id(request)
        is_logged_in = cookie_value in ACTIVE_SESSIONS
        
        # שליפת נתוני המשתמש הנוכחי אם הוא מחובר
        current_user = ACTIVE_SESSIONS[cookie_value]["username"] if is_logged_in else None
        current_role = ACTIVE_SESSIONS[cookie_value]["role"] if is_logged_in else None

        # --- א. דף הבית והצ'אט (GET /) ---
        if method == "GET" and path == "/":
            if is_logged_in:
                with open("secure_home.html", "r", encoding="utf-8") as file:
                    response_body = file.read()
                
                response_body = response_body.replace("USERNAME", current_user)
                
                # בניית היסטוריית הצ'אט
                history_html = ""
                for msg in CHAT_MESSAGES:
                    history_html += f'<div class="msg"><span class="user-name">{msg["from"]}:</span> {msg["text"]}</div>'
                
                # אם המשתמש הוא אדמין, נוסיף לו קישור סודי לדף הניהול בראש המסך!
                if current_role == "administrator":
                    admin_link = '<div style="background:#feebc8; padding:10px; margin-bottom:10px;">⭐ <b>Admin Mode:</b> <a href="/admin">Go to Admin Panel</a></div>'
                    response_body = response_body.replace("</h1>", "</h1>" + admin_link)

                response_body = response_body.replace("CHAT_HISTORY", history_html)
                status = "HTTP/1.1 200 OK"
            else:
                with open("login.html", "r", encoding="utf-8") as file:
                    response_body = file.read()
                status = "HTTP/1.1 200 OK"
            
            response = f"{status}\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: {len(response_body.encode('utf-8'))}\r\n\r\n{response_body}"
            client_socket.sendall(response.encode('utf-8'))

        # --- ב. תהליך התחברות (POST /login) ---
        elif method == "POST" and path == "/login":
            request_parts = request.split("\r\n\r\n")
            body = request_parts[1] if len(request_parts) > 1 else ""
            
            credentials = body.split("&")
            username, password = "", ""
            for cred in credentials:
                if "username=" in cred: username = cred.split("=")[1]
                if "password=" in cred: password = cred.split("=")[1]
            
            # בדיקה מול מבנה הנתונים החדש שלנו
            if username in USER_DB and USER_DB[username]["password"] == password:
                new_session_id = str(uuid.uuid4())
                
                # שומרים את השם וגם את התפקיד בתוך הסשן!
                ACTIVE_SESSIONS[new_session_id] = {
                    "username": username,
                    "role": USER_DB[username]["role"]
                }
                print(f"[{addr}] Login success for '{username}' ({USER_DB[username]['role']}).")
                
                response = (
                    "HTTP/1.1 303 See Other\r\n"
                    "Location: /\r\n"
                    f"Set-Cookie: session_id={new_session_id}; Path=/; HttpOnly\r\n"
                    "\r\n"
                )
            else:
                fail_body = "<h1>Login Failed!</h1><a href='/'>Try Again</a>"
                response = f"HTTP/1.1 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n{fail_body}"
            
            client_socket.sendall(response.encode('utf-8'))

        # --- ג. שליחת הודעה בצ'אט (POST /send_message) ---
        elif method == "POST" and path == "/send_message":
            if not is_logged_in:
                response = "HTTP/1.1 303 See Other\r\nLocation: /\r\n\r\n"
                client_socket.sendall(response.encode('utf-8'))
                continue
                
            request_parts = request.split("\r\n\r\n")
            body = request_parts[1] if len(request_parts) > 1 else ""
            
            message_text = ""
            if "chat_msg=" in body:
                message_text = body.split("chat_msg=")[1]
                message_text = message_text.replace("+", " ") 
                
            if message_text:
                CHAT_MESSAGES.append({"from": current_user, "text": message_text})
                
            response = "HTTP/1.1 303 See Other\r\nLocation: /\r\n\r\n"
            client_socket.sendall(response.encode('utf-8'))

        # --- ד. דף הניהול המאובטח והמוגבל (GET /admin) ---
        elif method == "GET" and path == "/admin":
            if is_logged_in:
                # בדיקת הרשאות קשוחה בצד השרת!
                if current_role == "administrator":
                    print(f"[{addr}] Admin '{current_user}' successfully accessed /admin panel.")
                    admin_body = f"""
                    <!DOCTYPE html>
                    <html>
                    <head><meta charset='utf-8'><title>Admin Panel</title></head>
                    <body>
                        <h1>👑 Super Secret Admin Control Panel 👑</h1>
                        <p>Hello {current_user}, you have full authorization to view this page.</p>
                        <h3>Current System Stats:</h3>
                        <ul>
                            <li>Total Active Sessions: {len(ACTIVE_SESSIONS)}</li>
                            <li>Total Messages Sent: {len(CHAT_MESSAGES)}</li>
                        </ul>
                        <a href="/">Back to Chat Room</a>
                    </body>
                    </html>
                    """
                    status = "HTTP/1.1 200 OK"
                    response = f"{status}\r\nContent-Type: text/html; charset=utf-8\r\n\r\n{admin_body}"
                else:
                    # המשתמש מחובר, אבל הוא regular_user! חוסמים אותו!
                    print(f"[{addr}] 🚨 SECURITY WARNING: User '{current_user}' tried to illegal access /admin!")
                    forbidden_body = "<h1>403 Forbidden</h1><p>Access Denied. You do not have administrator privileges to view this area.</p><a href='/'>Back to Chat Room</a>"
                    status = "HTTP/1.1 403 Forbidden"
                    response = f"{status}\r\nContent-Type: text/html; charset=utf-8\r\n\r\n{forbidden_body}"
            else:
                # משתמש אנונימי לחלוטין מנסה להיכנס - נעיף אותו ללוגין
                response = "HTTP/1.1 303 See Other\r\nLocation: /\r\n\r\n"
            
            client_socket.sendall(response.encode('utf-8'))

        # --- ה. התנתקות (GET /logout) ---
        elif method == "GET" and path == "/logout":
            if cookie_value in ACTIVE_SESSIONS:
                ACTIVE_SESSIONS.pop(cookie_value)
            response = "HTTP/1.1 303 See Other\r\nLocation: /\r\nSet-Cookie: session_id=; Path=/; Max-Age=0\r\n\r\n"
            client_socket.sendall(response.encode('utf-8'))
            
        else:
            not_found = "<h1>404 Not Found</h1>"
            response = f"HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n{not_found}"
            client_socket.sendall(response.encode('utf-8'))
            
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client_socket.close()