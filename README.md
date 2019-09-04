# 字符云监控开源版

#### 介绍
集成监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。  
项目演示链接：[字符云监控开源版演示](http://open.xrkmonitor.com)  
演示账号在登录窗下方查看, 提供两类账号：管理员账号、普通账号做为演示账号  

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
1. 打完整部署包：cd  tools_sh; ./make_all.sh 生成： slog_all.tar.gz 部署包
2. 将部署包 slog_all.tar.gz 拷贝到部署机器的部署目录下   
3. 解压部署包： tar -zxf slog_all.tar.gz; tar -xf slog_all.tar
4. 



#### 使用说明

1. xxxx
2. xxxx
3. xxxx

