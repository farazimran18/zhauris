import math
import h5py
import numpy as np

OUTFILE = "synthetic_frames.h5"
NUM_FRAMES = 100
NBINS = 128
N = 256
ENERGY_THR = 14000

def make_bins(frame):
    # moving peaks + noise
    bins = [0] * NBINS
    p1 = (frame * 2) % NBINS
    p2 = (NBINS - 1) - ((frame * 3) % NBINS)

    for i in range(NBINS):
        d1 = abs(i - p1)
        d2 = abs(i - p2)
        peak1 = max(0, 160 - 10 * d1)
        peak2 = max(0, 120 - 12 * d2)
        noise = int(20 * (0.5 + 0.5 * math.sin(0.15 * frame + 0.3 * i)))
        v = peak1 + peak2 + noise
        bins[i] = max(0, min(255, v))

    return bins

# def make_bins(frame):
#     level = 80
#     return [level] * NBINS

def broadband_energy(bins):
    return sum(bins[40:96])

def main():
    bins_arr = np.zeros((NUM_FRAMES, NBINS), dtype=np.uint8)
    cav_arr = np.zeros(NUM_FRAMES, dtype=np.uint8)
    dir_arr = np.zeros(NUM_FRAMES, dtype=np.uint8)
    energy_arr = np.zeros(NUM_FRAMES, dtype=np.uint32)

    for frame in range(NUM_FRAMES):
        bins = make_bins(frame)
        energy = broadband_energy(bins)
        cav_on = 1 # if energy > ENERGY_THR else 0
        dir_ = (frame // 20) % 3

        bins_arr[frame, :] = np.array(bins, dtype=np.uint8)
        cav_arr[frame] = cav_on
        dir_arr[frame] = dir_
        energy_arr[frame] = energy

    with h5py.File(OUTFILE, "w") as f:
        f.create_dataset("bins", data=bins_arr, compression="gzip")
        f.create_dataset("cav_on", data=cav_arr, compression="gzip")
        f.create_dataset("direction", data=dir_arr, compression="gzip")
        f.create_dataset("energy", data=energy_arr, compression="gzip")

        # metadata
        f.attrs["num_frames"] = NUM_FRAMES
        f.attrs["nbins"] = NBINS
        f.attrs["payload_bytes"] = N
        f.attrs["energy_threshold"] = ENERGY_THR
        f.attrs["energy_bin_start"] = 40
        f.attrs["energy_bin_end"] = 96

    print(f"wrote {NUM_FRAMES} frames to {OUTFILE}")

if __name__ == "__main__":
    main()
