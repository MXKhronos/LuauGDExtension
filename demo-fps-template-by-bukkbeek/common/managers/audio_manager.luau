--- @extends Node3D
-- Translated from GodotFPS-Template-main/common/managers/audio_manager.gd

player = nil :: CharacterBody3D;

local groundDetectRayCast: RayCast3D = nil;
local footstepGround: AudioStreamPlayer3D = nil;
local footstepJump: AudioStreamPlayer3D = nil;
local footstepMetal: AudioStreamPlayer3D = nil;

local isOnMetal: boolean = false;

function _ready(): nil
	groundDetectRayCast = GetNode("GroundDetectRayCast");
	footstepGround = GetNode("FootstepGround");
	footstepJump = GetNode("FootstepJump");
	footstepMetal = GetNode("FootstepMetal");

	if groundDetectRayCast ~= nil then
		groundDetectRayCast.Enabled = true;
		groundDetectRayCast.TargetPosition = Vector3(0, -1.5, 0);
		groundDetectRayCast.CollisionMask = 1; -- Adjust based on your collision layers
	end
end

function _physics_process(_delta: number): nil
	CheckSurfaceType();
end

function CheckSurfaceType()
	if groundDetectRayCast ~= nil and groundDetectRayCast:IsColliding() then
		local collider = groundDetectRayCast:GetCollider();
		if collider ~= nil and collider:IsInGroup("metal") then
			isOnMetal = true;
		else
			isOnMetal = false;
		end
	else
		isOnMetal = false;
	end
end

function PlayFootstep(isJump: boolean)
	if isJump then
		if footstepJump ~= nil and not footstepJump.Playing then
			footstepJump:Play();
		end
	else
		if isOnMetal then
			if footstepMetal ~= nil and not footstepMetal.Playing then
				footstepMetal:Play();
				footstepGround:Play();
			end
		else
			if footstepGround ~= nil and not footstepGround.Playing then
				footstepGround:Play();
			end
		end
	end
end
