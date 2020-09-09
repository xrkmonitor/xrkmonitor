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
	var op = {"mask":true,"maxable":false,"height":420,"width":600, "close":dmapAddPlugin_reload}; 
	$.pdialog.open(url, "dlg_ddap_install_plugin", "一键部署插件进度", op); 
}

