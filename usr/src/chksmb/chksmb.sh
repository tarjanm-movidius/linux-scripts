#!/bin/sh

OPTS="uid=1000,gid=100"
LOGGER="/usr/bin/logger -t ${SELF}"

HOST1="192.168.100.16"
SERVICE1="//192.168.100.16/mp3"
MNTDIR1="/mnt/mp3"
CREDS1="username=user,password=pass"

HOST2="192.168.100.16"
SERVICE2="//192.168.100.16/movies"
MNTDIR2="/mnt/movies"
CREDS2="username=user,password=pass"

chkhost() {

if ping -qc1w1t1 $1 > /dev/null; then
	if test -e /var/lock/${SELF}_$1; then
		return
	fi
	if ! grep -q $3 /etc/mtab; then
		${LOGGER} "Mounting Samba directory $2 to $3"
		ERRMSG="`mount.cifs $2 $3 -o ${OPTS},$4 2>&1`"
		if test $? -ne 0; then
			${LOGGER} "${ERRMSG}"
			echo -n "1" > /var/lock/${SELF}_$1
		fi
	fi
else
	rm -f /var/lock/${SELF}_$1 >/dev/null 2>&1
	if grep -q $3 /etc/mtab; then
		${LOGGER} "Unmounting Samba directory from $3"
		${LOGGER} "`umount $3 2>&1`"
	fi
fi

}

if test "${PARMS}" == "-u"; then
	umount -f $MNTDIR1 $MNTDIR2
	rm -f /var/lock/${SELF}* >/dev/null 2>&1
	${LOGGER} "Unmounting all Samba directories"
else
	chkhost $HOST1 $SERVICE1 $MNTDIR1 $CREDS1
	chkhost $HOST2 $SERVICE2 $MNTDIR2 $CREDS2
fi

unset ERRMSG
