#! /bin/bash

while [ true ]
do
    echo "running..."
    if ! ./rsm_tester.pl 16 ; then
		exit 1
	fi
    if grep -q "clear_and_notify" lock_server-*.log ; then
        echo "Found"
        exit 1
    fi
    killall lock_server; rm -rf *.log
done
