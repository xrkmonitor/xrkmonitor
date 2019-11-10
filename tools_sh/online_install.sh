#!/bin/bash

localos=`cat /etc/issue`

wget http://install.xrkmonitor.com/online_install?osinfo="$localos"

