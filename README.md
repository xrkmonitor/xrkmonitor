# 字符云监控开源版

#### 介绍
集成监控点监控、日志监控、数据可视化以及监控告警为一体的国产开源监控系统，直接部署即可使用。  
项目演示链接：[字符云监控开源版演示](http://open.xrkmonitor.com)  
演示账号在登录窗下发查看, 提供两类账号：管理员账号、普通账号做为演示账号  

#### 软件架构
软件架构说明


#### 编译说明 
项目目前只支持 linux 系统中编译，推荐使用 opensuse 或者 ubuntu 系统   
项目依赖 mysql 开发库 libmysqlclient.so、protobuf、curl、fcgi，其中 protobuf、curl、fcgi 的源  
码已经集成到项目中，您只需要安装 libmysqlclient.so 开发库即可，安装完成后执行如下操作即可编译：  
1. 修改 make_env 文件，指定 libmysqlclient 库/头文件的安装路径
2. 执行脚本 ./install_dev.sh，按提示根据需要完成编译依赖包的安装
3. 执行 make 命令即可编译整个项目，后续如需编译指定模块在模块目录执行 make 命令即可  

#### 部署说明


#### 使用说明

1. xxxx
2. xxxx
3. xxxx

#### 参与贡献

1. Fork 本仓库
2. 新建 Feat_xxx 分支
3. 提交代码
4. 新建 Pull Request


#### 码云特技

1. 使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2. 码云官方博客 [blog.gitee.com](https://blog.gitee.com)
3. 你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解码云上的优秀开源项目
4. [GVP](https://gitee.com/gvp) 全称是码云最有价值开源项目，是码云综合评定出的优秀开源项目
5. 码云官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6. 码云封面人物是一档用来展示码云会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
