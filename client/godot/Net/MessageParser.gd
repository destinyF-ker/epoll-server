extends Node

signal gen_message
signal some_one_join(player_id, player_name)
signal some_one_quit(player_id)
signal some_one_reborn(player_id, name, position)
signal init_player_instance
signal uuid_got
signal global_player_info_got

var wait_queue: Array = []

func _ready():
# warning-ignore:return_value_discarded
	GameState.connect("parse_and_exe", Callable(self, "_parse_and_exe"))
	
func _parse_and_exe():
	while not wait_queue.is_empty():
		var message: Dictionary = wait_queue.pop_front()
		# print_debug("message type: ", message["type"])
		match message["type"]:
			GameState.messageType.GAME_UPDATE:
				_handle_game_update(message)
			GameState.messageType.RESPONSE_UUID:
				_handle_response_uuid(message)
			GameState.messageType.GLOBAL_PLAYER_INFO:
				_handle_global_player_info(message)
			GameState.messageType.SOME_ONE_JOIN:
				_handle_some_one_join(message)
			GameState.messageType.SOME_ONE_QUIT:
				_handle_some_on_quit(message)
			_:
				print_debug("fatal error!")

func _handle_response_uuid(message: Dictionary):
	# print_debug("response UUID: ", message["data"].get_string_from_ascii())
	GameState.myId = message["data"].get_string_from_ascii()
	
	message["type"] = GameState.messageType.PLAYER_INFO_CERT
	message["data"] = []
	MessagePacker.raw_messages.append(message)
	emit_signal("gen_message")
	emit_signal("uuid_got")

func _handle_global_player_info(message: Dictionary):
	if not message["data"].is_empty():
		var global_player_info_str: String = message["data"].get_string_from_ascii()
		# print_debug("gloal player info: ", global_player_info_str)
	
		var global_player_infos = global_player_info_str.split("@")
		for player_info in global_player_infos:
			var id: String = player_info.substr(0, GameState.UUID_LEN)
			var name: String = player_info.substr(GameState.UUID_LEN, -1)
		
			var info_dict: Dictionary = Dictionary()
			var game_update_queue: Array = []
			info_dict["name"] = name
			info_dict["game_update_queue"] = game_update_queue
			info_dict["is_alive"] = false
		
			GameState._allPlayers[id] = info_dict
	
	message.clear()
	message["type"] = GameState.messageType.CLIENT_READY
	var data: Array = []
	data.append(GameState.myId)
	data.append(GameState.myName)
	data.append("@")
	message["data"] = data
	
	MessagePacker.raw_messages.append(message)
	GameState.isGameReady = true
	emit_signal("gen_message")
	emit_signal("init_player_instance")
	emit_signal("global_player_info_got")

func _handle_some_one_join(message: Dictionary):
	var player_info_str: String = message["data"].get_string_from_ascii()
	print_debug(player_info_str)
	
	var id: String = player_info_str.substr(0, GameState.UUID_LEN)
	var name: String = player_info_str.substr(GameState.UUID_LEN, -1)
	
	var info_dict: Dictionary = Dictionary()
	var game_update_queue: Array = []
	info_dict["name"] = name
	info_dict["game_update_queue"] = game_update_queue
	info_dict["is_alive"] = false
	GameState._allPlayers[id] = info_dict
	
	emit_signal("some_one_join", id, name)

func _handle_some_on_quit(message: Dictionary):
	print_debug("start to handle some one on quit.")
	var player_id: String = message["data"].get_string_from_ascii()
	print_debug(player_id)
	emit_signal("some_one_quit", player_id)

func _handle_game_update(message: Dictionary):
	if !GameState.isGameReady:
		return
	var player_id: String = message["data"].subarray(0, GameState.UUID_LEN - 1).get_string_from_ascii()
	var transform_bytes: PackedByteArray = message["data"].subarray(GameState.UUID_LEN, 
		GameState.UUID_LEN + GameState.TRANSFORM_SIZE - 1)
	# print_debug(transform_bytes.size())
	var update_transform: Transform3D = bytes_to_var(transform_bytes)
	
	if !GameState._allPlayers[player_id]["is_alive"]:
		GameState._allPlayers[player_id]["is_alive"] = true
		var name: String = GameState._allPlayers[player_id]["name"]
		emit_signal("some_one_reborn", player_id, name, update_transform.origin)
		return
	
	GameState._allPlayers[player_id]["game_update_queue"].append(update_transform)

