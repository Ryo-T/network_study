#!/bin/sh


expect -c "

log_file ~/network/l-sctp-msdemo/log.txt
set timeout 10

#spawn env LANG=C ./client
spawn ping 127.0.0.1

expect \"$\"

sleep 3

"
