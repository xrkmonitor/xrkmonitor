all:
	make -C lib/my_proto 
	make install -C lib/my_proto 
	make -C lib/libsockets
	make install -C lib/libsockets
	make -C lib/mtagent_api_open
	make install -C lib/mtagent_api_open
	make -C lib/mysqlwrapped
	make install -C lib/mysqlwrapped
	cd lib/clearsilver/;./gen.sh; make; make install
	cd ../../
	make -C lib/cgi_comm
	make install -C lib/cgi_comm
	make -C lib/mtreport_api
	make install -C lib/mtreport_api
	make -C cgi_fcgi 
	make -C slog_client 
	make -C slog_config 
	make -C slog_write 
	make -C slog_server 
	make -C slog_monitor_client
	make -C slog_monitor_server
	make -C slog_check_warn
	make -C slog_deal_warn
	make -C slog_tool
	make -C slog_mtreport_client
	make -C slog_mtreport_server 

clean:
	make clean -C lib/my_proto 
	make clean -C lib/mtagent_api_open
	make clean -C lib/libsockets
	make clean -C lib/mysqlwrapped
	make clean -C lib/cgi_comm
	make clean -C lib/mtreport_api
	make clean -C cgi_fcgi 
	make clean -C slog_client 
	make clean -C slog_config 
	make clean -C slog_write 
	make clean -C slog_server 
	make clean -C slog_monitor_client
	make clean -C slog_monitor_server
	make clean -C slog_check_warn
	make clean -C slog_deal_warn
	make clean -C slog_tool
	make clean -C slog_mtreport_client
	make clean -C slog_mtreport_server

