--- @extends Node3D
-- Translated from GodotFPS-Template-main/resources/vfx/muzzle_flash/vfx_muzzle_1.gd

local mesh: MeshInstance3D = nil;
local omniLight: OmniLight3D = nil;
local smoke: GPUParticles3D = nil;
local sparks: GPUParticles3D = nil;

flashDuration = 0.05 :: number;

function _ready(): nil
	mesh = GetNode("MuzzleFlashFPS");
	omniLight = GetNode("OmniLight3D");
	smoke = GetNode("Smoke");
	sparks = GetNode("Sparks");

	Activate();
end

function Activate()
	ShowEffects();

	await(flashDuration);

	HideEffects();
end

function ShowEffects()
	mesh.Visible = true;
	omniLight.Visible = true;
	smoke:Restart();
	sparks:Restart();
end

function HideEffects()
	mesh.Visible = false;
	omniLight.Visible = false;

	await(2.0);

	QueueFree();
end
