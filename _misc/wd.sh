#!/bin/bash

[ -z "$1" ] && echo "Usage: $0 <PID> [flexTC_IP]" 1>&2 && exit 1

n=0
WDFN="/dev/shm/$1"

# Wait for 12 x 5s
while [ $n -lt 12 ]; do
    [ -e $WDFN ] && n=`cat $WDFN`
    echo $(($n + 1)) > $WDFN
    sleep 5
done

# Set FlexTC back to 25C
if [ -n "$2" ]; then
    FTCPATH=$(dirname $0)/../labEquipment
    LD_LIBRARY_PATH=$FTCPATH $FTCPATH/flexTcExec $2 MI0699,0250
fi
rm $WDFN
kill $1 2> /dev/null
