--- @extends Node3D
-- Translated from GodotFPS-Template-main/common/projectile/projectile.gd

local rayCast: RayCast3D = nil;
local mesh: Node3D = nil;
local decal: Node3D = nil;
local sparks: GPUParticles3D = nil;

local SPEED: number = 60.0;
local DAMAGE: number = 20.0;
local bulletHit: boolean = false;

function _ready(): nil
	rayCast = GetNode("RayCast3D");
	mesh = GetNode("mesh");
	decal = GetNode("decal");
	sparks = GetNode("ElectricSparks_1");

	decal.Visible = false;
end

function _physics_process(delta: number): nil
	if not rayCast:IsColliding() then
		Position += GlobalTransform.Basis * Vector3(0, 0, -SPEED) * delta;
	elseif not bulletHit then
		BulletImpact();
		bulletHit = true;
	end
end

function BulletImpact()
	local collider = rayCast:GetCollider();
	if collider ~= nil and collider:HasMethod("GetDamage") then
		collider:GetDamage(DAMAGE, Vector3(0, 1, 0));
	end

	mesh.Visible = false;
	decal.Visible = true;
	sparks.Emitting = true;

	await(5.0);

	QueueFree();
end
