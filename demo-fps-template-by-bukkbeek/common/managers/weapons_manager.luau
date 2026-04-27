--- @extends Node3D

WeaponsArray = {} :: {Node3D};

local currentWeaponIndex: number = 0;
local currentWeapon: Node3D = nil;
local currentAimMode: boolean = false;
local isSwitching: boolean = false;


function _ready(): nil
	currentWeaponIndex = 0;
	setupCurrentWeapon();

	Global.OnFireInput:connect(OnFireInput);
	Global.OnReloadInput:connect(OnReloadInput);
	Global.OnWeaponSwitchInput:connect(OnWeaponSwitchInput);
	Global.OnAimModeChanged:connect(OnAimModeChanged);
end

function setupCurrentWeapon(): nil
	currentWeapon = WeaponsArray[currentWeaponIndex];
	
	for _, weapon in ipairs(WeaponsArray) do
		weapon.initialize(mainCamera, weaponRayCast, rayCastOrigin);
		weapon.deactivate();
	end
end

function OnFireInput(): nil
	currentWeapon.TryFire(true);
end

function OnReloadInput(): nil
	if currentWeapon.IsMeleeWeapon then return end;
	currentWeapon.StartReload();
end

function OnWeaponSwitchInput(i): nil
	if currentWeapon.IsReloading then return end;
	
	if i > 0 then
		local nextIndex = (currentWeaponIndex +1) % WeaponsArray.size();
		SwitchWeapon(nextIndex);
	else
		local prevIndex = (currentWeaponIndex -1 +WeaponsArray.size()) % WeaponsArray.size();
		SwitchWeapon(prevIndex);
	end
end

function OnAimModeChanged(aimMode: boolean): nil
	currentAimMode = aimMode;
end

function SwitchWeapon(index: number): nil
	if index < 0 or index >= WeaponsArray.size() then return end;
	if index == currentWeaponIndex and not isSwitching then return end;
	
	if switchTween then
		switchTween:kill();
	end
	
	for _, weapon in pairs(WeaponsArray) do
		weapon.deactivate();
	end
	
	currentWeaponIndex = index;
	currentWeapon = WeaponsArray[currentWeaponIndex];
	isSwitching = true;
	
	switchTween = create_tween();
	switchTween:set_ease(Tween.EASE_IN_OUT);
	switchTween:set_trans(Tween.TRANS_SINE);
	
	switchTween:tween_property(self, "rotation_degrees:x", -30.0, 0.2);
	
	switchTween:tween_callback(function()
		currentWeapon.activate(currentAimMode)
	end)
	
	switchTween:tween_property(self, "rotation_degrees:x", 10.0, 0.2);
	
	switchTween:tween_property(self, "rotation_degrees:x", 0.0, 0.2);
	
	switchTween:tween_callback(function()
		isSwitching = false;
	end)
	
end
