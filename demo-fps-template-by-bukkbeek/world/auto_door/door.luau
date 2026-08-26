--- @extends Node3D
-- Translated from GodotFPS-Template-main/world/auto_door/door.gd

local animationPlayer: AnimationPlayer = nil;

function _ready(): nil
	animationPlayer = GetNode("AnimationPlayer");
	CloseDoor();
end

function OpenDoor()
	animationPlayer:Play("door_open");
	GetNode("DoorSFX"):Play();
end

function CloseDoor()
	animationPlayer:Play("door_close");
	GetNode("DoorSFX"):Play();
end

function OnArea3DBodyEntered(body: Node3D)
	if body:IsInGroup("door_access") then
		OpenDoor();
	end
end

function OnArea3DBodyExited(body: Node3D)
	if body:IsInGroup("door_access") then
		CloseDoor();
	end
end
