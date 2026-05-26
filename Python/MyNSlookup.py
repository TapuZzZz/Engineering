from scapy.all import IP, UDP, DNS, DNSQR, DNSRR, sr1

# 1. יצירת החבילה עם שכבת ה-DNS והגדרת השאילתה (rd=1 אומר שזו שאילתה רקורסיבית)
p = IP()/UDP()/DNS(rd=1)

# 2. הגדרת הפרמטרים
p[IP].dst = "8.8.8.8"
p[UDP].sport = 5555
p[UDP].dport = 53

# תיקון 1: כדי להגדיר את שם האתר, צריך ליצור אובייקט מסוג DNSQR בתוך ה-DNS
p[DNS].qd = DNSQR(qname="www.one.co.il.")

# 3. שליחת החבילה והמתנה לתשובה (sr1 מחזיר חבילה אחת)
res = sr1(p)

# 4. הדפסת התשובות
if res and res.haslayer(DNSRR):
    # תיקון 2: מכיוון שיכולות לחזור כמה תשובות (Records), נרוץ עליהן בלולאה
    print("\n--- IP Addresses Received ---")
    
    # סריקה של כל אובייקטי ה-DNSRR (תשובות) שחזרו בחבילה
    for i in range(res[DNS].ancount):
        print(res[DNSRR][i].rdata)
        
    print("\n--- Full Packet Show ---")
    res.show()
else:
    print("No response received or no DNS Answer found.")