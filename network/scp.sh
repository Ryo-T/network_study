#!/bin/sh

PW="d89mer"

expect -c "
set timeout 1000
spawn env LANG=C scp ryo@192.168.211.134:~/network/sample2.txt ~
expect \"password:\"
send \"${PW}\n\"
expect \"$\"
exit 0
"
