

M = "attack at dawn"
K = ""
C = 0xe1c5f70a65ac519458e7e53f3609

M_bytes = M.encode('utf-8')
M_int = int.from_bytes(M_bytes, byteorder='big')
K_int = M_int ^ C

print("Key (hex):", hex(K_int))



M2 = "attack at dusk"
C2 = ""

M2_bytes = M2.encode('utf-8')
M2_int = int.from_bytes(M2_bytes, byteorder='big')
C2_int = M2_int ^ K_int
C2 = hex(C2_int)

print("Ciphertext: ", C2)
