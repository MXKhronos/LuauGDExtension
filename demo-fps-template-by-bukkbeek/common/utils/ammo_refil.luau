--- @extends Area3D
-- Translated from GodotFPS-Template-main/common/utils/ammo_refil.gd

outlineMesh = nil :: Node3D;

-- MARK: Ammo Stock
pistolAmmo = 999 :: number;
smgAmmo = 999 :: number;
assaultAmmo = 999 :: number;
shotgunAmmo = 999 :: number;
lmgAmmo = 999 :: number;
sniperAmmo = 999 :: number;
handGrenadeAmmo = 999 :: number;
smokeGrenadeAmmo = 999 :: number;
rocketAmmo = 999 :: number;
fuelAmmo = 999 :: number;
plasmaAmmo = 999 :: number;

local playerInRange: boolean = false;
local isDepleted: boolean = false;

function _ready(): nil
	if outlineMesh ~= nil then
		outlineMesh.Visible = false;
	end
end

function _unhandled_input(event: InputEvent): nil
	if isDepleted or not playerInRange or Global.IsInVehicle then
		return;
	end
	if event:IsActionPressed("activate") then
		RefillPlayer();
	end
end

function RefillPlayer()
	local gaveAny: boolean = false;

	local refillMap = {
		{ Bullets.AmmoType.PISTOL,        pistolAmmo },
		{ Bullets.AmmoType.SMG,           smgAmmo },
		{ Bullets.AmmoType.ASSAULT,       assaultAmmo },
		{ Bullets.AmmoType.SHOTGUN,       shotgunAmmo },
		{ Bullets.AmmoType.LMG,           lmgAmmo },
		{ Bullets.AmmoType.SNIPER,        sniperAmmo },
		{ Bullets.AmmoType.HAND_GRENADE,  handGrenadeAmmo },
		{ Bullets.AmmoType.SMOKE_GRENADE, smokeGrenadeAmmo },
		{ Bullets.AmmoType.ROCKET,        rocketAmmo },
		{ Bullets.AmmoType.FUEL,          fuelAmmo },
		{ Bullets.AmmoType.PLASMA,        plasmaAmmo },
	};

	for _, entry in ipairs(refillMap) do
		local ammoType = entry[1];
		local stock = entry[2];
		if stock <= 0 then
			continue;
		end

		local before = Bullets:GetAmmo(ammoType);
		local maxAmmo = Bullets:GetMaxAmmo(ammoType);
		if before == -1 or before >= maxAmmo then
			continue;
		end

		local canGive = math.min(stock, maxAmmo - before);
		Bullets:AddAmmo(ammoType, canGive);
		DrainStock(entry, canGive);
		gaveAny = true;
	end

	if gaveAny then
		Global.OnBulletsChanged:Emit();
		CheckDepleted();
	end
end

function DrainStock(entry, amount: number)
	if     entry[1] == Bullets.AmmoType.PISTOL        then pistolAmmo        -= amount;
	elseif entry[1] == Bullets.AmmoType.SMG          then smgAmmo           -= amount;
	elseif entry[1] == Bullets.AmmoType.ASSAULT      then assaultAmmo       -= amount;
	elseif entry[1] == Bullets.AmmoType.SHOTGUN      then shotgunAmmo       -= amount;
	elseif entry[1] == Bullets.AmmoType.LMG          then lmgAmmo           -= amount;
	elseif entry[1] == Bullets.AmmoType.SNIPER       then sniperAmmo        -= amount;
	elseif entry[1] == Bullets.AmmoType.HAND_GRENADE then handGrenadeAmmo   -= amount;
	elseif entry[1] == Bullets.AmmoType.SMOKE_GRENADE then smokeGrenadeAmmo -= amount;
	elseif entry[1] == Bullets.AmmoType.ROCKET       then rocketAmmo        -= amount;
	elseif entry[1] == Bullets.AmmoType.FUEL         then fuelAmmo          -= amount;
	elseif entry[1] == Bullets.AmmoType.PLASMA       then plasmaAmmo        -= amount;
	end
end

function CheckDepleted()
	local total: number = (pistolAmmo + smgAmmo + assaultAmmo + shotgunAmmo +
		lmgAmmo + sniperAmmo + handGrenadeAmmo + smokeGrenadeAmmo +
		rocketAmmo + fuelAmmo + plasmaAmmo);
	if total <= 0 then
		isDepleted = true;
	end
end

function OnBodyEntered(body: Node3D)
	if body ~= nil and body:IsInGroup("player") then
		playerInRange = true;
		if outlineMesh ~= nil then
			outlineMesh.Visible = true;
		end
	end
end

function OnBodyExited(body: Node3D)
	if body ~= nil and body:IsInGroup("player") then
		playerInRange = false;
		if outlineMesh ~= nil then
			outlineMesh.Visible = false;
		end
	end
end
