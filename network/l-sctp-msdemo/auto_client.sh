#!/usr/bin/expect

log_file ~/network/l-sctp-msdemo/log.txt
set timeout 10

set timer 3

spawn env LANG=C ./client
expect {expect \"$\";exit 0}
sleep $timer

spawn env LANG=C ./client
expect {expect \"$\";exit 0}
sleep $timer

spawn env LANG=C ./client
expect {expect \"$\";exit 0}
sleep $timer

spawn env LANG=C ./client
expect {expect \"$\";exit 0}
sleep $timer

spawn env LANG=C ./client
expect {expect \"$\";exit 0}
sleep $timer
