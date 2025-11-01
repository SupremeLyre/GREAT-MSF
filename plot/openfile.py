#!/usr/bin/python
# author: zhshen
# date: 20200711
import numpy as np
import math
import glv
import trans

def open_ins_file(insfile,onlyfixed=False):
	t,x,y,z,vx,vy,vz,pitch,roll,yaw=[],[],[],[],[],[],[],[],[],[]
	bgx,bgy,bgz,bax,bay,baz=[],[],[],[],[],[]
	sgx,sgy,sgz,sax,say,saz=[],[],[],[],[],[]
	pdop=[]
	with open(insfile,'rt') as f:
		for line in f:
			if line[0] == "#" or line[0] == "%" :
				continue
			value=line.split()
			if onlyfixed:
				if value[20]=='Float':
					continue
			t.append(float(value[0]))
			x.append(float(value[1]))
			y.append(float(value[2]))
			z.append(float(value[3]))
			vx.append(float(value[4]))
			vy.append(float(value[5]))
			vz.append(float(value[6]))
			pitch.append(float(value[7]))
			roll.append(float(value[8]))
			yaw.append(float(value[9]))
			bgx.append(float(value[10]))
			bgy.append(float(value[11]))
			bgz.append(float(value[12]))
			bax.append(float(value[13]))
			bay.append(float(value[14]))
			baz.append(float(value[15]))

	time=np.array(t).T
	pos=np.array([x,y,z]).T
	vel=np.array([vx,vy,vz]).T
	att=np.array([pitch,roll,yaw]).T
	bias=np.array([bgx,bgy,bgz,bax,bay,baz]).T
	scale=np.array([sgx,sgy,sgz,sax,say,saz]).T
	scale=np.array([pdop]).T
	return (time,pos,vel,att,bias,scale)


def open_ref_file(IE_file):
	t,x,y,z,vx,vy,vz,pitch,roll,yaw,state=[],[],[],[],[],[],[],[],[],[],[]
	float_num,fixed_num=0,0
	i=0
	with open(IE_file,'rt') as f:
		for line in f:
			if "Easting" in line:
				i=3
			if line[0] == "%" or line[0] =="#":
				continue
			if "Week" in line or "weeks" in line:
				continue
			if line=="\n":
				continue
			if line == "Errors/warnings:\n":
				break
			value=line.split()
			t.append(float(value[1]))
			x.append(float(value[2]))
			y.append(float(value[3]))
			z.append(float(value[4]))
			vx.append(float(value[12+i]))
			vy.append(float(value[13+i]))
			vz.append(float(value[14+i]))
			yaw.append(-float(value[21+i]))
			pitch.append(float(value[22+i]))
			roll.append(float(value[23+i]))
			if value[24]== 'Fixed':
				sta=1
				fixed_num=fixed_num+1
			else:
				sta=0
				float_num=float_num+1
			state.append(sta)
	time=np.array(t).T
	pos=np.array([x,y,z]).T
	vel=np.array([vx,vy,vz]).T
	att=np.array([pitch,roll,yaw]).T
	pct=fixed_num/(fixed_num+float_num)
	amb=np.array(state).T
	return (time,pos,vel,att,amb)
	