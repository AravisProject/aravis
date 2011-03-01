-- dissects the Gvcp Protocol
gvcp_proto = Proto("gvcp", "Gigabit ethernet camera control protocol")

local f = gvcp_proto.fields

local packet_types = {
	[0x0000] = "Acknowledge",
	[0x4201] = "Command"
}

local commands = {
	[0x0002] = "DiscoverCmd",
	[0x0003] = "DiscoverAck",
	[0x0004] = "ByeCmd",
	[0x0005] = "ByeAck",
	[0x0040] = "PacketResendCmd",
	[0x0041] = "PacketResendAck",
	[0x0080] = "RegisterReadCmd",
	[0x0081] = "RegisterReadAck",
	[0x0082] = "RegisterWriteCmd",
	[0x0083] = "RegisterWriteAck",
	[0x0084] = "MemoryReadCmd",
	[0x0085] = "MemoryReadAck",
	[0x0086] = "MemoryWriteCmd",
	[0x0087] = "MemoryWriteAck"
}

f.packet_type = ProtoField.uint16 ("gvcp.packet_type",  "PacketType", nil, packet_types)
f.command = ProtoField.uint16 ("gvcp.command",  "Command", nil, commands)
f.size = ProtoField.uint16 ("gvcp.size", "Size");
f.count = ProtoField.uint16 ("gvcp.count", "Count");
f.register_address = ProtoField.uint32 ("gvcp.register_address", "RegisterAddress");
f.register_value = ProtoField.uint32 ("gvcp.register_value", "RegisterValue");
f.memory_address = ProtoField.uint32 ("gvcp.memory_address", "MemoryAddress");
f.memory_size = ProtoField.uint32 ("gvcp.memory_size", "MemorySize");
f.memory_value = ProtoField.string ("gvcp.memory_value", "MemoryValue");

local packet_counter

function gvcp_proto.init ()
	packet_counter = 0
end

-- create a function to dissect it
function gvcp_proto.dissector(buffer,pinfo,tree)

	local subtree = tree:add (gvcp_proto, buffer())
	local offset = 0

	local packet_type = buffer (offset, 2)
	subtree:add (f.packet_type, packet_type)
	offset = offset + 2

	local command = buffer (offset, 2)
	cmd = command:uint()

	subtree:add (f.command, command)
	offset = offset + 2

	subtree:add (f.size, buffer (offset, 2))
	offset = offset + 2

	subtree:add (f.count, buffer (offset, 2))

	offset = offset + 2

	if cmd == 0x80 then -- Register read cmd
		subtree:add (f.register_address, buffer (offset, 4))
		offset = offset + 4
	elseif cmd == 0x81 then -- Register read ans
		subtree:add (f.register_value, buffer (offset, 4))
		offset = offset + 4
	elseif cmd == 0x84 then -- Memory read cmd
		subtree:add (f.memory_address, buffer (offset, 4))
		offset = offset + 4
		subtree:add (f.memory_size, buffer (offset, 4))
		offset = offset + 4
	elseif cmd == 0x85 then -- Memory read ans
		subtree:add (f.memory_address, buffer (offset, 4))
		offset = offset + 4
		subtree:add (f.memory_value, buffer (offset, 16):string())
	end

end

-- load the tcp.port table
udp_table = DissectorTable.get("udp.port")
-- register our protocol to handle udp port 3956
udp_table:add(3956,gvcp_proto)

