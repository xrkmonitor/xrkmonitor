/*** xrkmonitor license ***

   Copyright (c) 2020 by rockdeng

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

   开源版主页：http://open.xrkmonitor.com
   云版本主页：http://xrkmonitor.com
  

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

   javascript 监控插件 monitor_website:
   		监控并上报网站用户的基本信息包括浏览器类型、操作系统类型、地理位置等，
	并可上报网站 js 错误信息、img 加载失败信息，方便对不同浏览器做兼容。使用
	方法：下载部署配置文件, 将该文件以及配置文件放入 js api 所在目录，网站入
	口出引入 js api 文件即可，详情请查看在线文档.

****/


if(typeof window.self_xrk == 'undefined') {
	// 参数为 true 表示开启 debug 模式使用 console 对象输出详细日志
	//Xrkmonitor(true);
	Xrkmonitor(false);
}

// 非插件模式运行时, 在这里修改配置
// 加载插件模式运行, 脚本会自动从插件配置中读取, 无需在这里指定配置
function Xrkmonitor_config() {
	var x_config =  {
		// 监控系统 fcgi - mt_slog_reportinfo 的调用地址, 注意开源版需要改为自己的地址
		report_url:'http://xrkmonitor.com/cgi-bin/mt_slog_reportinfo',

		// 需要加载的插件列表, 插件和插件配置文件需和当前脚本放相同目录
		plugins_ary: ['monitor_website', ''],

		// 当前文件名, 如修改了文件名这里也需要相应修改
		js_self_name: 'dmt_xrkmonitor.js', 

		// json2.js 的压缩文件, 需跟当前文件放相同目录下
		json2_js_name: 'dmt_json2_min.js', 
	
		// 监控 api 的日志配置id, 如插件需上报日志需需插件实现日志接口，可参考 : monitor_website
		log_config_id:0, 

		// 主账号id -- 用于云版本
		master_user_id:0, 

		// 授权访问 key ---  用于云版本
		access_key:'' 
	};
	return x_config;
}

function Xrkmonitor(b_debug) {
	var x_config = Xrkmonitor_config();
	var xrk = Xrkmonitor_obj();
	xrk.report_url = x_config.report_url;
	xrk.json2_js_name = x_config.json2_js_name;
	xrk.js_self_name = x_config.js_self_name;
	xrk.log_config_id = x_config.log_config_id;
	xrk.master_user_id = x_config.master_user_id;
	xrk.access_key = x_config.access_key;
	xrk.plugins_ary = x_config.plugins_ary;

	try {
		// 首次加载监控脚本
		if(typeof(window.top.self_xrk) == 'undefined' || !window.top.self_xrk) {
			window.top.self_xrk = xrk;
			window.self_xrk = xrk;
			xrk._init_start(b_debug);
		}
		else {
			// 同域下的 window 共用同一个: xrk 对象
			window.self_xrk = window.top.self_xrk;
			window.self_xrk.load_more++;
			window.self_xrk.plugins_add_monitor();
		}
	}catch(e) {
		// 跨域使用新的 xrk 对象
		window.self_xrk = xrk;
		xrk._init_more(b_debug);
		xrk.local_msg('(api) get exception:' + e.message); 
	}
}

function Xrkmonitor_obj() {
	var xrk = {
		report_attr_add: function(id, number) {
			return this._inner_xrk_report_attr('add', id, number); 
		},
		report_attr_set: function(id, number) {
			return this._inner_xrk_report_attr('set', id, number);
		},
		report_strattr_add: function(id, string, number) {
			return this._inner_xrk_report_strattr('add', id, string, number); 
		},
		report_strattr_set: function(id, string, number) {
			return this._inner_xrk_report_strattr('set', id, string, number);
		},
		init_log: function(log_config_id) {
			if(typeof log_config_id == 'undefined' || log_config_id == 0) {
				this.local_msg('xrkmonitor - init failed, invalid log_config_id');
				return;
			}
			this.log_config_id = log_config_id;
		},
		set_debug: function(b_debug) { (typeof b_debug != 'undefined') ? this.debug=b_debug : this.debug=true; },
		open_init_report: function(log_id, rurl) {
			if(typeof rurl != 'undefined' && this.set_report_url == 0)
				this.report_url = rurl;
			if(typeof log_id != 'undefined')
				this.log_config_id = log_id;
		},
		cloud_init_report: function(log_id, uid, ackey) {
			if(typeof uid != 'undefined')
				this.master_user_id = uid;
			if(typeof ackey != 'undefined')
				this.access_key = ackey;
			if(typeof log_id != 'undefined')
				this.log_config_id = log_id;
		},
		report_error_log: function(msg, pos) { 
			if(typeof pos != 'undefined')
				this._report_log('error', pos + msg);
			else
				this._report_log('error', '[js:log:35]' + msg);
		},
		report_warn_log: function(msg, pos) { 
			if(typeof pos != 'undefined')
				this._report_log('warn', msg); 
			else
				this._report_log('warn', '[js:log:41]' + msg);
		},
		report_reqerr_log: function(msg, pos) {
			if(typeof pos != 'undefined')
				this._report_log('reqerr', msg); 
			else
				this._report_log('reqerr', '[js:log:47]' + msg);
		},
		report_info_log: function(msg, pos) { 
			if(typeof pos != 'undefined')
				this._report_log('info', msg); 
			else
				this._report_log('info', '[js:log:53]' + msg);
		},
		report_debug_log: function(msg, pos) { 
			if(typeof pos != 'undefined')
				this._report_log('debug', msg); 
			else
				this._report_log('debug', '[js:log:59]' + msg);
		},
		report_to_server: function(delay) { 
			if(this.report_url == '') {
				this.local_msg('xrkmonitor - report failed, request url not set !');
				return;
			}
			if(typeof delay == 'undefined' || delay <= 0) {
				// 用 request seq 防止并发调用 api 接口
				if(this.last_req_seq == 0) {
					if(this.last_delay_timer_id != 0) {
						try { clearTimeout(this.last_delay_timer_id); }catch(e) {
							this.local_msg('(api - report_to_server) get exception:' + e.message);
						}
						this.last_delay_timer_id = 0;
					}
					return this._report_to_server();
				}
				delay = 50;
			}
	
			if(this.last_delay_timer_id != 0)
				return;
			if(delay < 10)
				delay = 10;
			else if(delay >= this.try_timer_ms)
				delay = this.try_timer_ms;
			this._report_to_server(delay);
		},
		reg_on_response: function(fun) {
			this.on_xrk_response = fun;
		},
		local_msg: function(msg) {
			if(this.debug) {
				var pre = '' + new Date().getTime() + ' : xrkmonitor - ' + msg;
				if (typeof(console) != "undefined") 
					console.log(pre);
			}
		},
		_try_set_config: function(xrk_config) {
			var xrk_self = this;
			if(typeof(xrk_config.report_url) === 'string' && xrk_self.set_report_url == 0) {
				xrk_self.report_url = xrk_config.report_url;
				xrk_self.set_report_url = 1;
				xrk_self.local_msg('set new report url address:' + xrk_self.report_url);
			}
			if(typeof xrk_config.cloud_master_user === 'number' && typeof xrk_config.cloud_access_key==='string'
				&& xrk_self.master_user_id == 0) 
			{
				xrk_self.master_user_id = xrk_config.cloud_master_user;
				xrk_self.access_key = xrk_config.cloud_access_key;
				xrk_self.local_msg('cloud init - user:'+xrk_self.master_user_id+', key:'+xrk_self.access_key);
			}
		},
		load_script: function(url, callback) {
			try {
			var script = document.createElement("script");
			script.type = "text/javascript";
			if (typeof(script.readyState) != 'undefined') {
				script.onreadystatechange = function() {
					if (script.readyState == "loaded" || script.readyState == "complete" || script.readyState == "done"){
						script.onreadystatechange = null;
						if(typeof callback === 'function')
							callback(); 
					}
				};
			} else { 
				script.onload = function() { 
					if(typeof callback === 'function') 
						callback(); 
				};
			}
			script.src = url;
			document.getElementsByTagName("head")[0].appendChild(script); 
			}catch(e) { this.local_msg('(api - load_script) get exception:' + e.message); }
		},

		_load_plugin: function(plugin_name) {
			var plugin_js = plugin_name + '.js'
			var config_js = plugin_name + '_conf.js';
			var xrk_self = this;
			this.load_script(this.api_js_url.replace(this.js_self_name, config_js), function() {
				var xrk_config = eval('xrk_config_' + plugin_name);
				if(typeof xrk_config === 'object' && xrk_config.plugin_name == plugin_name) {
					xrk_self.debug ? xrk_self.local_msg('load plugin:' + plugin_name + ' config file:' + config_js + ' ok'): '';
					xrk_self.load_script(xrk_self.api_js_url.replace(xrk_self.js_self_name, plugin_js), function() {
							var xrk_monitor = eval('xrk_monitor_'+plugin_name+'()');
							if(typeof xrk_monitor === 'object' && xrk_monitor.plugin_name == plugin_name) {
								xrk_self.debug ? xrk_self.local_msg('load plugin:' + plugin_name + ' ok') : '';
								xrk_self._try_set_config(xrk_config);

								var o_plugin = new Object();
								o_plugin.create = eval('xrk_monitor_'+plugin_name);
								o_plugin.config = xrk_config;
								o_plugin.monitor_win_list = new Array();
								o_plugin.logs = new Array();
								o_plugin.cur_logs_strlen = 0;
								xrk_monitor.monitor_window(o_plugin, window);
								o_plugin.monitor_win_list.push(xrk_monitor);
								xrk_self.plugins.push(o_plugin);
								xrk_self.debug ? xrk_self.local_msg('plugin count:' + xrk_self.plugins.length) : '';
							}
							else
								xrk_self.debug ? xrk_self.local_msg('load plugin:' + plugin_name + ' js failed !') : '';
						});
				}
				else {
					this.debug ? this.local_msg('load plugin:' + plugin_name + ' config:' + plugin_js + ' failed !'): '';
				}
			});
		},
		is_xrkmonitor_js : function(js_file) {
			var p_self = this;
			if(js_file.indexOf(p_self.json2_js_name) != -1
				|| js_file.indexOf(p_self.js_self_name) != -1) {
				return true;
			}
			for(var i=0; i < p_self.plugins.length; i++) {
				if(js_file.indexOf(p_self.plugins[i].config.plugin_name+'.js') != -1
					|| js_file.indexOf(p_self.plugins[i].config.plugin_name+'_conf.js') != -1)
				{
					return true;
				}
			}
			return false;
		},
		get_cur_js_url: function(curJsName) {
			if(typeof curJsName == 'undefined')
				curJsName = this.js_self_name;
			var xrk_api_js_url = '';
			var stack = '';
			var rExtractUri = /((?:http|https|file):\/\/.*?\/[^:]+)(?::\d+)?:\d+/;
			try {
				xrk_api_js_url = document.currentScript.src;
			}catch(e) {
				this.local_msg('(api- get_cur_js_url) get exception:' + e.message);
				stack = e.fileName || e.stack || e.sourceURL || e.stacktrace || '';
			}
	
			try {
			if(stack != '') 
				xrk_api_js_url = rExtractUri.exec(stack)[1];
			if(!xrk_api_js_url || xrk_api_js_url.indexOf(curJsName) < 0) {
				var scripts = document.scripts;
				for (var i = scripts.length-1; i >= 0; i--) {
					if (scripts[i].src.indexOf(this.js_self_name) >= 0) {
						xrk_api_js_url = scripts[i].src;
						return xrk_api_js_url;
					}
				}
				this.local_msg('(api- get_cur_js_url) - api js url get failed !');
				return '';
			}
			}catch(e) { this.local_msg('(api- get_cur_js_url-2) get exception:' + e.message); }
			return xrk_api_js_url;
		},
	
		_report_log: function(log_type, msg) {
			if(this.log_config_id == 0) {
				this.debug ? this.local_msg('log config id not set, use init_log() to set, msg:' + msg) : '';
				return;
			}
			if(log_type != 'error' && log_type != 'warn' 
				&& log_type != 'reqerr' && log_type != 'info' && log_type != 'debug') 
			{
				this.local_msg('xrkmonitor - report failed, invalid log type(must - error|warn|reqerr|info|debug!');
				return;
			}
			this.debug ? this.local_msg('report ' + log_type + ' log:' + msg) : '';
	
			try {
			var log = new Object();
			log.type = log_type;
			log.msg = msg;
			log.time = Date.parse(new Date());
			this.logs.push(log);
			this.cur_logs_strlen += msg.length;
			if(this.cur_logs_strlen >= this.max_logs_strlen) {
				if(this.last_delay_timer_id != 0) {
					clearTimeout(this.last_delay_timer_id);
					this.last_delay_timer_id = 0;
				}
				this._report_to_server();
			}
			}catch(e) { this.local_msg('(api - _report_log) get exception:' + e.message); }
		}, 
		_try_report_to_server: function() {
			var p_self = this;
			var i=0, j=0;
			if(p_self.debug) {
				var d = new Date().getTime();
				if(d >= p_self.last_show_try_info_time+30000) {
					p_self.last_show_try_info_time = d;
					p_self.local_msg('domain:' + p_self.init_win_domain 
						+ ', try send data to server, new request seq:' + p_self.req_seq); 
					for(i=0; i < p_self.plugins.length; i++) {
						p_self.local_msg('plugin:' + p_self.plugins[i].config.plugin_name + ' monitor window count:'
							+ p_self.plugins[i].monitor_win_list.length);
					}
				}
			}
			for(i=0; i < p_self.plugins.length; i++) {
				for(j=0; j < p_self.plugins[i].monitor_win_list.length; j++) {
					if(p_self.plugins[i].monitor_win_list[j].b_exception) {
						p_self.local_msg('remove window from plugin:' + p_self.plugins[i].config.plugin_name
							+ ', window id:' + p_self.plugins[i].monitor_win_list[j].xrk_window_id);
						p_self.plugins[i].monitor_win_list[j] = null;
						p_self.plugins[i].monitor_win_list.splice(j, 1);
						break;
					}
				}
			}

			p_self.report_to_server();
			setTimeout(function(){ p_self._try_report_to_server(); }, p_self.try_timer_ms);
		},

		_report_to_server: function(delay) { 
			var p_self = this;
			if(typeof delay != 'undefined' || p_self.last_req_seq != 0) {
				if(p_self.last_delay_timer_id == 0) {
					if(typeof delay == 'undefined')
						delay = 100;
					p_self.last_delay_timer_id = setTimeout(function(){
							p_self.last_delay_timer_id = 0;
							if(p_self.request_start_time > 0 
								&& p_self.request_start_time+p_self.request_timeout_ms<= new Date().getTime()) {
								p_self.last_req_seq = 0;
								p_self.local_msg('clear delay and request seq check, response may timeout');
							}
							p_self._report_to_server();
						}, delay);
				}
				if(p_self.debug) {
					var msg = 'set delay report to server, delay:' + delay + ' ms' ;
					msg += ', timer id:' + p_self.last_delay_timer_id;
					p_self.local_msg(msg);
				}
				return;
			}
	
			if(p_self.last_delay_timer_id != 0) {
				clearTimeout(p_self.last_delay_timer_id);
				p_self.last_delay_timer_id = 0;
			}
	
			var need_report = false;
			var reports = new Object();
			var log_plugin_obj = null;
			if(p_self.logs.length > 0) {
				reports.log_config_id = p_self.log_config_id;
				reports.logs = p_self.logs;
				need_report = true;
				p_self.debug ? p_self.local_msg('report log count:' + reports.logs.length) : '';
			}
			else {
				for(var i=0; i < p_self.plugins.length; i++) { 
					if(typeof(p_self.plugins[i].logs) == 'undefined' || p_self.plugins[i].logs.length <= 0)
						continue;
	
					log_plugin_obj = p_self.plugins[i];
					reports.log_config_id = log_plugin_obj.config.logconfig_id;
					reports.logs = log_plugin_obj.logs;
					need_report = true;
					p_self.debug ? p_self.local_msg('report plugin:' 
						+ log_plugin_obj.config.plugin_name + ' log count:' + log_plugin_obj.logs.length) : '';
					break;
				}
			}
	
			if(p_self.attrs.length > 0) {
				reports.attrs = p_self.attrs;
				need_report = true;
				p_self.debug ? p_self.local_msg('report attr count:' + reports.attrs.length) : '';
			}
			if(p_self.strattrs.length > 0) {
				reports.strattrs = p_self.strattrs;
				need_report = true;
				p_self.debug ? p_self.local_msg('report strattr count:' + reports.strattrs.length) : '';
			}
	
			if(p_self.master_user_id != 0)
				reports.master_user_id = p_self.master_user_id;
			if(p_self.access_key != '')
				reports.access_key = p_self.access_key;
			p_self.cur_strattrs_strlen = 0;
			p_self.cur_logs_strlen = 0;
			if(!need_report)
				return;
	
			if(typeof JSON === 'object' && JSON.stringify) {
				var strrep = JSON.stringify(reports);
				if(p_self._to_server(strrep)) {
					p_self.logs = new Array();
					p_self.attrs = new Array();
					p_self.strattrs = new Array();
					if(log_plugin_obj != null) {
						log_plugin_obj.logs = new Array();
						log_plugin_obj.cur_logs_strlen = 0;
					}
				}
			}
			else {
				p_self.local_msg('xrkmonitor - report failed, not support JSON.stringify !');
			}
		},
	
		_inner_xrk_report_attr: function(opr, id, number) {
			if(typeof id == 'undefined' || typeof number != 'number' || number <= 0) {
				this.debug ? this.local_msg('invalid paramter, on attr report !') : '';
				return;
			}
	
			this.debug ? this.local_msg(opr + ' attr:' + id + ', value:' + number) : '';
			var i=0;
			for(i=0; i < this.attrs.length; i++) {
				if(this.attrs[i].id == id) {
					if(opr == 'set')
						this.attrs[i].number = number;
					else
						this.attrs[i].number += number;
					return;
				}
			}
			var rinfo = new Object();
			rinfo.id = id;
			rinfo.number = number;
			this.attrs.push(rinfo);
	
			if(this.attrs.length >= this.max_attrs_per_req) {
				if(this.last_delay_timer_id != 0) {
					clearTimeout(this.last_delay_timer_id);
				}
				this._report_to_server();
			}
		},
		_inner_xrk_report_strattr: function(opr, id, string, number) {
			if(typeof id == 'undefined' || typeof string != 'string' || typeof number != 'number' || number <= 0) {
				this.debug ? this.local_msg('invalid paramter, on str attr report !') : '';
				return;
			}
	
			this.debug ? this.local_msg(opr + ' strattr:' + id + ', string:' + string + ', value:' + number) : '';
			var i=0;
			for(i=0; i < this.strattrs.length; i++) {
				if(this.strattrs[i].id == id) {
					for(var j=0; j < this.strattrs[i].strings.length; j++) {
						if(this.strattrs[i].strings[j].string == string) {
							if(opr == 'set')
								this.strattrs[i].strings[j].number = number;
							else
								this.strattrs[i].strings[j].number += number;
							return;
						}
					}
					var strinfo = new Object();
					if(string.length > 50)
						strinfo.string = string.substring(0, 50);
					else
						strinfo.string = string;
					strinfo.number = number;
					this.strattrs[i].strings.push(strinfo);
	
					this.cur_strattrs_strlen += strinfo.string.length;
					if(this.cur_strattrs_strlen >= this.max_strattrs_strlen) {
						if(this.last_delay_timer_id != 0) {
							clearTimeout(this.last_delay_timer_id);
						}
						this._report_to_server();
					}
					return;
				}
			}
		
			var rinfo = new Object();
			rinfo.id = id;
			rinfo.strings = new Array();
			var strinfo = new Object();
			if(string.length > 50)
				strinfo.string = string.substring(0, 50);
			else
				strinfo.string = string;
			strinfo.number = number;
			rinfo.strings.push(strinfo);
			this.strattrs.push(rinfo);
	
			this.cur_strattrs_strlen += strinfo.string.length;
			if(this.cur_strattrs_strlen >= this.max_strattrs_strlen) {
				if(this.last_delay_timer_id != 0) {
					clearTimeout(this.last_delay_timer_id);
				}
				this._report_to_server();
			}
		},
	
		_on_response: function(retmsg) {
			var t_end = new Date().getTime();
			this.last_api_delay = t_end-this.request_start_time;
			if(this.last_api_delay > this.max_api_delay)
				this.max_api_delay = this.last_api_delay;
			if(typeof JSON === 'object' && JSON.parse) {
				var ret = JSON.parse(retmsg);
				this.local_out_ip = ret.client_ip;
				if(ret.cgi_run_time > this.max_api_run)
					this.max_api_run = ret.cgi_run_time;
				this.debug ? this.local_msg('last delay:'+this.last_api_delay+', max delay:'+this.max_api_delay+
					', cgi run:'+ret.cgi_run_time+', ip:' + this.local_out_ip + ', end:' + t_end + 
					', start:' + this.request_start_time) : '';
			}
	
			if(this.debug) {
				var msg = 'response:' + retmsg + ', last request seq:' + this.last_req_seq;
				this.local_msg(msg);
			}
	
			this.last_req_seq = 0;
			if(typeof (this.on_xrk_response) === 'function')
				this.on_xrk_response();
		},
		_to_server: function (data) {
			var p_self = this;
			var xmlHttp;
			try {
				xmlHttp = new XMLHttpRequest();
			} 
			catch (e) {
				try {
					xmlHttp = new ActiveXObject("Msxml2.XMLHTTP");
				} catch (e) {
					try {
						xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
					} catch (e) {
						p_self.local_msg('xrkmonitor - report failed, not support xmHttp request !');
						return false;
					}
				}
			};
	
			p_self.request_start_time = new Date().getTime();
	
			// 频繁调用检测 500ms 内不得超过3次
			if(p_self.freq_send_start_time == 0 
				|| p_self.request_start_time > p_self.freq_send_start_time+p_self.freq_check_time) {
				p_self.freq_send_start_time = p_self.request_start_time;
				p_self.freq_request_times = 1;
			}
			else if(p_self.request_start_time < p_self.freq_send_start_time+p_self.freq_check_time 
				&& p_self.freq_request_times >= p_self.freq_check_limit_times) {
				p_self.last_delay_timer_id = setTimeout(function() {
						p_self.self_xrk.last_delay_timer_id = 0;
						p_self.self_xrk._report_to_server();
					}, 500);
				return false;
			}
			else
				p_self.freq_request_times++;
	
			p_self.last_req_seq = p_self.req_seq;
			p_self.req_seq++;
			var reports = 'action=http_report_data&';
			reports += 'req_seq=' + p_self.last_req_seq;
			reports += '&data=' + data;
			if(p_self.debug) {
				p_self.local_msg('send request to server data length:' + reports.length + ', req seq:' + p_self.last_req_seq);
			}
			try {
			xmlHttp.open('POST', p_self.report_url, true);
			xmlHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded;charset=utf-8');
			xmlHttp.send(reports);
			xmlHttp.onreadystatechange = function () {
			    if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
					if(typeof(xmlHttp.responseText) == 'string')
						p_self._on_response(xmlHttp.responseText);
					else {
						p_self._on_response('{ "client_ip":"error!", "cgi_run_time":0}'); 
					}
			    }
			}; 
			}catch(e) { p_self.local_msg('(api - _to_server) get exception:' + e.message); }
			return true;
		},

		_init: function() {
			var p_self = this;
		  try {
			if(typeof(window.document.domain) == 'string')
				p_self.init_win_domain = window.document.domain;
			p_self.api_js_url = p_self.get_cur_js_url();
			if(p_self.api_js_url == '') {
				p_self.local_msg('get :' + p_self.js_self_name + ' url path failed !');
				return ;
			}
			p_self.debug ? p_self.local_msg('get js:' + p_self.js_self_name + ', url:' + p_self.api_js_url) : '';
			p_self.debug ? p_self.local_msg('domain:' + p_self.init_win_domain + ', report url:' + p_self.report_url) : '';
			p_self.req_seq++;
			if(typeof JSON === 'undefined' || !JSON.stringify) {
				p_self.load_script(p_self.api_js_url.replace(p_self.js_self_name, p_self.json2_js_name), 
					function() {
						if(typeof JSON != 'undefined' && JSON.stringify) {
							p_self.local_msg('load json2:' + p_self.json2_js_name + ' ok');
							p_self._add_plugins();
							setTimeout(function(){ p_self._try_report_to_server(); }, p_self.try_timer_ms);
						}
						else
							p_self.local_msg('load json2:' + p_self.json2_js_name + ' failed !');
					}
				);
			}
			else {
				p_self._add_plugins();
				setTimeout(function(){ p_self._try_report_to_server(); }, p_self.try_timer_ms); 
			}
		  }catch(e) { 
			  p_self.local_msg('(api-init) get exception:' + e.message); 
		  }
		},

		_init_more: function(b_debug) { 
			this.debug = b_debug;
			this.load_more++;
			this._init();
		},

		plugins_add_monitor: function() {
			var p_self = this;
			for(var i=0; i < p_self.plugins.length; i++) {
				var xrk_monitor = p_self.plugins[i].create();
				xrk_monitor.monitor_window(p_self.plugins[i], window);
				p_self.plugins[i].monitor_win_list.push(xrk_monitor);
			}
		},

		_init_start: function(b_debug) {
			this.debug = b_debug;
			this._init();
			this.local_msg('api first load - domain:' + this.init_win_domain);
		},
		_add_plugins: function() {
			var p_self = this;
			for(var i=0; i < p_self.plugins_ary.length; i++) {
				if(p_self.plugins_ary[i] != '')
					p_self._load_plugin(p_self.plugins_ary[i]);
			}
		},

		report_url:'',
		json2_js_name: '', 
		js_self_name: '', 
		log_config_id:0, 
		master_user_id:0, 
		access_key:'', 
		on_xrk_response:null, // 数据上报响应回调函数, 可通过 reg_on_response 设置
		debug:0,  
		try_timer_ms:1200, // 尝试发送数据的定时器时间间隔
		last_show_try_info_time: 0, // 定时器终端日志显示控制 - 请勿修改
		last_delay_timer_id: 0, // 延迟上报定时器id - 请勿修改
		last_req_seq:0, // 用于请求响应包校验 - 请勿修改
		req_seq:0, // 用于请求响应包校验 - 请勿修改
		max_attrs_per_req:120, // 每次最多上传的监控点数目 - 请勿修改
		max_strattrs_strlen:1000, // 每次最多上报的字符串监控点长度 - 请勿修改
		cur_strattrs_strlen:0, // 当前累计的待上报字符串长度 - 请勿修改
		max_logs_strlen:900, // 每次最多上报的日志长度 - 请勿修改
		cur_logs_strlen:0, // 当前累计的待上报日志长度 - 请勿修改
		request_start_time:0, // 请求开始时间 - 请勿修改
		max_api_delay:0, // api 调用耗时 ms - 请勿修改
		last_api_delay:0, // 最近一次耗时 ms - 请勿修改
		max_api_run:0, // api 处理耗时 ms - 请勿修改
		local_out_ip:'', // 从服务器响应中获取到的外网 ip - 请勿修改
		freq_send_start_time:0, // 用于避免频繁访问 - 请勿修改
		freq_request_times:0, // 用于避免频繁访问 - 请勿修改
		api_init_time:0, // js api 文件加载时间 - 请勿修改
		api_js_url:'', // 当前js 文件的url 路径 - 请勿修改
		load_more:0, // 是否多次加载 - 请勿修改
		init_win_domain:'', // 调用初始化函数的窗口的域 - 请勿修改
		set_report_url:0, // 上报地址是否从配置文件设置
		freq_check_time:1500, // 频繁调用检测周期
		freq_check_limit_times:3, // 频繁调用检测周期内最大请求次数
		request_timeout_ms: 8000, // 数据上报等待响应超时时间
		plugins_ary: new Array(), 
		plugins: new Array(), // 监控插件
		logs:new Array(),
		attrs:new Array(),
		strattrs:new Array()
	};
	return xrk;
}

