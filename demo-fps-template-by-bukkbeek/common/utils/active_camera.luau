--- @extends Node3D
-- Translated from GodotFPS-Template-main/common/utils/active_camera.gd

shakeIntensity = 0.02 :: number;
shakeDuration = 0.1 :: number;

local BASE_FOV: number = 75.0;
local SPRINT_FOV: number = 85.0;
local targetFov: number = BASE_FOV;
local isSprinting: boolean = false;
local isAiming: boolean = false;
local weaponAimFov: number = 60.0;

local shakeTween: Tween = nil;
local originalCameraRotation: Vector3 = Vector3(0, 0, 0);

local cameraHolder: Node3D = nil;
local mainCamera: Camera3D = nil;

function _ready(): nil
	cameraHolder = GetNode("CameraHolder");
	mainCamera = GetNode("%MainCamera");

	Global.OnCameraShake:Connect(OnCameraShake);
	Global.OnActiveCameraFovChanged:Connect(OnWeaponAimFovChanged);
	Global.OnPlayerSprintingChanged:Connect(OnPlayerSprintingChanged);
	Global.OnAimModeChanged:Connect(OnAimModeChanged);
end

function _process(delta: number): nil
	-- Smoothly interpolate camera FOV
	mainCamera.Fov = lerp(mainCamera.Fov, targetFov, delta * 10.0);
end

function OnWeaponAimFovChanged(fovAngle: number)
	-- Store weapon aim FOV for when we're in aim mode
	weaponAimFov = fovAngle;
	UpdateTargetFov();
end

function OnPlayerSprintingChanged(sprinting: boolean)
	isSprinting = sprinting;
	UpdateTargetFov();
end

function OnAimModeChanged(aimMode: boolean)
	isAiming = aimMode;
	UpdateTargetFov();
end

function UpdateTargetFov()
	-- Priority: Aim mode > Sprint > Base
	if isAiming then
		targetFov = weaponAimFov;
	elseif isSprinting then
		targetFov = SPRINT_FOV;
	else
		targetFov = BASE_FOV;
	end
end

function OnCameraShake()
	-- Kill existing shake tween if running
	if shakeTween then
		shakeTween:Kill();
	end

	-- Store original rotation
	originalCameraRotation = cameraHolder.Rotation;

	-- Apply random shake offset
	local shakeOffset = Vector3(
		randfRange(-shakeIntensity, shakeIntensity),
		randfRange(-shakeIntensity, shakeIntensity),
		randfRange(-shakeIntensity, shakeIntensity)
	);

	cameraHolder.Rotation = originalCameraRotation + shakeOffset;

	-- Tween back to original rotation
	shakeTween = createTween();
	shakeTween:Connect("finished", function()
		shakeTween = nil;
	end);
	shakeTween:TweenProperty(cameraHolder, "rotation", originalCameraRotation, shakeDuration);
end
