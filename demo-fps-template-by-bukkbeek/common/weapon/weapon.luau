--- @extends Node3D
-- Translated from GodotFPS-Template-main/common/weapon/weapon.gd

-- MARK: exports (weapon settings)
isMeleeWeapon = false :: boolean;
isProjectile = false :: boolean;
firingDistance = 20.0 :: number;

fpsArmsRoot = nil :: Node3D;
animationPlayer = nil :: AnimationPlayer;
animationFire = "_fire" :: string;
animationReload = "_reload" :: string;

-- MARK: exports (bullets)
bulletType = 0 :: number;
damage = 100.0 :: number;
cooldown = 1.0 :: number;
burstMode = false :: boolean;
consume = 1 :: number;
reloadRequired = true :: boolean;
magCapacity = 30 :: number;
reloadTime = 1.5 :: number;

-- MARK: exports (gameplay connections)
focusedAimFov = 60.0 :: number;
focusedAimAngle = 0.0 :: number;
standardAimAngle = 2.0 :: number;

-- MARK: exports (sfx)
firingSfx = nil :: AudioStreamPlayer3D;
reloadSfx = nil :: AudioStreamPlayer3D;
emptyMagSfx = nil :: AudioStreamPlayer3D;

-- MARK: exports (vfx)
firingVfx = nil :: PackedScene;
muzzleFlashPosition = nil :: Marker3D;
bulletDecal = nil :: PackedScene;

-- MARK: state read by weapons_manager.luau
IsMeleeWeapon = false :: boolean;
IsReloading = false :: boolean;

local AIM_TRANSITION_DURATION = 0.2;

local standardAimPosition: Marker3D = nil;
local focusedAimPosition: Marker3D = nil;
local cooldownTimer: Timer = nil;
local reloadTimer: Timer = nil;

local isAimed: boolean = false;
local canFire: boolean = true;
local playedEmptySoundThisPress: boolean = false;

local mainCamera: Camera3D = nil;
local weaponRayCast: RayCast3D = nil;
local rayCastOrigin: Node3D = nil;

local aimTween: Tween = nil;
local currentMag: number = 0;

-- MARK: helpers

local bulletsChecked = false;
local bulletsAutoload = nil;

-- Resolves the (not yet ported) Bullets autoload exactly once so failed
-- lookups do not spam the __index warning per frame.
local function getBullets()
	if not bulletsChecked then
		bulletsChecked = true;
		bulletsAutoload = Bullets;
	end
	return bulletsAutoload;
end

local function getInventoryAmmo(): number
	local bullets = getBullets();
	if bullets == nil then
		return -1; -- -1 == infinite
	end
	return bullets:GetAmmo(bulletType);
end

local function consumeInventoryAmmo(amount: number): boolean
	local bullets = getBullets();
	if bullets == nil then
		return true; -- inventory not ported: always allow
	end
	return bullets:ConsumeAmmo(bulletType, amount);
end

local function playAnim(animName: string)
	if animationPlayer ~= nil and animName ~= nil and animName ~= "" then
		animationPlayer:Play(animName);
	end
end

local function playSfx(sfx)
	if sfx ~= nil then
		sfx:Play();
	end
end

-- MARK: lifecycle

function _ready(): nil
	IsMeleeWeapon = isMeleeWeapon;

	standardAimPosition = GetNode("StandardAimPosition");
	focusedAimPosition = GetNode("FocusedAimPosition");
	cooldownTimer = GetNode("CooldownTimer");
	reloadTimer = GetNode("ReloadTimer");

	local testCamera = GetNode("TestCamera");
	if testCamera ~= nil then testCamera:Hide(); end
	local testTarget = GetNode("TestTarget");
	if testTarget ~= nil then testTarget:Hide(); end

	-- Timer timeouts are connected in weapon_base.tscn ([connection] blocks).

	Global.OnAimModeChanged:Connect(OnAimModeChanged);

	-- Initialize magazine
	currentMag = magCapacity;
end

function Initialize(camera: Camera3D, raycast: RayCast3D, raycastOrigin: Node3D)
	mainCamera = camera;
	weaponRayCast = raycast;
	rayCastOrigin = raycastOrigin;
end

function Activate(currentAimMode: boolean?)
	visible = true;
	canFire = true;
	IsReloading = false;

	if isValid(aimTween) then
		aimTween:Kill();
	end

	-- Update raycast distance for this weapon
	if weaponRayCast then
		weaponRayCast.TargetPosition = Vector3(0, 0, -firingDistance);
	end

	-- Snap to the correct aim position for this weapon
	isAimed = currentAimMode == true;
	if fpsArmsRoot then
		if isAimed then
			fpsArmsRoot.Transform = focusedAimPosition.Transform;
		else
			fpsArmsRoot.Transform = standardAimPosition.Transform;
		end
	end

	-- Emit FOV only if weapon is activated while aiming
	if isAimed then
		Global.OnActiveCameraFovChanged:Emit(focusedAimFov);
	end
end

function Deactivate()
	visible = false;
end

-- MARK: firing

function TryFire(isInitialPress: boolean?): boolean
	-- Reset empty sound flag on initial press
	if isInitialPress then
		playedEmptySoundThisPress = false;
	end

	if not canFire or IsReloading then
		return false;
	end

	-- Melee weapons skip all ammo and reload checks
	if not IsMeleeWeapon then
		-- Check if reload is needed
		if reloadRequired and currentMag < consume then
			StartReload();
			-- Play empty mag sound only once per press
			if emptyMagSfx ~= nil and not playedEmptySoundThisPress then
				emptyMagSfx:Play();
				playedEmptySoundThisPress = true;
			end
			return false;
		end

		-- Check ammo in inventory (0 = just check, don't consume yet)
		if not consumeInventoryAmmo(0) then
			if emptyMagSfx ~= nil and not playedEmptySoundThisPress then
				emptyMagSfx:Play();
				playedEmptySoundThisPress = true;
			end
			return false;
		end
	end

	FireWeapon();
	return true;
end

function FireWeapon()
	-- Consume ammo from magazine or inventory (melee skips this entirely)
	if not IsMeleeWeapon then
		if reloadRequired then
			currentMag -= consume;
		else
			if not consumeInventoryAmmo(consume) then
				return;
			end
		end
		Global.OnBulletsChanged:Emit();
	end

	-- Play fire animation
	playAnim(animationFire);

	-- Play fire SFX (melee weapons are silent)
	if not IsMeleeWeapon then
		playSfx(firingSfx);
	end

	-- Muzzle flash VFX (ranged weapons only)
	if not IsMeleeWeapon and firingVfx ~= nil and muzzleFlashPosition ~= nil then
		local muzzleInstance = firingVfx:Instantiate();
		muzzleFlashPosition:AddChild(muzzleInstance);
	end

	-- Perform raycast hit detection
	if weaponRayCast ~= nil and rayCastOrigin ~= nil then
		-- Apply recoil/spread rotation
		local recoilAngle = standardAimAngle;
		if isAimed then
			recoilAngle = focusedAimAngle;
		end
		rayCastOrigin.RotationDegrees = Vector3(
			randfRange(-recoilAngle, recoilAngle),
			randfRange(-recoilAngle, recoilAngle),
			0
		);

		weaponRayCast:ForceRaycastUpdate();

		-- Reset rotation
		rayCastOrigin.Rotation = Vector3(0, 0, 0);

		if weaponRayCast:IsColliding() then
			local collider = weaponRayCast:GetCollider();
			local hitPoint = weaponRayCast:GetCollisionPoint();
			local hitNormal = weaponRayCast:GetCollisionNormal();

			-- Bullet decal (ranged weapons only)
			if not IsMeleeWeapon and bulletDecal ~= nil then
				local decalInstance = bulletDecal:Instantiate();
				GetTree().Root:AddChild(decalInstance);
				decalInstance:SetGlobalPosition(hitPoint);
				-- Use a different up vector if hit normal is nearly vertical
				local upVector = Vector3(0, 1, 0);
				if math.abs(hitNormal:Dot(Vector3(0, 1, 0))) > 0.99 then
					upVector = Vector3(0, 0, -1);
				end
				decalInstance:LookAt(hitPoint + hitNormal, upVector);
			end

			-- Check if collider can take damage
			if collider ~= nil and collider:HasMethod("GetDamage") then
				local direction = (hitPoint - GlobalPosition):Normalized();
				collider:GetDamage(damage, direction);
			end
		end
	end

	-- Start cooldown
	canFire = false;
	cooldownTimer:Start(cooldown);

	-- Camera shake
	Global.OnCameraShake:Emit();
end

-- MARK: reloading

function StartReload()
	if IsReloading or not reloadRequired then
		return;
	end

	-- Check if we have ammo to reload
	local availableAmmo = getInventoryAmmo();
	if availableAmmo <= 0 and availableAmmo ~= -1 then
		return;
	end

	IsReloading = true;

	-- Play reload animation
	playAnim(animationReload);

	-- Play reload SFX
	playSfx(reloadSfx);

	-- Start reload timer
	reloadTimer:Start(reloadTime);
end

function OnCooldownTimerTimeout()
	canFire = true;
end

function OnReloadTimerTimeout()
	IsReloading = false;

	-- Calculate ammo needed
	local ammoNeeded = magCapacity - currentMag;
	local availableAmmo = getInventoryAmmo();

	-- Handle infinite ammo
	if availableAmmo == -1 then
		currentMag = magCapacity;
	else
		local ammoToLoad = math.min(ammoNeeded, availableAmmo);
		consumeInventoryAmmo(ammoToLoad);
		currentMag += ammoToLoad;
	end

	Global.OnBulletsChanged:Emit();
end

-- MARK: aim mode

function OnAimModeChanged(aimMode: boolean)
	-- Only respond to aim changes if this weapon is currently active
	if not visible then
		return;
	end

	isAimed = aimMode;

	-- Kill any existing aim tween to prevent conflicts
	if isValid(aimTween) then
		aimTween:Kill();
	end

	if fpsArmsRoot ~= nil and focusedAimPosition ~= nil and standardAimPosition ~= nil then
		-- Create a new tween for smooth transition
		aimTween = createTween();
		-- Finished tweens are freed by the engine; drop the reference so the
		-- `if aimTween then aimTween:Kill() end` guards stay valid.
		aimTween:Connect("finished", function()
			aimTween = nil;
		end);
		aimTween:SetParallel(true);
		aimTween:SetEase(Tween.EASE_IN_OUT);
		aimTween:SetTrans(Tween.TRANS_CUBIC);

		-- Tween to the target position
		local targetTransform = standardAimPosition.Transform;
		if aimMode then
			targetTransform = focusedAimPosition.Transform;
		end
		aimTween:TweenProperty(fpsArmsRoot, "transform", targetTransform, AIM_TRANSITION_DURATION);
	end

	-- Emit FOV change only when entering aim mode
	if aimMode then
		Global.OnActiveCameraFovChanged:Emit(focusedAimFov);
	end
end

-- MARK: hud helpers

function GetCurrentMag(): number
	if IsMeleeWeapon then return -1; end
	return currentMag;
end

function GetCurrentAmmo(): number
	if IsMeleeWeapon then return -1; end
	return getInventoryAmmo();
end

function GetMaxAmmo(): number
	if IsMeleeWeapon then return -1; end
	local bullets = getBullets();
	if bullets == nil then return -1; end
	return bullets:GetMaxAmmo(bulletType);
end
