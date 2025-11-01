# author: zhshen
# date: 20200711
import numpy as np
import openfile as of
import diff
import draw as dw
import matplotlib.pyplot as plt
import glv

# ins_file=r'D:\GREAT-MSF\sample_data\MSF_20201027\result\campus-02\CAMP-MSF.ins'
# ref_file=r'D:\GREAT-MSF\sample_data\MSF_20201027\groundtruth\campus02-groundtruth.txt'

ins_file=r'E:\GREAT-MSF\sample_data\MSF_20201029\result\SEPT-MSF.ins'
ref_file=r'E:\GREAT-MSF\sample_data\MSF_20201029\groundtruth\urban-groundtruth.txt'

# Open File
[t,pos,vel,att,bias,scale]=of.open_ins_file(ins_file,True)
[t_ref,pos_ref,vel_ref,att_ref,state_ref]=of.open_ref_file(ref_file)

# Diff
[time,diff_pos]=diff.diff_enu(t,pos,t_ref,pos_ref)
[time,diff_vel]=diff.diff_vel(t,vel,t_ref,vel_ref)
[time,diff_att]=diff.diff_att(t,att,t_ref,att_ref)

# Plot
dw.plot_pos(time,diff_pos)
dw.plot_vel(time,diff_vel)
dw.plot_att(time,diff_att)

plt.show()
