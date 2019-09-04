#!/bin/bash
ipcs |awk '{if($6==0) print "ipcrm -m "$2;}' > _zero.sh
chmod +x _zero.sh
./_zero.sh
rm -f _zero.sh
