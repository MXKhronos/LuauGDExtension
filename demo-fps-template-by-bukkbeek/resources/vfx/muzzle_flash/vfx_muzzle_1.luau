--- @extends Node3D

local mesh: MeshInstance3D = get_node("MuzzleFlashFPS");
local omniLight: OmniLight3D = get_node("OmniLight3D");
local smoke: GPUParticles3D = get_node("Smoke");
local sparks: GPUParticles3D = get_node("Sparks");

FlashDuration = 0.05;

function _ready(): nil
	activate();
end

function activate(): nil
	showEffects();
	
	local muzzleTimer: Timer = get_tree().create_timer(flash_duration);
	muzzleTimer.timeout:connect(hideEffects);
end

function showEffects(): nil
	mesh.visible = true;
	omniLight.visible = true;
	smoke:restart();
	sparks:restart();
end

function hideEffects(): nil
	mesh.visible = false;
	omniLight.visible = false;
	
end
