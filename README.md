## 项目简介
集监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。    

它能为您做什么？   
1. 通过自定义监控功能，助力您掌握您系统的相关业务指标   
   举例假如您的系统是网站系统，您可能感兴趣的指标有：网站每日访问量、独立IP访问量、IP 地域分布等等   
   通过使用它您可以得到这些指标的监控图表，监控系统的最小粒度是分钟，很自然的可以生成这些指标的周图、   
   月图、季图等；通过图表您还可以很清楚的知道您的网站每天、每周、每月访问量的高峰/低谷期。  
2. 通过告警通道，实现未雨绸缪、及时预警   
   仍以网站系统为例，通过预估或者测试您知道了您的网站系统目前能承受的用户量、访问量，基于此您可以在   
   监控系统中设置预警，在您的网站系统业务还未到达瓶颈时就能提前预知，从而有时间轻松从容的扩容。   
   目前告警通道支持邮件、短信、微信等方式。   
3. 通过日志中心，在定位问题时可以方便的查找日志，也可以使用日志做告警   
   以网站系统为例，若您的网站系统有上百台机器，某天某个用户业务出现异常需要定位问题，通过使用它您可以   
   不用登陆机器，直接在日志中心查看，日志中心支持多种方式过滤日志，能轻松的帮您定位到相关日志。   

使用的技术方案：   
1. apache + mysql(监控点数据、配置信息使用 mysql 存储, 支持分布式部署)   
2. 前端 web 控制台采用 [dwz 开源框架](http://jui.org/)   
3. 前端监控图表采用开源 [echarts](https://www.echartsjs.com/zh/index.html) 绘制
4. 后台 cgi 使用开源的cgi模板引擎 - [clearsilver](http://www.clearsilver.net/), 所有cgi支持以fastcgi方式部署    
5. 后台服务使用了开源的 socket 开发框架 - [C++ Sockets](http://www.alhem.net/Sockets/)   

相比其它开源监控系统优势：  
1.	集成告警功能, 支持多种告警方式，告警通道无需开发    
2.	集成分布式日志系统功能    
3.  支持多种部署方式    
	a.支持集中部署, 小规模需求只需一台服务器即可部署   
	b.支持分布式部署
4.	支持自动化配置
5.  监控 api 接口清晰简单，轻松集成到您的软件系统中   
6.  支持多用户访问
7.  支持插件功能, 无需开发即可使用监控插件完成监控需求
   
当前监控上报API支持的语言如下（更多语言支持在开发中）： 
1. c/c++ 
2. php
	   
**web 控制台页面展示:**     
![字符云监控系统](http://open.xrkmonitor.com/html/images/web_page.gif)

**分布式日志系统日志查看演示:**   
![开源版监控系统](http://open.xrkmonitor.com/html/images/web_log.gif)

**监控点数据图表查看演示:**   
![开源版监控系统](http://open.xrkmonitor.com/html/images/web_attr.gif)

**微信告警示例:**(支持邮件、短信、微信、PC客户端等告警方式，告警功能无需开发直接可用)  
![开源版监控系统告警示例](http://xrkmonitor.com/monitor/images/open_wx_2.png)

**在线文档：- [在线文档 http://xrkmonitor.com/monitor/dmt_open_doc.html](http://xrkmonitor.com/monitor/dmt_open_doc.html)**   
**项目演示链接：[字符云监控项目演示 http://open.xrkmonitor.com](http://open.xrkmonitor.com)**   

## 联系我们
QQ 群 699014295 (加群密码：xrkmonitor):   
![字符云监控系统QQ群](http://xrkmonitor.com/monitor/main/img/new_qq_group.png)  

微信公众号:   
![字符云监控系统微信公众号](http://xrkmonitor.com/monitor/main/img/main_wx_qrcode.jpg)  

邮箱：1820140912@qq.com

