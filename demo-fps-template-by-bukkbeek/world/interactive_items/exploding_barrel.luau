--- @extends StaticBody3D
-- Translated from GodotFPS-Template-main/world/interactive_items/exploding_barrel.gd

local destroyableItemsInRange = {};
local isDestroyed: boolean = false;
local barrelDamage: number = 200.0;
local blastRadius: Area3D = nil;
local explosionVfx: Node3D = nil;

function _ready(): nil
	blastRadius = GetNode("BlastRadius");
	explosionVfx = GetNode("ExplosionVFX");
end

function GetDamage(_damage: number, _direction: Vector3)
	Destroy();
end

function Destroy()
	if isDestroyed then return; end
	isDestroyed = true;

	GetNode("barrel").Visible = false;
	GetNode("CollisionShape3D").Disabled = true;
	explosionVfx:Activate();

	destroyableItemsInRange = blastRadius:GetOverlappingBodies();
	if #destroyableItemsInRange > 0 then
		for _, item in destroyableItemsInRange do
			if item:HasMethod("GetDamage") then
				local direction = GlobalPosition:DirectionTo(item.GlobalPosition);
				item:GetDamage(barrelDamage, direction);
			end
		end
	end

	await(2.0);
	QueueFree();
end

