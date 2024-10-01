extends Control
class_name MainPage

signal online(player_name, ip, port)
signal offline

@onready var _textTitle := $GameName as Label
@onready var _textConnectTo := $ConnectToServer/Connect_to as Label
@onready var _editorName := $ConnectToServer/HBoxContainer2/Editors/name_editor as LineEdit
@onready var _editorIP := $ConnectToServer/HBoxContainer2/Editors/host_editor as LineEdit
@onready var _editorPort := $ConnectToServer/HBoxContainer2/Editors/port_editor as LineEdit
@onready var _buttonJoin := $ConnectToServer/HBoxContainer/Join as Button
@onready var _buttonBack := $ConnectToServer/HBoxContainer/Back as Button
@onready var _buttonOnline := $CommandButtons/Online as Button
@onready var _buttonOffline := $CommandButtons/Offline as Button
@onready var _connectToServer := $ConnectToServer as VBoxContainer
@onready var _commandButtons := $CommandButtons as HBoxContainer
#@onready var _tween := $Tween as Tween

func _ready():
	_connectToServer.hide()
	_commandButtons.show()

func _on_offline_button_up():
	emit_signal("offline")

func _on_online_button_up():
	_connectToServer.show()
	_commandButtons.hide()
	
func _on_Join_pressed():
	var player_name: String = _editorName.get_text()
	var host_addr: String = _editorIP.get_text()
	var port_str: String = _editorPort.get_text()
	
	if player_name.is_empty():
		_editorName.set_placeholder("please enter your name!")
		return
	if player_name.length() > 8 or -1 != player_name.find("@"):
		_editorName.clear()
		_editorName.set_placeholder("please enter the right player name, " 
								+ "which length <= 8 and doesn't contain @")
		return
	if (!port_str.is_valid_int()):
		_editorPort.clear()
		_editorPort.set_placeholder("please enter the right port number!")
		return
	if(!host_addr.is_valid_ip_address()):
		_editorIP.clear()
		_editorIP.set_placeholder("please enter the server ipv4 address!")
		return
	
	var port: int = port_str.to_int()
	print_debug("Join game1")
	emit_signal("online", player_name, host_addr, port)

func _on_Back_pressed():
	_connectToServer.hide()
	_commandButtons.show()
