## 项目简介
集监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。    

### 相比其它开源监控系统优势  
1.	集成告警功能, 支持多种告警方式   
1.	集成分布式日志系统功能    
1.  支持多种部署方式    
	a.支持集中部署, 小规模需求只需一台服务器即可部署   
	b.支持分布式部署
1.	支持自动化配置
1.  支持多用户访问
1.  支持插件功能, 无需开发即可使用监控插件完成监控需求


### 特色功能推荐
1. 内含IP地址库，支持通过IP地址上报时将IP地址转为物理地址，相同物理地址归并展示  
   一个监控API 即可轻松生成监控数据的物理地址分布图，参考插件：[monitor_apache_log](https://gitee.com/xrkmonitorcom/monitor_apache_log)  
   ![](http://xrkmonitor.com/monitor/images/china_map.png)    
   <br >

1. 监控插件市场，让监控成为可以复用的组件，更多监控插件持续开发中      
   ![](http://xrkmonitor.com/monitor/images/plugin_show.png)    
   <br >

1. 分布式日志系统支持，日志上报支持频率限制、日志染色、自定义字段等高级功能，控制台   
   控制台日志查看支持按关键字、排除关键字、上报时间、上报机器等方式过滤日志，从茫茫   
   日志中轻松找到您需要的日志
   ![](http://xrkmonitor.com/monitor/images/web_log.gif)
   <br >

1. 监控图表支持视图定制模式，视图可按上报服务器、监控点随意组合，轻松   
   定制您需要的监控视图，并可在监控图表上直接设置告警值
   ![](http://xrkmonitor.com/monitor/images/web_attr_git.gif)
   <br >

1. 集成告警功能, 支持邮件、短信、微信、PC客户端等告警方式，告警功能无需开发直接可用
   ![](http://xrkmonitor.com/monitor/images/open_warn_git.png)
   <br >

### 在线部署

安装脚本: install.sh  
从以下链接下载后, 按提示执行即可, 需要系统支持 bash  
(wget http://xrkmonitor.com/monitor/download/install.sh; chmod +x install.sh; ./install.sh ) 

在线部署说明:  
安装脚本会先检查当前系统是否支持在线安装, 如不支持您可以下载源码后在系统上编译安装   
在线部署目前只支持集中部署方式, 即所有服务部署在一台机器上, 该机器上需要安装 mysql/apache    
安装脚本使用中文 utf8 编码, 安装过程请将您的终端设置为 utf8, 以免出现乱码   
安装脚本同时支持 root 账号和普通账号操作, 使用普通账号执行安装部署要求如下: 
1. 在线部署使用动态链接库, 需要在指定目录下执行安装脚本, 目录为: /home/mtreport   
2. 普通账号某些目录可能无权操作, 需要授权才能正常安装    

卸载脚本: uninstall_xrkmonitor.sh   
在线部署过程中会下载该脚本, 如需卸载可执行该脚本 
在线部署详细说明文档：[在线部署](http://xrkmonitor.com/monitor/showdoc/showdoc/web/#/4?page_id=55)  

我们强烈建议您先在本地虚拟机上执行在线安装, 熟悉安装流程后在实际部署到您的服务器上.   

### 离线部署(自行编译源码)

如果在线安装失败或者需要二次开发, 可以使用源码编译方式安装  
安装脚本: local_install.sh  
卸载脚本: uninstall_xrkmonitor.sh   

安装环境变量同在线安装一样, 具体可以查看说明文档: [源码编译-集中部署](http://xrkmonitor.com/monitor/showdoc/showdoc/web/#/4?page_id=38)  
控制台默认账号密码: sadmin/sadmin  


### 使用的技术方案
1. apache + mysql(监控点数据、配置信息使用 mysql 存储, 支持分布式部署)   
2. 前端 web 控制台采用 [dwz 开源框架](http://jui.org/)   
3. 前端监控图表采用开源 [echarts](https://www.echartsjs.com/zh/index.html) 绘制
4. 后台 cgi 使用开源的cgi模板引擎 - [clearsilver](http://www.clearsilver.net/), 所有cgi支持以fastcgi方式部署    
5. 后台服务使用了开源的 socket 开发框架 - [C++ Sockets](http://www.alhem.net/Sockets/)   

### 当前监控上报API支持的语言如下(更多语言支持在开发中)
1. c/c++ 
2. php
	   

**项目演示链接：[字符云监控项目演示 http://open.xrkmonitor.com](http://open.xrkmonitor.com)**   


**在线文档：- [在线文档 http://xrkmonitor.com/monitor/dmt_open_doc.html](http://xrkmonitor.com/monitor/dmt_open_doc.html)**   


### 联系我们
QQ 群 699014295 (加群密码：xrkmonitor):   
![字符云监控系统QQ群](http://xrkmonitor.com/monitor/main/img/new_qq_group.png)  

微信公众号:   
![字符云监控系统微信公众号](http://xrkmonitor.com/monitor/main/img/main_wx_qrcode.jpg)  

邮箱：1820140912@qq.com

