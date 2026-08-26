--- @extends Node3D
-- Translated from GodotFPS-Template-main/resources/vfx/explosions/vfx_standard_explosion.gd

local omniLight: OmniLight3D = nil;
local fire: GPUParticles3D = nil;
local sparks: GPUParticles3D = nil;
local smoke: GPUParticles3D = nil;
local debri: GPUParticles3D = nil;
local debriSmoke: GPUParticles3D = nil;
local explosionSfx: AudioStreamPlayer3D = nil;

function _ready(): nil
	omniLight = GetNode("OmniLight3D");
	fire = GetNode("Fire");
	sparks = GetNode("Sparks");
	smoke = GetNode("Smoke");
	debri = GetNode("Debri");
	debriSmoke = GetNode("DebriSmoke");
	explosionSfx = GetNode("ExplosionSFX");
end

function Activate()
	omniLight.Visible = true;
	fire:Restart();
	sparks:Restart();
	smoke:Restart();
	debri:Restart();
	explosionSfx:Play();

	await(0.1);

	omniLight.Visible = false;
end
