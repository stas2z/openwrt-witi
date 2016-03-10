#!/bin/sh
append DRIVERS "mt7612e"

. /lib/wifi/ralink_common.sh

prepare_mt7612e() {
	prepare_ralink_wifi mt7612e
}

scan_mt7612e() {
	scan_ralink_wifi mt7612e mt76x2e
}

disable_mt7612e() {
	disable_ralink_wifi mt7612e
}

enable_mt7612e() {
	enable_ralink_wifi mt7612e mt76x2e
}

detect_mt7612e() {
#	detect_ralink_wifi mt7612e mt76x2e
	ssid=WITI-5G-`ifconfig eth0 | grep HWaddr | cut -c 51- | sed 's/://g'`
	cd /sys/module/
	[ -d $module ] || return
	[ -e /etc/config/wireless ] && return
	 cat <<EOF
config wifi-device      mt7612e
        option type     mt7612e
        option vendor   ralink
        option band     5G
        option channel  0
        option autoch   2
        option bw       2
        option txpreamble 1
        option bgprotect 2

config wifi-iface
        option device   mt7612e
        option ifname   rai0
        option wmm      1
        option apsd     1
        option rekeyinteval    1000
        option network  lan
        option mode     ap
        option ssid     $ssid
        option encryption none

EOF

}


