#!/bin/sh

LASTLINE=$((`grep -c '$' installer.sh` + 1))
DIR=$(cd `dirname $0`; pwd)

cd ..
rm -vf opt/*.py[co] `find usr/lib/python* -name *.py[co]`
INSTALLER="eurotech_installer-`grep -m1 'VERSION_STRING.*=' opt/acetech_van.py | sed 's/[^0-9.]//g'`.sh"
if [ -e "${INSTALLER}" ]; then
  mv -v "${INSTALLER}" "${INSTALLER}~"
  chmod 644 "${INSTALLER}~"
fi
(sed "s/exit #LASTLINE/LASTLINE=${LASTLINE}/" "${DIR}/installer.sh"; tar cj --totals `find . -type d | cut -b 3-`) > ${INSTALLER}
chmod 755 ${INSTALLER}
