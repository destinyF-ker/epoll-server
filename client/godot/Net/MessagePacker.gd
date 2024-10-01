extends Node

#var Player = null

var raw_messages: Array = []
var packaged_byte_messages: Array = []

func _ready():
	MessageParser.connect("gen_message", Callable(self, "_pack_messages"))

func _pack_messages():
	if raw_messages.is_empty():
		return
	
	while not raw_messages.is_empty():
		var packaged_byte_message: PackedByteArray = PackedByteArray()
		var raw_message = raw_messages.pop_front()
		# print_debug(raw_message["type"])
		var type_byte_array = var_to_bytes(raw_message["type"]).slice(4, 8)
		
		var data_byte_array: PackedByteArray = []
		if not raw_message["data"].is_empty():
			for data in raw_message["data"]:
				if typeof(data) == TYPE_STRING:
					data_byte_array.append_array(data.to_ascii_buffer())
				else:
					data_byte_array.append_array(var_to_bytes(data))
			
		var data_length_byte_array: PackedByteArray = \
			var_to_bytes(data_byte_array.size()).slice(4, 7)
		
		packaged_byte_message.append_array(type_byte_array)
		packaged_byte_message.append_array(data_length_byte_array)
		packaged_byte_message.append_array(data_byte_array)
	
		packaged_byte_messages.push_back(packaged_byte_message)
