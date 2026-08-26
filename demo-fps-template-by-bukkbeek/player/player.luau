--- @extends CharacterBody3D
-- Translated from GodotFPS-Template-main/player/player.gd

walkSpeed = 5.0 :: number;
sprintSpeed = 10.0 :: number;
jumpVelocity = 6.0 :: number;
sensitivity = 0.01 :: number;

local speed: number = 0.0;
local GRAVITY: number = -20;
local inertiaAir: number = 7.5;
local inertiaGround: number = 10.0;

-- Head bob
local BOB_FREQ: number = 2.0;
local BOB_AMP: number = 0.08;
local tBob: number = 0.0;

local isAiming: boolean = false;

local cameraPivot: Node3D = nil;
local cameraHolder: Node3D = nil;
local camera: Camera3D = nil;
local audioManager: Node3D = nil;

-- Animation blend speeds
local ANIM_BLEND_SPEED: number = 20.0;

-- Audio tracking
local wasOnFloor: boolean = false;
local footstepTimer: number = 0.0;
local FOOTSTEP_INTERVAL_WALK: number = 0.5;
local FOOTSTEP_INTERVAL_SPRINT: number = 0.3;

local MAX_HEALTH: number = 100.0;
local currentHealth: number = 100.0;
local isAlive: boolean = true;
local HIT_STAGGER: number = 40.0;

function _ready(): nil
	Global.Player = self;
	Global.OnAimModeChanged:Connect(OnAimModeChanged);

	cameraPivot = GetNode("CameraPivot");
	cameraHolder = GetNode("CameraPivot/CameraHolder");
	camera = GetNode("%MainCamera");
	audioManager = GetNode("AudioManager");
end

function _unhandled_input(event: InputEvent): nil
	if typeof(event) ~= "InputEventMouseMotion" then return end;
	if Input:GetMouseMode() ~= Input.MOUSE_MODE_CAPTURED then return end;

	cameraPivot:RotateY(-event.Relative.X * sensitivity);
	cameraHolder:RotateX(-event.Relative.Y * sensitivity);

	local rot = cameraHolder.Rotation;
	cameraHolder.Rotation = Vector3(
		clamp(rot.X, degToRad(-60), degToRad(60)),
		rot.Y,
		rot.Z
	);
end

function _physics_process(delta: number): nil
	local vel = velocity;

	-- Add the gravity
	if not IsOnFloor() then
		vel.Y += GRAVITY * delta;
	end

	-- Detect landing from air
	if IsOnFloor() and not wasOnFloor then
		-- Player just landed
		if audioManager ~= nil then
			audioManager:PlayFootstep(true); -- Play jump/land sound
		end
	end

	-- Update floor state tracker
	wasOnFloor = IsOnFloor();

	-- Handle jump
	if Input:IsActionJustPressed("jump") and IsOnFloor() then
		vel.Y = jumpVelocity;
	end

	-- Handle sprint
	if Input:IsActionPressed("sprint") then
		HandleSprint();
	else
		HandleWalk();
	end

	local inputDir: Vector3 = Input:GetVector("left", "right", "forward", "backward");
	local direction = (cameraPivot.Transform.Basis * Vector3(inputDir.X, 0, inputDir.Y)):Normalized();

	if IsOnFloor() then
		if not direction:IsZeroApprox() then
			vel.X = direction.X * speed;
			vel.Z = direction.Z * speed;
		else
			vel.X = lerp(vel.X, direction.X * speed, delta * inertiaGround);
			vel.Z = lerp(vel.Z, direction.X * speed, delta * inertiaGround);
		end
	else
		vel.X = lerp(vel.X, direction.X * speed, delta * inertiaAir);
		vel.Z = lerp(vel.Z, direction.Z * speed, delta * inertiaAir);
	end

	velocity = vel;

	-- Head bob
	tBob += vel:Length() * (IsOnFloor() and 1 or 0) * delta;
	local holderTransform = cameraHolder.Transform;
	holderTransform.Origin = Headbob(tBob);
	cameraHolder.Transform = holderTransform;

	-- Handle footstep sounds
	HandleFootsteps(delta);

	-- Emit velocity for crosshair
	Global.OnPlayerVelocityChanged:Emit(vel);

	MoveAndSlide();
end

function HandleFootsteps(delta: number): nil
	-- Only play footsteps when on ground and moving
	if IsOnFloor() and velocity:Length() > 0.1 then
		footstepTimer -= delta;

		if footstepTimer <= 0.0 then
			-- Determine interval based on sprint/walk
			local interval = FOOTSTEP_INTERVAL_WALK;
			if Input:IsActionPressed("sprint") then
				interval = FOOTSTEP_INTERVAL_SPRINT;
			end
			footstepTimer = interval;

			-- Play footstep (not a jump landing)
			if audioManager ~= nil then
				audioManager:PlayFootstep(false);
			end
		end
	else
		-- Reset timer when not moving
		footstepTimer = 0.0;
	end
end

function HandleWalk()
	speed = walkSpeed;
	Global.OnPlayerSprintingChanged:Emit(false);
end

function HandleSprint()
	-- Prohibit sprinting when in aim mode
	if isAiming then
		HandleWalk();
		return;
	end

	speed = sprintSpeed;
	Global.OnPlayerSprintingChanged:Emit(true);
end

function OnAimModeChanged(aimMode: boolean)
	isAiming = aimMode;
	-- If we were sprinting and now aiming, revert to walk speed
	if aimMode and Input:IsActionPressed("sprint") then
		HandleWalk();
	end
end

function Headbob(time: number): Vector3
	local pos = Vector3(0, 0, 0);
	pos.Y = math.sin(time * BOB_FREQ) * BOB_AMP;
	pos.X = math.cos(time * BOB_FREQ / 2) * BOB_AMP;
	return pos;
end

function GetDamage(damage: number, direction: Vector3, isEnemyDamage: boolean?)
	local vel = velocity;

	currentHealth -= damage;
	if currentHealth < 0.0 then
		currentHealth = 0.0;
		isAlive = false;
		Global.OnPlayerDead:Emit();
	else
		vel += direction * HIT_STAGGER;
	end

	velocity = vel;
	Global.OnPlayerHealthChanged:Emit(-damage, currentHealth, MAX_HEALTH, isEnemyDamage == true);
end
