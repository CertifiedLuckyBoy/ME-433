# HW18 - System block diagram, from computer graphics to microcontrollers,
# sensors, and h-bridge. Pins/voltages from the HW16 wiring.
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

fig, ax = plt.subplots(figsize=(13, 8))
ax.set_xlim(0, 13)
ax.set_ylim(0, 8)
ax.axis("off")


def box(x, y, w, h, title, lines=(), fc="#e8f0fe"):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.08",
                                facecolor=fc, edgecolor="black", linewidth=1.2))
    ax.text(x + w / 2, y + h - 0.32, title, ha="center", va="center",
            fontsize=10, fontweight="bold")
    for i, line in enumerate(lines):
        ax.text(x + w / 2, y + h - 0.72 - i * 0.34, line,
                ha="center", va="center", fontsize=8)


def arrow(x1, y1, x2, y2, label="", style="<|-|>", dy=0.18):
    ax.add_patch(FancyArrowPatch((x1, y1), (x2, y2), arrowstyle=style,
                                 mutation_scale=14, linewidth=1.2, color="black"))
    if label:
        ax.text((x1 + x2) / 2, (y1 + y2) / 2 + dy, label,
                ha="center", va="bottom", fontsize=8, color="#1a4d8f")


# --- computer ---
box(0.4, 5.6, 3.0, 2.0, "Computer",
    ["Python graphics (pygame)", "shows paddle position,", "force, and haptic effect"],
    fc="#fde8e8")

# --- Pico: high-level / sensing ---
box(4.6, 5.0, 3.6, 2.6, "Pico 2 (3.3V)",
    ["PD controller @ 80 Hz",
     "pos + force -> desired current",
     "AS5600 via I2C (0x36)",
     "HX711 via GPIO bit-bang"])

# --- STM32: current control ---
box(9.2, 5.0, 3.4, 2.6, "STM32 Nucleo (3.3V)",
    ["PI current control @ 1 kHz",
     "TIM1 PWM 20 kHz (PA8, PA1)",
     "INA219 via I2C1 (PB8/PB9)",
     "pot on ADC1_IN0 (PA0) = e-stop"])

# --- sensors on the Pico side ---
box(4.6, 2.4, 1.7, 1.7, "AS5600", ["magnetic", "encoder", "paddle angle"], fc="#e8f8e8")
box(6.6, 2.4, 1.7, 1.7, "HX711 +", ["load cell", "80 Hz", "finger force"], fc="#e8f8e8")

# --- power / drive on the STM32 side ---
box(9.0, 2.4, 1.8, 1.7, "DRV8833", ["h-bridge", "IN1/IN2 PWM", "SLEEP=3.3V"], fc="#fff4e0")
box(11.2, 2.4, 1.6, 1.7, "RC Servo", ["motor (geared)", "+ pot", "paddle output"], fc="#fff4e0")
box(9.0, 0.3, 1.8, 1.4, "4x AA battery", ["6V, to DRV8833", "VCC only"], fc="#fff4e0")
box(11.2, 0.3, 1.6, 1.4, "INA219", ["current sensor", "in series w/ motor"], fc="#e8f8e8")

# --- connections ---
arrow(3.4, 6.6, 4.6, 6.6, "USB serial\npos, force, effect state")
arrow(8.2, 6.3, 9.2, 6.3, "CAN/UART\ndesired current ->")
arrow(5.45, 5.0, 5.45, 4.1, "I2C", style="<|-")
arrow(7.45, 5.0, 7.45, 4.1, "SCK out / DT in", style="<|-")
arrow(9.9, 5.0, 9.9, 4.1, "PWM x2", style="-|>")
arrow(10.8, 3.25, 11.2, 3.25, "OUT1/OUT4", style="-|>")
arrow(9.9, 2.4, 9.9, 1.7, "6V", style="<|-")
arrow(12.0, 2.4, 12.0, 1.7, "motor current", style="-|>")
arrow(11.0, 1.0, 11.2, 1.0, "", style="-|>")
ax.text(12.55, 4.55, "shaft + magnet\nback to AS5600", fontsize=7,
        ha="center", color="#1a4d8f")

fig.suptitle("HW18 - Haptic Paddle System Block Diagram", fontsize=13)
fig.savefig("block_diagram.png", dpi=200, bbox_inches="tight")
print("saved block_diagram.png")
