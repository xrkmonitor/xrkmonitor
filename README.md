## 项目简介
集成自定义监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。    

它能为您做什么？   
1. 通过自定义监控功能，掌握您的系统指标   
   举例假如您的待监控系统是网站，您可能感兴趣的指标有：网站每日访问量、独立IP访问量、IP 地域分布等等   
   通过使用它您可以得到这些指标的监控图表，监控系统的最小粒度是分钟，很自然的可以生成这些指标的周图、   
   月图、季图等；通过图表您还可以很清楚的知道您的网站每天、每周、每月访问量的高峰/低谷期。  
2. 通过告警通道，实现未雨绸缪、及时预警
   仍以网站系统为例，通过预估或者测试您知道了您的网站系统目前能承受的用户量、访问量，基于此您可以在   
   监控系统中设置预警，在您的网站系统业务还未到达瓶颈时就能提前预知，从而有时间轻松从容的扩容。   
3. 通过日志中心，在定位问题时可以方便的查找日志，也可以使用日志做告警   
   以网站系统为例，若您的网站系统有上百台机器，某天某个用户业务出现异常需要定位问题，通过使用它您可以   
   不用登陆机器，直接在日志中心查看，日志中心支持多种方式过滤日志，能轻松的帮您定位到相关日志。   

**字符云监控的目标是助力您更好的掌控您的系统，一切尽在掌握是我们追求的目标！**   

使用的技术方案：   
1. apache + mysql(监控点数据、配置信息使用 mysql 存储, 支持分布式部署)   
2. 前端 web 控制台采用 [dwz 开源框架](http://jui.org/)   
3. 前端监控图表采用开源 [echarts](https://www.echartsjs.com/zh/index.html) 绘制
4. 后台 cgi 使用开源的cgi模板引擎 - [clearsilver](http://www.clearsilver.net/), 所有cgi支持以fastcgi方式部署    
5. 后台服务使用了开源的 socket 开发框架 - [C++ Sockets](http://www.alhem.net/Sockets/)   

相比其它开源监控系统优势：  
1.	集成告警功能，支持多种告警方式，告警通道无需开发    
2.	集成分布式日志系统功能    
3.  支持多种部署方式    
	a.支持集中部署，小规模需求只需一台服务器即可部署   
	b.支持分布式部署
4.	支持自动化配置
5.  监控点 api 接口清晰简单，轻松集成到您的软件系统中   
   
**项目演示链接：[字符云监控项目演示 http://open.xrkmonitor.com](http://open.xrkmonitor.com)**   
     
	   
**web 控制台页面展示:**     
![字符云监控系统](http://open.xrkmonitor.com/monitor/images/web_page.gif)

**分布式日志系统日志查看演示:**   
![开源版监控系统](http://open.xrkmonitor.com/monitor/images/web_log.gif)

**监控点数据图表查看演示:**   
![开源版监控系统](http://open.xrkmonitor.com/monitor/images/web_attr.gif)

**微信告警示例:**(支持邮件、短信、微信、PC客户端等告警方式，告警功能无需开发直接可用)  
![开源版监控系统告警示例](http://xrkmonitor.com/monitor/images/open_wx_2.png)

## 视频教程列表
更多视频教程制作中，感谢大家关注   

1. [源码介绍和编译(ubuntu系统) https://www.bilibili.com/video/av66685598](https://www.bilibili.com/video/av66685598)  
2. [集中部署(ubuntu系统) https://www.bilibili.com/video/av66819048](https://www.bilibili.com/video/av66819048)   


## 编译说明 
视频教程：[源码介绍和编译(ubuntu系统) https://www.bilibili.com/video/av66685598](https://www.bilibili.com/video/av66685598)  

项目目前只支持在 linux 系统中编译，推荐使用 opensuse 或者 ubuntu 系统   
项目依赖 mysql 开发库 libmysqlclient、protobuf、fcgi、memcached，其中 protobuf、fcgi memcached 的源  
码压缩包已经集成到项目中，您只需要安装 libmysqlclient 开发库即可，安装完成后执行如下操作即可编译：
1. 修改 make_env 文件，指定 libmysqlclient 库/头文件的安装路径
2. 执行脚本 ./install_dev.sh，按提示根据需要完成依赖包的安装
3. 执行 make 命令即可编译整个项目，后续如需编译指定模块在模块目录执行 make 命令即可   
   
如您在编译源码包时遇到问题，请给我们留言或者加入QQ群在QQ群中提出您的问题   

## 部署说明
字符云监控系统部署需要安装  memcached，mysql，apache 软件   
memcached 在编译源码执行 install_dev.sh 的时候会提示是否编译生成 memcached 可执行文件   
若您没有选择编译生成 memcached 文件，则您需要自行生成 memcached 文件，并将 memcached    
文件放入 slog_memcached 目录下，并重命名为 slog_memcached，以便能使用监控系统脚本控制   
memcached 的启停   

控制台cgi 在部署时支持普通cgi以及 fastcgi 方式，如需使用 fastcgi 方式部署需要apache导入模块  
mod_fastcgi , 模块源码在 lib 目录下，fastcgi 的配置可参考文件：cgi_fcgi/fastcgi.conf   

部署方式支持集中部署、分布式部署，部署方法如下：
### 集中部署
视频教程：[集中部署(ubuntu系统) https://www.bilibili.com/video/av66819048](https://www.bilibili.com/video/av66819048)   

全部服务部署在一台服务器上的操作步骤：
1. 在部署机上安装 mysql, apache 软件
2. 确保目录 slog_memcached 下存在 slog_memcached 可执行文件    
3. 打完整部署包：cd  tools_sh; ./make_all.sh 生成： slog_all.tar.gz 部署包
4. 将部署包 slog_all.tar.gz 拷贝到部署机器的部署目录下   
5. 解压部署包： tar -zxf slog_all.tar.gz 
6. 初始化 mysql 数据库，将 mtreport_db.sql, attr_db.mysql 导入到 mysql 中(文件在源码 db 目录下)  
7. 授权 mysql 账号：mtreport 访问密码：mtreport875, 访问操作  mtreport_db,attr_db 数据库
8. cd slog_mtreport_client, 修改 slog_mtreport_client.conf 的SERVER_MASTER 配置项改为本机IP，./start.sh 启动模块    
9. 再启动 slog_config 服务: cd slog_config; ./start.sh   
10. 拷贝 html、cgi 文件到 apache 网站，网站根目录设为： /srv/www/htdocs，按如下方法拷贝文件：   
   a. 部署 html/js 文件：将源码中 html 目录下的文件/目录全部拷贝到 /srv/www/htdocs/monitor 目录下   
   b. 将入口文件 html/index.html 拷贝到根目录下 /srv/www/htdocs   
   c. 部署 cgi：将部署机 cgi_fcgi 目录下的文件全部拷贝到 /srv/www/cgi-bin 目录下   
   d. 创建 cgi 本地日志目录：/var/log/mtreport，cgi 调试目录：/srv/www/htdocs/cgi_debug   
11. 启动 apache，使用内置账号：sadmin, 密码：sadmin 访问控制台，将系统服务器配置的IP 全部改为部署机IP  
12. 启动所有服务：进入部署目录，cd tools_sh; ./check_proc_monitor.sh 1，约1分钟后即可查看日志和监控点图表  

### 分布式部署说明
开源版监控系统包含以下服务器类型：
1. mysql 配置服务器，用于存储监控系统的相关配置(分布式部署时，需要在 slog_config.conf 中配置)  
2. mysql 监控点服务器，用于存储监控点数据(可在控制台配置，系统自动调度)  
3. web 控制台服务器，用于部署web 控制台    
4. 监控点服务器，用于接收监控点数据上报(可在控制台配置，系统自动调度)   
5. 日志服务器，用于接收日志，并提供日志查询功能(可在控制台配置，系统自动调度)   
6. agent 接入服务器，用于控制 agent 接入以及下发配置到 agent(agent 模块为：slog_mtreport_client)   

分布式部署推荐部署方式：  
分布式部署时 slog_mtreport_client 模块的 SERVER_MASTER 请修改为 agent 接入服务器 的IP   
分布式部署的基本包括如下模块：   
1. slog_mtreport_client   
2. slog_client   
3. slog_config    
4. tools_sh 目录下全部脚本    

1. mysql 配置服务/web 控制台服务/agent 接入服务, 同机部署, 需要部署如下模块： (1台)   
	a: 部署基本包(基本包的内容如上文)   
	b: 部署 slog_mtreport_server 模块   
	c: 注意打包文件中需要包含 slog_memached 模块, web 控制台服务依赖该模块    
2. mysql 监控点服务器/监控点服务器, 部署在一台机器上需要部署如下模块：(1台)      
	a: 部署基本包(基本包的内容如上文)   
	b: 部署 slog_monitor_server/slog_check_warn/slog_deal_warn 模块    
3. 日志服务器 (1台或多台)   
	a: 部署基本包(基本包的内容如上文)   
	b: 部署 slog_server/slog_write 模块    
	c: 部署 apache 服务，部署 cgi 模块：mt_slog，提供日志查询服务   

集中部署或者分布式部署部署完成后，即可以使用自定义监控了，在被监控机器上部署 slog_mtreport_client    
agent 模块即可，目前 slog_mtreport_client 模块集成了 linux 服务器基础监控(cpu/内存/磁盘/网络)，部署    
即可使用，被监控机可以部署多台。    

## 联系我们
QQ 群 699014295 (加群密码：xrkmonitor):   
![字符云监控系统QQ群](http://xrkmonitor.com/monitor/main/img/new_qq_group.png)  

微信公众号:   
![字符云监控系统微信公众号](http://xrkmonitor.com/monitor/main/img/main_wx_qrcode.jpg)  

邮箱：1820140912@qq.com


