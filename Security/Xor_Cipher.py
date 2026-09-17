M = "attack at dawn"
K = ""
C = "e1c5f70a65ac519458e7e53f3609"

for i in range(len(M)):
    K += chr(ord(M[i]) ^ int(C[i*2:i*2+2], 16))

print("Key: ", K)

M2 = "attack at dusk"
C2 = ""

for i in range(len(M2)):
    C2 += format(ord(M2[i]) ^ ord(K[i]), '02x')

print("Ciphertext: ", C2)
