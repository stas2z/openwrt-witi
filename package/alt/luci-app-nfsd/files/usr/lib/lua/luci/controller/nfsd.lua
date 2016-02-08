
module("luci.controller.nfsd", package.seeall)

function index()

	require("luci.i18n")
	luci.i18n.loadc("vsftpd")

	
	if not nixio.fs.access("/etc/config/nfsd") then
		return
	end

	local page = entry({"admin", "services", "nfsd"}, cbi("nfsd"), _("NFS Service"))
	page.i18n="nfsd"
	page.dependent = true
end
