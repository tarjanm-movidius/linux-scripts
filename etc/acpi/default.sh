#!/bin/sh
# /etc/acpi/default.sh
# Default acpi script that takes an entry for all actions

[ -n "$BRTNS_DEV" ] || . /etc/conf.d/brightness || BRTNS_DEV="/sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0/drm/card0/card0-LVDS-1/radeon_bl0/brightness"
set $*

group=${1%%/*}
action=${1#*/}
device=$2
id=$3
value=$4

log_unhandled() {
	logger "ACPI event unhandled: $*"
}

case "$group" in
	button)
		case "$action" in
			power)
				[ "$device" == "PBTN" ] && /sbin/hiber | logger -t hiber
#				/etc/acpi/actions/powerbtn.sh
				;;
			sleep)
				if [ "$device" == "SBTN" ]; then
					echo -n deep > /sys/power/mem_sleep
					echo -n mem > /sys/power/state
				fi
				;;
			#lid)
			#	xset dpms force off
			#	;;
			volumeup)
				amixer sset Master,0 on, 5%+
				;;
			volumedown)
				amixer sset Master,0 on, 5%-
				;;
			mute)
				amixer sset Master,0 toggle
				;;
			*)	log_unhandled $* ;;
		esac
		;;

	video)
		case "$action" in
			brightnessup)
				n=`expr $(cat $BRTNS_DEV) '*' 2`
				[ $n -gt 255 ] && n=255
				echo $n > $BRTNS_DEV
				;;
			brightnessdown)
				n=`expr $(cat $BRTNS_DEV) / 2`
				[ $n -lt 1 ] && n=1
				echo $n > $BRTNS_DEV
				;;
			*)	log_unhandled $* ;;
		esac
		;;

	cd)
		case "$action" in
			play)
#				DISPLAY=":0.0" sudo -u m xmms -t
				;;
			next)
#				DISPLAY=":0.0" sudo -u m xmms -f
				;;
			prev)
#				DISPLAY=":0.0" sudo -u m xmms -r
				;;
			*)	log_unhandled $* ;;
		esac
		;;

	jack)
		case "$action" in
			headphone)
				if [ "$id" == "plug" ]; then
					amixer sset Headphone,0 84,84 unmute
					amixer sset Speaker,0 84,84 mute
				else
					amixer sset Speaker,0 84,84 unmute
					amixer sset Headphone,0 84,84 mute
				fi
				;;
			microphone)
				;;
			*)	log_unhandled $* ;;
		esac
		;;

	ac_adapter)
		case "$value" in
			# Add code here to handle when the system is unplugged
			# (maybe change cpu scaling to powersave mode).  For
			# multicore systems, make sure you set powersave mode
			# for each core!
			#*0)
			#	cpufreq-set -g powersave
			#	;;

			# Add code here to handle when the system is plugged in
			# (maybe change cpu scaling to performance mode).  For
			# multicore systems, make sure you set performance mode
			# for each core!
			#*1)
			#	cpufreq-set -g performance
			#	;;

			*)	log_unhandled $* ;;
		esac
		;;

	*)	log_unhandled $* ;;
esac
