extends Control
class_name RestartInterface

signal back_to_main_page

@onready var _retry = $Retry as ColorRect
@onready var _labelScore = $ScoreLabel as Label
@onready var _labelFinalScore = $Retry/VBoxContainer2/VBoxContainer/FinalScoreLabel as Label
@onready var _labelMessage = $Retry/VBoxContainer2/VBoxContainer/MessageLabel as Label
@onready var _buttonCheck = $Retry/VBoxContainer2/CheckButton as Button

func _on_Mob_squashed():
	GameState.score += 1
	_labelScore.text = "Score: %s" % GameState.score

func _ready():
	_retry.show()
	_labelScore.show()

func _on_CheckButton_pressed():
	emit_signal("back_to_main_page")

func _on_Player_dead():
	_labelFinalScore.text = "You got "+ str(GameState.score) +" Score!"
	
func show_score_label() -> void:
	_labelScore.show()
	# print_debug("scoreLabel: ", _labelScore.visible)
	
func hide_score_label() -> void:
	# print_debug("scoreLabel: ", _labelScore.visible)
	_labelScore.hide()
	# print_debug("scoreLabel: ", _labelScore.visible)

func show_retry() -> void:
	_retry.show()

func hide_retry() -> void:
	_retry.hide()
