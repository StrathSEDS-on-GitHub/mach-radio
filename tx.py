from hashlib import sha256, sha3_512, sha512
counter = 0

while True:
    hash = sha512(counter.to_bytes(8, 'little')).hexdigest()
    hash += sha3_512(hash.encode()).hexdigest()
    hash = hash[:255-len("[> Packet #0000000000:  <]")]
    packet = f"[> Packet #{counter:10}: {hash} <]"

    print(packet, end='')
    counter += 1
