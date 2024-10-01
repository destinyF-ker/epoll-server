extends CharacterBody3D
class_name Mob

signal squashed

@export var min_speed = 10
@export var max_speed = 20

#var velocity = Vector3.ZERO

func _physics_process(_delta):
	set_velocity(velocity)
	move_and_slide()
	# $AnimationPlayer.speed_scale = 4
	
func initialize(start_position, player_position):
	look_at_from_position(start_position, player_position, Vector3.UP)
	rotate_y(randf_range( - PI / 4, PI / 4))
	
	var random_speed = randf_range(min_speed, max_speed)
	velocity = Vector3.FORWARD * random_speed
	velocity = velocity.rotated(Vector3.UP, rotation.y)
	
	$AnimationPlayer.set_speed_scale(random_speed / min_speed * 2)

func _on_VisibilityNotifier_screen_exited():
	queue_free()

func squash():
	emit_signal("squashed")
	queue_free()
