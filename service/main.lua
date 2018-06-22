local skynet = require "skynet"

skynet.start(function()
	skynet.error("Server start")
	if not skynet.getenv "daemon" then
		local console = skynet.newservice("console")
	end
	local proto = skynet.uniqueservice "protoloader"
	skynet.call(proto, "lua","load",{
		"c2s",
		"s2c",
	})
end)