#!/bin/sh

restore7530Gsw()
{
    echo "restore GSW to dump switch mode"
    #port matrix mode
    switch reg w 2004 ff0000 #port0
    switch reg w 2104 ff0000 #port1
    switch reg w 2204 ff0000 #port2
    switch reg w 2304 ff0000 #port3
    switch reg w 2404 ff0000 #port4
    switch reg w 2504 ff0000 #port5
    switch reg w 2604 ff0000 #port6
    switch reg w 2704 ff0000 #port7

    #LAN/WAN ports as transparent mode
    switch reg w 2010 810000c0 #port0
    switch reg w 2110 810000c0 #port1
    switch reg w 2210 810000c0 #port2
    switch reg w 2310 810000c0 #port3   
    switch reg w 2410 810000c0 #port4
    switch reg w 2510 810000c0 #port5
    switch reg w 2610 810000c0 #port6
    switch reg w 2710 810000c0 #port7
    
    #clear mac table if vlan configuration changed
    switch clear
}

config7530Gsw()
{
	#LAN/WAN ports as security mode
	switch reg w 2004 ff0003 #port0
	switch reg w 2104 ff0003 #port1
	switch reg w 2204 ff0003 #port2
	switch reg w 2304 ff0003 #port3
	switch reg w 2404 ff0003 #port4

	#LAN/WAN ports as transparent port
	switch reg w 2010 810000c0 #port0
	switch reg w 2110 810000c0 #port1
	switch reg w 2210 810000c0 #port2
	switch reg w 2310 810000c0 #port3	
	switch reg w 2410 810000c0 #port4

	#set CPU/P7 port as user port
	switch reg w 2510 81000000 #port6
	switch reg w 2610 81000000 #port7

	switch reg w 2504 20ff0003 #port6, Egress VLAN Tag Attribution=tagged
	switch reg w 2604 20ff0003 #port7, Egress VLAN Tag Attribution=tagged

	#set PVID
	switch reg w 2014 10001 #port0
	switch reg w 2114 10001 #port1
	switch reg w 2214 10001 #port2
	switch reg w 2314 10001 #port3
	switch reg w 2414 10002 #port4
	#VLAN member port
	switch vlan set 1 1111011
	switch vlan set 2 0000111

	#clear mac table if vlan configuration changed
	switch clear
}

setup_switch()
{
	config7530Gsw
}

reset_switch()
{
	restore7530Gsw
}
