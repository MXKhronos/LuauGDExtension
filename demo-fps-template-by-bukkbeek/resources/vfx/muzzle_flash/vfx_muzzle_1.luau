--- @extends Node3D

local mesh: MeshInstance3D = GetNode("MuzzleFlashFPS");
local omniLight: OmniLight3D = GetNode("OmniLight3D");
local smoke: GPUParticles3D = GetNode("Smoke");
local sparks: GPUParticles3D = GetNode("Sparks");

FlashDuration = 0.05;

function _ready(): nil
	activate();
end

function activate(): nil
	showEffects();
	
	local muzzleTimer: Timer = GetTree():CreateTimer(flash_duration);
	muzzleTimer.OnTimeout:Connect(hideEffects);
end

function showEffects(): nil
	mesh.Visible = true;
	omniLight.Visible = true;
	smoke:Restart();
	sparks:Restart();
end

function hideEffects(): nil
	mesh.Visible = false;
	omniLight.Visible = false;
	
end
