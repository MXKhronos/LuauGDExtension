--- @extends Node3D
-- Translated from GodotFPS-Template-main/resources/undead/vfx/undead_death_vfx.gd

local fire: GPUParticles3D = nil;
local sparks: GPUParticles3D = nil;
local smoke: GPUParticles3D = nil;
local debri: GPUParticles3D = nil;
local debriPile: GPUParticles3D = nil;

function _ready(): nil
	fire = GetNode("Fire");
	sparks = GetNode("Sparks");
	smoke = GetNode("Smoke");
	debri = GetNode("Debri");
	debriPile = GetNode("DebriPile");
end

function Activate()
	fire.Emitting = true;
	sparks.Emitting = true;
	smoke.Emitting = true;
	debri.Emitting = true;
	debriPile.Emitting = true;
end
