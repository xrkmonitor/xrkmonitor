// in dmt_dlg_machine_install_plugin.html use
var g_addplugin_timerid = 0;

function dmapAddPlugin_reload()
{
    navTab.reload();
    if(g_addplugin_timerid != 0) {
        clearInterval(g_addplugin_timerid);
        g_addplugin_timerid = 0;
    }
    return true;
}

function dmapAddPlugin(cgip, machid, plugid)
{
	var url = cgip + "mt_slog?action=ddap_install_plugin&mach=" + machid;
    url += "&plugin=" + plugid;
	url += "&self_domain=" + window.document.domain;
	var op = {"mask":true,"maxable":false,"height":440,"width":600, "close":dmapAddPlugin_reload}; 
	$.pdialog.open(url, "dlg_ddap_install_plugin", "一键部署插件进度", op); 
}

function dmtAddPluginStatusShow(step, rid, wid)
{
    switch(step) {
        case 2:
            $('#'+rid).append('<li>已分派任务</li>');
            break;
        case 3:
            $('#'+rid).append('<li>接入服务已接收任务</li>');
            break;
        case 4:
            $('#'+rid).append('<li>接入服务已下发任务到机器</li>');
            break;
        case 5:
            $('#'+rid).append('<li>agent 已向云服务查询到安装包下载地址</li>');
            break;
        case 6:
            $('#'+rid).append('<li>agent 已下载插件部署包</li>');
            break;
        case 7:
            $('#'+rid).append('<li>agent 已部署插件并启动</li>');
            break;
        case 8:
            $('#'+rid).append('<li>接入服务已收到插件消息，插件部署成功</li>');
            break;
        case 9:
            $('#'+rid).append('<li>agent 已部署插件，部署完成</li>');
            break;

        case 20:
            $('#'+wid).html('<font color="red">部署失败：agent 获取插件部署包地址失败, 请检查机器是否安装 wget 工具');
            break;
        case 21:
            $('#'+wid).html('<font color="red">部署失败：agent 获取插件部署包地址, 返回错误码');
            break;
        case 22:
            $('#'+wid).html('<font color="red">部署失败：agent 获取插件部署包地址, 返回内容解析错误');
            break;
        case 23:
            $('#'+wid).html('<font color="red">部署失败：agent 下载插件部署包失败');
            break;
        case 24:
            $('#'+wid).html('<font color="red">部署失败：agent 解压缩部署包失败, 请检查机器是否安装 tar 工具');
            break;
        case 25:
            $('#'+wid).html('<font color="red">部署失败：agent 创建部署目录失败, 请 agent 是否有目录创建权限');
            break;
        case 26:
            $('#'+wid).html('<font color="red">部署失败：agent 启动插件失败, 可能是插件与系统不兼容导致');
            break;
        case 27:
            $('#'+wid).html('<font color="red">部署失败：agent 下载开源版配置文件失败');
            break;

        default:
            $('#'+wid).html('<font color="red">部署失败，进度错误：' + step);
            break;
    }
}

