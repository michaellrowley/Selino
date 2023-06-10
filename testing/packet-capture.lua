callbacks = {}

function userdata_to_string(array)
    local binary_string = ""
    for _, value in ipairs(array) do
        binary_string = binary_string .. string.char(value)
    end
    return binary_string
end

ChunkType = { CONNECTION_END = "\x00", CONNECTION_START = "\x01", PACKET_RECEIVED = "\x05", PACKET_LENGTH = "\x02",
    SOURCE_STRING = "\x03", DESTINATION_STRING = "\x04" }
function generate_log_chunk(type, data)
    return type .. data .. ((type == ChunkType.SOURCE_STRING or type == ChunkType.DESTINATION_STRING) and "." or "" )
end

callbacks.init = function()
    pcap_file = io.open("./packet.cap", "w+")
    pcap_file:seek("end", 0);
    return true
end

callbacks.tunnel_opened = function(source_address, source_port, destination_address, destination_port)
    packet_log = generate_log_chunk(ChunkType.CONNECTION_START, "")
    .. generate_log_chunk(ChunkType.SOURCE_STRING, source_address .. ":" .. tostring(source_port))
    .. generate_log_chunk(ChunkType.DESTINATION_STRING, destination_address .. ":" .. tostring(destination_port))
    pcap_file:write(packet_log)
    pcap_file:flush()
end

callbacks.tunneling_data = function(source_address, source_port, destination_address, destination_port, data_buffer, data_size)
    packet_log = generate_log_chunk(ChunkType.PACKET_RECEIVED, "")
    .. generate_log_chunk(ChunkType.SOURCE_STRING, source_address .. ":" .. tostring(source_port))
    .. generate_log_chunk(ChunkType.DESTINATION_STRING, destination_address .. ":" .. tostring(destination_port))
    .. generate_log_chunk(ChunkType.PACKET_LENGTH, string.char(data_size & 0xFF, (data_size >> 8) & 0xFF, (data_size >> 16) & 0xFF, (data_size >> 24) & 0xFF))
    .. userdata_to_string(data_buffer)
    pcap_file:write(packet_log)
    pcap_file:flush()
end

callbacks.tunnel_closed = function(source_address, source_port, destination_address, destination_port)
    packet_log = generate_log_chunk(ChunkType.CONNECTION_END, "")
    .. generate_log_chunk(ChunkType.SOURCE_STRING, source_address .. ":" .. tostring(source_port))
    .. generate_log_chunk(ChunkType.DESTINATION_STRING, destination_address .. ":" .. tostring(destination_port))
    pcap_file:write(packet_log)
    pcap_file:flush()
end
