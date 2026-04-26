--- @extends Node

local aimMode: boolean = false;
local mouseModeCaptured: boolean = true;

function _ready(): nil
	Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED);
end

function _input(event: InputEvent): nil
	if event:is_action_pressed("aim_mode") then
		aimMode = true;
		
		local onAimModeSig = Global.OnAimModeChanged;
		print("onAimModeSig",onAimModeSig, type(onAimModeSig), typeof(onAimModeSig))
		print("onAimModeSig connections", #onAimModeSig:get_connections())
		Global.OnAimModeChanged:emit(aimMode);
		
	elseif event:is_action_released("aim_mode") then
		aimMode = false;
		Global.OnAimModeChanged:emit(aimMode);
		
	end
end

function _process(_delta: number): nil
	if Input.is_action_just_pressed("exit") then
		print("exit pressed");
		mouseModeCaptured = not mouseModeCaptured;
		if mouseModeCaptured then
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED);
		else
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE);
		end
	end
end
