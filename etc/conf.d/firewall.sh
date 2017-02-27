#!/bin/sh

IPT="/sbin/iptables"

IF_VLAN="vboxnet0"
IF_ETH="eth0"
IF_WLAN="wlan0"
TCP_IN_PORTS="60000"
UDP_IN_PORTS="0"
#FTP_PORTS="20,21,113"
FTP_PORTS="21"
NET_ETH="192.168.1.0/24"

#URAL="152.66.130.2"

###############################################################################
#==============================================================================
#==============================================================================
###############################################################################

ipt_start() {

# lancok uritese, szamlalok nullazasa

    $IPT -F INPUT;
    $IPT -F FORWARD;
    $IPT -F OUTPUT;

    $IPT -Z INPUT;
    $IPT -Z FORWARD;
    $IPT -Z OUTPUT;

###############################################################################
#		INPUT
###############################################################################

    $IPT -P INPUT DROP;

# Connection tracking

    $IPT -A INPUT -p tcp --tcp-flags ALL ALL -j DROP;
    $IPT -A INPUT -p tcp --tcp-flags ALL NONE -j DROP;
    $IPT -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT;
    $IPT -A INPUT -m state --state INVALID -j LOG --log-level info --log-prefix SPOOF: ;
    $IPT -A INPUT -m state --state INVALID -j DROP;
    $IPT -A INPUT -p tcp ! --syn -m state --state NEW -j DROP;

# Synflood ellen

    $IPT -N SYNFLOOD;
    $IPT -A INPUT -p tcp --syn -j SYNFLOOD;
    $IPT -A SYNFLOOD -m limit --limit 10/s --limit-burst 20 -j RETURN;
    $IPT -A SYNFLOOD -j LOG --log-level info --log-prefix SYNFLOOD: ;
    $IPT -A SYNFLOOD -j DROP;

# Floodping ellen

    $IPT -A INPUT -p icmp --icmp-type echo-request -m limit --limit 1/s --limit-burst 10 -j ACCEPT;
    $IPT -A INPUT -p icmp --icmp-type echo-reply -m limit --limit 1/s --limit-burst 10 -j ACCEPT;
    $IPT -A INPUT -p icmp -j DROP;

# Loopback / tunnel IF-ek

    $IPT -A INPUT -i lo -s 127.0.0.1 -j ACCEPT;
#    $IPT -A INPUT -i tap+ -j ACCEPT;

# Bejovo forgalom, IF-enkent es protokollonkent

    $IPT -A INPUT -i $IF_ETH -p tcp -m multiport --dports $TCP_IN_PORTS -m state --state NEW -j ACCEPT;
    $IPT -A INPUT -i $IF_ETH -p udp -m multiport --dports $UDP_IN_PORTS -m state --state NEW -j ACCEPT;
    $IPT -A INPUT -i $IF_WLAN -p tcp -m multiport --dports $TCP_IN_PORTS -m state --state NEW -j ACCEPT;
    $IPT -A INPUT -i $IF_WLAN -p udp -m multiport --dports $UDP_IN_PORTS -m state --state NEW -j ACCEPT;

# Ha az ftp-t be akarod engedni a vezetekesen
#
    $IPT -A INPUT -i $IF_ETH -p tcp -m multiport --dports $FTP_PORTS -m state --state NEW -j ACCEPT;
    $IPT -A INPUT -i $IF_ETH -p udp -m multiport --dports $FTP_PORTS -m state --state NEW -j ACCEPT;

# Ha csak bizonyos IP-rol (portonkent, IP-nkent :S)
#
#    $IPT -A INPUT -i $IF_ETH -p tcp -s $URAL --dport 20 -m state --state NEW -j ACCEPT;
#    $IPT -A INPUT -i $IF_ETH -p tcp -s $URAL --dport 21 -m state --state NEW -j ACCEPT;
#    $IPT -A INPUT -i $IF_ETH -p tcp -s $URAL --dport 113 -m state --state NEW -j ACCEPT;
#
#    $IPT -A INPUT -i $IF_ETH -p udp -s $URAL --dport 20 -m state --state NEW -j ACCEPT;
#    $IPT -A INPUT -i $IF_ETH -p udp -s $URAL --dport 21 -m state --state NEW -j ACCEPT;
#    $IPT -A INPUT -i $IF_ETH -p udp -s $URAL --dport 113 -m state --state NEW -j ACCEPT;

# NAT
#	$IPT -I FORWARD -i $IF_VLAN -d 192.168.0.0/255.255.0.0 -j DROP
#	$IPT -I FORWARD -i $IF_VLAN -s 192.168.0.0/255.255.0.0 -j ACCEPT
#	$IPT -I FORWARD -i $IF_VLAN -d 192.168.0.0/255.255.0.0 -j ACCEPT
#	$IPT -t nat -A PREROUTING -i $IF_ETH -j DNAT --to-destination 192.168.56.1:
#	$IPT -t nat -A POSTROUTING -s 192.168.56.0/24 -o $IF_ETH -j MASQUERADE
#	$IPT -t nat -A POSTROUTING -s 192.168.100.0/24 -o $IF_WLAN -j MASQUERADE
#	$IPT -t nat -A POSTROUTING -s 172.16.150.0/24 -o $IF_ETH -j MASQUERADE

 echo -ne "1" > /proc/sys/net/ipv4/ip_forward;
# $IPT -t nat -A PREROUTING -i $IF_ETH -p tcp --dport 53 -j DNAT --to-destination 127.0.0.1
# $IPT -t nat -A PREROUTING -i $IF_ETH -p udp --dport 53 -j DNAT --to-destination 127.0.0.1
# $IPT -t nat -A PREROUTING -i $IF_ETH -p tcp --dport 53 -j DNAT --to 127.0.0.1:53
# $IPT -t nat -A PREROUTING -i $IF_ETH -p udp --dport 53 -j DNAT --to 127.0.0.1:53

 $IPT -A INPUT -i $IF_ETH -p tcp -s $NET_ETH --dport 53 -m state --state NEW -j ACCEPT;
 $IPT -A INPUT -i $IF_ETH -p udp -s $NET_ETH --dport 53 -m state --state NEW -j ACCEPT;
# $IPT -t nat -A POSTROUTING -j MASQUERADE;
 $IPT -t nat -A POSTROUTING -o $IF_WLAN -j MASQUERADE;

 $IPT -I FORWARD -i $IF_WLAN -o $IF_ETH -m state --state RELATED,ESTABLISHED -j ACCEPT;
#echo "I'm alive"
 $IPT -A FORWARD -i $IF_ETH -o $IF_WLAN -j ACCEPT;
# $IPT -A FORWARD -s $NET_ETH -o $IF_WLAN -j ACCEPT;

# A tobbi loggolva a lecsoba

    $IPT -A INPUT -j LOG --log-level info --log-prefix REJECT: ;

}

###############################################################################
#==============================================================================
#==============================================================================
###############################################################################

ipt_stop() {


    $IPT -F SYNFLOOD;
    $IPT -F OUTPUT;
    $IPT -F INPUT;
    $IPT -F FORWARD;
    $IPT -t nat -F POSTROUTING;
    $IPT -t nat -F PREROUTING;

    $IPT -Z SYNFLOOD;
    $IPT -Z OUTPUT;
    $IPT -Z INPUT;
    $IPT -Z FORWARD;
    $IPT -t nat -Z POSTROUTING;
    $IPT -t nat -Z PREROUTING;

    $IPT -X SYNFLOOD;
    $IPT -P OUTPUT ACCEPT;
    $IPT -P INPUT ACCEPT;
    $IPT -P FORWARD ACCEPT;

    echo -ne "0" > /proc/sys/net/ipv4/ip_forward;
}

case "$1" in
'start')
  ipt_start
  ;;
'stop')
  ipt_stop
  ;;
'restart')
  ipt_stop;
  sleep 0.5;
  ipt_start;
  ;;
*)
  echo "usage $0 start|stop|restart"
esac

