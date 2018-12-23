#!/bin/bash

/etc/init.d/networking restart

ifconfig eth0 172.16.20.1/24

route add default gw 172.16.20.254 de

printf "search netlab.fe.up.pt \nnameserver 172.16.1.1" > /etc/resolv.conf
