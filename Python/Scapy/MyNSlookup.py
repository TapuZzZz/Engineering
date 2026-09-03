from scapy.all import IP, UDP, DNS, DNSQR, DNSRR, sr1
p = IP()/UDP()/DNS()

p[IP].dst = "8.8.8.8"
p[UDP].sport = 5555
p[UDP].dport = 53

p[DNS].qd = DNSQR(qname="www.one.co.il.")

res = sr1(p, verbose=0)

if res and res.haslayer(DNSRR):
    for i in range(res[DNS].ancount):
        answer = res[DNS].an[i]
        
        if answer.type == 1:
            print(answer.rdata)