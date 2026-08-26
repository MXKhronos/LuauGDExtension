--- @extends PhysicalBone3D
-- Translated from GodotFPS-Template-main/common/enemy/enemy_bone_damage.gd

damageMultiplier = 1.0 :: number;

OnUpdateDamage = signal("OnUpdateDamage");

function GetDamage(damage: number, direction: Vector3)
	local finalDamage = damage * damageMultiplier;
	OnUpdateDamage:Emit(finalDamage, direction);
end
