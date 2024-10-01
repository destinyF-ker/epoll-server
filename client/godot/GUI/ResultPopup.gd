extends Control
class_name ResultPopup

signal back_to_main_page
signal stay

@onready var _dialog := $Popup as Popup
@onready var _labelMessage := $Popup/MarginContainer/VBoxContainer/CenterContainer/Label as Label
@onready var _labelTitle := $Popup/MarginContainer/VBoxContainer/LabelTitle as Label
@onready var _buttonStay := $Popup/MarginContainer/VBoxContainer/HBoxContainer/ButtonStay
@onready var _buttonBack := $Popup/MarginContainer/VBoxContainer/HBoxContainer/ButtonBack

func _ready():
	MessageParser.connect("uuid_got", Callable(self, "_on_uuid_got"))
	MessageParser.connect("global_player_info_got", Callable(self, "_on_global_player_info_got"))
	GameState.connect("error_message", Callable(self, "_on_error"))

func _on_ButtonStay_pressed() -> void:
	emit_signal("stay")

func _on_ButtonBack_pressed() -> void:
	emit_signal("back_to_main_page")

func hidePopup() -> void:
	_dialog.hide()

func showPopup() -> void:
	_dialog.show()

func _on_uuid_got() -> void:
	_labelTitle.text = "Connecting……"
	_labelMessage.text = "got uuid! waiting for\nglobal player info sync."

func _on_global_player_info_got() -> void:
	_labelTitle.text = "Connecting……"
	_labelMessage.text = "got global player info!\nprepare to start the game."
	await get_tree().create_timer(GameState.BORN_TIMEOUT).timeout # Delay for timeout
	hidePopup()
	self.hide()
	
func _on_error(message: String) -> void:
	_labelTitle.text = "Error!!!"
	_labelMessage.text = message

func disable_Stay_Button() -> void:
	_buttonStay.set_disabled(true)

func enable_Stay_Button() -> void:
	_buttonStay.set_disabled(false)

func disable_Back_Button() -> void:
	_buttonBack.set_disabled(true)

func enable_Back_Button() -> void:
	_buttonBack.set_disabled(false)
