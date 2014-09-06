# Decompile some binary script files found in Level-5's games (in particular, Youkai Watch)

import struct
import zlib
import sys

pretty_print = False

data = bytearray(open(sys.argv[1],"rb").read())

lines = struct.unpack("<I", data[0:4])[0]
string_table_offset = struct.unpack("<I", data[4:8])[0]
string_table_size = struct.unpack("<I", data[8:12])[0]

strings = str(data[string_table_offset:string_table_offset+string_table_size])
string_table = {}

key_table = {}

key_table_offset = string_table_offset + string_table_size
key_table_offset += 16 - (key_table_offset % 16)
key_table_size = struct.unpack("<I", data[key_table_offset:key_table_offset+4])[0]
key_count = struct.unpack("<I", data[key_table_offset+4:key_table_offset+8])[0]
key_offset = struct.unpack("<I", data[key_table_offset+8:key_table_offset+12])[0]
key_string_length = struct.unpack("<I", data[key_table_offset+12:key_table_offset+16])[0]
key_strings = data[key_table_offset + key_offset:key_table_offset + key_offset + key_string_length]

#print "%08x %08x %08x %08x %08x" % (key_table_offset, key_table_size, key_count, key_offset, key_string_length)
key_hash_table_length = key_offset - 0x10
for i in range(0, key_count):
	crc32 = struct.unpack("<I", data[key_table_offset + 0x10 + (i * 8):key_table_offset + 0x10 + (i * 8) + 4])[0]
	offset = struct.unpack("<I", data[key_table_offset + 0x10 + (i * 8) + 4:key_table_offset + 0x10 + (i * 8) + 8])[0]

	key = key_strings[offset:key_strings.index('\0', offset + 1)]
	
	#print "%08x %08x %s" % (crc32, offset, key)
	
	key_table[crc32] = key
	
i = 0
start = 0
while i < len(strings):
	if strings[i] == '\x00':
		string_table[start] = strings[start:i]
		start = i + 1
	i+=1

code_data = data[0x10:string_table_offset]

i = 0
depth = 0
cnt = 0
add_newline = False
last_param_type = 0xff
commands = []
while i < len(code_data) and cnt < lines:
	if i >= len(code_data):
		break

	output = ""

	crc32 = struct.unpack("<I", code_data[i:i+4])[0]
	param_info = struct.unpack("<H", code_data[i+4:i+6])[0]
	param_type = param_info >> 8
	param_count = param_info & 0xff
	terminator = struct.unpack("<H", code_data[i+6:i+8])[0]
	
	#print "%08x: %02x %08x %d %d %04x %d" % (0x10+i, cnt, crc32, param_count, param_type, terminator, depth)
	
	i += 8
				
	output += key_table[crc32] + " "
	for x in range(0, param_count):
		param = struct.unpack("<I", code_data[i:i + 4])[0]
		type = (param_type >> (2 * x)) & 0x03
		
		if type == 1: # Integer
			output += "%d" % param
		elif type == 2: # Float
			param = struct.unpack("<f", code_data[i:i + 4])[0]
			output += ('%f' % param).rstrip('0').rstrip('.')
		else:
		#elif type == 0: # String
			if param in string_table:
				output += '"%s"' % (string_table[param])
			else:
				output += "[%08x]" % param
		
		if x + 1 < param_count:
			output += ", "
			
		i += 4
	
	output = output.rstrip() + ";"
	
	if add_newline:
		output += "test\n"
	
	cnt += 1
	last_param_type = param_type
		
		
	commands.append((output, param_type))

#commands.append(("\n// End of file;", 0xff)) # How the real source scripts seemed to end

depth = 0
last_type = 0
#print commands
final_output = ""
for i in range(0, len(commands)):
	command = commands[i]
	
	if i - 1 >= 0 and commands[i-1][0].startswith("PTVAL") and commands[i][0].startswith("PTVAL"):
		depth -= 1
	
	if command[1] == 0xff and last_type == 0xff:
		depth -= 1
	
	if i - 1 >= 0 and commands[i-1][0].startswith("PTVAL") and commands[i][0].startswith("PTREE"):
		depth -= 1
		
	if pretty_print:
		final_output += "%s%s\n" % ('\t' * depth, command[0]) 
	else:
		final_output += "%s\n" % (command[0]) 
		
	last_type = command[1]		
		
	if command[1] != 0xff:
		if i + 1 < len(commands) and commands[i+1][1] == 0xff:
			if depth - 1 != 0:
				depth -= 1
		else:
			depth += 1
				
	if depth == 0:
		final_output += "\n"
		
print final_output	