-- Copyright 2008 Steven Barth <steven@midlink.org>
-- Copyright 2011 Jo-Philipp Wich <jow@openwrt.org>
-- Licensed to the public under the Apache License 2.0.

local sys   = require "luci.sys"
local fs    = require "nixio.fs"
local conf  = require "luci.config"

local m, s, o, p, po, ps, h, g

p = Map("platform", translate("Platform settings"), translate("Here you can configure several platform specific options."))
p:chain("luci")

s = p:section(TypedSection, "vgaclamp", translate("Radio RX sensitivity reduction"))
s.anonymous = true
s.addremove = false

po = s:option(Value, "ra0", translate("2.4G radio (0 = OFF, 10 = MAX)"))
po.optional    = true
po.placeholder = 8
po.default = 0

po = s:option(Value, "rai0", translate("5G radio (0 = OFF, 10 = MAX)"))
po.optional    = true
po.placeholder = 8
po.default = 0

m = p:section(TypedSection, "greenap", translate("Enable Green AP mode"))
m.anonymous = true
m.addremove = false

po = m:option(Flag, "ra0", translate("Green AP mode for 2.4G radio"))

po = m:option(Flag, "rai0", translate("Green AP mode for 5G radio"))

ps = p:section(TypedSection, "usb3", translate("USB 3.0"))
ps.anonymous = true
ps.addremove = false

po = ps:option(Flag, "disable", translate("Disable USB3.0 to prevent interference"))
po.rmempty = false

h = p:section(TypedSection, "hwnat", translate("Hardware NAT control"))
h.anonymous = true
h.addremove = false

po = h:option(Flag, "disable", translate("Disable Hardware NAT"))
po.rmempty = false

po = h:option(ListValue, "debug", translate("Debug level"))
po:value(0, translate("None"))
po:value(1, translate("Error"))
po:value(2, translate("Notice"))
po:value(4, translate("Info"))
po:value(7, translate("Loud"))
po.default = 0

po = h:option(ListValue, "direction", translate("Offload direction"))
po:value(0, translate("Upstream"))
po:value(1, translate("Downstream"))
po:value(2, translate("Bi-directional"))
po.default = 0

po = h:option(Value, "entries1", translate("Max entries allowed (>3/4 free)"))
po.optional    = true
po.placeholder = 8
po.datatype    = "uinteger"

po = h:option(Value, "entries2", translate("Max entries allowed (>1/2 free)"))
po.optional    = true
po.placeholder = 8
po.datatype    = "uinteger"

po = h:option(Value, "entries3", translate("Max entries allowed (<1/2 free)"))
po.optional    = true
po.placeholder = 8
po.datatype    = "uinteger"

g = p:section(TypedSection, "stuff", translate("Extra options"))
g.anonymous = true
g.addremove = false

po = g:option(Value, "wps", translate("WPS button GPIO"))

po.optional    = true
po.placeholder = 4
po.datatype    = "uinteger"

function p.on_commit(map)
	sys.call ("RELOAD=true /sbin/services > /dev/null")
end
return p
