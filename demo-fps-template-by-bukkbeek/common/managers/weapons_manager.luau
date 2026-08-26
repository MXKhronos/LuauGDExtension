--- @extends Node3D
-- Translated from GodotFPS-Template-main/common/managers/weapons_manager.gd

local rayCastOrigin: Node3D = nil;
local weaponRayCast: RayCast3D = nil;
local mainCamera: Camera3D = nil;

-- Smooth follow settings
local followSpeed: number = 60.0;

-- Weapon index for switching
-- INFO: AVAILABLE_WEAPONS - Pistol is index 1 (default starting weapon)
WeaponsArray = {} :: {Node3D};
local currentWeaponIndex: number = 1;
local currentWeapon: Node3D = nil;

-- Track aim mode state
local currentAimMode: boolean = false;

-- Weapon switch state
local isSwitching: boolean = false;
local switchTween: Tween = nil;

function _ready(): nil
	rayCastOrigin = GetNode("RayCastOrigin");
	weaponRayCast = GetNode("RayCastOrigin/WeaponRayCast");
	mainCamera = GetNode("%MainCamera");

	-- Start with pistol (index 1)
	currentWeaponIndex = 1;
	SetupCurrentWeapon();

	-- Connect to input signals
	Global.OnFireInput:Connect(OnFireInput);
	Global.OnReloadInput:Connect(OnReloadInput);
	Global.OnWeaponSwitchInput:Connect(OnWeaponSwitchInput);
	Global.OnAimModeChanged:Connect(OnAimModeChanged);
end

function SetupCurrentWeapon(): nil
	currentWeapon = WeaponsArray[currentWeaponIndex];

	-- Initialize all weapons
	for _, weapon in ipairs(WeaponsArray) do
		weapon:Initialize(mainCamera, weaponRayCast, rayCastOrigin);
		weapon:Deactivate();
	end

	-- Activate current weapon (pistol) with current aim mode
	currentWeapon:Activate(currentAimMode);
end

function _process(delta: number): nil
	-- Smoothly interpolate position and rotation to follow camera
	local targetTransform = mainCamera.GlobalTransform;

	-- Clamp the interpolation weight: followSpeed * delta exceeds 1 on
	-- long frames and Basis:Slerp extrapolation past 1 can degenerate to
	-- NaN, which then poisons every subsequent GlobalTransform read.
	local w = clamp(followSpeed * delta, 0.0, 1.0);

	-- Lerp position
	GlobalPosition = GlobalPosition:Lerp(targetTransform.Origin, w);

	-- Slerp rotation (spherical linear interpolation for smooth rotation)
	local gt = GlobalTransform;
	gt.Basis = gt.Basis:Slerp(targetTransform.Basis, w);
	GlobalTransform = gt;

	-- Continuous firing for automatic weapons (burst mode)
	-- Pass false to indicate this is NOT the initial press
	if Input:IsActionPressed("fire") and currentWeapon.burstMode then
		currentWeapon:TryFire(false);
	end
end

function OnFireInput(): nil
	-- This is called on initial press, so pass true
	currentWeapon:TryFire(true);
end

function OnReloadInput(): nil
	if currentWeapon.IsMeleeWeapon then
		return;
	end
	currentWeapon:StartReload();
end

function OnWeaponSwitchInput(direction: number): nil
	if direction > 0 then
		SwitchToNextWeapon();
	else
		SwitchToPreviousWeapon();
	end
end

function OnAimModeChanged(aimMode: boolean): nil
	-- Update tracked aim mode
	currentAimMode = aimMode;

	-- The weapon will handle its own animation through its connected signal
end

-- MARK: Weapon switching functions

function SwitchToNextWeapon(): nil
	if currentWeapon.IsReloading then
		return;
	end
	local nextIndex = (currentWeaponIndex % #WeaponsArray) + 1;
	SwitchWeapon(nextIndex);
end

function SwitchToPreviousWeapon(): nil
	if currentWeapon.IsReloading then
		return;
	end
	local prevIndex = ((currentWeaponIndex - 2 + #WeaponsArray) % #WeaponsArray) + 1;
	SwitchWeapon(prevIndex);
end

function SwitchWeapon(index: number): nil
	if index < 1 or index > #WeaponsArray then
		return;
	end
	if index == currentWeaponIndex and not isSwitching then
		return;
	end

	-- Kill any in-progress tween to prevent stale callbacks running
	if isValid(switchTween) then
		switchTween:Kill();
	end

	-- Deactivate ALL weapons immediately - prevents double-visibility on rapid scroll
	for _, weapon in ipairs(WeaponsArray) do
		weapon:Deactivate();
	end

	-- Update the reference now so rapid calls always operate on the correct weapon
	currentWeaponIndex = index;
	currentWeapon = WeaponsArray[currentWeaponIndex];
	isSwitching = true;

	switchTween = createTween();

	switchTween:SetEase(Tween.EASE_IN_OUT);
	switchTween:SetTrans(Tween.TRANS_SINE);

	-- Rotate down to -30 degrees (0.2s)
	switchTween:TweenProperty(self, "rotation_degrees:x", -30.0, 0.2);

	-- Activate the new weapon at the lowest point of the animation
	switchTween:TweenCallback(function()
		currentWeapon:Activate(currentAimMode);
	end);

	-- Rotate up to 10 degrees (0.2s)
	switchTween:TweenProperty(self, "rotation_degrees:x", 10.0, 0.2);

	-- Return to 0 degrees (0.1s)
	switchTween:TweenProperty(self, "rotation_degrees:x", 0.0, 0.1);

	switchTween:TweenCallback(function()
		isSwitching = false;
	end);
end

-- MARK: Helper functions to get weapon state

function GetCurrentMag(): number
	return currentWeapon:GetCurrentMag();
end

function GetCurrentAmmo(): number
	return currentWeapon:GetCurrentAmmo();
end

function GetMaxAmmo(): number
	return currentWeapon:GetMaxAmmo();
end
