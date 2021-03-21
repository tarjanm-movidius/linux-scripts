#!/bin/bash

myfunc() {
	echo "in myfunc()" >> logggg
sleep 5
}
MYVAR="blah"

export MYVAR
export -f myfunc
#screen -d -m echo "" bash
screen -d -m sh -c "myfunc; sleep 1; echo $MYVAR >> logggg"
