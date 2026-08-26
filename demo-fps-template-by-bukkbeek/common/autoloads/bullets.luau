--- @extends Node
-- Translated from GodotFPS-Template-main/common/autoloads/bullets.gd
-- Autoload as `Bullets`.

AmmoType = {
	KNIFE = 0,
	PISTOL = 1,
	SMG = 2,
	ASSAULT = 3,
	SHOTGUN = 4,
	LMG = 5,
	SNIPER = 6,
	HAND_GRENADE = 7,
	SMOKE_GRENADE = 8,
	ROCKET = 9,
	FUEL = 10,
	PLASMA = 11,
};

-- Ammo counts for each type (-1 == infinite)
local ammoInventory = {
	[AmmoType.KNIFE]        = { Current = -1,  Max = -1 },
	[AmmoType.PISTOL]       = { Current = 60,  Max = 120 },
	[AmmoType.SMG]          = { Current = 150, Max = 300 },
	[AmmoType.ASSAULT]      = { Current = 480, Max = 480 },
	[AmmoType.SHOTGUN]      = { Current = 20,  Max = 40 },
	[AmmoType.LMG]          = { Current = 100, Max = 400 },
	[AmmoType.SNIPER]       = { Current = 10,  Max = 30 },
	[AmmoType.HAND_GRENADE] = { Current = 3,   Max = 10 },
	[AmmoType.SMOKE_GRENADE]= { Current = 2,   Max = 8 },
	[AmmoType.ROCKET]       = { Current = 2,   Max = 4 },
	[AmmoType.FUEL]         = { Current = 200, Max = 400 },
	[AmmoType.PLASMA]       = { Current = 50,  Max = 150 },
};

-- Get current ammo for a type
function GetAmmo(ammoType: number): number
	local entry = ammoInventory[ammoType];
	if entry ~= nil then
		return entry.Current;
	end
	return 0;
end

-- Get max ammo for a type
function GetMaxAmmo(ammoType: number): number
	local entry = ammoInventory[ammoType];
	if entry ~= nil then
		return entry.Max;
	end
	return 0;
end

-- Add ammo (never adds to infinite types)
function AddAmmo(ammoType: number, amount: number)
	local entry = ammoInventory[ammoType];
	if entry ~= nil then
		if entry.Current == -1 then
			return;
		end
		entry.Current = math.min(entry.Current + amount, entry.Max);
	end
end

-- Consume ammo; returns false when there is not enough
function ConsumeAmmo(ammoType: number, amount: number): boolean
	local entry = ammoInventory[ammoType];
	if entry ~= nil then
		if entry.Current == -1 then
			return true;
		end
		if entry.Current >= amount then
			entry.Current -= amount;
			return true;
		end
	end
	return false;
end
