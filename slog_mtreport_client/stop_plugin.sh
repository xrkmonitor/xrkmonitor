#!/bin/bash

plugin_dir=./xrkmonitor_plugin
cfg_plugin_dir=`cat ./slog_mtreport_client.conf |grep "^PLUS_PATH"`
if [ $? -eq 0 -a "$cfg_plugin_dir" != '' ]; then
	plugin_dir=`echo $cfg_plugin_dir|awk '{print $2}'`
fi

function stop_plugin()
{
    if [ ! -d "$plugin_dir" ]; then
        return;
    fi

    cd $plugin_dir 
    curpwd=`pwd`
    dirlist=`find . -maxdepth 1 -type d`
    for dr in $dirlist
    do
        if [ -x "$dr/stop.sh" ]; then
            cd "$dr"
            pname=`basename $dr`
            echo "try stop plugin: $pname"
            ./stop.sh
            cd "$curpwd"
        fi
    done
}
stop_plugin

