# 字符云监控开源版

#### 介绍
集成监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。  

项目演示链接：[字符云监控开源版演示 :http://open.xrkmonitor.com](http://open.xrkmonitor.com)  
(演示账号在登录窗下方查看, 提供两类账号：管理员账号、普通账号做为演示账号)  

#### 软件架构
软件架构说明


#### 编译说明 
项目目前只支持在 linux 系统中编译，推荐使用 opensuse 或者 ubuntu 系统   
项目依赖 mysql 开发库 libmysqlclient.so、protobuf、curl、fcgi，其中 protobuf、curl、fcgi 的源  
码已经集成到项目中，您只需要安装 libmysqlclient.so 开发库即可，安装完成后执行如下操作即可编译：  
1. 修改 make_env 文件，指定 libmysqlclient 库/头文件的安装路径
2. 执行脚本 ./install_dev.sh，按提示根据需要完成编译依赖包的安装
3. 执行 make 命令即可编译整个项目，后续如需编译指定模块在模块目录执行 make 命令即可  

#### 部署说明
字符云监控系统部署需要安装  memcached，mysql，apache 软件   
部署前需要将 memcached 可执行文件放入 slog_memcached 目录下，并重命名为 slog_memcached  
(memcached 可执行文件通过编译 memcache 源码或者从 rpm 发布包中获取)

部署方式支持集中部署、分布式部署，部署方法如下：   

##### 集中部署
全部服务部署在一台服务器上的操作步骤：
1. 在部署机上安装 mysql, apache 软件
2. 打完整部署包：cd  tools_sh; ./make_all.sh 生成： slog_all.tar.gz 部署包
3. 将部署包 slog_all.tar.gz 拷贝到部署机器的部署目录下   
4. 解压部署包： tar -zxf slog_all.tar.gz; tar -xf slog_all.tar
5. 初始化 mysql 数据库，将 mtreport_db.sql, attr_db.mysql 导入到 mysql 中(文件在源码 db 目录下)  
6. 授权 mysql 账号：mtreport 访问密码：mtreport875, 访问操作  mtreport_db,attr_db 数据库
7. 启动 slog_config 服务: cd slog_config; ./start.sh
8. 拷贝 html、cgi 文件到 apache 网站，假设网站根目录为： /srv/www/htdocs，按如下方法拷贝文件： 
   a. 部署 html/js 文件：将源码中 html 目录下的文件/目录全部拷贝到 /srv/www/htdocs/monitor 目录下
   b. 将入口文件 html/index.html 拷贝到根目录下 /srv/www/htdocs
   c. 部署 cgi：将部署机 cgi_fcgi 目录下的文件全部拷贝到 /srv/www/cgi-bin 目录下
   d. 创建 cgi 本地日志目录：/var/log/mtreport，cgi 调试目录：/srv/www/htdocs/cgi_debug
9. 启动 apache，使用内置账号：sadmin, 密码：sadmin 访问控制台，将系统服务器配置的IP 全部改为部署机IP
10. 启动所有服务：进入部署目录，cd tools_sh; ./check_proc_monitor.sh 1，约1分钟后即可查看日志和监控点图表

##### 分布式部署




