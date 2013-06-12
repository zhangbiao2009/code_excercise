#! /bin/bash

while [ true ]
do
    echo "running..."
    ./rsm_tester.pl 16
    if grep -q "clear_and_notify" lock_server-*.log ; then
        echo "Found"
        exit 1
    fi
    killall lock_server; rm -rf *.log
done
