#!/usr/local/bin/python3
# author: zhshen
# date: 20200711
import sys

sys.path.append("..")
import numpy as np
import Bound as bd
import math
import trans
import glv


def diff_enu(t1=[], xyz=[], t2=[], xyz_ref=[]):
    t, E, N, U = [], [], [], []
    cnt = 0
    for i in range(len(t1)):
        index = bd.lower_bound(t2, t1[i])
        if abs(t1[i] - t2[index]) > 1e-2:
            continue
        [e, n, u] = trans.xyz2enu(xyz[i], xyz_ref[index])
        t.append(t1[i])
        E.append(float(e))
        N.append(float(n))
        U.append(float(u))
    time = np.array(t).T
    diff = np.array([E, N, U]).T
    return (time, diff)


def diff_vel(t1=[], vel=[], t2=[], vel_ref=[]):
    t, E, N, U = [], [], [], []
    for i in range(len(t1)):
        index = bd.lower_bound(t2, t1[i])
        if abs(t1[i] - t2[index]) > 1e-6:
            continue
        [e, n, u] = vel[i] - vel_ref[index]
        t.append(t1[i])
        E.append(e)
        N.append(n)
        U.append(u)
    time = np.array(t).T
    diff = np.array([E, N, U]).T
    return (time, diff)


def diff_att(t1=[], att=[], t2=[], att_ref=[]):
    t, p, r, y = [], [], [], []
    for i in range(len(t1)):
        index = bd.lower_bound(t2, t1[i])
        if abs(t1[i] - t2[index]) > 1e-6:
            continue
        t.append(t1[i])
        [pp, rr, yy] = att[i] - att_ref[index]
        if abs(yy) > 100:
            if yy < 0:
                yy = yy + 360
            else:
                yy = 360 - yy
        p.append(pp)
        r.append(rr)
        y.append(yy)
    time = np.array(t).T
    diff = np.array([p, r, y]).T
    return (time, diff)


def correct_base(xyz=[], div_ref=[], plus_ref=[]):
    x = xyz[:, 0] - div_ref[0] + plus_ref[0]
    y = xyz[:, 1] - div_ref[1] + plus_ref[1]
    z = xyz[:, 2] - div_ref[2] + plus_ref[2]
    xyz_corrected = np.array([x, y, z]).T
    return xyz_corrected


def available(threshold, diff=[]):
    size = len(diff)
    ava = 0
    for i in range(size):
        x = abs(diff[i, 0])
        y = abs(diff[i, 1])
        z = abs(diff[i, 2])
        if x < threshold and y < threshold and z < 0.1:
            ava = ava + 1
    return ava


def rms3(diff=[]):
    r1 = rms(diff[:, 0])
    r2 = rms(diff[:, 1])
    r3 = rms(diff[:, 2])
    s = "RMS:({0:.3f},{1:.3f},{2:.3f})m With 3D RMSE::{3:.3f}".format(
        r1, r2, r3, np.sqrt(r1 * r1 + r2 * r2 + r3 * r3)
    )
    print(s)


def rms(diff=[]):
    size = len(diff)
    sum = 0
    for i in range(size):
        sum = sum + diff[i] * diff[i]
    return math.sqrt(sum / size)


def mae(diff=[]):
    size = len(diff)
    sum = 0
    for i in range(size):
        sum = sum + abs(diff[i])
    return sum / size


def mae3(diff=[]):
    r1 = mae(diff[:, 0])
    r2 = mae(diff[:, 1])
    r3 = mae(diff[:, 2])
    s = "MAE:({0:.3f},{1:.3f},{2:.3f})m With 3D MAE:{3:.3f}".format(
        r1, r2, r3, np.mean([r1, r2, r3])
    )
    print(s)
