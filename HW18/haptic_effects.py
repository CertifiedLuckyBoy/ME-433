# HW18 - Haptic effect force vs. displacement curves
# All values normalized to +/- 1 (displacement = paddle angle range, force = max motor torque)
import numpy as np
import matplotlib.pyplot as plt

x = np.linspace(-1, 1, 1000)


def bump(x, center=0.0, width=0.3, amp=1.0):
    # A bump is a potential hill: the force pushes the paddle away from the
    # center as you climb in, so F is the negative derivative of a gaussian.
    d = x - center
    return amp * (-d / width) * np.exp(-(d ** 2) / (2 * width ** 2)) * np.e ** 0.5


def dip(x, center=0.0, width=0.3, amp=1.0):
    # A dip is the opposite of a bump: the force pulls the paddle toward the
    # center, like a valley you fall into and have to climb out of.
    return -bump(x, center, width, amp)


def toggle(x, threshold=0.0, k=3.0, amp=1.0):
    # A toggle switch resists motion until you cross the threshold, then
    # snaps over center to the other side (bistable spring).
    f = -k * (x - np.sign(x - threshold))
    return amp * np.clip(f, -1, 1)


def detents(x, n=5, amp=0.6):
    # Periodic detents: spring force toward the nearest of n notch positions,
    # like the clicks of a rotary knob.
    return -amp * np.sin(n * np.pi * x)


fig, axs = plt.subplots(2, 2, figsize=(10, 7))
effects = [
    ("Bump (pushes paddle away from center)", bump(x)),
    ("Dip (pulls paddle toward center)", dip(x)),
    ("Toggle switch (bistable snap at x=0)", toggle(x)),
    ("Detents (5 notches, like a rotary knob)", detents(x)),
]

for ax, (title, f) in zip(axs.flat, effects):
    ax.plot(x, f, linewidth=2)
    ax.axhline(0, color="gray", linewidth=0.5)
    ax.axvline(0, color="gray", linewidth=0.5)
    ax.set_title(title, fontsize=10)
    ax.set_xlabel("normalized displacement")
    ax.set_ylabel("normalized force")
    ax.set_xlim(-1, 1)
    ax.set_ylim(-1.1, 1.1)
    ax.grid(True, alpha=0.3)

fig.suptitle("HW18 - Haptic Effect Force vs. Displacement Curves", fontsize=13)
fig.tight_layout()
fig.savefig("haptic_effects.png", dpi=200)
print("saved haptic_effects.png")
