#!/bin/bash

/etc/init.d/networking restart

ifconfig eth0 172.16.21.1/24

route add -net 172.16.20.0/24 gw 172.16.21.253

printf "search netlab.fe.up.pt \nnameserver 172.16.1.1" > /etc/resolv.conf
