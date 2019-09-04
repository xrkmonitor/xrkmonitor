#!/bin/bash
cd ../cgi_fcgi
Name=slog_cgi_bin
TarF=${Name}.tar.gz

tar -czf ${TarF} mt_slog_user mt_slog_warn mt_slog slog_flogin mt_slog_view mt_slog_showview mt_slog_attr mt_slog_monitor mt_slog_machine

mv ${TarF} ../tools_sh

