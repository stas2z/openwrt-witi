-- Copyright 2008 Yanira <forum-2008@email.de>
-- Copyright 2012 Jo-Philipp Wich <jow@openwrt.org>
-- Licensed to the public under the Apache License 2.0.

local uci = luci.model.uci.cursor_state()
local net = require "luci.model.network"
local m, s, p, b

m = Map("p910nd", translate("Printer Server"),
        translatef("p910nd Printer Server is a small non-spooling printer daemon intended for disk-less workstations."))

net = net.init(m.uci)

s = m:section(TypedSection, "p910nd", translate("Settings"))
s.addremove = true
s.anonymous = true

s:option(Flag, "enabled", translate("Enable"))

s:option(Value, "device", translate("Device")).rmempty = true

b = s:option(Value, "bind", translate("Interface"), translate("Specifies the interface to listen on."))
b.template = "cbi/network_netlist"
b.nocreate = true
b.unspecified = true

function b.cfgvalue(...)
	local v = Value.cfgvalue(...)
	if v then
		return (net:get_status_by_address(v))
	end
end

function b.write(self, section, value)
	local n = net:get_network(value)
	if n and n:ipaddr() then
		Value.write(self, section, n:ipaddr())
	end
end

p = s:option(ListValue, "port", translate("Port"), translate("TCP listener port."))
p.rmempty = true
for i=0,9 do
	p:value(i, 9100+i)
end

s:option(Flag, "bidirectional", translate("Bidirectional mode"))

return m
