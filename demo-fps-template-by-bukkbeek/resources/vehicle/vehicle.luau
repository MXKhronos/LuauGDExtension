--- @extends VehicleBody3D
-- Translated from GodotFPS-Template-main/resources/vehicle/vehicle.gd

local playerInRange: boolean = false;
local isDriving: boolean = false;

-- MARK: exports (Visuals)
doorMesh = nil :: Node3D;
doorOutlineMesh = nil :: Node3D;
vehicleOutlineMesh = nil :: Node3D;
animationPlayer = nil :: AnimationPlayer;
getInAnimation = "get_in" :: string;

-- MARK: exports (Engine)
engineForceValue = 80.0 :: number;
maxRpm = 200.0 :: number;
brakeStrength = 10.0 :: number;

-- MARK: exports (Steering)
steeringLimit = 0.4 :: number;
steeringSpeed = 3.0 :: number;

-- MARK: exports (Camera)
cameraSensitivity = 0.003 :: number;
cameraPitchMin = -40.0 :: number;
cameraPitchMax = 60.0 :: number;

-- MARK: exports (Audio)
engineAudio = nil :: AudioStreamPlayer3D;
engineMinPitch = 0.6 :: number;
engineMaxPitch = 2.2 :: number;
engineMaxSpeed = 28.0 :: number; -- Speed in m/s at which pitch reaches its maximum.

local camera: Camera3D = nil;
local playerExit: Marker3D = nil;

local steeringInput: number = 0.0;
local camYaw: number = 0.0;
local camPitch: number = 0.0;

local bumperDamage: number = 50.0;

function _ready(): nil
	camera = GetNode("Camera3D");
	playerExit = GetNode("PlayerExit");

	camera.Current = false;
	Global.OnActivatePressed:Connect(OnActivatePressed);
end

function _unhandled_input(event: InputEvent): nil
	if not isDriving then
		return;
	end
	if typeof(event) ~= "InputEventMouseMotion" then return end;
	if Input:GetMouseMode() ~= Input.MOUSE_MODE_CAPTURED then return end;

	camYaw -= event.Relative.X * cameraSensitivity;
	camPitch -= event.Relative.Y * cameraSensitivity;
	camPitch = clamp(camPitch, degToRad(cameraPitchMin), degToRad(cameraPitchMax));
end

function _process(delta: number): nil
	if not isDriving then
		return;
	end

	-- --- Free-look camera (independent of vehicle body) ---
	camera.Rotation = Vector3(camPitch, camYaw, 0.0);

	-- --- Driving inputs ---
	local throttle = Input:IsActionPressed("forward");
	local reverse = Input:IsActionPressed("backward");
	local turnLeft = Input:IsActionPressed("left");
	local turnRight = Input:IsActionPressed("right");
	local braking = Input:IsActionPressed("brake");

	if throttle then
		EngineForce = engineForceValue;
	elseif reverse then
		EngineForce = -engineForceValue;
	else
		EngineForce = 0.0;
	end

	if braking then
		Brake = brakeStrength;
	else
		Brake = 0.0;
	end

	local targetSteer = 0.0;
	if turnLeft then
		targetSteer = steeringLimit;
	elseif turnRight then
		targetSteer = -steeringLimit;
	end

	steeringInput = lerp(steeringInput, targetSteer, steeringSpeed * delta);
	Steering = steeringInput;

	-- --- Engine audio pitch based on speed ---
	UpdateEngineAudio();
end

function OnActivatePressed()
	if isDriving then
		ExitVehicle();
	elseif playerInRange then
		EnterVehicle();
	end
end

function EnterVehicle()
	local player = Global.Player;
	if player == nil then
		return;
	end

	isDriving = true;
	Global.IsInVehicle = true;

	-- Align free-look yaw to the vehicle's current world yaw so the view
	-- doesn't snap when entering.
	camYaw = 0.0;
	camPitch = 0.0;

	player:SetPhysicsProcess(false);
	player:SetProcessUnhandledInput(false);
	player.Visible = false;

	camera:MakeCurrent();

	EngineForce = 0.0;
	Brake = 0.0;
	steeringInput = 0.0;
	Steering = 0.0;

	if engineAudio ~= nil then
		engineAudio.PitchScale = engineMinPitch;
		engineAudio:Play();
	end

	if doorMesh ~= nil then
		doorMesh.Visible = true;
	end

	if animationPlayer ~= nil and getInAnimation ~= "" then
		animationPlayer:Play(getInAnimation);
	end

	UpdateOutline();
end

function ExitVehicle()
	local player = Global.Player;
	if player == nil then
		return;
	end

	isDriving = false;
	Global.IsInVehicle = false;

	player.GlobalPosition = playerExit.GlobalPosition;

	player.Visible = true;
	player:SetPhysicsProcess(true);
	player:SetProcessUnhandledInput(true);

	player.camera:MakeCurrent();

	EngineForce = 0.0;
	Brake = brakeStrength;
	steeringInput = 0.0;
	Steering = 0.0;

	if engineAudio ~= nil then
		engineAudio:Stop();
	end

	UpdateOutline();
end

function UpdateEngineAudio()
	if engineAudio == nil then
		return;
	end
	local speed = LinearVelocity:Length();
	local t = clamp(speed / engineMaxSpeed, 0.0, 1.0);
	engineAudio.PitchScale = lerp(engineMinPitch, engineMaxPitch, t);
end

function UpdateOutline()
	local show = playerInRange and not isDriving;
	if vehicleOutlineMesh ~= nil then
		vehicleOutlineMesh.Visible = show;
	end
	if doorOutlineMesh ~= nil then
		doorOutlineMesh.Visible = show;
	end
end

function OnDoorArea3DBodyEntered(body: Node3D)
	if body ~= nil and body:IsInGroup("player") then
		playerInRange = true;
		UpdateOutline();
	end
end

function OnDoorArea3DBodyExited(body: Node3D)
	if body ~= nil and body:IsInGroup("player") then
		playerInRange = false;
		UpdateOutline();
	end
end

function OnBumperBodyEntered(body: Node3D)
	if Global.IsInVehicle and body ~= nil and body:HasMethod("GetDamage") then
		local direction = GlobalPosition:DirectionTo(body.GlobalPosition);
		body:GetDamage(bumperDamage, direction);
	end
end
