extends Node 

signal timeout
signal parse_and_exe
signal disconnected

signal error_message(message)

enum messageType {
	UNKWON_TYPE = 1,    # 未知类型, 用于初始化
	RESPONSE_UUID,      # 响应UUID, 用于客户端初始化自己的UUID
	GLOBAL_PLAYER_INFO, # 全局玩家信息, 用于客户端初始化其他玩家的相关信息
	SOME_ONE_JOIN,      # 有玩家加入, 用于客户端初始化其他玩家的相关信息
	SOME_ONE_QUIT,      # 有玩家退出, 用于客户端初始化其他玩家的相关信息
	GAME_UPDATE,        # 游戏更新, 用于客户端更新游戏状态
	PLAYER_INFO_CERT,   # 玩家信息认证, 用于客户端向服务器端认证玩家信息
	CLIENT_READY        # 客户端准备就绪, 用于客户端通知服务器自己已经准备就绪
}

@export var HOST: String = "127.0.0.1"
@export var PORT: int = 3456
const RECONNECT_TIMEOUT: float = 3.0
const UUID_LEN: int = 36
const TRANSFORM_SIZE: int = 52
const BORN_TIMEOUT: float = 3.0

var is_header_handled: bool = false
var header_size: int = 8
var prepare_to_read: int = header_size 

var myId: String
var myName: String
var playerInstanceName: String
var _allPlayers: Dictionary
var playerMessageQueue: Dictionary
var isGameStarted: bool = false 
var isOnline: bool = false
var isGameReady: bool = false

var retry_count: int = 0
var score: int = 0

const Client = preload("res://Net//basic_client_service.gd")
var _client: Client = null
var _byteArraySendTimer: Timer
var _mainPageUI: Control
var _resultPopup: Control

func _ready() -> void:
	myId = ""
	myName = "You"
	playerInstanceName = ""
	playerMessageQueue = Dictionary()
	_allPlayers = Dictionary()

func connect_after_timeout(timeout: float) -> void:
	if !isGameStarted:
		return
	retry_count += 1
	emit_signal("timeout", retry_count)
	if isOnline:
		_client.connect_to_host(HOST, PORT)

func _handle_client_connected() -> void:
	print("Client connected to server.")

func _handle_server_data() -> void:
	# print_debug("start to handle server data")
	var message: Dictionary
	while prepare_to_read <= _client.recv_buffer.size():
		if is_header_handled:
			if prepare_to_read != 0:
				var body_bytes: PackedByteArray = _client.recv_buffer.slice(0, prepare_to_read - 1, 1, true)
				for i in range(prepare_to_read):
					_client.recv_buffer.pop_front()
				message["data"] = body_bytes
			else:
				message["data"] = PackedByteArray()
				
			prepare_to_read = header_size
			# print_debug("message: ", message)
			MessageParser.wait_queue.append(message)
			emit_signal("parse_and_exe")
			
			is_header_handled = false
		else:
			message = Dictionary()
			var header_bytes: Array = _client.recv_buffer.slice(0, prepare_to_read - 1, 1, true)
			for i in range(prepare_to_read):
				_client.recv_buffer.pop_front()
			
			var message_type_bytes: PackedByteArray = [2, 0, 0, 0]
			var message_length_bytes: PackedByteArray = [2, 0, 0, 0]
			message_type_bytes.append_array(header_bytes.slice(0, 3))
			message_length_bytes.append_array(header_bytes.slice(4, 7))
		
			var message_type: int = bytes_to_var(message_type_bytes)
			var message_length: int = bytes_to_var(message_length_bytes)
			
			# print_debug("messageType: ", message_type)
			# print_debug("messageLength: ", message_length)
			
			prepare_to_read = message_length - header_size
			message["type"] = message_type
			message["length"] = message_length
			
			is_header_handled = true

func _handle_client_disconnected() -> void:
	print("Client disconnected from server.")
	emit_signal("error_message", "Client disconnected from\nserver.Press stay to retry")
	if isGameReady:
		emit_signal("disconnected")
	# _connect_after_timeout(RECONNECT_TIMEOUT) # Try to reconnect after 3 seconds

func _handle_client_error() -> void:
	print("Client error.")
	emit_signal("error_message", "Client error.\nPress stay to retry")
	if isGameReady:
		emit_signal("disconnected")
	# _connect_after_timeout(RECONNECT_TIMEOUT) # Try to reconnect after 3 seconds

func joinGame(player_name: String, ip: String, port: int):
	var root = get_tree().root
	var current_scene = root.get_child(root.get_child_count() - 1)
	_byteArraySendTimer = current_scene.get_node("ByteArraySendTimer")
	_byteArraySendTimer.connect("timeout", Callable(self, "_send_data"))
	
	myName = player_name
	
	_client = Client.new()
	_client.connect("connected", Callable(self, "_handle_client_connected"))
	_client.connect("disconnected", Callable(self, "_handle_client_disconnected"))
	_client.connect("error", Callable(self, "_handle_client_error"))
	_client.connect("data", Callable(self, "_handle_server_data"))
	
	add_child(_client)
	HOST = ip
	PORT = port
	_client.connect_to_host(ip, port)

func _send_data():
	if MessagePacker.packaged_byte_messages.is_empty():
		return
	var message = MessagePacker.packaged_byte_messages.pop_back()
	_client.send(message)

func resetNetwork() -> void:
	print_debug("reset")
	isGameStarted = false
	isGameReady = false
	isOnline = false
	is_header_handled = false
	if _client != null:
		_client.disconnect_from_host()
		_client.queue_free()
		_client = null
	_allPlayers.clear()
	
func sizeof(param) -> void:
	var sizeof = 0
	for bit in var_to_bytes(param):
		sizeof += bit
	print(param)

func rebind() -> void:
	print_debug("rebind")
	var root = get_tree().root
	var current_scene = root.get_child(root.get_child_count() - 1)
	_mainPageUI = current_scene.get_node("MainPage")
	_mainPageUI.connect("online", Callable(self, "_joinGame"))
	_resultPopup = current_scene.get_node("ResultPopup")
	_resultPopup.connect("stay", Callable(self, "_connect_after_timeout"))
