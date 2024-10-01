extends Label

func _on_Connection_timeout(retry_count: int):
	var message: String = "can't connect to the server, retrying...(" + str(retry_count) + ")"
	self.set_text(message)

func _on_Player_hit():
	var message: String = "Press enter to retry"
	self.set_text(message)
