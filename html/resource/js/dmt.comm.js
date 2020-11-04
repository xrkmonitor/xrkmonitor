/*** xrkmonitor license ***

   Copyright (c) 2019 by rockdeng

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   字符云监控(xrkmonitor) 开源版 (c) 2019 by rockdeng
   当前版本：v1.0
   使用授权协议： apache license 2.0

   云版本主页：http://xrkmonitor.com

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

****/

var g_dmtChartWidth;
var g_dmtLastLeftShow = null;
var g_dmtLastType = null;
var g_dmtLastTypeId = null;
var g_dmtChartMargin = 20;
var g_dmtChartMinWidth = 560;
var g_dmtChartHeight = 460;
var g_dmtSingleChartMinWidth = 1100;
var g_dmtSingleChartHeight = 520;
var g_all_charts = new Array();
var g_dmtRedrawChartCountShowProc = 5;

var g_PluginMargin = 10;

function dmtMathtrunc(d) {
    if(typeof Math.trunc != 'function') 
        return Math.round(d);
    return Math.trunc(d);   
}

function dmtGetHumanTime(val)
{
    var h = '';
    if(val >= 86400) {
        h += dmtMathtrunc(val/86400) + '天 ';
        val %= 86400;
    }
    if(val >= 3600) {
        h += dmtMathtrunc(val/3600) + '小时 ';
        val %= 3600;
    }
    if(val >= 60) {
        h += dmtMathtrunc(val/60) + '分钟 ';
        val %= 60;
    }
    if(val >= 1) {
        h += dmtMathtrunc(val) + '秒 ';
    }
    return h;
}

function dmtSetPluginMarginInfo(tab)
{
	var iSub = DWZ.ui.sbar ? $("#sidebar").width() + 10 : 24;
	var iContentW = $(window).width() - iSub - 20 - 45;
	var xLeft = iContentW % 452;
	var xCount = dmtMathtrunc(iContentW/452);
	var newM = dmtMathtrunc(xLeft/xCount/2);
	if(newM != g_PluginMargin) {
		g_PluginMargin = newM;
		if(typeof tab == 'undefined')
			return true;

		var ptype = '';
		if(tab == "dmt_plugin_open")
			ptype='#dpopen_plugin_list';
		else {
			dmtJsBugReport('dmt.comm.js', 'dmtSetPluginMarginInfo', 'unknow plugin tab:'+tab);
			return true;
		}

		$(ptype).children().each(function(){
			$(this).css('margin', '10px '+g_PluginMargin+'px');
		});
		return true;
	}
	return false;
}


// 判断浏览器类型是否支持
function dmtIsExplorerSupport()
{
	//var ua = navigator.userAgent.toLowerCase();
	//if(ua.indexOf("firefox") == -1 && ua.indexOf("chrome") == -1)
	//	return false;
	return true;
}

function dmtGetCookie(name)
{
	var arr = document.cookie.match(new RegExp("(^| )"+name+"=([^;]*)(;|$)"));
	if(arr != null) return unescape(arr[2]); return null;
}

function dmtCheckFull() 
{
	var isFull = document.fullscreenElement || document.mozFullScreenElement || document.webkitFullscreenElement;
	if (isFull == null) 
		return false;
	return isFull;
}

function dmtToggleFullScreen(element) 
{  
	var full = '';
	if(dmtCheckFull()) {
		var exitMethod = document.exitFullscreen || //W3C
            document.mozCancelFullScreen || //FireFox
            document.webkitExitFullscreen || //Chrome等
            document.webkitExitFullscreen; //IE11
        if (exitMethod) {
            exitMethod.call(document);
        } else if (typeof window.ActiveXObject !== "undefined") { //for Internet Explorer
            var wscript = new ActiveXObject("WScript.Shell");
            if (wscript !== null) {
                wscript.SendKeys("{F11}");
            }
        }
		full = 'no';
	}
	else {
		var requestMethod = element.requestFullScreen || //W3C
			element.webkitRequestFullScreen || //FireFox
			element.mozRequestFullScreen || //Chrome等
			element.msRequestFullScreen; //IE11
		full = 'error';
		if (requestMethod) {
			requestMethod.call(element);
			full = 'yes';
		} else if (typeof window.ActiveXObject !== "undefined") { //for Internet Explorer
			var wscript = new ActiveXObject("WScript.Shell");
			if (wscript !== null) {
				wscript.SendKeys("{F11}");
				full = 'yes';
			}
		}
	}
	return full;
}  


// 清除 cookie
function dmtDelCookie(name)
{
	var exp = 7*24*60*60*1000;  
	var date = new Date(+new Date()-exp);
	var cval = dmtGetCookie(name);
	if(cval != null)
		document.cookie = name + "=" + cval + "; expires="+ date.toUTCString() + ";path=/";
}

// 计算图表展示区的图表的尺寸，图表尺寸最小为 620*500
function dmtSetChartSize()
{
	// 45 -- 预留给滚动条
	var iSub = DWZ.ui.sbar ? $("#sidebar").width() + 10 : 24;
	var iContentW = $(window).width() - iSub - 5 - 45;
	if(typeof g_dmtLastLeftShow != 'undefined' && g_dmtLastLeftShow)
		iContentW -= $('.chartLeftMenu').width();

	var xWidth = g_dmtChartMinWidth+g_dmtChartMargin*2;
	var xLeft = iContentW % xWidth;
	var xCount = dmtMathtrunc(iContentW/xWidth);
	var xBlank = dmtMathtrunc(xLeft/xCount);
	g_dmtChartWidth = g_dmtChartMinWidth+xBlank;
}

function dmtSetRedrawChartsInfo(type_id, type, isLeftShow)
{
	g_dmtLastTypeId = type_id;
	g_dmtLastType = type;
	g_dmtLastLeftShow = isLeftShow;
}

// 重新计算图表尺寸, 重绘图表
function dmtRedrawCharts(type_id, type, isLeftShow)
{
	if(typeof type_id != 'undefined')
	{
		g_dmtLastTypeId = type_id;
		g_dmtLastType = type;
		g_dmtLastLeftShow = isLeftShow;
	}
	else {
		type_id = g_dmtLastTypeId;
		type = g_dmtLastType;
		isLeftShow = g_dmtLastLeftShow;
	}
	if(type_id == null) 
		return;

	dmtSetChartSize();
	if(typeof g_all_charts != 'undefined')
	{
		var keystr = '_'+type_id+type;
		var iNeedRedraw = 0;
		for(ct in g_all_charts) {
		    if(ct.match(keystr)) {
				if(typeof g_all_charts[ct].setwidth != 'undefined' && g_all_charts[ct].setwidth == g_dmtChartWidth) 
					return; 
				iNeedRedraw++; 
			}
		}

		var rchart = $("#my_background,#my_progressBar"); 
		if(iNeedRedraw > g_dmtRedrawChartCountShowProc) { 
			$('#my_progressBar').text('图表重绘中, 请稍等...'); 
			rchart.show(); 
		}

		setTimeout(function() {
			for(ct in g_all_charts)
			{
				if(ct.match(keystr)) 
				{
					$('#'+ct).css('width', g_dmtChartWidth);
					g_all_charts[ct].resize();
					g_all_charts[ct].setwidth = g_dmtChartWidth;
				}
			}
			rchart.hide();
		}, 5);
	}
}

function dmtMaskWarnConfig(warn_type, warn_type_id, attr_id)
{
	var reqpara = {};

	var cur_chart_idx = 'attr_' + attr_id + '_' + warn_type_id + warn_type;
	if(typeof g_all_charts[cur_chart_idx] == 'undefined'
		|| typeof g_all_charts[cur_chart_idx].warnInfo == 'undefined')
	{
		alertMsg.error('javascript 脚本错误, 无告警信息:' + cur_chart_idx);
		return;
	}
	var warnInfo = g_all_charts[cur_chart_idx].warnInfo;
	
	reqpara.action = "mask_warn_config";
	reqpara.mask = warnInfo.mask;
	reqpara.warn_cfg_id = warnInfo.warn_cfg_id;

	var cgi_path; 
	if(typeof g_siteInfo.cgi_path != 'undefined' && g_siteInfo.cgi_path != '')
		cgi_path = g_siteInfo.cgi_path;
	else
		cgi_path = warnInfo.cgi_path;
	var requrl = cgi_path+'mt_slog_warn';

	$.ajax({
		url:requrl, 
		data:reqpara, 
		global:false,
		success:function(result){
			if(dmtFirstDealAjaxResponse(result))
				return;
			if(result.statusCode != 200)
			{
				alertMsg.warn( DWZ.msg(result.msgid) );
				return;
			}

			var cur_chart = g_all_charts[cur_chart_idx];
			var opold = cur_chart.getOption();
			var subtitles = opold.title[0].subtext;
			var subtitle = subtitles.split('\n')[0];

			// 重新设置告警配置部分
			subtitle += "\n 告警配置【 最大值：";
			var showclass = "";
			if(warnInfo.show_class == 'percent')
				showclass = "%";

			if(result.warn_flag & 1)
				subtitle += result.warn_max + showclass;
			else
				subtitle += "无 ";
			subtitle += "，最小值：";
			if(result.warn_flag & 2)
				subtitle += result.warn_min + showclass;
			else
				subtitle += "无 ";
			subtitle += "，波动值：";
			if(result.warn_flag & 4)
				subtitle += result.warn_wave + "% ";
			else
				subtitle += "无 ";
			if(result.warn_flag & 32)
				subtitle += "，屏蔽状态：已屏蔽";
			subtitle +=	" 】";

			var op = {
				title:{
					subtext:subtitle,
				},
				toolbox: {
					feature: {
						myUnMaskWarningData:{
							show:true
						},
						myMaskWarningData:{
							show:true
						},
						mySetWarningData:{
							show:true
						}
					}
				}
			};

			warnInfo.warn_cfg_id = result.warn_config_id;
			if(result.warn_flag & 32)
			{
				op.toolbox.feature.myUnMaskWarningData.show = true;
				op.toolbox.feature.mySetWarningData.show = false;
				op.toolbox.feature.myMaskWarningData.show = false;
				warnInfo.mask = 0;
			}
			else
			{
				op.toolbox.feature.myUnMaskWarningData.show = false;
				op.toolbox.feature.mySetWarningData.show = true;
				if((result.warn_flag&1) || (result.warn_flag&2) || (result.warn_flag&4)) {
					op.toolbox.feature.mySetWarningData.show = true;
					warnInfo.mask = 1;
				}
				else {
					op.toolbox.feature.mySetWarningData.show = false;
					warnInfo.mask = 0;
				}
			}
			cur_chart.setOption(op);
		},
		dataType:'json'
	});
}

function dmtSetWarnConfig(warn_type, warn_type_id, attr_id)
{
	var cur_chart_idx = 'attr_' + attr_id + '_' + warn_type_id + warn_type;
	if(typeof g_all_charts[cur_chart_idx] == 'undefined'
		|| typeof g_all_charts[cur_chart_idx].warnInfo == 'undefined')
	{
		alertMsg.error('javascript 脚本错误, 无告警信息:' + cur_chart_idx);
		return;
	}

	var warnInfo = g_all_charts[cur_chart_idx].warnInfo;
	var cgi_path; 
	if(typeof g_siteInfo.cgi_path != 'undefined' && g_siteInfo.cgi_path != '')
		cgi_path = g_siteInfo.cgi_path;
	else
		cgi_path = warnInfo.cgi_path;
	var url = cgi_path + "/mt_slog_warn?action=chart_set_attr_warn";
	var attr_name = warnInfo.attr_name;

	url += "&warn_cfg_id=" + warnInfo.warn_cfg_id;
	url += "&show_class=" + warnInfo.show_class;
	url += "&attr_name=" + attr_name;

	url += "&warn_type_id=" + warn_type_id;
	url += "&warn_attr_id=" + attr_id;
	url += "&warn_type=" + warn_type ;

	var dlg = "dc_dlg_set_warn_" + warn_type_id + "_" + attr_id;
	var op = $.parseJSON('{"mask":true,"maxable":false,"height":260,"width":530,"resizable":false}'); 
	var title = "";
	if(warn_type == "view")
		title = "设置视图告警：视图【" + warn_type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	else
		title = "设置服务器告警：服务器【" + warn_type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	$.pdialog.open(url, dlg, title, op); 
}

function dmtShowSingle(cust_date, attr_name, show_single_type, cgi_path, type, type_id, attr_id, site_url)
{
	var url = cgi_path + "/mt_slog_showview?action=show_single";
	url += "&type_id=" + type_id;
	url += "&cust_date=" + cust_date;
	url += "&show_type=" + type;
	url += "&attr_id=" + attr_id;
	url += "&show_single_type=" + show_single_type;

	var dlg = "dc_dlg_show_single_" + type_id + "_" + attr_id;

	var strOp = '{"mask":true,"maxable":false,"height":';
		strOp += g_dmtSingleChartHeight;
		strOp += ',"width":' + g_dmtSingleChartMinWidth + ',"resizable":true}';
	var op = $.parseJSON(strOp);
	var title = "";
	if(type == "view")
		title = "视图【" + type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	else if(type == 'machine')
		title = "服务器【" + type_id + "】" + "监控点【" + attr_id + "_" + attr_name + "】";
	else  {
		title = "监控网点【" + attr_id + "_" + attr_name + "】 站点链接：" + site_url;
	}
	title += " - 上报数据查看";
	$.pdialog.open(url, dlg, title, op); 
}

function dmtSetShowSingleSite(op, showtype, attr_val_list, attr_val, attr_info)
{
	op.toolbox.feature.myShowSingleData.onclick = function() {
		return dmtShowSingle(
			attr_val_list.cust_date, 
			attr_info.name, 
			attr_val_list.show_single_type,
			attr_val_list.cgi_path, 
			showtype,
			attr_val_list.site_id ,
			attr_info.id,
			attr_val_list.site_url
		);
	};
}

function dmtGetDateStr(d, bTime)
{
	var dt = new Date(d+new Date().getTimezoneOffset()*60*1000);
	var dtstr = '';

	if(typeof(bTime) == 'undefined') {
		dtstr = dt.getFullYear();
		if(dt.getMonth() < 9)
			dtstr += '-0'+(dt.getMonth()+1);
		else
			dtstr += '-'+(dt.getMonth()+1);
		if(dt.getDate() < 10)
			dtstr += '-0'+dt.getDate();
		else
			dtstr += '-'+dt.getDate();
	}

	if(dt.getHours() < 10)
		dtstr += ' 0'+dt.getHours();
	else
		dtstr += ' '+dt.getHours();
	if(dt.getMinutes() < 10)
		dtstr += ':0'+dt.getMinutes();
	else
		dtstr += ':'+dt.getMinutes();

	return dtstr;
}

function dmtSetCustToolBox(op, showtype, attr_val_list, attr_val, attr_info, warnInfo)
{
	var warn_type_id;
	if(showtype == 'view') {
		op.toolbox.feature.myShowSingleData.onclick = function() {
			return dmtShowSingle(
				attr_val_list.cust_date, 
				attr_info.name,
				attr_val_list.show_single_type,
				attr_val_list.cgi_path,
				showtype,
				attr_val_list.view_id, 
				attr_info.id
			);
		};
		warn_type_id = attr_val_list.view_id;
	}
	else {
		op.toolbox.feature.myShowSingleData.onclick = function() {
			return dmtShowSingle(
				attr_val_list.cust_date, 
				attr_info.name,
				attr_val_list.show_single_type,
				attr_val_list.cgi_path,
				showtype,
				attr_val_list.machine_id, 
				attr_info.id
			);
		}
		warn_type_id = attr_val_list.machine_id;
	};

	if(attr_val.warn_flag & 24)
		warnInfo.warn_cfg_id = attr_val.warn_config_id;
	else
		warnInfo.warn_cfg_id = 0;
	if(typeof g_siteInfo.cgi_path == 'undefined' || g_siteInfo.cgi_path != '')
		warnInfo.cgi_path = attr_val_list.cgi_path;
	warnInfo.show_class = attr_info.show_class;
	warnInfo.attr_name = attr_info.name;

	if(attr_val.warn_flag & 32) {
		op.toolbox.feature.myUnMaskWarningData.show = true;
		op.toolbox.feature.mySetWarningData.show = false;
		op.toolbox.feature.myMaskWarningData.show = false;
		warnInfo.mask = 0;
	}
	else {
		op.toolbox.feature.myUnMaskWarningData.show = false;
		op.toolbox.feature.mySetWarningData.show = true;
		if((attr_val.warn_flag&1) || (attr_val.warn_flag&2) || (attr_val.warn_flag&4)) {
			op.toolbox.feature.myMaskWarningData.show = true;
			warnInfo.mask = 1;
		}
		else {
			op.toolbox.feature.myMaskWarningData.show = false;
			warnInfo.mask = 0;
		}
	}

	op.toolbox.feature.myUnMaskWarningData.onclick = function() {
		return dmtMaskWarnConfig(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	op.toolbox.feature.mySetWarningData.onclick = function() {
		return dmtSetWarnConfig(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};

	op.toolbox.feature.myMaskWarningData.onclick = function() {
		return dmtMaskWarnConfig(
			showtype,
			warn_type_id,
			attr_info.id
		);
	};
}

function dmtShowHumanStaticTime(t)
{
    switch(t)
    {
        case 1: return '1分钟';
        case 5: return '1分钟';
        case 10: return '5分钟';
        case 15: return '15分钟';
        case 30: return '30分钟';
        case 60: return '1小时';
        case 120: return '2小时';
        case 180: return '3小时';
    }
    return 'unknow';
}

function dmtGetViewSubTitle(showtype, attr_val_list, attr_val, attr_info)
{
	var subtitle = '';
	var subtitle = '';
	if(attr_val.max > 0) {
		subtitle = " 【";
		subtitle += "最大值：" + dmtShowChangeValue(attr_val.max);
		subtitle += ", 图表累计上报：" + dmtShowChangeValue(attr_val.total) + ', 当前统计周期：';
		subtitle += dmtShowChangeValue(attr_val.cur) + "/" + dmtShowHumanStaticTime(attr_info.static_time) + "】\n "
	}
	else if(attr_val.cur > 0) {
		subtitle = " 当前时间：" + attr_val_list.date_time + ', ';
		subtitle += " 当前统计周期：" + dmtShowChangeValue(attr_val.cur) + '/';
		subtitle += dmtShowHumanStaticTime(attr_info.static_time) + "\n\n";
	}
	else {
		subtitle = '暂无数据上报\n\n';
	}

	subtitle += "告警配置【 最大值：";
	var showclass = "";
	if(attr_info.show_class == 'percent')
		showclass = "%";

	if(attr_val.warn_flag & 1)
		subtitle += attr_val.warn_max + showclass;
	else
		subtitle += "无 ";
	subtitle += "，最小值：";
	if(attr_val.warn_flag & 2)
		subtitle += attr_val.warn_min + showclass;
	else
		subtitle += "无 ";
	subtitle += "，波动值：";
	if(attr_val.warn_flag & 4)
		subtitle += attr_val.warn_wave + "% ";
	else
		subtitle += "无 ";
	if(attr_val.warn_flag & 32)
		subtitle += "，屏蔽状态：已屏蔽";
	subtitle +=	" 】";
	return subtitle;
}

function dmtGetViewTitle(showtype, attr_val_list, attr_info)
{
	var title;
	if(showtype == 'view')
	{
		title = "视图ID【" + attr_val_list.view_id + "】";
		title += "监控点【" + attr_info.name + "-" + attr_info.id + "】";
	}
	else if(showtype == 'machine')
	{
		title = "服务器ID【" + attr_val_list.machine_id + "】";
		title += "监控点【" + attr_info.name + "-" + attr_info.id + "】";
	}
	return title;
}

// tstr: yyyy-MM-dd HH:mm:ss
function dmtGetTimeStamp(tstr)
{
	if(typeof tstr=="undefined" || tstr == "")
		return 0;
	var tmp = tstr.split(' ');          
	var temp = tmp[0].split('-');
	var y = temp[0] - 0;              
	var m = temp[1] - 0;             
	var d = temp[2] - 0;              

	temp = tmp[1].split(':');
	var h = temp[0] - 0;
	var mm = temp[1] - 0;
	var s = temp[2] - 0;
	return (new Date(y-0, m-1, d-0, h-0, mm-0, s-0)-0)/1000;
}

function dmtEncodeHTML(str)
{
    var s = "";
    if (str.length === 0)
        return "";
    s = str.replace(/&#39;/g, "'")
        .replace(/&#160;/g, " ")
        .replace(/&#92;/g, "\\")
        .replace(/>/g, "&gt;")
        .replace(/</g, "&lt;")
        .replace(/_r_n/g, "<br>")
        .replace(/\\x0A/g, "<br>")
        .replace(/\\x2F/g, "/")
        .replace(/\\x3B/g, ";")
        .replace(/\\x22/g, "\"")
        .replace(/\\x3C/g, "<")
        .replace(/\\x26/g, "&")
        .replace(/\\x27/g, "\'")
        .replace(/\\x5C/g, "\\")
        .replace(/\\x3E/g, ">");

    return s;
}

function dmtJsBugReport(file, func, msg)
{
	var bugmsg = "Js bug report info -- file:" + file + "   ";
		bugmsg += "function:" + func + "   ";
		bugmsg += "info msg:" + msg + "   ";
}

function dmtExport(url, data)
{
	alertMsg.confirm("确实要导出这些记录吗?", {
	    okCall: function() {
			$.ajax({
				type:'post',
				url:url,
				data:{data:data},
				dataType:"json",
				global: false,
				cache: false,
				success: function(js){
					if(js.statusCode != 0)
					    alertMsg.warn(js.msg);
					else if(js.record_count == 0)
					    alertMsg.info('记录为空');
					else
					    window.location = js.file;
				},
			    error: DWZ.ajaxError
			});
		}
	});
}

function dmtSetTypeTree(treeinfo)
{
    var list = "<li><a href='#' " + "name=" + treeinfo.type + ">" + treeinfo.name + "</a>";
    if(treeinfo.subcount == 0) {
        list += "</li>";
        return list;
    }   

    list += "<ul>";
    var listsub = treeinfo.list;
    for(var i=0; i < treeinfo.subcount && i < listsub.length; i++)
        list += dmtSetTypeTree(listsub[i]);
    list += "</ul>";

    list += "</li>";
    return list;
}

// 通过监控点类型号或者类型信息
function dmtGetTypeInfo(treeinfo, type)
{
	if(treeinfo.type == type)
		return treeinfo;

	var listsub = treeinfo.list;
	for(var i=0; i < treeinfo.subcount && i < listsub.length; i++)
	{
		var info = dmtGetTypeInfo(listsub[i], type);
		if(info != "null")
			return info; 
	}
	return "null";
}

// 根据监控点id 查找监控点信息
function dmtGetAttrInfo(attrinfo, attr_id)
{
	var listattr = attrinfo.list;
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
	{
		if(listattr[i].id == attr_id)
			return listattr[i];
	}
	return "null";
}

function dmtJumpToAttrPic(attr_show_id, ctview, callBack)
{
	// ctattr 为监控点曲线图 id
	var ctattr = '#'+attr_show_id;
	if($(ctattr).length <= 0) 
	{
		var arrStr = attr_show_id.split('_');
		if(callBack == 'click' || !$.isFunction(callBack))
		{
			alertMsg.info('监控点:' + arrStr[1] + ' 无数据上报');
			return false;
		}

		if(false == callBack(attr_show_id))
			alertMsg.info('监控点:' + arrStr[1] + ' 无数据上报');
		else
			alertMsg.info('监控点:' + arrStr[1] + ' 上报数据获取中...');
		return false;
	}

	// 当前滚动条位置
	var sctop = $(ctview).scrollTop();
	// 容器起始偏移
	var viewoff = $(ctview).offset().top;
	// 当前容器偏移
	var attroff = $(ctattr).offset().top;
	// 当前容器置顶的偏移量
	var off = attroff - viewoff + sctop;
	$(ctview).animate({ scrollTop: off });
	return false;
}

// 机器、视图绑定的监控点列表
function dmtSetAttrList(attrinfo, ds_attr_type_list, type, attr_list_arry, clickCallBack)
{
	var listattr = attrinfo.list;
	var list = "<li>";
	var ctview, jumpid;
	if(type == 'view')
	{
		list += "<a href='#'>视图【" + attrinfo.view_id + "】已绑定的监控点列表";
		ctview = '#ds_ct_attr_show_list_' + attrinfo.view_id;
		jumpid = attrinfo.view_id;
	}
	else
	{
		list += "<a href='#'>机器【" + attrinfo.machine_id + "】有上报的监控点列表";
		ctview = '#dsm_ct_attr_show_list_' + attrinfo.machine_id;
		jumpid = attrinfo.machine_id;
	}
	list += "(共:" + attrinfo.count + "个)</a>";

	// 循环遍历，将相同监控点类型的监控点放一起
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
		listattr[i].pushed = false;
	for(var i=0; i < attrinfo.count && i < listattr.length; i++)
	{
		if(listattr[i].pushed)
			continue;

		if(type != 'site') {
			list += "<ul>"; 
			list += "<li><a href='#'>" + ds_attr_type_list[listattr[i].attr_type] + "</a>";
		}
		list += "<ul>";
		for(var j=i; j < attrinfo.count && j < listattr.length; j++)
		{
			if(listattr[j].pushed || listattr[j].attr_type != listattr[i].attr_type)
				continue;
			
			listattr[j].pushed = true;
			var attr_show_id = "attr_" + listattr[j].id + "_" + jumpid + type;
			list += "<li>";
			list += "<a onclick=\"dmtJumpToAttrPic('"+attr_show_id+"', '"+
				ctview+"', "+clickCallBack+")\" href='#' ctid='" + attr_show_id + "'>";

			list += listattr[j].id + "_" + listattr[j].name;
			if(listattr[j].global == 1) 
				list += "<font color='blue'>&nbsp;[全局]</font>";
		
			attr_list_arry.push(listattr[j].id);
			list += "</a>";
			list += "</li>";
		}
		list += "</ul>";
		list += "</li>";
		list += "</ul>";
	}

	list += "</li>";
	return list;
}

function dmtShowChangeValue(value)
{
	if(value >= 1073741824)
	{
		var f = value/1073741824;
		return Math.round(parseFloat(f)*100)/100 + 'G';
	}
	if(value >= 1048576)
	{
		var f = value/1048576;
		return Math.round(parseFloat(f)*100)/100 + 'M';
	}
	if(value >= 1024)
	{
		var f = value/1024;
		return Math.round(parseFloat(f)*100)/100 + 'K';
	}
	return value;
}

function dmtSetStrAttrInfoChartIpGeo(ct_id, attr_info, js, attr_val_list, showtype)
{
	var op = {
		title: {
			x: 'left',
			subtext:'',
			top:'top',
			subtextStyle: {},
			show:true,
			text:''
		},
		tooltip: {
			trigger:'item',
			confine:true,
			formatter: '{b}<br/>访问次数: {c}' 
		},
		visualMap: {
			min:0,
			max:10000,
			realtime:true,
			calculable: true,
			inRange: {
				color: ['lightskyblue', 'yellow', 'orangered']
			}
		},
		toolbox: {
			show: true,
			orient: 'vertical',
			top: 'center',
			right:20,
			feature: {
				dataView: {readOnly: false},
				restore: {},
				saveAsImage: {}
			}
		},
		legend : {
			type:'scroll',
			orient: 'vertical',
			left:'left',
			top:'70',
			show:true
		},
		series: [
			{
				type:'map',
				mapType: 'china',
				label: {
					show:true
				},
				data:[],
				geoIndex:0
			}
		]
	};

	$('#'+ct_id).css('height', g_dmtChartHeight);
	if(showtype == 'view')
		op.title.text = '视图ID【'+attr_val_list.view_id+'】'+'监控点：'+attr_info.name;
	else
		op.title.text = '服务器ID【'+attr_val_list.machine_id+'】'+'监控点：'+attr_info.name;
	op.title.text += '-'+attr_info.id;

	if(js.str_count > 0) {
		var total = 0;
		for(var i=0; i < js.str_count; i++) {
			total += js.str_list[i].value;
		}
		op.title.subtext = '统计时间：' + attr_val_list.date_time_cur
			+ '(总访问次数:' + total + ', 部分地理位置不能识别的访问将不显示在地图上) \n';
		op.visualMap.max = js.str_list[0].value+100;
		op.series[0].data = js.str_list;
	}
	else {
		op.series[0].data = [];
		op.title.subtextStyle.color = 'red';
		op.title.subtextStyle.fontWeight = 'bold';
		op.title.subtext = "暂无数据上报";
	}
	g_all_charts[ct_id] = echarts.init(document.getElementById(ct_id));
	g_all_charts[ct_id].setOption(op);
	g_all_charts[ct_id].setwidth = g_dmtChartWidth;
}


function dmtSetStrAttrInfoChart(ct_id, attr_info, js, attr_val_list, showtype)
{
	var op = {
		title: {
			x: 'left',
			subtext:'',
			top:'top',
			subtextStyle: {},
			show:true,
			text:''
		},
		tooltip: {
			trigger:'item',
			confine:true,
			formatter: '上报字符串：{b}<br>上报值: {c} ({d}%)'
		},
		legend : {
			type:'scroll',
			orient: 'vertical',
			right:'right',
			top:'70',
			show:true
		},
		series: [
			{
				type:'pie',
				radius:'55%',
				center:['40%', '55%'],
				data:[],
				label: { 
					show: true,
            	    emphasis: {
            	        show: true,
            	    }
            	},
				itemStyle: {
					emphasis: {
						shadowBlur: 10,
						shadowOffsetX: 0,
						shadowColor: 'rgba(0, 0, 0, 0.5)'
					}
				}
			}
		]
	};

	$('#'+ct_id).css('height', g_dmtChartHeight);
	if(showtype == 'view')
		op.title.text = '视图ID【'+attr_val_list.view_id+'】'+'字符串型监控点：'+attr_info.name;
	else
		op.title.text = '服务器ID【'+attr_val_list.machine_id+'】'+'字符串型监控点：'+attr_info.name;

	if(js.str_count > 0) {
		op.title.subtext = '统计时间：'+attr_val_list.date_time_cur+', 显示排名前 20 个';
		op.series[0].data = js.str_list;
	}
	else {
		op.series[0].data = [];
		op.title.subtextStyle.color = 'red';
		op.title.subtextStyle.fontWeight = 'bold';
		op.title.subtext = "暂无数据上报";
		op.title.x = 'center';
		op.title.top = '20%';
	}

	g_all_charts[ct_id] = echarts.init(document.getElementById(ct_id));
	g_all_charts[ct_id].setOption(op);
	g_all_charts[ct_id].setwidth = g_dmtChartWidth;
}


function dmtGetXAxisTimeInfo(dateStart, count_day, attr_info)
{
	var time_info = [];
	for(var i=0; i < attr_info.static_idx_max*count_day; i++) {
		time_info.push(dateStart+i*attr_info.static_time*60*1000);
	}    
	return time_info;
}

function dmtGetYAxisData(time_info, dstr)
{
	var e_data_y = [];
	var attr_data = dstr.split(",");
	for(var j=0; j < attr_data.length; j++) {
		var objd = new Object;
		if(attr_data[j] != "-") {
			objd.value = new Array(time_info[j], attr_data[j]);
			e_data_y.push(objd);
		}
	}
	return e_data_y;
}

function dmtGetUsePerPieOption(options) 
{
	var op = $.extend({ c_items:1, c_sublink:'' }, options);

	var subtext_color = 'blue';
	var text_color = '#044';
	if(op.c_colors.length > 4)
		subtext_color = op.c_colors[4];
	if(op.c_items > 1) 
		text_color = 'blue';
	var pie_option = {
	    title: {
			id:op.c_name,
	        text: op.c_text,
			triggerEvent: true,
			subtext: op.c_subtext,
	        left: 'center',
	        top: 'center',
	        textStyle: {
	            color: text_color,
	            fontSize: 14,
	            fontFamily: 'PingFangSC-Regular'
	        },
        	subtextStyle: {
        	    color: subtext_color,
        	    fontSize: op.c_items > 1 ? 14 : 18,
        	    fontFamily: 'PingFangSC-Regular',
				fontWeight: 'bold',
        	    top: 'center'
        	},
			itemGap: -1,
	    },
	    series: [{
	        name: op.c_name,
	        type: 'pie',
	        clockWise: true,
	        radius: ['80%', '95%'],
	        itemStyle: {
	            normal: {
	                label: {
	                    show: false
	                },
	                labelLine: {
	                    show: false
	                }
	            }
	        },
	        hoverAnimation: false,
	        data: [{
	            value: op.c_val,
	            name: 'completed',
	            itemStyle: {
	                normal: {
	                    borderWidth: 8,
	                    borderColor: { 
	                        colorStops: [{
	                            offset: 0,
	                            color: op.c_colors[0] || op.c_colors[1]
	                        }, {
	                            offset: 1,
	                            color: op.c_colors[2] || op.c_colors[3]
	                        }]
	                    },
	                    color: {
	                        colorStops: [{
	                            offset: 0,
	                            color: op.c_colors[0] || op.c_colors[1]
	                        }, {
	                            offset: 1,
	                            color: op.c_colors[2] || op.c_colors[3]
	                        }]
	                    },
	                    label: {
	                        show: false
	                    },
	                    labelLine: {
	                        show: false
	                    }
	                }
	            }
	        }, {
	            name: 'gap',
	            value: 100 - op.c_val,
	            itemStyle: {
	                normal: {
	                    label: {
	                        show: false
	                    },
	                    labelLine: {
	                        show: false
	                    },
	                    color: 'rgba(0, 0, 0, 0)',
	                    borderColor: 'rgba(0, 0, 0, 0)',
	                    borderWidth: 0
	                }
	            }
	        }]
	    }]
	};

	if(op.c_sublink != '') {
		pie_option.title.sublink = op.c_sublink;
		pie_option.title.subtarget = 'self';
	}
	return pie_option;
}

function dmtShowAttrInfo(attr_list, attr_val_list, ct_div, showtype)
{
	var STR_REPORT_D = 6, STR_REPORT_D_IP = 7;
	var op = {
		legend: {
			show:false,
			bottom:10
		},
		title:{
			text:'',
			x:'center',
			subtext:'',
			top:'top',
			subtextStyle: {},
			show:true
		},
		useUTC:true,
		toolbox: {
			show:true,
			orient: 'vertical',
			top: 'center',
			right:20,
			feature: {
				myUnMaskWarningData:{
					show:true,
					title:'取消屏蔽告警',
					icon:'image://'+g_siteInfo.doc_path+'images/phone_sound.png',
					onclick:function(){}
				},
				myMaskWarningData:{
					show:true,
					title:'屏蔽告警',
					icon:'image://'+g_siteInfo.doc_path+'images/phone_delete.png',
					onclick:function(){}
				},
				mySetWarningData:{
					show:true,
					title:'设置告警',
					icon:'image://'+g_siteInfo.doc_path+'images/phone_edit.png',
					onclick:function(){}
				},
				myShowSingleData:{
					show:true,
					title:'单独显示图表数据',
					icon:'image://'+g_siteInfo.doc_path+'images/arrow_out_longer.png',
					onclick:function(){}
				},
				restore: { 
					show:true,
					title: '还原图表显示'
				},
				dataZoom:{
					show:true,
					xAxisIndex:0,
					yAxisIndex:false
				},
				saveAsImage: { show: true }
			}
		},
		grid: {
			tooltip: {
				show:true,
				trigger:'axis'
			},
			top:70,
			bottom:50
		},
	    xAxis: {
			show:true,
			type: 'time',
			axisPointer: {
			    label: {
			        formatter: function (params) {
			            if(attr_val_list.type != 1)
			                return dmtGetDateStr(params.value);
			            return  dmtGetDateStr(params.value, true);
			        }
			    }
			}
	    },
		yAxis: {
			splitArea: {
				show: true
			},
			show:true,
			axisLabel:{
				formatter: function (value, index) {
				   if(value >= 1073741824)
				   {
					   var f = value/1073741824;
					   return f.toFixed(2) + 'G';
				   }
				   if(value >= 1048576)
				   {
					   var f = value/1048576;
					   return f.toFixed(2) + 'M';
				   }
				   if(value >= 1024)
				   {
					   var f = value/1024;
					   return f.toFixed(2) + 'K';
				   }
				   return value;
		   		}
			},
			show:true,
			type: 'value'
		},
		tooltip: {
			trigger: 'none',
			axisPointer: {
				type: 'cross',
				snap: true
			}
		},
		dataZoom: [
			{
				type:'inside',
	 			xAxisIndex: 0
		  	}
		],
		series: [
			{
				name:'',
				showSymbol:false,
				cursor:'pointer',
				smooth: true,
				smoothMonotone:'x',
				type:'line',
				data:[]
			},
			{
				name:'',
				showSymbol:false,
				cursor:'pointer',
				smooth: true,
				smoothMonotone:'x',
				type:'line',
				data:[]
			},
			{
				name:'',
				showSymbol:false,
				cursor:'pointer',
				smooth: true,
				smoothMonotone:'x',
				type:'line',
				data:[]
			}
		]
	};

	var attr_vals = attr_val_list.list;
	if(typeof attr_vals == "undefined")
		return;

	var jumpid;
	if(showtype == 'view')
		jumpid = attr_val_list.view_id;
	else  if(showtype == 'machine')
		jumpid = attr_val_list.machine_id;

	var dateStart = attr_val_list.date_time_start_utc*1000 - new Date().getTimezoneOffset()*60*1000;
	var count_day = 1;
	if(typeof attr_val_list.date_time_monday != 'undefined')
		count_day = 7;

	if(attr_val_list.type == 1)
	{
		op.legend.show = true;
		op.grid.bottom = 80;
	}

	var bHasData = true;
	for(var i=0; i < attr_vals.length; i++)
	{
		var attr_show_id = "attr_" + attr_vals[i].id + "_" + jumpid + showtype;
		if($('#'+attr_show_id).length > 0)
		{
			dmtJsBugReport('dmt.comm.js', 'dmtShowAttrInfo', 'bug:'+attr_show_id);
			$('#'+attr_show_id).html("");
		}

		var attr_show_container_str = '<div style="width:' + g_dmtChartWidth + 'px; ';
		attr_show_container_str += ' height:' + g_dmtChartHeight + 'px; ';
		attr_show_container_str += ' border:2px solid #335cad; padding-top:5px;';
		attr_show_container_str += ' float:left; margin:' + g_dmtChartMargin + 'px;" ';
		attr_show_container_str += ' type="' + showtype + '" ';
		attr_show_container_str += ' id="' + attr_show_id+ '" ';
		attr_show_container_str += ' type_id="' + jumpid + '" >';
		attr_show_container_str += '</div>';

		$(ct_div).append( $(attr_show_container_str) );

		var attr_info = attr_list.list[i];
		if(attr_info.id != attr_vals[i].id)
			attr_info = dmtGetAttrInfo(attr_list, attr_vals[i].id);

		var time_info = dmtGetXAxisTimeInfo(dateStart, count_day, attr_info);
		if(typeof(g_all_charts[attr_show_id]) != "undefined") 
			g_all_charts[attr_show_id].dispose();

		// 字符串型监控点
		if(attr_info.data_type == STR_REPORT_D_IP) {
			dmtSetStrAttrInfoChartIpGeo(attr_show_id, attr_info, attr_vals[i], attr_val_list, showtype);
			continue;
		}
		else if(attr_info.data_type == STR_REPORT_D) {
			dmtSetStrAttrInfoChart(attr_show_id, attr_info, attr_vals[i], attr_val_list, showtype);
			continue;
		}

		if(typeof attr_vals[i].value_list_str != 'undefined' && attr_vals[i].max > 0) {
			op.series[0].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_str);
			bHasData = true;
		}
		else  {
			op.series[0].data = [];
			bHasData = false;
		}

		if(attr_val_list.type == 1)
		{
			op.series[0].name = '今日 [' + attr_val_list.date_time_cur + ']';
			if(attr_vals[i].value_list_yst_str != "0")
				op.series[1].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_yst_str);
			else
				op.series[1].data = [];
			op.series[1].name = '昨日 [' + attr_val_list.date_time_yst + ']';
			if(attr_vals[i].value_list_lwk_str != '0')
				op.series[2].data = dmtGetYAxisData(time_info, attr_vals[i].value_list_lwk_str);
			else
				op.series[2].data = [];
			op.series[2].name = '上周同日 [' + attr_val_list.date_time_wkd + ']';
			if(attr_vals[i].value_list_yst_str != "0" || attr_vals[i].value_list_lwk_str != '0')
				bHasData = true;
		}

		// 5 -- 历史积累监控点类型
		if(attr_info.data_type == 5)
			op.yAxis.min = attr_vals[i].min;
		else
			op.yAxis.min = 0;

		attr_val_list.show_single_type = attr_val_list.type;
		op.title.text = dmtGetViewTitle(showtype, attr_val_list, attr_info);
		var warnInfo = new Object();
		op.title.subtext = dmtGetViewSubTitle(showtype, attr_val_list, attr_vals[i], attr_info);
		dmtSetCustToolBox(op, showtype, attr_val_list, attr_vals[i], attr_info, warnInfo);

		if(!bHasData) {
			op.title.subtextStyle.color = 'red';
			op.title.top = '20%';
			op.xAxis.show = false;
			op.yAxis.show = false;
			op.toolbox.feature.dataZoom.show = false;
			op.toolbox.feature.myShowSingleData.show  = false;
		}
		else {
			op.title.subtextStyle.color = 'gray';
			op.title.top = 'top';
			op.xAxis.show = true;
			op.yAxis.show = true;
			op.toolbox.feature.dataZoom.show = true;
			op.toolbox.feature.myShowSingleData.show  = true;
		}

		g_all_charts[attr_show_id] = echarts.init(document.getElementById(attr_show_id));
		g_all_charts[attr_show_id].setOption(op);

		// echarts toolbox onclick 函数一旦设置后不能改变，所有将告警相关参数保存到数组对象中
		g_all_charts[attr_show_id].warnInfo = warnInfo;
		g_all_charts[attr_show_id].setwidth = g_dmtChartWidth;
	}
}

function dmtGetSyssetTypeHtml(shtmlid)
{
	var html = '<label>服务器类型：</label>';
	html += '<select name="';
	html += shtmlid + '" id="';
	html += shtmlid + '">" ';
	html += '<option value="0">请选择</option>';
	html += '<option value="1">日志服务器</option>';
	html += '<option value="2">监控点服务器</option>';
	html += '<option value="3">mysql 监控点服务器</option>';
	html += '<option value="4">中心服务器</option>';
	html += '<option value="11">web 服务器</option>';
	html += '</select>';
	g_html_systype = html;
	return g_html_systype;
}

function dmtGetServerTypeName(type_id)
{
	switch(type_id)
	{
		case 1:
			return "日志服务器";
		case 2:
			return "监控点服务器";
		case 3:
			return "mysql 监控点服务器";
		case 4:
			return "中心服务器";
		case 11:
			return "web 服务器";
		default:
			return "未知类型";
	}
}


function dmtTriggerLeftMenu()
{
	if(DWZ.ui.sbar == true)
	{
		$("#sidebar .toggleCollapse div").trigger("click");
		return;
	}
	
	if(DWZ.ui.sbar == false)
	{
		$("#sidebar_s .toggleCollapse div").trigger("click");
		return;
	}
}

function dmtLv2CheckCodeDlgClose()
{
	if(typeof(g_dclc_TimerId) != "undefined" && g_dclc_TimerId != null)
	{
		clearTimeout(g_dclc_TimerId);
		g_dclc_TimerId = null;
	}
	return true;
}

function dmtPopDaemonTipMsg()
{
	if(typeof($.pdialog._current) != "undefined" && $.pdialog._current != null)
	    $.pdialog.closeCurrent();
	var op = { mask:true, maxable:false, height:280, width:410, resizable:false, drawable:true };
	$.pdialog.openLocal('dct_dlg_show_daemon_tip_msg', 'dct_dlg_show_daemon_tip_msg', '演示版操作提示', op);
}

// ajax 返回
// 该函数返回 true 则后续逻辑不处理, 返回 false 继续处理
function dmtFirstDealAjaxResponse(result)
{
	if(typeof(result) == "undefined")
	{
		return false;
	}

	if(result == null || result == 'null' || typeof(result.ec) == "undefined")
		return false;

	if(result.ec == 666) {
		dmtPopDaemonTipMsg();
		return true;
	}

	if(result.ec == 111) {
		navTab.closeCurrentTab(); 
		if(typeof($.pdialog._current) != "undefined" && $.pdialog._current != null)
			$.pdialog.closeCurrent();
		// 登录过期重新登录
		location = result.redirect_url;
		return true;
	}

	if(result.ec == 300  || (result.ec >= 1 && result.ec < 200)) {
		var msg = '服务器返回错误，错误码：';
		msg += result.ec;
		if(typeof(result.msg) != "undefined")
		{
			msg += '，错误消息：';
			msg += result.msg;
		}
		else if(typeof(result.msgid) != "undefined")
		{
			msg += '，错误消息：';
			msg += DWZ.msg(json.msgid);
		}
		alertMsg.warn(msg);
		return true;
	}

	return false;
}

function dmtGetHumanReadDigitByKB(val)
{
	var sp = '';
	if(val >= 1048576){
		var f = val / 1048576;
		sp = Math.round(parseFloat(f)*100)/100 + ' GB';
	}
	else if(val >= 1024) {
		var f = val / 1024;
		sp = Math.round(parseFloat(f)*100)/100 + ' MB';
	}
	else if(typeof val != 'undefined')
		sp = val + ' KB';
	return sp;
}

function dmt_duc_dlg_modify_name_close()
{
	if(typeof g_dci_TimerId != 'undefined' && g_dci_TimerId != null)
	{
		clearTimeout(g_dci_TimerId);
		g_dci_TimerId = null;
	}
	return true;
}

function dmtGetFastWebIdx()
{
	if(typeof r_siteInfo == 'undefined')
		return 0;

	r_siteInfo.web_fast_ms = 24*60*60*1000;
	r_siteInfo.web_slow_ms = 0;

	var ips = r_siteInfo.web_list;
	var fastIdx = ips.length;
	for(var i=0; i < ips.length; i++)
	{
		if(typeof ips[i].end == 'undefined')
			continue;

		var s = ips[i].end.getTime() - ips[i].start.getTime();
		if(s > r_siteInfo.web_slow_ms)
			r_siteInfo.web_slow_ms = s;
		if(s < r_siteInfo.web_fast_ms)
		{
			r_siteInfo.web_fast_ms = s;
			fastIdx = i;
		}
	}
	return fastIdx;
}

function dmtTestWebSvr()
{
	if(typeof r_siteInfo == 'undefined')
		return 0;

	for(var i=0; i < r_siteInfo.web_list.length; i++)
	{
		r_siteInfo.web_list[i].start = new Date();
		var requrl = 'http://' + r_siteInfo.web_list[i].ip + '/cgi-bin/slog_flogin?action=check_web_speed';
		requrl += '&check_idx=' + i;
		$.ajax({
			url:requrl,
			cache:false,
			success:function(result){
				if(result.check_idx >= 0 && result.check_idx < r_siteInfo.web_list.length)
					r_siteInfo.web_list[ result.check_idx ].end = new Date();
			},
			dataType:'json'
		});
	}
}

function dmtGetBytesLength(str)
{
	var total = 0;
	for(var i=0, len = str.length; i < len; i++)
	{
	    charCode = str.charCodeAt(i);
	    if(charCode <= 0x007f) {
	        total += 1;
	    }else if(charCode <= 0x07ff){
	        total += 2;
	    }else if(charCode <= 0xffff){
	        total += 3;
	    }else{
	        total += 4;
	    }
	}
	return total;
}

// ysy add - 2019-01-31
function redrawChartsOnSwitchTab()
{
	var nid = navTab.getCurrentTabId();
	if(nid != null) {
		var type_id;
		var type;
		var isLeftShow;
		if(nid.match('showmachine_')) {
			type = 'machine';
			type_id = nid.split('_')[1];
			if($('#dsmLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else if(nid.match('innersite_')) {
			type = 'site';
			type_id = nid.split('_')[1];
			if($('#dsLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else if(nid.match('showview_')) {
			type = 'view';
			type_id = nid.split('_')[1];
			if($('#dsLeftMenu_'+type_id).css('display') == 'none')
				isLeftShow = false;
			else
				isLeftShow = true;
		}
		else  {
			return;
		}
		dmtSetRedrawChartsInfo(type_id, type, isLeftShow);
		dmtRedrawCharts();
	}
}

// ysy add - 2019-02-08 --- 主页图表大小自适应
function redrawChartsMain()
{	
	// 当前tab 是主页
	if($('#navMenuLast').hasClass('selected')) {
 	   	dcMainPageSetChartSize();
 	   	dcMainPageRedrawChart();
	}
}

// ysy add - 2019-01-30 --- 单机视图图表大小自适应
function redrawCharts()
{
	var nid = navTab.getCurrentTabId();
	if(nid != null) {
		if(nid.match('showmachine_') || nid.match('showview_') || nid.match('innersite_')) {
			dmtRedrawCharts();
		}
		if(nid.match("dmt_plugin_")) {
		    dmtSetPluginMarginInfo(nid);
		}
	}
	else {
		// 主页
		dcOnMainPage('main');
	}
}

