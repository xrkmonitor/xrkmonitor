## 项目简介
集成监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。    

使用的技术方案：   
1. apache + mysql(监控点数据、配置信息使用 mysql 存储, 支持分布式部署)   
2. 前端 web 控制台采用 [dwz 开源框架](http://jui.org/)   
3. 前端监控图表采用开源 [echarts](https://www.echartsjs.com/zh/index.html) 绘制
4. 后台 cgi 使用开源的cgi模板引擎 - [clearsilver](http://www.clearsilver.net/), 支持以fastcgi方式部署      
5. 后台服务使用了开源的 socket 开发框架 - [C++ Sockets](http://www.alhem.net/Sockets/)   

相比其它开源监控系统优势：  
1.	集成告警功能，支持多种告警方式，告警通道无需开发    
2.	集成分布式日志系统功能    
3.  支持多种部署方式    
	a.集中部署，小规模需求只需一台服务器即可部署   
	b.分布式部署，轻松应对超大规模监控需求    
4.	支持自动化配置，轻松应对超大规模部署   
5.  监控点 api 接口清晰简单，轻松集成到您的软件系统中   
   
**项目演示链接：[字符云监控项目演示 http://open.xrkmonitor.com](http://open.xrkmonitor.com)**   
     
	   
**web 控制台页面展示:**     
![字符云监控系统](http://open.xrkmonitor.com/monitor/images/web_page.gif)

**分布式日志系统日志查看:**   
![开源版监控系统](http://open.xrkmonitor.com/monitor/images/web_log.png)

**监控点数据图表查看界面:**   
![开源版监控系统](http://xrkmonitor.com/monitor/images/open_chart.png)

**微信告警示例:**(支持邮件、短信、微信、PC客户端等告警方式，告警功能无需开发直接可用)  
![开源版监控系统告警示例](http://xrkmonitor.com/monitor/images/open_wx_2.png)

## 编译说明 
项目目前只支持在 linux 系统中编译，推荐使用 opensuse 或者 ubuntu 系统   
项目依赖 mysql 开发库 libmysqlclient、protobuf、fcgi、memcached，其中 protobuf、fcgi memcached 的源  
码已经集成到项目中，您只需要安装 libmysqlclient 开发库即可，安装完成后执行如下操作即可编译：
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

控制台cgi 在部署时支持普通cgi以及 fastcgi 方式，如需使用 fastcgi 方式部署需要 apache 导入模块  
mod_fastcgi , 模块源码在 lib 目录下，fastcgi 参考配置文件：cgi_fcgi/fastcgi.conf   

部署方式支持集中部署、分布式部署，部署方法如下：
### 集中部署
全部服务部署在一台服务器上的操作步骤：
1. 在部署机上安装 mysql, apache 软件
2. 确保目录 slog_memcached 下存在 slog_memcached 可执行文件    
3. 打完整部署包：cd  tools_sh; ./make_all.sh 生成： slog_all.tar.gz 部署包
4. 将部署包 slog_all.tar.gz 拷贝到部署机器的部署目录下   
5. 解压部署包： tar -zxf slog_all.tar.gz 
6. 初始化 mysql 数据库，将 mtreport_db.sql, attr_db.mysql 导入到 mysql 中(文件在源码 db 目录下)  
7. 授权 mysql 账号：mtreport 访问密码：mtreport875, 访问操作  mtreport_db,attr_db 数据库
8. 启动 slog_config 服务: cd slog_config; ./start.sh
9. 拷贝 html、cgi 文件到 apache 网站，网站根目录设为： /srv/www/htdocs，按如下方法拷贝文件：   
   a. 部署 html/js 文件：将源码中 html 目录下的文件/目录全部拷贝到 /srv/www/htdocs/monitor 目录下  
   b. 将入口文件 html/index.html 拷贝到根目录下 /srv/www/htdocs  
   c. 部署 cgi：将部署机 cgi_fcgi 目录下的文件全部拷贝到 /srv/www/cgi-bin 目录下  
   d. 创建 cgi 本地日志目录：/var/log/mtreport，cgi 调试目录：/srv/www/htdocs/cgi_debug  
10. 启动 apache，使用内置账号：sadmin, 密码：sadmin 访问控制台，将系统服务器配置的IP 全部改为部署机IP  
11. 启动所有服务：进入部署目录，cd tools_sh; ./check_proc_monitor.sh 1，约1分钟后即可查看日志和监控点图表  

### 分布式部署说明
开源版监控系统包含以下服务器类型：
1. mysql 配置服务器，用于存储监控系统的相关配置(分布式部署时，需要在 slog_config.conf 中配置)  
2. mysql 监控点服务器，用于存储监控点数据(可在控制台配置，系统自动调度)  
3. web 控制台服务器，用于部署web 控制台    
4. 监控点服务器，用于接收监控点数据上报(可在控制台配置，系统自动调度)   
5. 日志服务器，用于接收日志，并提供日志查询功能(可在控制台配置，系统自动调度)   
6. agent 接入服务器，用于控制 agent 接入以及下发配置到 agent(agent 模块为：slog_mtreport_client)   

监控系统部署的基本包，包含如下模块(关于模块的说明在各模块源码文件的头部，这里不做说明)   
1. slog_config    
2. slog_client    
3. slog_monitor_client   
4. tools_sh 目录以及其下的全部脚本文件  

监控系统各模块部署时需从模块源码目录中拷贝如下文件(以下使用 slog_config 模块作为示例说明)   
1. 模块可执行文件 (slog_config)   
2. 模块配置文件 (slog_config.conf)   
3. 模块目录下的全部脚本文件 (start.sh,stop.sh等)   

分布式部署推荐部署方式：  
1. mysql 配置服务/web 控制台服务/agent 接入服务, 同机部署, 需要部署如下模块： <font color='red'>(1台)</font>   
	a: 部署基本包(基本包的内容如上文)   
	b: 部署 slog_mtreport_server 模块   
	c: 注意打包文件中需要包含 slog_memached 模块, web 控制台服务依赖该模块
2. mysql 监控点服务器/监控点服务器, 部署在一台机器上需要部署如下模块： <font color=red>(1台)</font>    
	a: 部署基本包(基本包的内容如上文)   
	b: 部署 slog_monitor_server/slog_check_warn/slog_deal_warn 模块   
3. 日志服务器 <font color=red>(1台或多台)</font>    
	a: 部署基本包(基本包的内容如上文)  
	b: 部署 slog_server/slog_write 模块   
	c: 部署 apache 服务，部署 cgi 模块：mt_slog，提供日志查询服务   
4. 被监控机器 <font color=red>(目前只支持 linux 系统，1台或多台)</font>      
	a: 只需部署监控系统 agent 模块：slog_mtreport_client   

## 联系我们
QQ 群 699014295 (加群密码：xrkmonitor):   
![字符云监控系统QQ群](http://xrkmonitor.com/monitor/main/img/new_qq_group.png)  

微信公众号:   
![字符云监控系统微信公众号](http://xrkmonitor.com/monitor/main/img/main_wx_qrcode.jpg)  

邮箱：1820140912@qq.com

