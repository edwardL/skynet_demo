local skynet = require "skynet"
local socket = require "skynet.socket"
local proxy = require "socket_proxy"
local service = require "service"
local log = require "log"

local hub = {}
local data = {socket = {} }

local function auth_socket(fd)
	return (skynet.call(service.auth,"lua","shakehand",fd))
end

function new_socket(fd,addr)
	data.socket[fd] = "[AUTH]"
	print(57,"new_socket",fd,addr)
	proxy.subscribe(fd)

	---- 转发到
	local ok, userid = pcall(auth_socket,fd)
	if ok then
		print("auth ok")
	else
		print("auth fail")
	end
	
end

function hub.open(ip,port)
	log("Listen %s:%d",ip,port)
	assert(data.fd == nil , "Already open")
	data.fd = socket.listen(ip,port)
	data.ip = ip
	data.port = port
	socket.start(data.fd, new_socket)
end

service.init {
	command = hub,
	info = data,
	require = {
		"auth"
	}
}