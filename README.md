# GREAT-MSF: 武汉大学GREAT团队多传感器融合导航软件（Beta版）

## 实验日志

- doppler符号定义可能不同，`_addObsD`里面$l$矩阵改一下符号正负

## 概述

&emsp;&emsp;GREAT (GNSS+ REsearch, Application and Teaching) 软件由武汉大学测绘学院设计开发，是一个用于空间大地测量数据处理、精密定位和定轨以及多源融合导航的综合性软件平台。<br />
&emsp;&emsp;GREAT-MSF是GREAT软件中的一个重要模块，主要用于多传感器融合 (Multi-Sensor Fusion, MSF) 导航解算，包括卫星导航、惯性导航、相机、激光雷达、高精度地图、超宽带等，由GREAT-PVT扩展而来 (https://github.com/GREAT-WHU/GREAT-PVT.git )。软件中，核心计算模块使用C++语言编写，辅助脚本模块使用Python3语言实现结果绘制。GREAT-MSF软件使用CMAKE工具进行编译管理，用户可以灵活选择GCC、Clang、MSVC等主流C++编译器。目前支持在Windows下编译运行，Linux系统需要用户自行编译测试。<br />
&emsp;&emsp;GREAT-MSF由2个可移植程序库组成，分别是LibGREAT和LibGnut。除了原GREAT-PVT中的GNSS定位解决方案外，LibGREAT库还进一步集成了多传感器融合导航功能，包括滤波估计中涉及的数据解码与存储、惯导解算以及传感器融合算法的实现。<br />
&emsp;&emsp;本次开源的GREAT-MSF Beta版本主要包括卫星导航与惯性导航融合部分，在GREAT-PVT基础上进一步扩展了以下功能：
1. 惯性导航机械编排与误差补偿校正

2. PPP/INS松耦合和紧耦合，包括无电离层组合、非差非组合等PPP定位模型

3. RTK/INS松耦合和紧耦合，支持载波相位模糊度固定

4. 组合系统动态快速初始化，包括位移矢量和速度矢量辅助对准

5. 支持自定义IMU数据格式、噪声模型

6. 支持轨迹动态显示和谷歌地图查看

7. 支持GPS、GLONASS、Galileo、BDS-2/3、QZSS等卫星导航系统

8. 软件包还提供结果绘图脚本，便于用户对数据进行结果分析。


&emsp;&emsp;为服务大地测量与导航领域的青年学子，现开源GREAT软件多传感器融合导航部分的代码。此为内测版，时间仓促，不足之处恳请批评指正，我们将持续完善。后面会陆续开源更多传感器融合，包括相机、激光雷达、高精地图、轮速、运动约束、超宽带等，同时开展系列代码培训。

## 软件包目录结构
```shell
GREAT-MSF_1.0
  ./src	                 源代码 *
    ./app                  GREAT-MSF和GREAT-PVT主程序 *
    ./LibGREAT             多传感器融合导航和GNSS定位核心算法库 *
    ./LibGnut              Gnut库 *
    ./third-party          第三方库 *
  ./sample_data          算例数据 *
    ./MSF_20201027         GNSS挑战场景算例 *
    ./MSF_20201029         GNSS开阔场景算例 *
  ./plot                 绘图工具 *
  ./doc                  文档 *
    ./GREAT-MSF_1.0.pdf    GREAT-MSF用户指南 *
```

## 安装和使用

参见GREAT-MSF_1.0.pdf

## 修改记录
    
暂无。

## 参与贡献

开发人员：

武汉大学GREAT团队, Wuhan University.

三方库：

* GREAT-MSF使用G-Nut库(http://www.pecny.cz)
  Copyright (C) 2011-2016 GOP - Geodetic Observatory Pecny, RIGTC.
  
* GREAT-MSF使用pugixml库(http://pugixml.org)
  Copyright (C) 2006-2014 Arseny Kapoulkine.

* GREAT-MSF使用Newmat库(http://www.robertnz.net/nm_intro.htm)
  Copyright (C) 2008: R B Davies.

* GREAT-MSF使用spdlog库(https://github.com/gabime/spdlog)
  Copyright(C) 2015-present, Gabi Melman & spdlog contributors.

* GREAT-MSF使用GLFW库(https://www.glfw.org)
  Copyright (C) 2002-2006 Marcus Geelnard, Copyright (C) 2006-2019 Camilla Löwy

* GREAT-MSF使用Eigen库(https://eigen.tuxfamily.org)
  Copyright (C) 2008-2011 Gael Guennebaud

* GREAT-MSF使用PSINS库(https://psins.org.cn)
  Copyright(c) 2015-2025 Gongmin Yan


## 下载地址

GitHub：https://github.com/GREAT-WHU/GREAT-MSF

欢迎加入QQ群(1009827379)参与讨论与交流。

