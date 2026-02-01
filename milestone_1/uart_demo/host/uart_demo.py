import serial, time

PORT = "COM4"
BAUD = 9600
N = 256

def read_exactly(ser, n, timeout_s=3.0):
    data = b""
    t0 = time.time()
    while len(data) < n and (time.time() - t0) < timeout_s:
        chunk = ser.read(n - len(data))
        if chunk:
            data += chunk
    return data

ser = serial.Serial(PORT, BAUD, timeout=0.2)
time.sleep(0.2)

ser.reset_input_buffer()
ser.reset_output_buffer()

# wait for ready byte 'R' (0x52)
print("[PY] waiting for ready byte")
while True:
    b = ser.read(1)
    if b == b'\x52':
        break
print("[PY] got ready")

# send payload
payload = bytes([i & 0xFF for i in range(N)])
ser.write(payload)
ser.flush()
print("[PY] sent payload")

# read echoed payload
echo = read_exactly(ser, N, timeout_s=3.0)
print("echo_len =", len(echo))
print("match    =", echo == payload)

# read checksum (2 bytes, little-endian)
ck = read_exactly(ser, 2, timeout_s=1.0)
if len(ck) == 2:
    checksum_rx = ck[0] | (ck[1] << 8)
    checksum_py = sum(payload) & 0xFFFF
    print("checksum_rx =", checksum_rx, "checksum_py =", checksum_py, "ok =", checksum_rx == checksum_py)
else:
    print("checksum_rx = <missing>")

ser.close()
