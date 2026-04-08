import struct
import time
import h5py
import serial

PORT = "COM4"
BAUD = 9600
H5_FILE = "synthetic_frames.h5"

N = 256
NBINS = 128
LOOP_FOREVER = False

def read_exactly(ser, n, timeout_s=3.0):
    data = b""
    t0 = time.time()
    while len(data) < n and (time.time() - t0) < timeout_s:
        chunk = ser.read(n - len(data))
        if chunk:
            data += chunk
    return data

def build_payload(bins, cav_on, dir_, energy):
    payload = bytearray(N)

    # bins[0:128]
    payload[0:NBINS] = bytes(bins)

    # status bytes
    payload[128] = int(cav_on) & 0xFF
    payload[129] = int(dir_) & 0xFF
    payload[130:134] = struct.pack("<I", int(energy))

    return payload

def main():
    with h5py.File(H5_FILE, "r") as f:
        bins_ds = f["bins"]
        cav_ds = f["cav_on"]
        dir_ds = f["direction"]
        energy_ds = f["energy"]
        num_frames = bins_ds.shape[0]

        print(f"Loaded {num_frames} frames from {H5_FILE}")

        ser = serial.Serial(PORT, BAUD, timeout=0.2)
        time.sleep(0.2)

        ser.reset_input_buffer()
        ser.reset_output_buffer()

        frame = 0
        print("[PY] Streaming packets from HDF5")

        try:
            while True:
                if frame >= num_frames:
                    if LOOP_FOREVER:
                        frame = 0
                    else:
                        break

                # wait for ready byte 'R'
                while True:
                    b = ser.read(1)
                    if b == b'\x52':
                        break

                bins = bins_ds[frame]
                cav_on = int(cav_ds[frame])
                dir_ = int(dir_ds[frame])
                energy = int(energy_ds[frame])

                payload = build_payload(bins, cav_on, dir_, energy)

                # send payload
                ser.write(payload)
                ser.flush()

                # read echoed payload + checksum
                echo = read_exactly(ser, N, timeout_s=3.0)
                ck = read_exactly(ser, 2, timeout_s=1.0)

                echo_ok = (echo == bytes(payload))
                if len(ck) == 2:
                    checksum_rx = ck[0] | (ck[1] << 8)
                    checksum_py = sum(payload) & 0xFFFF
                    sum_ok = (checksum_rx == checksum_py)

                    print(
                        f"frame={frame:6d} dir={dir_} cav={cav_on} "
                        f"energy={energy:6d} echo_ok={echo_ok} sum_ok={sum_ok}"
                    )
                else:
                    print(f"frame={frame:6d} (missing checksum)")

                frame += 1

        finally:
            ser.close()

if __name__ == "__main__":
    main()
