--- @extends CharacterBody3D

local walkSpeed: number = 5.0;
local sprintSpeed: number = 10.0;
local jumpVelocity: number = 6.0;
local speed: number = 0.0;
local GRAVITY: number = -20;

local sensitivity: number = 0.01;
local inertiaAir: number = 7.5;
local inertiaGround: number = 10.0;

local cameraPivot: Node3D = GetNode("CameraPivot");
local cameraHolder: Node3D = GetNode("CameraPivot/CameraHolder");

local wasOnFloor: boolean = false;
local isAiming: boolean = false;

function _ready(): nil
	Global.Player = self;
	Global.OnAimModeChanged:Connect(OnAimModeChanged);
end

function _unhandled_input(event: InputEvent): nil
	if typeof(event) ~= "InputEventMouseMotion" then return end;
	if Input.getMouseMode() ~= Input.MOUSE_MODE_CAPTURED then return end;
	
	cameraPivot:RotateY(-event.Relative.X * sensitivity);
	cameraHolder:RotateX(-event.Relative.Y * sensitivity);
	
	local camRot = cameraHolder.Rotation;
	cameraHolder.Rotation = Vector3(
		clamp(camRot.X, degToRad(-60), degToRad(60)),
		camRot.Y,
		camRot.Z
	);
end

function _physics_process(delta: number): nil
	local newVelocity = velocity;
	-- Add the gravity
	if not IsOnFloor() then
		newVelocity.Y += GRAVITY * delta;
	end;

	-- Detect landing from air
	if IsOnFloor() and not wasOnFloor then
		-- MARK: TODO;
	end;

	-- Update floor state tracker
	wasOnFloor = IsOnFloor();

	-- Handle jump
	if Input.isActionJustPressed("jump") and IsOnFloor() then
		newVelocity.Y = jumpVelocity;
	end;

	-- Handle sprint
	if Input.isActionPressed("sprint") then
		speed = sprintSpeed;
	else
		speed = walkSpeed;
	end;

	local inputDir: Vector3 = Input.GetVector("left", "right", "forward", "backward");
	local moveDir: Vector3 = cameraPivot.Transform.Basis * Vector3(inputDir.X, 0, inputDir.Y);
	moveDir = moveDir:Normalized();

	if IsOnFloor() then
		if not moveDir:IsZeroApprox() then
			newVelocity.X = moveDir.X * speed;
			newVelocity.Z = moveDir.Z * speed;
		else
			newVelocity.X = lerp(newVelocity.X, moveDir.X * speed, delta * inertiaGround);
			newVelocity.Z = lerp(newVelocity.Z, moveDir.Z * speed, delta * inertiaGround);
		end;
	else
		newVelocity.X = lerp(newVelocity.X, moveDir.X * speed, delta * inertiaAir);
		newVelocity.Z = lerp(newVelocity.Z, moveDir.Z * speed, delta * inertiaAir);
	end;

	velocity = newVelocity;
	MoveAndSlide();
end

function OnAimModeChanged(aimMode: boolean): nil
	isAiming = aimMode;
end
