#!/bin/sh


die() {

echo ""
echo "Usage: $ME [-b] <device filename | -r filename device>" 1>&2
echo ""
echo "-b : Use Bzip2 instead of Gzip"
echo "-r : restore saved partition"
echo ""
echo "Examples:"
echo "$ME hda1 XP_SP2_en"
echo "  This will save /dev/hda1 to XP_SP2_en_YYYYMMDD.tar.gz and the MBR of /dev/hda to MBR_YYYYMMDD, where YYYYMMDD is the current date"
echo ""
echo "$ME -b -r XP_SP2_en_20030123.tar.bz2 sda2"
echo "  restores the previously saved image to /dev/sda2 using Bzip2"
echo "  Warning: ALL DATA on /dev/sda2 will be OVERWRITTEN!"
exit $1

}


###  M A I N  ###

ME="`basename $0`"

if [ $# -lt 2 ]; then
  echo "Error: parameter(s) missing!" 1>&2
  die 1
fi

COMPR="gzip"
DECOMPR="gunzip"
EXT="tar.gz"
RESTORE=0

while [ $# -gt 2 ]; do

  case "$1" in
  '-b')
    COMPR="bzip2"
    DECOMPR="bunzip2"
    EXT="tar.bz2"
    ;;
  '-r')
    RESTORE=1
    ;;
  *)
    echo "Parameter error: '$1'" 1>&2
    die 2
  esac

  shift

done

if [ "$RESTORE" == "1" ]; then
  S_DEV="/dev/$2"
  S_FILE="$1"
  if [ ! -e ${S_FILE} ]; then
    echo "Error: file '${S_FILE}' doesn't exist" 1>&2
    die 4
  fi
  S_CMD="${DECOMPR} < ${S_FILE} | dd of=${S_DEV}"
  S_CMD2=""
else
  DATE=`date +%Y%m%d`
  S_DEV="/dev/$1"
  S_FILE="${2}_${DATE}.${EXT}"
  S_CMD="dd if=${S_DEV} | ${COMPR} > ${S_FILE}"
  S_CMD2="dd if=`tr -d [0-9] <<< ${S_DEV}` bs=512 count=1 > MBR_${DATE}"
fi

if [ ! -e ${S_DEV} ]; then
  echo "Error: device '${S_DEV}' doesn't exist" 1>&2
  die 3
fi

echo "${S_CMD}   <-- Is that what we're doing?"
echo -n "Press Ctrl+C to abort!"
for I in `seq 5 -1 0`; do echo -n " $I"; sleep 1; done
echo ""

${S_CMD}
${S_CMD2}
