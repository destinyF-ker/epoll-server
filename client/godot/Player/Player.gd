extends CharacterBody3D
class_name Player

signal dead(byWho)
signal game_update
signal squashed

@export var speed = 14
@export var fall_acceleration = 75
@export var jump_impulse = 20
@export var bounce_impulse = 20
@export var deadAudio: AudioStream = null

@onready var _aminationPlayer := $AnimationPlayer as AnimationPlayer
@onready var _audioPlayer := $AudioStreamPlayer as AudioStreamPlayer3D
@onready var _labelName := $PlayerName as Label3D
@onready var _updateTimer := $UpdateTimer as Timer

var _velocity = Vector3.ZERO

var playerId: String = "1231231"
var playerName := "You"

func initialize(start_position):
	var target_position = start_position + Vector3.FORWARD
	look_at_from_position(start_position, target_position, Vector3.UP)

func _ready():
	_labelName.text = playerName
	_updateTimer.connect("timeout", Callable(self, "_game_update"))
	if !GameState.isOnline:
		_updateTimer.stop()
	
func _game_update():
	# print_debug("_game_update")
	if playerId == GameState.myId:
		# print_debug("true player")
		var update_transform: Transform3D = self.get_global_transform()
		var message: Dictionary = Dictionary()
		var data: Array = []
		data.append(GameState.myId)
		data.append(update_transform)
		message["type"] = GameState.messageType.GAME_UPDATE
		message["data"] = data
		
		# var tmp: PoolByteArray = var2bytes(update_transform)
		# print_debug(tmp.size())
		
		MessagePacker.raw_messages.append(message)
		emit_signal("game_update")
	else:
		if GameState._allPlayers[playerId]["game_update_queue"].is_empty():
			return
		var update_tarnsform: Transform3D = \
			GameState._allPlayers[playerId]["game_update_queue"].pop_front()
		
		var previous_position = self.get_global_transform().origin
		var new_position: Vector3 = update_tarnsform.origin
		
		var direction: Vector3 = new_position - previous_position
		if direction != Vector3.ZERO:
			direction = direction.normalized()
			# $Pivot.look_at(translation + direction, Vector3.UP)
			$AnimationPlayer.set_speed_scale(3)
		else:
			$AnimationPlayer.set_speed_scale(1)
		
		look_at_from_position(new_position, new_position + direction, Vector3.UP)
		for index in range(get_slide_collision_count()):
			var collision = get_slide_collision(index)
			if collision.collider.is_in_group("mob") or collision.collider.is_in_group("player"):
				var collider = collision.collider
				if Vector3.UP.dot(collision.normal) > 0.1:
					collider.squash()
		
func _physics_process(delta):
	if playerId != GameState.myId and GameState.isOnline:
		return
	
	var direction = Vector3.ZERO
	if Input.is_action_pressed("move_right"):
		direction.x += 1
	if Input.is_action_pressed("move_left"):
		direction.x -= 1
	if Input.is_action_pressed("move_forward"):
		direction.z -= 1
	if Input.is_action_pressed("move_back"):
		direction.z += 1
	
	if direction != Vector3.ZERO:
		direction = direction.normalized()
		$Pivot.look_at(position + direction, Vector3.UP)
		if not Input.is_action_pressed("jump"):
			$AnimationPlayer.speed_scale = 4.0
		else:
			$AnimationPlayer.speed_scale = 0.0
	else:
		$AnimationPlayer.speed_scale = 1.0
	
	_velocity.x = direction.x * speed
	_velocity.z = direction.z * speed
		
	_velocity.y -= fall_acceleration * delta
	
	if is_on_floor() and Input.is_action_pressed("jump"):
		_velocity.y += jump_impulse
		
	set_velocity(_velocity)
	set_up_direction(Vector3.UP)
	move_and_slide()
	_velocity = velocity
	_labelName.set_rotation_degrees(Vector3( - 45, 0, 0))
	
	for index in range(get_slide_collision_count()):
		var collision = get_slide_collision(index)
		
		if collision.get_collider().is_in_group("mob") or \
			collision.get_collider().is_in_group("player"):
			var collider = collision.get_collider()
			if Vector3.UP.dot(collision.get_normal()) > 0.1:
				collider.squash()
				_velocity.y = bounce_impulse
	
	$Pivot.rotation.x = PI / 6 * _velocity.y / jump_impulse

func die():
	queue_free()
	if playerId != GameState.myId and GameState.isOnline:
		GameState._allPlayers[playerId]["is_alive"] = false
		return
	print_debug("dead!")
	emit_signal("dead")

func squash() -> void:
	emit_signal("squashed")
	queue_free()

func _on_MobDetector_body_entered(_body):
	print_debug("mob_collide_detected!")
	GameState.isGameStarted = false
	die()

func _on_VisibilityNotifier_screen_exited():
	print_debug("screen_exited! in main")
	GameState.isGameStarted = false
	die()

func _on_PlayerDetector_body_entered(body):
	print_debug("playerDetector body entered!")
	GameState.isGameStarted = false
	#die()
