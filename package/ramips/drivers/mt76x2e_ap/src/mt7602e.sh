#!/bin/sh
append DRIVERS "mt7602e"

. /lib/wifi/ralink_common.sh

prepare_mt7602e() {
	prepare_ralink_wifi mt7602e
}

scan_mt7602e() {
	scan_ralink_wifi mt7602e mt76x2e
}

disable_mt7602e() {
	disable_ralink_wifi mt7602e
}

enable_mt7602e() {
	enable_ralink_wifi mt7602e mt76x2e
}

detect_mt7602e() {
#	detect_ralink_wifi mt7602e mt76x2e
	ssid=`cat /tmp/sysinfo/model | sed -e 's/^.*[ -]//gi'`-2G-`ifconfig eth0 | grep HWaddr | cut -c 51- | sed 's/://g'`
	cd  /sys/module/
	[ -d $module ] || return
        [ -e /etc/config/wireless ] && return
         cat <<EOF
config wifi-device      mt7602e
        option type     mt7602e
        option vendor   ralink
        option band     2.4G
        option channel  0
        option autoch   2
        option bw       1
        option txpreamble 1
        option bgprotect 2
        option region '1'

config wifi-iface
        option device   mt7602e
        option ifname   ra0
        option wmm      1
        option apsd     1
	option rekeyinteval	1000
        option network  lan
        option mode     ap
        option ssid     $ssid
        option encryption none

EOF

}


