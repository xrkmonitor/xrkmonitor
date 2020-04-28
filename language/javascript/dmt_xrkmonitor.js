var xrk = {
	set_debug: function() { this.debug=1; },
	init_report: function(report_url, log_config_id, uid, ackey) {
		if(typeof report_url == 'undefined' || report_url == '') {
			this.local_msg('xrkmonitor - init failed, invalid report_url !');
			return;
		}
		if(typeof log_config_id == 'undefined' || log_config_id == 0) {
			this.local_msg('xrkmonitor - init failed, invalid log_config_id');
			return;
		}
		this.report_url = report_url;
		this.log_config_id = log_config_id;
		if(typeof uid != 'undefined')
			this.master_user_id = uid;
		if(typeof ackey != 'undefined')
			this.access_key = ackey;
		this.debug ? this.local_msg('xrkmonitor - init ok, report_url:'+report_url+', config id:'+log_config_id) : '';
	},
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
			this.local_msg('xrkmonitor - report failed, need call init_report first!');
			return;
		}
		if(typeof delay == 'undefined' || delay <= 0) {
			if(this.last_delay_timer_id != 0) {
				clearTimeout(this.last_delay_timer_id);
				this.last_delay_timer_id = 0;
			}
			return this._report_to_server();
		}

		if(this.last_delay_timer_id != 0)
			return;
		if(delay < 10)
			delay = 10;
		else if(delay > 60000)
			delay = 60000;
		this.last_delay_timer_id = setTimeout(this._report_to_server, delay);
		this.debug ? this.local_msg('set delay report to server, delay:' + delay + ' ms') : '';
	},
	reg_on_response: function(fun) {
		if(typeof fun === 'function')
			this.on_xrk_response = fun;
	},
	local_msg: function(msg) {
		if(this.debug) {
			if (typeof(console) != "undefined") 
				console.log(msg);
			else 
				alert(msg);
		}
	},


	_report_log: function(log_type, msg) {
		if(log_type != 'error' && log_type != 'warn' 
			&& log_type != 'reqerr' && log_type != 'info' && log_type != 'debug') 
		{
			this.local_msg('xrkmonitor - report failed, invalid log type(must - error|warn|reqerr|info|debug!');
			return;
		}
		this.debug ? this.local_msg('report ' + log_type + ' log:' + msg) : '';

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
	}, 

	_report_to_server: function() { 
		if(this.last_delay_timer_id != 0) {
			clearTimeout(this.last_delay_timer_id);
			this.last_delay_timer_id = 0;
		}

		var need_report = false;
		var reports = new Object();
		if(this.logs.length > 0) {
			reports.log_config_id = this.log_config_id;
			reports.logs = this.logs;
			need_report = true;
			this.debug ? this.local_msg('report log count:' + reports.logs.length) : '';
		}
		if(this.attrs.length > 0) {
			reports.attrs = this.attrs;
			need_report = true;
			this.debug ? this.local_msg('report attr count:' + reports.attrs.length) : '';
		}
		if(this.strattrs.length > 0) {
			reports.strattrs = this.strattrs;
			need_report = true;
			this.debug ? this.local_msg('report strattr count:' + reports.strattrs.length) : '';
		}

		if(this.master_user_id != 0)
			reports.master_user_id = this.master_user_id;
		if(this.access_key != '')
			reports.access_key = this.access_key;
		this.cur_strattrs_strlen = 0;
		this.cur_logs_strlen = 0;
		if(!need_report)
			return;

		if(typeof JSON === 'object' && JSON.stringify) {
			var strrep = JSON.stringify(reports);
			this._to_server(strrep);
			this.logs = new Array();
			this.attrs = new Array();
			this.strattrs = new Array();
		}
		else {
			this.local_msg('xrkmonitor - report failed, not support JSON.stringify !');
		}
	},

	_inner_xrk_report_attr: function(opr, id, number) {
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
		if(typeof JSON === 'object' && JSON.parse) {
			var t_end = Date.parse(new Date());
			var ret = JSON.parse(retmsg);
			this.last_net_delay = t_end-this.request_start_time-ret.cgi_run_time;
			if(this.last_net_delay > this.max_net_delay)
				this.max_net_delay = this.last_net_delay;
			else if(this.last_net_delay < 0)
				this.last_net_delay = 0;
			this.local_out_ip = ret.client_ip;
			this.debug ? this.local_msg('net delay:'+this.last_net_delay+', max delay:'+this.max_net_delay+
				', cgi run:'+ret.cgi_run_time+', ip:' + this.local_out_ip) : '';
		}

		if(this.debug) {
			var msg = 'response:' + retmsg + ', last request seq:' + this.last_req_seq;
			this.local_msg(msg);
		}

		if(typeof this.on_xrk_response === 'function')
			this.on_xrk_response();
	},
	_to_server: function (data) {
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
					this.local_msg('xrkmonitor - report failed, not support xmHttp request !');
					return false;
				}
			}
		};

		this.request_start_time = Date.parse(new Date());

		// 频繁调用检测 500ms 内不得超过3次
		if(this.freq_send_start_time == 0 || this.request_start_time > this.freq_send_start_time+500) {
			this.freq_send_start_time = this.request_start_time;
			this.freq_request_times = 1;
		}
		else if(this.request_start_time < this.freq_send_start_time+500 && this.freq_request_times >= 3) {
			this.last_delay_timer_id = setTimeout(this._report_to_server, 500);
			return;
		}
		else
			this.freq_request_times++;

		this.last_req_seq = this.request_start_time%120000;
		var reports = 'action=http_report_data&';
		reports += 'req_seq=' + this.last_req_seq;
		reports += '&data=' + encodeURI(data);
		if(this.debug) {
			this.local_msg('send request to server data length:' + reports.length + ', req seq:' + this.last_req_seq);
		}
		xmlHttp.open('POST', this.report_url, true);
		xmlHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded;charset=utf-8');
		xmlHttp.send(reports);
		xmlHttp.onreadystatechange = function () {
		    if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
				if(typeof xmlHttp.responseText != 'undefined')
					xrk._on_response(xmlHttp.responseText);
		    }
		}; 
	},

	debug:0, 
	log_config_id:0, // 日志配置 id, 初始化接口指定
	master_user_id:0, // 主账号id, 用于云版本
	access_key:'', // 授权访问 key, 用于云版本
	report_url:'', // 监控系统 fcgi - mt_slog_reportinfo 的调用地址
	last_delay_timer_id: 0, // 延迟上报定时器id - 请勿修改
	last_req_seq:0, // 用于请求响应包校验 - 请勿修改
	max_attrs_per_req:120, // 每次最多上传的监控点数目 - 请勿修改
	max_strattrs_strlen:1000, // 每次最多上报的字符串监控点长度 - 请勿修改
	cur_strattrs_strlen:0, // 当前累计的待上报字符串长度 - 请勿修改
	max_logs_strlen:900, // 每次最多上报的日志长度 - 请勿修改
	cur_logs_strlen:0, // 当前累计的待上报日志长度 - 请勿修改
	request_start_time:0, // 请求开始时间 - 请勿修改
	max_net_delay:0, // 根据请求响应信息计算得出的网络延迟 ms - 请勿修改
	last_net_delay:0, // 最近一次调用的网络延迟 ms - 请勿修改
	local_out_ip:'', // 从服务器响应中获取到的外网 ip - 请勿修改
	freq_send_start_time:0, // 用于避免频繁访问 - 请勿修改
	freq_request_times:0, // 用于避免频繁访问 - 请勿修改
	on_xrk_response:null, // 数据上报响应回调函数, 可通过 reg_on_response 设置
	logs:new Array(),
	attrs:new Array(),
	strattrs:new Array()
};

