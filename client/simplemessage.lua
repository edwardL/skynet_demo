local socket = require "simplesocket"
local sproto = require "sproto"

local message = {}
local var = {
	session_id = 0,
	session = {},
	object = {},
}

local function print_lua_table (lua_table, indent)

	indent = indent or 0
	for k, v in pairs(lua_table) do
		if type(k) == "string" then
			k = string.format("%q", k)
		end
		local szSuffix = ""
		if type(v) == "table" then
			szSuffix = "{"
		end
		local szPrefix = string.rep("    ", indent)
		formatting = szPrefix.."["..k.."]".." = "..szSuffix
		if type(v) == "table" then
			print(formatting)
			print_lua_table(v, indent + 1)
			print(szPrefix.."},")
		else
			local szValue = ""
			if type(v) == "string" then
				szValue = string.format("%q", v)
			else
				szValue = tostring(v)
			end
			print(formatting..szValue..",")
		end
	end
end

function message.register(name)
	local f = assert(io.open(name .. "s2c.sproto"))
	local t = f:read "a"
	f:close()
	var.host = sproto.parse(t):host "package"
	local f = assert(io.open(name .. "c2s.sproto"))
	local t = f:read "a"
	f:close()
	var.request = var.host:attach(sproto.parse(t))
end

function message.peer(addr,port)
	var.addr = addr
	var.port = port
end

function message.connect()
	socket.connect(var.addr,var.port)
	socket.isconnect()
end

function message.bind(obj,handler)
	var.object[obj] = handler
end

function message.request(name,args)
	var.session_id = var.session_id
	var.session[var.session_id] = {name = name, req = args}
	socket.write(var.request(name,args,var.session_id))
	return var.session_id
end

function message.update(ti)
	local msg = socket.read(ti)
	if not msg then
		return false
	end
	local t, session_id , resp , err = var.host:dispatch(msg)
	if t == "REQUEST" then
		for obj , handler in pairs(var.object) do
			local f = handler[session_id]
			if f then
				local ok, err_msg = pcall(f,obj,resp)
				if not ok then
					print(string.format("push %s for [%s] error: %s",session_id, tostring(obj), err_msg))
				end
			end
		end
	else
		-- local session = var.session[session_id]
		-- var.session[session_id] = nil
		print("response", t , session_id , resp , err)
		local session = var.session[session_id]
		var.session[session_id] = nil
		print_lua_table(resp)
		print("----------------")
		print_lua_table(session)

		for obj , handler in pairs(var.object) do
			print_lua_table(handler)
			local f = handler[session.name]
			if f then
				local ok, err_msg = pcall(f, handler ,session,resp)
				print("asdasd",ok,err_msg)
			end
		end
	end
end

return message