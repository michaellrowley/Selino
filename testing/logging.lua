function array_to_hex(arr, len, shorten)
    local string_formatted = ""
    local put_trailing_dots = false
    for i = 1, len, 1 do
        -- Rounds down in the division so that if the caller only wants 'n' (where 'n' is odd) bytes in the string, the first half of the string has the extra byte.
        if len > 20 and shorten and i > math.ceil(shorten / 2) and i < (len + 1) - math.floor(shorten / 2) then
            if not put_trailing_dots then
                -- This appends a few dots (implied '*snip*') to ensure only 'shortlen' bytes are included in the hex string.
                string_formatted = string_formatted .. " ... "
                put_trailing_dots = true
            end
        else
            string_formatted = string_formatted .. "\\x" .. string.format("%02x", arr[i])
        end
    end
    return string_formatted
end

-- Create an empty table where callback functions are to be located.
callbacks = {}

callbacks.init = function()
    -- This function doesn't get any args...
    print("Initialization function called in logging.lua")
    return true
end

callbacks.connection_received = function(incoming_address, incoming_port, remote_address, remote_port)
    print("Received a connection on local '" .. incoming_address .. ":" .. tostring(incoming_port) .. "' from '" .. remote_address .. ":" .. tostring(remote_port) .. "'")
end

callbacks.connection_closed = function(incoming_address, incoming_port, remote_address, remote_port)
    print("Terminated a connection on local '" .. incoming_address .. ":" .. tostring(incoming_port) .. "' from '" .. remote_address .. ":" .. tostring(remote_port) .. "'")
end

callbacks.protocol_packet = function(protocol_name, packet_index, packet_buffer, packet_size, local_addr, local_port, remote_addr, remote_port)
	print("Packet received:\n\tProtocol: '" .. protocol_name .. "'\n\tPacket Index: '" .. tostring(packet_index) .. "'\n\tLocal Endpoint: '" .. local_addr .. ":" .. tostring(local_port) .. "'\n\tRemote Endpoint: '" .. remote_addr .. "'\n\tRemote Port: '" .. tostring(remote_port) .. "'\n\tRaw Packet: '" .. array_to_hex(packet_buffer, packet_size) .. "'")
end

callbacks.tunneling_data = function(source_address, source_port, destination_address, destination_port, data_buffer, data_size)
    print("Forwarding '" .. tostring(data_size) .. "' bytes ('" .. source_address .. ":" .. tostring(source_port) .. "' ==> '" .. destination_address .. ":" .. tostring(destination_port) .. "':\n\tRaw Data: '" .. array_to_hex(data_buffer, data_size, 11) .. "')")
end

callbacks.tunnel_closed = function(source_address, source_port, destination_address, destination_port)
    print("Tunnel ('" .. source_address .. ":" .. tostring(source_port) .. "' <==> '" .. destination_address .. ":" .. tostring(destination_port) .. "') has been closed")
end

callbacks.socks4_command = function(local_port, conn_addr, conn_port, user_id, command_str, dest_addr, dest_port, dest_addr_original)
    -- For the User_ID check, '#' is LUA's length operator (but it works with more diverse data types than in C++), think of it as std::string::length, std::vector::size(), or strlen() wrapped into one.
	print("Received a SOCKS4 request:\n\tIncoming Port: '" .. tostring(local_port) .. "'\n\tIncoming Endpoint: '" .. conn_addr .. ":" .. tostring(conn_port) .. "'\n\tUser ID: " .. (#user_id > 0 and ("'" .. user_id .. "'") or "NONE") .. "\n\tCommand: '" .. command_str .. "'\n\tDestination: '" .. dest_addr .. ":" .. tostring(dest_port) .. "'" .. (dest_addr == dest_addr_original and "" or (" ('" .. dest_addr_original .. "')")))
end

callbacks.socks5_command = function(incoming_address, incoming_port, remote_address, remote_port, request_type, destination_address, destination_port, original_destination_location)
    print("Received a SOCKS5 request:\n\tIncoming Port: '" .. tostring(local_port) .. "'\n\tIncoming Endpoint: '" .. remote_address .. ":" .. tostring(remote_port) .. "'\n\tCommand: '" .. request_type .. "'\n\tDestination: '" .. destination_address .. ":" .. tostring(destination_port) .. "'" .. (destination_address == original_destination_location and "" or (" ('" .. original_destination_location .. "')")))
end

-- callbacks.socks5_choose_auth = function(options_arr, options_len)
--     for i = 1, options_len do
--         if not (options_arr[i] == 0x02) then
--             options_arr[i] = 0xFF
--         end
--     end
-- end

-- callbacks.socks5_auth_loop_2 = function(config, buffer)
--     if (config["loop_count"] == nil) then
--         config["loop_count"] = "1"
--     end

--     local loop_count = tonumber(config["loop_count"])

--     if loop_count == 1 then
--         config["status"] = "receive"
--         config["receive_amount"] = "2"
--     elseif loop_count == 2 then
--         print(array_to_hex(buffer, tonumber(config["received_amount"])))
--         config["status"] = "end"
--     end

--     config["loop_count"] = tostring(loop_count + 1) -- Lua has no '++' operator
-- end

callbacks.protocol_fail = function(protocol_name, reason)
	print("Protocol failed:\n\tProtocol: '" .. protocol_name .. "'\n\tError Message: '" .. reason .. "'")
end
