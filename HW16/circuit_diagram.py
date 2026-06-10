# HW16 - circuit diagram: STM32 Nucleo-C092RC, RC servo pot, DRV8833,
# INA219, motor, and 4xAA battery. Same drawing style as HW18/block_diagram.py.
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


def wire(x1, y1, x2, y2, label="", style="-", dy=0.16):
    ax.add_patch(FancyArrowPatch((x1, y1), (x2, y2), arrowstyle=style,
                                 mutation_scale=14, linewidth=1.2, color="black"))
    if label:
        ax.text((x1 + x2) / 2, (y1 + y2) / 2 + dy, label,
                ha="center", va="bottom", fontsize=8, color="#1a4d8f")


# --- STM32 Nucleo ---
box(0.4, 2.6, 3.4, 4.6, "STM32 Nucleo-C092RC",
    ["3.3V / GND",
     "PA0  = ADC1_IN0 (pot)",
     "PA8  = TIM1_CH1 (PWM 20 kHz)",
     "PA1  = TIM1_CH2 (PWM 20 kHz)",
     "PB9  = I2C1_SDA",
     "PB8  = I2C1_SCL",
     "USB VCP -> computer"])

# --- pot (inside the RC servo) ---
box(5.2, 5.9, 3.2, 1.6, "RC servo potentiometer",
    ["3.3V / GND", "wiper -> PA0 (e-stop)"], fc="#e8f8e8")

# --- DRV8833 ---
box(5.2, 2.6, 3.2, 2.6, "DRV8833 h-bridge",
    ["IN1 = PA8 (tied to IN3)",
     "IN2 = PA1 (tied to IN4)",
     "OUT1 || OUT3,  OUT2 || OUT4",
     "SLEEP = 3.3V,  VCC = 6V"], fc="#fff4e0")

# --- battery ---
box(5.2, 0.4, 3.2, 1.4, "4x AA battery pack",
    ["6V -> DRV8833 VCC", "(via breadboard rail)", "GND common"], fc="#fff4e0")

# --- motor ---
box(9.6, 4.6, 3.0, 1.8, "RC servo motor",
    ["lead A <- OUT1/OUT3", "lead B -> INA219 VIN+"], fc="#fff4e0")

# --- INA219 ---
box(9.6, 1.6, 3.0, 2.2, "INA219 current sensor",
    ["VIN+ <- motor lead B", "VIN- -> OUT2/OUT4",
     "SDA/SCL -> PB9/PB8", "VCC = 3.3V"], fc="#e8f8e8")

# --- wires ---
wire(3.8, 6.7, 5.2, 6.7, "3.3V / GND / PA0")
wire(3.8, 4.2, 5.2, 4.2, "PA8, PA1 (PWM)", style="-|>")
wire(3.8, 3.0, 9.6, 3.0, "I2C1: PB9 SDA, PB8 SCL", dy=-0.42)
wire(6.8, 2.6, 6.8, 1.8, "6V", style="<|-")
wire(8.4, 4.6, 9.6, 5.3, "OUT1/OUT3 -> motor", style="-|>")
wire(11.1, 4.6, 11.1, 3.8, "motor current", style="-|>")
wire(9.6, 2.3, 8.4, 2.9, "VIN- -> OUT2/OUT4", style="-|>", dy=-0.46)

ax.text(6.5, 7.8, "7 connections to the STM32: 3.3V, GND, 1 analog, 2 PWM, 2 I2C",
        ha="center", fontsize=9, color="#1a4d8f")

fig.suptitle("HW16 - Current Control Circuit", fontsize=13)
fig.savefig("circuit_diagram.png", dpi=200, bbox_inches="tight")
print("saved circuit_diagram.png")
