#!/bin/bash

/etc/init.d/networking restart

ifconfig eth0 172.16.20.254/24
ifconfig eth1 172.16.21.253/24

route add default gw 172.16.21.254

printf "search netlab.fe.up.pt \nnameserver 172.16.1.1" > /etc/resolv.conf

echo 1 > /proc/sys/net/ipv4/ip_forward
