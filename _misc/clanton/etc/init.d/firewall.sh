#!/bin/bash

IPT=`which iptables`

# Logging packets, "$IPT" to enable, "false" to disable
LOG="false"
IF_TUN="tun+"
IF_ETH="eth0"
LOOP="127.0.0.1"
NET_ETH="192.168.1.0/24"
NET_VPN="10.10.0.0/24"
TCP_IN_PORTS="22,137,138,139,443,445"
UDP_IN_PORTS="60001"
#FTP_PORTS="20,21,113"

#URAL="152.66.130.2"

###############################################################################
#==============================================================================
#==============================================================================
###############################################################################

ipt_start() {

# Flushing chains, zeroing counters

	$IPT -F
	$IPT -Z INPUT
	$IPT -Z FORWARD
	$IPT -Z OUTPUT

###############################################################################
#		INPUT
###############################################################################

	$IPT -P OUTPUT ACCEPT
	$IPT -P INPUT DROP
	$IPT -P FORWARD DROP

# Connection tracking
	$IPT -A INPUT -p tcp --tcp-flags ALL ALL -j DROP
	$IPT -A INPUT -p tcp --tcp-flags ALL NONE -j DROP
	$IPT -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	$LOG -A INPUT -m state --state INVALID -j LOG --log-level info --log-prefix SPOOF:
	$IPT -A INPUT -m state --state INVALID -j DROP
	$IPT -A INPUT -p tcp ! --syn -m state --state NEW -j DROP

# Against synflood
	$IPT -N SYNFLOOD
	$IPT -A INPUT -p tcp --syn -j SYNFLOOD
	$IPT -A SYNFLOOD -m limit --limit 10/s --limit-burst 20 -j RETURN
	$LOG -A SYNFLOOD -j LOG --log-level info --log-prefix SYNFLOOD:
	$IPT -A SYNFLOOD -j DROP

# Against floodping
	$IPT -A INPUT -p icmp --icmp-type echo-request -m limit --limit 1/s --limit-burst 10 -j ACCEPT
	$IPT -A INPUT -p icmp --icmp-type echo-reply -m limit --limit 1/s --limit-burst 10 -j ACCEPT
	$IPT -A INPUT -p icmp -j DROP

# Prevent external packets from using loopback addr
	$IPT -A INPUT -i $IF_ETH -s $LOOP -j DROP
	$IPT -A FORWARD -i $IF_ETH -s $LOOP -j DROP
	$IPT -A INPUT -i $IF_ETH -d $LOOP -j DROP
	$IPT -A FORWARD -i $IF_ETH -d $LOOP -j DROP

# Checking if addresses belong to their interface
	$IPT -A INPUT -i $IF_ETH -s 10.0.0.0/8 -j DROP
	$IPT -A FORWARD -i $IF_ETH -s 10.0.0.0/8 -j DROP
	$IPT -A INPUT -i $IF_TUN -s 192.168.0.0/16 -j DROP
	$IPT -A FORWARD -i $IF_TUN -s 192.168.0.0/16 -j DROP

# Loopback / tunnel IFs
	$IPT -A INPUT -i lo -s $LOOP -j ACCEPT
	$IPT -A INPUT -i $IF_TUN -j ACCEPT
	$IPT -A FORWARD -i $IF_TUN -j ACCEPT

# Incoming traffic, per protocol and IF
	$IPT -A INPUT -i $IF_ETH -p tcp -m multiport --dports $TCP_IN_PORTS -m state --state NEW -j ACCEPT
	$IPT -A INPUT -i $IF_ETH -p udp -m multiport --dports $UDP_IN_PORTS -m state --state NEW -j ACCEPT

# Letting ftp in through the wired IF
if [ -n "$FTP_PORTS" ]; then
	$IPT -A INPUT -i $IF_ETH -p tcp -m multiport --dports $FTP_PORTS -m state --state NEW -j ACCEPT
	$IPT -A INPUT -i $IF_ETH -p udp -m multiport --dports $FTP_PORTS -m state --state NEW -j ACCEPT
fi

# Keep state of connections from local machine and private subnets
	$IPT -A OUTPUT -m state --state NEW -o $IF_ETH -j ACCEPT
	$IPT -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	$IPT -A FORWARD -m state --state NEW -o $IF_ETH -j ACCEPT
	$IPT -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

# NAT
	$IPT -t nat -A PREROUTING -i $IF_TUN -p tcp --dport 53 -j DNAT --to 192.168.1.1:53
	$IPT -t nat -A PREROUTING -i $IF_TUN -p udp --dport 53 -j DNAT --to 192.168.1.1:53
	$IPT -t nat -A POSTROUTING -s $NET_VPN -o $IF_ETH -j MASQUERADE
	echo -n "1" > /proc/sys/net/ipv4/ip_forward

# The rest logged/dropped
	$LOG -A INPUT -j LOG --log-level info --log-prefix REJECT:

}

###############################################################################
#==============================================================================
#==============================================================================
###############################################################################

ipt_stop() {

	$IPT -F

	$IPT -Z SYNFLOOD
	$IPT -Z OUTPUT
	$IPT -Z INPUT
	$IPT -Z FORWARD
	$IPT -t nat -Z POSTROUTING
	$IPT -t nat -Z PREROUTING

	$IPT -X SYNFLOOD
	$IPT -P OUTPUT ACCEPT
	$IPT -P INPUT ACCEPT
	$IPT -P FORWARD ACCEPT

	echo -n "0" > /proc/sys/net/ipv4/ip_forward
}

case "$1" in
'start')
  ipt_start
  ;;
'stop')
  ipt_stop
  ;;
'restart')
  ipt_stop
  sleep 1
  ipt_start
  ;;
*)
#  echo "Usage: $0 <start|stop|restart>"
  ipt_start
esac
