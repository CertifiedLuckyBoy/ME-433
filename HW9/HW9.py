import csv
import os

import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import numpy as np


files = ["sigA", "sigB", "sigC", "sigD"]

maf_points = {
    "sigA": 400,
    "sigB": 200,
    "sigC": 100,
    "sigD": 50,
}

iir_weights = {
    "sigA": (0.990, 0.010),
    "sigB": (0.980, 0.020),
    "sigC": (0.980, 0.020),
    "sigD": (0.900, 0.100),
}

fir_settings = {
    "sigA": (801, 10.0, "hamming"),
    "sigB": (301, 8.0, "hamming"),
    "sigC": (201, 5.0, "hamming"),
    "sigD": (101, 5.0, "hamming"),
}


def read_csv(filename):
    t = []
    data = []

    with open(filename) as f:
        reader = csv.reader(f)
        for row in reader:
            t.append(float(row[0]))
            data.append(float(row[1]))

    return np.array(t), np.array(data)


def sample_rate(t):
    return len(t) / t[-1]


def get_fft(data, fs):
    n = len(data)
    k = np.arange(n)
    T = n / fs
    frq = k / T
    frq = frq[range(int(n / 2))]

    Y = np.fft.fft(data) / n
    Y = Y[range(int(n / 2))]

    return frq, abs(Y)


def moving_average(data, points):
    filtered = []

    for i in range(len(data)):
        total = 0.0
        for j in range(points):
            index = i - j
            if index >= 0:
                total = total + data[index]
        filtered.append(total / points)

    return np.array(filtered)


def iir_filter(data, A, B):
    filtered = []

    for i in range(len(data)):
        if i == 0:
            filtered.append(B * data[i])
        else:
            filtered.append(A * filtered[i - 1] + B * data[i])

    return np.array(filtered)


def lowpass_weights(num_weights, cutoff, fs, window_name):
    weights = []
    middle = (num_weights - 1) / 2.0

    for i in range(num_weights):
        x = i - middle

        if x == 0:
            value = 2.0 * cutoff / fs
        else:
            value = np.sin(2.0 * np.pi * cutoff * x / fs) / (np.pi * x)

        if window_name == "hamming":
            window = 0.54 - 0.46 * np.cos(2.0 * np.pi * i / (num_weights - 1))
        else:
            window = 1.0

        weights.append(value * window)

    total = sum(weights)
    for i in range(len(weights)):
        weights[i] = weights[i] / total

    return weights


def fir_filter(data, weights):
    filtered = []

    for i in range(len(data)):
        total = 0.0
        for j in range(len(weights)):
            index = i - j
            if index >= 0:
                total = total + weights[j] * data[index]
        filtered.append(total)

    return np.array(filtered)


def plot_time_fft(name, t, data, fs):
    frq, Y = get_fft(data, fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 7))

    ax1.plot(t, data, "k")
    ax1.set_xlabel("Time [s]")
    ax1.set_ylabel("Signal")
    ax1.set_title(name + " signal vs time")

    ax2.loglog(frq[1:], Y[1:], "k")
    ax2.set_xlabel("Frequency [Hz]")
    ax2.set_ylabel("|Y(freq)|")
    ax2.set_title(name + " FFT")
    ax2.grid(True)

    fig.tight_layout()
    fig.savefig(name + "_fft.png", dpi=160)
    plt.close(fig)


def plot_filtered(name, t, data, filtered, fs, title, save_name):
    frq, Y = get_fft(data, fs)
    frq_filt, Y_filt = get_fft(filtered, fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 7))

    ax1.plot(t, data, "k", label="unfiltered")
    ax1.plot(t, filtered, "r", label="filtered")
    ax1.set_xlabel("Time [s]")
    ax1.set_ylabel("Signal")
    ax1.set_title(title)
    ax1.legend()

    ax2.loglog(frq[1:], Y[1:], "k", label="unfiltered")
    ax2.loglog(frq_filt[1:], Y_filt[1:], "r", label="filtered")
    ax2.set_xlabel("Frequency [Hz]")
    ax2.set_ylabel("|Y(freq)|")
    ax2.set_title(name + " FFT before and after filtering")
    ax2.grid(True)
    ax2.legend()

    fig.tight_layout()
    fig.savefig(save_name, dpi=160)
    plt.close(fig)


def main():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    for name in files:
        t, data = read_csv(name + ".csv")
        fs = sample_rate(t)

        print(name + " sample rate = " + str(round(fs, 2)) + " Hz")

        plot_time_fft(name, t, data, fs)

        points = maf_points[name]
        maf_data = moving_average(data, points)
        title = name + " MAF, " + str(points) + " points averaged"
        plot_filtered(name, t, data, maf_data, fs, title, name + "_maf.png")

        A, B = iir_weights[name]
        iir_data = iir_filter(data, A, B)
        title = name + " IIR, A = " + str(A) + ", B = " + str(B)
        plot_filtered(name, t, data, iir_data, fs, title, name + "_iir.png")

        num_weights, cutoff, window_name = fir_settings[name]
        weights = lowpass_weights(num_weights, cutoff, fs, window_name)
        fir_data = fir_filter(data, weights)
        transition = 4.0 * fs / num_weights
        title = (
            name
            + " FIR low-pass sinc, "
            + str(num_weights)
            + " weights, cutoff = "
            + str(cutoff)
            + " Hz, transition ~= "
            + str(round(transition, 1))
            + " Hz, "
            + window_name
            + " window"
        )
        plot_filtered(name, t, data, fir_data, fs, title, name + "_fir.png")


main()
