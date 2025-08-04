#!/usr/local/bin/python3
# author: zhshen
# date: 20200711
import numpy as np
import matplotlib.pyplot as plt

plt.rc("font", family="Times New Roman")
import trans
import diff as df

font = {
    "family": "Times New Roman",
    "weight": 500,
    "size": 12,
}


def plot_pos(time=[], diff=[]):
    # RMS
    size = len(time)
    beg = time[0]
    hour = time
    hour = (time - beg) / 60
    diff0 = np.mean(df.rms(diff[:, 0]))
    diff1 = np.mean(df.rms(diff[:, 1]))
    diff2 = np.mean(df.rms(diff[:, 2]))
    rms = np.sqrt(
        df.rms(diff[:, 0]) * df.rms(diff[:, 0])
        + df.rms(diff[:, 1]) * df.rms(diff[:, 1])
        + df.rms(diff[:, 2]) * df.rms(diff[:, 2])
    )
    s = "RMS:({0:.3f},{1:.3f},{2:.3f})m, 3D RMS: {3: .3f}m".format(
        diff0, diff1, diff2, rms
    )
    print(s)

    fig, ax = plt.subplots(1, 1, figsize=(8, 5), dpi=100, facecolor="w")
    ax.grid(linestyle="--", linewidth=0.3, color="blue", axis="y")

    # Plot
    ax.scatter(hour, diff[:, 0], s=10, color="#2B8E70")
    ax.scatter(hour, diff[:, 1], s=10, color="#3D509E")
    ax.scatter(hour, diff[:, 2], s=10, color="#DA6646")

    ax.set_xlabel("Time [min]", font)
    ax.set_ylabel("Error [m]", font)
    ax.set_xlim(-0.01, hour[size - 1] + 0.1)
    ax.set_ylim(-2, 2)
    ax.tick_params(
        axis="both",
        colors="black",
        direction="out",
        labelsize=15,
        width=1,
        length=2,
        pad=5,
    )

    # Legend
    ax.legend(["East", "North", "Up"], loc="upper right")
    ax.set_title(f"Position Error")

    plt.savefig("pos_error.png", bbox_inches="tight", dpi=300)


def plot_vel(time=[], diff=[]):

    # RMS
    size = len(time)
    beg = time[0]
    hour = (time - beg) / 60
    diff0 = np.mean(df.rms(diff[:, 0]))
    diff1 = np.mean(df.rms(diff[:, 1]))
    diff2 = np.mean(df.rms(diff[:, 2]))
    rms = np.sqrt(
        df.rms(diff[:, 0]) * df.rms(diff[:, 0])
        + df.rms(diff[:, 1]) * df.rms(diff[:, 1])
        + df.rms(diff[:, 2]) * df.rms(diff[:, 2])
    )
    s = "RMS:({0:.3f},{1:.3f},{2:.3f})m/s, 3D RMS: {3: .3f}m/s".format(
        diff0, diff1, diff2, rms
    )
    print(s)

    fig1, ax = plt.subplots(1, 1, figsize=(8, 5), dpi=100, facecolor="w")
    ax.grid(linestyle="--", linewidth=0.3, color="blue", axis="y")

    # Plot
    ax.scatter(hour, diff[:, 0], s=10, color="#2B8E70")
    ax.scatter(hour, diff[:, 1], s=10, color="#3D509E")
    ax.scatter(hour, diff[:, 2], s=10, color="#DA6646")

    # Label
    ax.set_xlabel("Time [min]", font)
    ax.set_ylabel("Error [m/s]", font)
    ax.set_xlim(-0.1, hour[size - 1] + 0.1)
    ax.set_ylim(-0.5, 0.5)
    ax.tick_params(
        axis="both",
        colors="black",
        direction="out",
        labelsize=15,
        width=1,
        length=2,
        pad=5,
    )

    # Legend
    ax.legend(["X", "Y", "Z"], loc="upper right")
    ax.set_title(f"Velocity Error")

    plt.savefig("vel_error.png", bbox_inches="tight", dpi=300)


def plot_att(time=[], diff=[]):

    # RMS
    size = len(time)
    beg = time[0]
    hour = (time - beg) / 60
    diff0 = np.mean(df.rms(diff[:, 0]))
    diff1 = np.mean(df.rms(diff[:, 1]))
    diff2 = np.mean(df.rms(diff[:, 2]))
    rms = np.sqrt(
        df.rms(diff[:, 0]) * df.rms(diff[:, 0])
        + df.rms(diff[:, 1]) * df.rms(diff[:, 1])
        + df.rms(diff[:, 2]) * df.rms(diff[:, 2])
    )
    s = "RMS:({0:.3f},{1:.3f},{2:.3f})deg, 3D RMS: {3: .3f}deg".format(
        diff0, diff1, diff2, rms
    )
    print(s)

    fig1, ax = plt.subplots(1, 1, figsize=(8, 5), dpi=100, facecolor="w")
    ax.grid(linestyle="--", linewidth=0.3, color="blue", axis="y")

    # Plot
    ax.scatter(hour, diff[:, 0], s=10, color="#2B8E70")
    ax.scatter(hour, diff[:, 1], s=10, color="#3D509E")
    ax.scatter(hour, diff[:, 2], s=10, color="#DA6646")

    # Label
    ax.set_xlabel("Time [min]", font)
    ax.set_ylabel("Error [deg]", font)
    ax.set_xlim(-0.1, hour[size - 1] + 0.1)
    ax.set_ylim(-10, 10)
    ax.tick_params(
        axis="both",
        colors="black",
        direction="out",
        labelsize=15,
        width=1,
        length=2,
        pad=5,
    )

    # Legend
    legend = ax.legend(["Pitch", "Roll", "Yaw"], loc="upper right")
    ax.set_title(f"Attitude Error")

    plt.savefig("att_error.png", bbox_inches="tight", dpi=300)
