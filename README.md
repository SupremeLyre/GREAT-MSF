# GREAT-MSF: Multi-Sensor Fusion Navigation Software by Wuhan University GREAT Group (Version 1.0)

## Overview

  The GREAT (GNSS+ **RE**search, **A**pplication and **T**eaching) software suite is designed and developed by the School of Geodesy and Geomatics, Wuhan University. It is a comprehensive platform for space geodesy data processing, precise positioning and orbit determination, as well as multi-source fusion navigation. <br />
  GREAT-MSF is a key module within the GREAT software, focusing on Multi-Sensor Fusion (MSF) navigation solutions that integrate satellite navigation, inertial navigation, camera, LiDAR, HD map, UWB, and more. It is extended from GREAT-PVT ([https://github.com/GREAT-WHU/GREAT-PVT.git](https://github.com/GREAT-WHU/GREAT-PVT.git)). In the software, core computation modules are implemented in C++, while auxiliary Python 3 scripts are provided for plotting results. GREAT-MSF uses CMAKE for build management, allowing users to flexibly choose mainstream C++ compilers such as GCC, Clang, and MSVC. It currently supports build and execution on Windows; for Linux, users are encouraged to compile and test locally. <br />
  GREAT-MSF consists of two portable libraries: LibGREAT and LibGnut. Beyond the GNSS positioning solutions in the original GREAT-PVT, the LibGREAT library further integrates multi-sensor fusion navigation functions, including data decoding and storage used in filtering, inertial navigation computation, and sensor-fusion algorithms. <br />
  This open-source release of GREAT-MSF 1.0 includes GNSS/INS fusion and provides support for odometry and motion-constraint measurements. Building on GREAT-PVT, it further extends the following capabilities:

1. Inertial navigation mechanization and error compensation/correction

2. PPP/INS loose and tight coupling, including ionosphere-free combination and un-differenced, un-combined PPP models

3. RTK/INS loose and tight coupling with carrier-phase ambiguity fixing

4. Rapid and dynamic initialization of the integrated system, including displacement-vector and velocity-vector aided alignment

5. Support for zero-velocity constraints and nonholonomic constraints, with wheel-speed odometry

6. Support for custom IMU data formats and noise models

7. Support for trajectory visualization and Google Maps viewing

8. Support for GPS, GLONASS, Galileo, and BDS-2/3 constellations

9. Plotting scripts are provided to facilitate result analysis.

  To serve young scholars in geodesy and navigation, we are open-sourcing the multi-sensor fusion navigation parts of GREAT. Feedback and criticism are sincerely welcome, and we will keep improving it. Additional sensors will be open-sourced in succession—camera, LiDAR, HD map, UWB, etc.—along with a series of code tutorials.

## Package Directory Structure

```shell
GREAT-MSF_1.0
  ./src                   Source code *
    ./app                 Main programs of GREAT-MSF and GREAT-PVT *
    ./LibGREAT            Core algorithm library for MSF navigation and GNSS positioning *
    ./LibGnut             Gnut library *
    ./third-party         Third-party libraries *
  ./sample_data           Example datasets *
  ./plot                  Plotting utilities *
  ./doc                   Documentation related to GREAT-MSF *
```

## Installation and Usage

See **GREAT-MSF Documentation.pdf**.

## Changelog

2025/10/19  Fixed two issues where variables were not initialized.

2025/10/30 GREAT-MSF was updated from Beta to version 1.0, adding support for odometry, non-holonomic constraints, and zero-velocity constraints with corresponding examples, fixing several issues, and updating the documentation. Sections 2.3 and 5.4 of the documentation provide detailed discussions of the principles and configuration methods for odometry, non-holonomic constraints, and zero-velocity constraints, respectively.

## Contributing

Developers:

Wuhan University GREAT Team, Wuhan University.

Third-party libraries:

* GREAT-MSF uses the G-Nut library ([http://www.pecny.cz](http://www.pecny.cz))
  Copyright (C) 2011–2016 GOP - Geodetic Observatory Pecny, RIGTC.

* GREAT-MSF uses the pugixml library ([http://pugixml.org](http://pugixml.org))
  Copyright (C) 2006–2014 Arseny Kapoulkine.

* GREAT-MSF uses the Newmat library ([http://www.robertnz.net/nm_intro.htm](http://www.robertnz.net/nm_intro.htm))
  Copyright (C) 2008: R B Davies.

* GREAT-MSF uses the spdlog library ([https://github.com/gabime/spdlog](https://github.com/gabime/spdlog))
  Copyright (C) 2015–present, Gabi Melman & spdlog contributors.

* GREAT-MSF uses the GLFW library ([https://www.glfw.org](https://www.glfw.org))
  Copyright (C) 2002–2006 Marcus Geelnard, Copyright (C) 2006–2019 Camilla Löwy

* GREAT-MSF uses the Eigen library ([https://eigen.tuxfamily.org](https://eigen.tuxfamily.org))
  Copyright (C) 2008–2011 Gael Guennebaud

* GREAT-MSF uses the PSINS library ([https://psins.org.cn](https://psins.org.cn))
  Copyright (c) 2015–2025 Gongmin Yan

## Download

GitHub: [https://github.com/GREAT-WHU/GREAT-MSF](https://github.com/GREAT-WHU/GREAT-MSF)

## Others

You are welcome to join the QQ group (1009827379) for discussion and exchange.

WeChat Official Account: **GREAT智能导航实验室** — we will continue to share team updates.

bilibili Account: **GREAT智能导航实验室** — we will continue to publish software walkthrough videos.

---





# GREAT-MSF: 武汉大学GREAT团队多传感器融合导航软件（1.0版）

## 概述

&emsp;&emsp;GREAT (GNSS+ REsearch, Application and Teaching) 软件由武汉大学测绘学院设计开发，是一个用于空间大地测量数据处理、精密定位和定轨以及多源融合导航的综合性软件平台。<br />
&emsp;&emsp;GREAT-MSF是GREAT软件中的一个重要模块，主要用于多传感器融合 (Multi-Sensor Fusion, MSF) 导航解算，包括卫星导航、惯性导航、相机、激光雷达、高精度地图、超宽带等，由GREAT-PVT扩展而来 (https://github.com/GREAT-WHU/GREAT-PVT.git )。软件中，核心计算模块使用C++语言编写，辅助脚本模块使用Python3语言实现结果绘制。GREAT-MSF软件使用CMAKE工具进行编译管理，用户可以灵活选择GCC、Clang、MSVC等主流C++编译器。目前支持在Windows下编译运行，Linux系统需要用户自行编译测试。<br />
&emsp;&emsp;GREAT-MSF由2个可移植程序库组成，分别是LibGREAT和LibGnut。除了原GREAT-PVT中的GNSS定位解决方案外，LibGREAT库还进一步集成了多传感器融合导航功能，包括滤波估计中涉及的数据解码与存储、惯导解算以及传感器融合算法的实现。<br />
&emsp;&emsp;本次开源的GREAT-MSF 1.0版本包括卫星导航与惯性导航融合部分，提供了里程计和运动约束量测支持，在GREAT-PVT基础上进一步扩展了以下功能：
1. 惯性导航机械编排与误差补偿校正

2. PPP/INS松耦合和紧耦合，包括无电离层组合、非差非组合等PPP定位模型

3. RTK/INS松耦合和紧耦合，支持载波相位模糊度固定

4. 组合系统动态快速初始化，包括位移矢量和速度矢量辅助对准

5. 支持零速约束和非完整性约束模型，支持轮速里程计

5. 支持自定义IMU数据格式与噪声模型

6. 支持轨迹动态显示和谷歌地图查看

7. 支持GPS、GLONASS、Galileo、BDS-2/3卫星导航系统

8. 软件包还提供结果绘图脚本，便于用户对数据进行结果分析。


&emsp;&emsp;为服务大地测量与导航领域的青年学子，现开源GREAT软件多传感器融合导航部分的代码。不足之处恳请批评指正，我们将持续完善。后面会陆续开源更多传感器融合，包括相机、激光雷达、高精地图、超宽带等，同时开展系列代码培训。

## 软件包目录结构
```shell
GREAT-MSF_1.0
  ./src	                 源代码 *
    ./app                  GREAT-MSF和GREAT-PVT主程序 *
    ./LibGREAT             多传感器融合导航和GNSS定位核心算法库 *
    ./LibGnut              Gnut库 *
    ./third-party          第三方库 *
  ./sample_data          算例数据 *
  ./plot                 绘图工具 *
  ./doc                  GREAT-MSF相关文档 *
```

## 安装和使用

参见《GREAT-MSF说明文档 1.1.pdf》《GREAT-MSF代码讲解视频资料_2025年3月.pdf》或关注我们团队在视频网站bilibili发布的讲解视频（见下）。

## 修改记录
    
2025/10/19 修复了两处变量未初始化的问题

2025/10/30 **GREAT-MSF由Beta版更新至1.0版本**，提供了里程计、非完整性约束和零速约束支持与相应算例，修复了若干问题，更新了说明文档。说明文档的2.3节与5.4节分别详细讨论了里程计、非完整性约束和零速约束的原理与配置方式。

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

## 其它

欢迎加入QQ群(1009827379)参与讨论与交流。

微信公众号：GREAT智能导航实验室，我们将持续推送团队成果。

bilibili账号：GREAT智能导航实验室，我们将持续发布软件讲解视频。


