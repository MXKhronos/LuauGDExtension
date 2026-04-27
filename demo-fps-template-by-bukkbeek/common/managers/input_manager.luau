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
		Global.OnAimModeChanged:emit(aimMode);
		
	elseif event:is_action_released("aim_mode") then
		aimMode = false;
		Global.OnAimModeChanged:emit(aimMode);
		
	end
end

function _process(_delta: number): nil
	if not Global.IsInVehicle then
		-- Firing
		if Input.is_action_just_pressed("fire") then
			Global.OnFireInput:emit();
		end
		
		-- Reload
		if Input.is_action_just_pressed("reload") then
			Global.OnReloadInput:emit();
		end
		
		-- Weapon Switching
		if Input.is_action_just_pressed("weapon_change_next") then
			Global.OnWeaponSwitchInput:emit(1);
		end
		if Input.is_action_just_pressed("weapon_change_prev") then
			Global.OnWeaponSwitchInput:emit(-1);
		end
	end
	
	-- Activate
	if Input.is_action_just_pressed("activate") then
		Global.OnActivatePressed:emit();
	end

	if Input.is_action_just_pressed("exit") then
		mouseModeCaptured = not mouseModeCaptured;
		if mouseModeCaptured then
			Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED);
		else
			Input.set_mouse_mode(Input.MOUSE_MODE_VISIBLE);
		end
	end
end
