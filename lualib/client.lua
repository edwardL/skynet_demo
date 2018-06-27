local skynet = require "skynet"
local proxy = require "socket_proxy"
local sprotoloader = require "sprotoloader"
local log = require "log"

local client = {}
local host --- 解析数据的
local sender 
local handler = {}

function client.handler()
	return handler
end

function client.dispatch( c )
	local fd = c.fd
	proxy.subscribe(fd)
	print("client.dispatch",c.fd)
	local ERROR = {}
	while true do
		local msg , sz = proxy.read(fd)
		print("msg , sz",msg , sz)
		local type, name, args, response = host:dispatch(msg,sz)
		print("type, name, args, response",type, name, args, response)		
		assert(type == "REQUEST")
		local f = handler[name]
		if f then
			--- f may block
			skynet.fork(function()
				local ok,result = pcall(f,c,args)
				if ok then
					proxy.write(fd,response(result))
				else
					log("raise errror = %s",result)
					proxy.write(fd,response(ERROR,result))
				end
			end)

		else
			error ("invalid command " .. name)
		end
	end
end

function client.close(fd)
	proxy.close(fd)
end

function client.push(c,t,data)
	proxy.write(c.fd,sender(t,data))
end

function client.init(name)
	return function ()
		local protoloader = skynet.uniqueservice "protoloader"
		local slot = skynet.call(protoloader,"lua","index",name .. "c2s")
		print(57,"slot",slot)
		host = sprotoloader.load(slot):host "package"
		local slot2 = skynet.call(protoloader,"lua","index",name .. "s2c")
		sender = host:attach(sprotoloader.load(slot2))
	end
end

return client

