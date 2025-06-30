# 抓取推流码小工具

本软件基于C++ Qt开发界面，使用 wpcap 进行抓包，不依赖 wireshark，但是需要安装npcap，[链接地址](https://npcap.com/#download),安装 Npcap installer 即可，然后下载项目中 Release 里的软件安装包，安装后可以直接使用，无需进行任何配置。

**依赖**
* Npcap

**使用说明**

先打开本软件，然后执行以下步骤
1. 选择合适的网卡
2. 选择监听时长
3. 点击开始获取按钮
4. 打开直播软件
5. 效果如图
![展示图](https://github.com/azh-1415926/rtmp_assistant/blob/main/doc/png/interface.png)

**如何获取合适的网卡**

1. 右键点击网络图标，点击网络和Internet设置

![网络与Internet设置](https://github.com/azh-1415926/rtmp_assistant/blob/main/doc/png/net_01.png)

2. 接下来可以选择WLAN或者以太网，无线选WLAN，有线选以太网

![wifi与以太网](https://github.com/azh-1415926/rtmp_assistant/blob/main/doc/png/net_02.png)

4. 如果选择WLAN，选择自己连接的Wifi，点击属性
!["WLAN"](https://github.com/azh-1415926/rtmp_assistant/blob/main/doc/png/wlan_01.png)

5. 查看WLAN属性中的网卡描述
!["WLAN网卡描述"](https://github.com/azh-1415926/rtmp_assistant/blob/main/doc/png/wlan_02.png)

6. 若为以太网，同理，查看以太网的网卡描述
!["LAN网卡描述"](https://github.com/azh-1415926/rtmp_assistant/blob/main/doc/png/lan.png)

我的例子中，选择了有线网卡

参考了 https://github.com/LFMJUN/douyin_rtmp.git