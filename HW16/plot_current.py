# HW16 - send 'a' to the Nucleo, collect the 400-sample current step
# response, and plot desired vs. actual current to tune Kp/Ki.
#
#   python plot_current.py /dev/tty.usbmodemXXXX --kp 0.5 --ki 0.05
import argparse

import matplotlib.pyplot as plt
import serial

NUMSAMPS = 400

parser = argparse.ArgumentParser(description="HW16 current control tuning plot")
parser.add_argument("port", help="serial port of the Nucleo, e.g. /dev/tty.usbmodem11203")
parser.add_argument("--kp", type=float, default=None, help="Kp used in main.c (for the title)")
parser.add_argument("--ki", type=float, default=None, help="Ki used in main.c (for the title)")
args = parser.parse_args()

ser = serial.Serial(args.port, 115200, timeout=5)
ser.reset_input_buffer()
ser.write(b"a")

index, desired, actual = [], [], []
while len(index) < NUMSAMPS:
    line = ser.readline().decode(errors="ignore").strip()
    if not line:
        raise SystemExit(f"timed out after {len(index)} samples - is the board running?")
    parts = line.split()
    if len(parts) != 3:
        continue  # skip the banner / partial lines
    try:
        i, d, a = (int(p) for p in parts)
    except ValueError:
        continue
    index.append(i)
    desired.append(d / 3.0)  # raw 1/3-mA units -> mA
    actual.append(a / 3.0)
ser.close()

err = sum(abs(d - a) for d, a in zip(desired, actual)) / NUMSAMPS

plt.plot(index, desired, "r-", label="desired")
plt.plot(index, actual, "b-", label="actual")
plt.xlabel("sample (1 kHz)")
plt.ylabel("current (mA)")
gains = ""
if args.kp is not None or args.ki is not None:
    gains = f"  Kp={args.kp}  Ki={args.ki}"
plt.title(f"HW16 PI current control{gains}  (mean |err| = {err:.1f} mA)")
plt.legend()
plt.grid(True)
plt.show()
