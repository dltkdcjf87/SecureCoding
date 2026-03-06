import hashlib

def hash_password(password):
    # Weak Cryptographic Hash: MD5 is broken and insecure for passwords
    return hashlib.md5(password.encode()).hexdigest()

print(hash_password("password123"))
