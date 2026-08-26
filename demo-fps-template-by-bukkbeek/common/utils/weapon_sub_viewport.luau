--- @extends SubViewport
-- Translated from GodotFPS-Template-main/common/utils/weapon_sub_viewport.gd

local camera: Camera3D = nil;
local mainCamera: Camera3D = nil;

function _ready(): nil
	camera = GetNode("Camera3D");
	mainCamera = GetNode("%MainCamera");
end

function _process(_delta: number): nil
	camera.GlobalTransform = mainCamera.GlobalTransform;
end
