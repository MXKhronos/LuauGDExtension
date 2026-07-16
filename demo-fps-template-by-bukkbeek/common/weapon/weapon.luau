--- @extends Node3D

local mainCamera: Camera3D;
local weaponRayCast: RayCast3D;
local rayCastOrigin: Node3D;

function _ready(): nil
	Global.OnAimModeChanged:Connect(_onAimModeChanged);
end

function _onAimModeChanged(aimMode: boolean): nil
end

function initialize(mc: Camera3D, rc: RayCast3D, rco: Node3D)
	mainCamera = mc;
	weaponRayCast = rc;
	rayCastOrigin = rco;
end

function deactivate(): nil
	visible = false;
end
