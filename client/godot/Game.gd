extends Node

@export var MobNode: PackedScene
@export var PlayerNode: PackedScene

@onready var _enemiesContainer: Node3D = $Enemies
@onready var _playersContainer: Node3D = $Players
@onready var _mainPageUI := $MainPage as MainPage
@onready var _resultPopup := $ResultPopup as ResultPopup
@onready var _restartInterface := $RestartInterface as RestartInterface
@onready var _mobTimer := $MobTimer as Timer
# onready var _spawnTimer := $SpawnTimer as Timer

var _allMobs := {}
var _enemyNameIndex := 0

func _ready():
	randomize()
	_restartInterface.hide()
	_restartInterface.hide_retry()
	_restartInterface.hide_score_label()
	_mainPageUI.show()
	
	_restartInterface.connect("back_to_main_page", Callable(self, "_reset_the_game"))
	_resultPopup.connect("back_to_main_page", Callable(self, "_reset_the_game"))
	_resultPopup.connect("stay", Callable(self, "_stay_and_reconnect"))
	
	MessageParser.connect("some_one_join", Callable(self, "_onPlayerJoin"))
	MessageParser.connect("some_one_quit", Callable(self, "_onPlauerQuit"))
	MessageParser.connect("some_one_reborn", Callable(self, "_born_player"))
	MessageParser.connect("init_player_instance", Callable(self, "_init_Player_Instance"))
	GameState.connect("disconnected", Callable(self, "_on_server_disconnected"))
	
func _on_MobTimer_timeout():
	var mob = MobNode.instantiate()
	
	var mob_spawn_location = get_node("SpawnPath/SpawnLocation")
	mob_spawn_location.progress_ratio = randf()
	
	var target_position
	if GameState.isGameStarted:
		target_position = $Player.transform.origin
	else:
		target_position = Vector3(0, 0, 0)
	mob.initialize(mob_spawn_location.position, target_position)
	
	_enemiesContainer.add_child(mob, true)
	# print_debug(mob.get_name())
	mob.connect("squashed", Callable(_restartInterface, "_on_Mob_squashed"))

func _on_Player_dead():
	# print_debug("hit?")
	_mobTimer.stop()
	_restartInterface.hide_score_label()
	_restartInterface.show_retry()
	
func _unhandled_input(event): # 用于在RestartInterface监控“重新游玩”事件
	if event.is_action_pressed("ui_accept") and _restartInterface._retry.visible:
		_restartInterface.hide_retry()
		_restartInterface._labelScore.text = "Score: " + str(0)
		_restartInterface.show_score_label()
		GameState.score = 0
		_mobTimer.start()
		
		GameState.isGameStarted = true;
		
		var born_position: Vector3 = _generate_random_position()
		_born_player(GameState.myId, GameState.myName, born_position)

func _on_MainPageUI_offline():
	#print_debug("有点无语")
	_restartInterface.show()
	_restartInterface.show_score_label()
	_mainPageUI.hide()
	
	GameState.isGameStarted = true
	GameState.isOnline = false
	
	var born_position: Vector3 = _generate_random_position()
	_born_player(GameState.myId, GameState.myName, born_position)
	
func _on_MainPageUI_online(player_name:String, host_addr: String, port: int):
	$MobTimer.set_paused(true)
	
	_restartInterface.show()
	_restartInterface.show_score_label()
	_mainPageUI.hide()
	_resultPopup.show()
	_resultPopup.showPopup()
	
	GameState.isGameStarted = true
	GameState.isOnline = true
	GameState.joinGame(player_name, host_addr, port)

func _onPlayerJoin(id: String, name: String) -> void:
	print_debug(GameState._allPlayers)
	
func _onPlayerQuit(id: String) -> void:
	var player_instance = \
		_playersContainer.get_node(GameState._allPlayers[id]["playerInstanceName"])
	if player_instance != null:
		player_instance.queue_free()
	
	GameState._allPlayers.erase(id)

func _onPlayerReborn(id: String, name: String, position: Vector3) -> void:
	_born_player(id, name, position)

func _init_Player_Instance():
	var born_position: Vector3 = _generate_random_position()
	_born_player(GameState.myId, GameState.myName, born_position)

func _on_CheckButton_button_up():
	get_tree().reload_current_scene()

func _reset_the_game() -> void:
	if GameState.isOnline:
		GameState.resetNetwork()
		MessagePacker.raw_messages.clear()
		MessagePacker.packaged_byte_messages.clear()
		MessageParser.wait_queue.clear()
	get_tree().reload_current_scene()

# 生成范围为[min, max]的随机整数
func _generate_random_integer(min_i: int, max_i: int):
	return randi() % (max_i - min_i + 1) + min_i

func _generate_random_position() -> Vector3:
	# print_debug("start to set initialize position")
	var x = _generate_random_integer(-20, 20)
	var z = _generate_random_integer(-15, 15)
	
	return Vector3(x, 5, z)

func _born_player(player_id:String, player_name: String, \
		born_position: Vector3) -> void:
	var player = PlayerNode.instantiate()
	player.playerId = player_id
	player.playerName = player_name
	player.initialize(born_position)
	
	if GameState.isOnline:
		if player_id == GameState.myId:
			await get_tree().create_timer(GameState.BORN_TIMEOUT).timeout # Delay for timeout
		_playersContainer.add_child(player)
		if player_id == GameState.myId:
			GameState.playerInstanceName = player.get_name()
		else:
			GameState._allPlayers[player_id]["playerInstanceName"] = player.get_name()
			player.connect("squashed", Callable(_restartInterface, "_on_Mob_squashed"))
	else:
		add_child(player)
	
	if player_id != GameState.myId and GameState.isOnline:
		return
	player.connect("dead", Callable(self, "_on_Player_dead"))
	player.connect("dead", Callable(_restartInterface, "_on_Player_dead"))
	player.connect("game_update", Callable(MessagePacker, "_pack_messages"))

func _stay_and_reconnect() -> void:
	GameState.connect_after_timeout(GameState.RECONNECT_TIMEOUT)

func _on_server_disconnected() -> void:
	var children_count: int = _playersContainer.get_child_count()
	for i in range(children_count):
		var player = _playersContainer.get_child(i)
		player.queue_free()
	
	GameState.resetNetwork()
	MessagePacker.raw_messages.clear()
	MessagePacker.packaged_byte_messages.clear()
	MessageParser.wait_queue.clear()
	
	_resultPopup.show()
	_resultPopup.showPopup()
