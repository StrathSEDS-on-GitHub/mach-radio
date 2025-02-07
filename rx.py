import sys
from hashlib import sha512, sha3_512
from time import time

# Reads a stream of data received from a radio with the following format:
# [> Packet #nnn: {sha512}{trunc sha3_512} <]
# Where the hashes are of the 8 byte little-endian packet number.
# where the sha3_512 hash is truncated such that the total length of the packet is 255 characters.
# We read the stream of data, counting packets and showing the total throughput and packet loss,
# averaged over the last 10s.

last_pkt = None
buffer = ""
pkts_received = 0
pkts_dropped = 0
start_time = time()

while True:
    buffer += sys.stdin.read(256)
    print(buffer[:64], end='\r')

    while True:
        start = buffer.find("[> Packet #")
        if start == -1:
            break

        maybepkt = buffer[start:start + 255]
        if len(maybepkt) < 255:
            break

        buffer = buffer[start + 255:]


        # Check packet structure
        if not maybepkt.startswith("[> Packet #") or not maybepkt.endswith(" <]"):
            continue

        # Check packet number
        try:
            pkt_num = int(maybepkt[11:21])
        except ValueError:
            continue

        hash = sha512(pkt_num.to_bytes(8, 'little')).hexdigest()
        hash += sha3_512(hash.encode()).hexdigest()
        hash = hash[:255-len("[> Packet #0000000000:  <]")]

        if hash != maybepkt[23:-3]:
            continue

        # Calculate dropped packets since last packet
        if last_pkt is not None:
            pkts_dropped += pkt_num - last_pkt - 1

        pkts_received += 1
        last_pkt = pkt_num

    # Calculate throughput and packet loss
    elapsed = time() - start_time
    if elapsed > 5:
        data_kbit = pkts_received * 255 * 8 / 1000
        print(f"Throughput: {data_kbit/elapsed:.2f} kbit/s, Packet loss: {pkts_dropped/(pkts_received+pkts_dropped)*100:.2f}%")
        pkts_received = 0
        pkts_dropped = 0
        start_time = time()



