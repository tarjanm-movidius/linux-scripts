#!/bin/sh

IFS='
'
if [ -n "$1" ]; then
	NAMES="$1"
else
	NAMES="`find . -name "*.mp3"`"
fi
for i in $NAMES; do
	if ! id3info "$i" | grep -q 'TIT\|TT2'; then
		NAME="`basename $i | sed 's/.* - \|.mp3\| lyrics\| official video\| video//gI'`"
		id3tag -1 -s"$NAME" "$i"
	fi
done
