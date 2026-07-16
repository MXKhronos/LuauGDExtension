--- @extends Node

local aimMode: boolean = false;
local mouseModeCaptured: boolean = true;

function _ready(): nil
	Input.setMouseMode(Input.MOUSE_MODE_CAPTURED);
end

function _input(event: InputEvent): nil
	if event:IsActionPressed("aim_mode") then
		aimMode = true;
		
		local onAimModeSig = Global.OnAimModeChanged;
		Global.OnAimModeChanged:Emit(aimMode);
		
	elseif event:IsActionReleased("aim_mode") then
		aimMode = false;
		Global.OnAimModeChanged:Emit(aimMode);
		
	end
end

function _process(_delta: number): nil
	if not Global.IsInVehicle then
		-- Firing
		if Input.isActionJustPressed("fire") then
			Global.OnFireInput:Emit();
		end
		
		-- Reload
		if Input.isActionJustPressed("reload") then
			Global.OnReloadInput:Emit();
		end
		
		-- Weapon Switching
		if Input.isActionJustPressed("weapon_change_next") then
			Global.OnWeaponSwitchInput:Emit(1);
		end
		if Input.isActionJustPressed("weapon_change_prev") then
			Global.OnWeaponSwitchInput:Emit(-1);
		end
	end
	
	-- Activate
	if Input.isActionJustPressed("activate") then
		Global.OnActivatePressed:Emit();
	end

	if Input.isActionJustPressed("exit") then
		mouseModeCaptured = not mouseModeCaptured;
		if mouseModeCaptured then
			Input.setMouseMode(Input.MOUSE_MODE_CAPTURED);
		else
			Input.setMouseMode(Input.MOUSE_MODE_VISIBLE);
		end
	end
end
