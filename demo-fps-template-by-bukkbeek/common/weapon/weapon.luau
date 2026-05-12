--- @extends Node3D

local mainCamera: Camera3D;
local weaponRayCast: RayCast3D;
local rayCastOrigin: Node3D;

function _ready(): nil
	Global.aim_mode_changed.connect(_on_aim_mode_changed);
end

function _on_aim_mode_changed(aimMode: boolean): nil
end

function initialize(mc: Camera3D, rc: RayCast3D, rco: Node3D): nil
	mainCamera = mc;
	weaponRayCast = rc;
	rayCastOrigin = rco;
end

function deactivate(): nil
	visible = false;
end
