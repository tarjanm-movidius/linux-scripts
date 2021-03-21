#!/bin/sh

if [ "$1" == "stop" ]; then
	killall cputemp
	rm -r /var/lock/cputemp
else
	/usr/bin/cputemp &
fi

exit 0

