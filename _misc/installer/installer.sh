#!/bin/sh

exit #LASTLINE
MCEXT="/etc/mc/mc.ext"
UDEV="/etc/init.d/udev"

if [ "$1" == "x" ]; then
  tail -n +${LASTLINE} "$0" | tar xj --totals
  exit
else
  tail -n +${LASTLINE} "$0" | tar xjv -C /
fi
python -O -m compileall usr/lib/python*
chmod -v 644 /etc/logrotate.d/assetace /opt/*.py
chmod -v 755 /etc/local.d/acetech.start /opt/*.sh
rm -vf /etc/rc2.d/*hostap* /etc/rc0.d/*kura* /etc/rc6.d/*kura*
[ -e /etc/hostapd.conf ] && mv -v /etc/hostapd.conf /etc/hostapd.conf~
[ -e /opt/eurotech ] && mv -v /opt/eurotech /opt/eurotech~
if ! grep -q "sqlist" "${MCEXT}"; then
  echo "Installing SQLite viewer ..."
  if grep -q "### Default ###" "${MCEXT}"; then
    mv "${MCEXT}" "${MCEXT}~"
    sed 's/### Default ###/# SQLite\nregex\/\\.sqlite$\n\tView=%view{ascii} sqlist %f\n\n### Default ###/' "${MCEXT}~" > "${MCEXT}"
  else
    cp "${MCEXT}" "${MCEXT}~"
    echo '# SQLite' >> "${MCEXT}"
    echo 'regex/\.sqlite$' >> "${MCEXT}"
    echo '	View=%view{ascii} sqlist %f' >> "${MCEXT}"
  fi
fi
if ! grep -q "ftdi_sio" "${UDEV}"; then
  echo "Installing USB/CAN driver ..."
  if grep -q "exit 0" "${UDEV}"; then
    mv "${UDEV}" "${UDEV}~"
    sed 's/exit 0/\/sbin\/modprobe ftdi_sio\n\nexit 0/' "${UDEV}~" > "${UDEV}"
    chmod 644 "${UDEV}~"
    chmod 755 "${UDEV}"
  else
    cp "${UDEV}" "${UDEV}~"
    echo '' >> "${UDEV}"
    echo '/sbin/modprobe ftdi_sio' >> "${UDEV}"
  fi
fi
exit

