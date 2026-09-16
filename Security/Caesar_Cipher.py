# Regular Caesar cipher (shift each letter, mod 26)

def caesar_encrypt_char(c, shift):
    if c == ' ':
        return ' '
    num = ord(c) - ord('a')
    shifted = (num + shift) % 26
    return chr(shifted + ord('a'))


def caesar_decrypt_char(c, shift):
    return caesar_encrypt_char(c, -shift)


def caesar_encrypt_text(text, shift):
    result = ""
    for c in text:
        result += caesar_encrypt_char(c, shift)
    return result


def caesar_decrypt_text(text, shift):
    result = ""
    for c in text:
        result += caesar_decrypt_char(c, shift)
    return result


# The assignment: block cipher using base 26

BASE = 26
BLOCK_SIZE = 4
MODULO = BASE ** BLOCK_SIZE  # 456976


def char_to_num(c):
    return ord(c) - ord('a')


def num_to_char(n):
    return chr(n + ord('a'))


def pad_block(block):
    missing = BLOCK_SIZE - len(block)
    return 'a' * missing + block


def split_word_to_blocks(word):
    blocks = []
    i = 0
    while i < len(word):
        chunk = word[i:i + BLOCK_SIZE]
        if len(chunk) < BLOCK_SIZE:
            chunk = pad_block(chunk)
        blocks.append(chunk)
        i += BLOCK_SIZE
    return blocks


def block_to_number(block):
    number = 0
    for c in block:
        number = number * BASE + char_to_num(c)
    return number


def number_to_block(number):
    digits = []
    for _ in range(BLOCK_SIZE):
        digits.append(number % BASE)
        number = number // BASE
    digits.reverse()
    block = ""
    for d in digits:
        block += num_to_char(d)
    return block


def encrypt_block(block, shift):
    number = block_to_number(block)
    number = (number + shift) % MODULO
    return number_to_block(number)


def decrypt_block(block, shift):
    number = block_to_number(block)
    number = (number - shift) % MODULO
    return number_to_block(number)


def encrypt_word(word, shift):
    blocks = split_word_to_blocks(word)
    cipher = ""
    for block in blocks:
        cipher += encrypt_block(block, shift)
    return cipher


def decrypt_word(cipher_word, shift):
    blocks = split_word_to_blocks(cipher_word)
    plain = ""
    for block in blocks:
        plain += decrypt_block(block, shift)
    return plain


def encrypt_text(text, shift):
    words = text.split(' ')
    cipher_words = []
    for word in words:
        cipher_words.append(encrypt_word(word, shift))
    return ' '.join(cipher_words)


def decrypt_text(cipher_text, shift):
    words = cipher_text.split(' ')
    plain_words = []
    for word in words:
        plain_words.append(decrypt_word(word, shift))
    return ' '.join(plain_words)


def read_text_file(filename):
    with open(filename, 'r') as f:
        return f.read().strip()


def write_text_file(filename, content):
    with open(filename, 'w') as f:
        f.write(content)


def build_encrypted_filename(filename):
    name, extension = filename.rsplit('.', 1)
    return name + "_enc." + extension


def encrypt_file(input_filename, shift):
    plain_text = read_text_file(input_filename)
    cipher_text = encrypt_text(plain_text, shift)
    output_filename = build_encrypted_filename(input_filename)
    write_text_file(output_filename, cipher_text)


if __name__ == "__main__":
    print(encrypt_block("aaaa", 12345))
    print(encrypt_block("zzza", 12345))

    shift = 12345
    encrypt_file("simple.txt", shift)

    cipher = read_text_file("simple_enc.txt")
    print("cipher:", cipher)
    print("decrypted back:", decrypt_text(cipher, shift))