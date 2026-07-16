--- @extends Node3D

Number = 123.456 :: number;
Word = "HelloWorld" :: string;
Words = {"A"; "B"; "C"} :: {string};
RayNode = nil :: Node3D;
WeaponsArray = {} :: {Node3D};
WeaponsDb = {} :: {Name: string; Config: {Index: number; Obj: Node3D}};

local currentWeaponIndex: number = 0;
local currentWeapon: Node3D = nil;
local currentAimMode: boolean = false;
local isSwitching: boolean = false;


function _ready(): nil
	currentWeaponIndex = 0;
	setupCurrentWeapon();

	Global.OnFireInput:Connect(OnFireInput);
	Global.OnReloadInput:Connect(OnReloadInput);
	Global.OnWeaponSwitchInput:Connect(OnWeaponSwitchInput);
	Global.OnAimModeChanged:Connect(OnAimModeChanged);
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
	if index < 0 or index >= WeaponsArray:Size() then return end;
	if index == currentWeaponIndex and not isSwitching then return end;
	
	if switchTween then
		switchTween:Kill();
	end
	
	for _, weapon in pairs(WeaponsArray) do
		weapon.deactivate();
	end
	
	currentWeaponIndex = index;
	currentWeapon = WeaponsArray[currentWeaponIndex];
	isSwitching = true;
	
	switchTween = createTween();
	switchTween:SetEase(Tween.EASE_IN_OUT);
	switchTween:SetTrans(Tween.TRANS_SINE);
	
	switchTween:TweenProperty(self, "rotation_degrees:x", -30.0, 0.2);
	
	switchTween:TweenCallback(function()
		currentWeapon.activate(currentAimMode)
	end)
	
	switchTween:TweenProperty(self, "rotation_degrees:x", 10.0, 0.2);
	
	switchTween:TweenProperty(self, "rotation_degrees:x", 0.0, 0.2);
	
	switchTween:TweenCallback(function()
		isSwitching = false;
	end)
	
end
