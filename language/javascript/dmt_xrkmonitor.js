var xrk = {
	init_report: function(report_url, log_config_id, uid) {
		if(typeof report_url == 'undefined' || report_url == '') {
			alert('xrkmonitor - init failed, invalid report_url !');
			return;
		}
		if(typeof log_config_id == 'undefined' || log_config_id == 0) {
			alert('xrkmonitor - init failed, invalid log_config_id');
			return;
		}
		this.report_url = report_url;
		this.log_config_id = log_config_id;
		if(typeof uid != 'undefined')
			this.master_user_id = uid;
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
	report_log: function(log_type, msg) {
		if(log_type != 'error' && log_type != 'warn' 
			&& log_type != 'reqerr' && log_type != 'info' && log_type != 'debug') 
		{
			alert('xrkmonitor - report failed, invalid log type(must - error|warn|reqerr|info|debug!');
			return;
		}

		var log = new Object();
		log.type = log_type;
		log.msg = msg;
		log.time = Date.parse(new Date());
		this.logs.push(log);
		this.cur_logs_strlen += msg.length;
		if(this.cur_logs_strlen >= this.max_logs_strlen) {
			if(this.last_delay_timer_id != 0) {
				clearTimeout(this.last_delay_timer_id);
			}
			this._report_to_server();
		}
	}, 
	report_to_server: function(delay) { 
		if(this.report_url == '') {
			alert('xrkmonitor - report failed, need call init_report first!');
			return;
		}

		if(this.last_delay_timer_id != 0)
			return;
		else if(typeof delay == 'undefined' || delay <= 0)
			return this._report_to_server();

		if(delay < 10)
			delay = 10;
		else if(delay > 60000)
			delay = 60000;
		this.last_delay_timer_id = setTimeout(this._report_to_server, delay);
	},


	_report_to_server: function() { 
		this.last_delay_timer_id = 0;
		var reports = new Object();
		if(this.logs.length > 0) {
			reports.log_config_id = this.log_config_id;
			reports.logs = this.logs;
		}
		if(this.attrs.length > 0)
			reports.attrs = this.attrs;
		if(this.strattrs.length > 0)
			reports.strattrs = this.strattrs;

		reports.master_user_id = this.master_user_id;
		this.cur_strattrs_strlen = 0;
		this.cur_logs_strlen = 0;

		if(typeof JSON === 'object' && JSON.stringify) {
			this.last_req_seq = Date.parse(new Date())/1000;
			var strrep = JSON.stringify(reports);
			var para = 'action=http_report_data&';
			para += 'req_seq=' + this.last_req_seq;
			para += '&data=' + encodeURI(strrep);
			this._to_server(para);

			if(this.debug) {
				var msg = 'request seq:' + this.last_req_seq + ', report data:' + strrep; 
				if (typeof(console) != "undefined")
					console.log(msg);
				else 
					alert(msg);
			}

			this.logs = new Array();
			this.attrs = new Array();
			this.strattrs = new Array();
		}
		else {
			alert('xrkmonitor - report failed, not support JSON.stringify !');
		}
	},

	_inner_xrk_report_attr: function(opr, id, number) {
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
		if(this.debug && typeof retmsg.seq != 'undefined') {
			var msg = 'response seq:' + retmsg.seq + ', last request seq:' + this.last_req_seq
				+ ', ret code:' + retmsg.ret;
			if(typeof(console) != "undefined")
				console.log(msg);
			else
				alert(msg);
		}
	},
	_to_server: function (reports) {
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
					alert('xrkmonitor - report failed, not support xmHttp request !');
					return false;
				}
			}
		};
		xmlHttp.open('POST', this.report_url, true);
		xmlHttp.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded;charset=utf-8');
		xmlHttp.send(reports);
		xmlHttp.onreadystatechange = function () {
		    if (xmlHttp.readyState == 4 && xmlHttp.status == 200) {
				if(typeof xmlHttp.responseText != 'undefined')
					xrk._on_response(JSON.stringify(xmlHttp.responseText));
		    }
		}; 
	},

	debug:1, 
	log_config_id:0, // 日志配置 id, 初始化接口指定
	master_user_id:1, // 主账号id, 用于云版本
	report_url:'', // 监控系统 fcgi - mt_slog_reportinfo 的调用地址
	last_delay_timer_id: 0, // 延迟上报定时器id
	last_req_seq:0, // 用于请求响应包校验
	max_attrs_per_req:120, // 每次最多上传的监控点数目
	max_strattrs_strlen:1000, // 每次最多上报的字符串监控点长度
	cur_strattrs_strlen:0, // 当前累计的待上报字符串长度
	max_logs_strlen:900, // 每次最多上报的日志长度
	cur_logs_strlen:0, // 当前累计的待上报日志长度
	logs:new Array(),
	attrs:new Array(),
	strattrs:new Array()
};

//
// javascript 数据上报方法步骤
// 1. 调用 xrk.init_report()  初始化
// 2. 根据需要调用 xrk.report_attr_add() / xrk.report_attr_set() 上报监控点数据
//    根据需要调用 xrk.report_strattr_add() / xrk.report_strattr_set() 上报字符串型监控点数据
//    根据需要调用 xrk.report_log() 上报日志
// 3. 调用 xrk.report_to_server() 上报数据到监控系统
//
// 关于调用接口的详细说明请参考文档： 
// 调用示例参考: dmt_test_report.html
//

