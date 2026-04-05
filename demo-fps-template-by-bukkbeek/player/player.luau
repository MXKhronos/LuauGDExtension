--- @extends CharacterBody3D

local walkSpeed: number = 5.0;
local sprintSpeed: number = 10.0;
local jumpVelocity: number = 6.0;
local speed: number = 0.0;
local GRAVITY: number = -20;

local sensitivity: number = 0.01;
local inertiaAir: number = 7.5;
local inertiaGround: number = 10.0;

local cameraPivot: Node3D = get_node("CameraPivot");
local cameraHolder: Node3D = get_node("CameraPivot/CameraHolder");

local wasOnFloor: boolean = false;

function _unhandled_input(event: InputEvent): nil
	if typeof(event) ~= "InputEventMouseMotion" then return end;
	--if Input.get_mouse_mode() ~= Input.MOUSE_MODE_CAPTURED then return end;
	
	cameraPivot:rotate_y(-event.relative.x * sensitivity);
	cameraHolder:rotate_x(-event.relative.y * sensitivity);
	
	local camRot = cameraHolder.rotation;
	cameraHolder.rotation = Vector3(
		clamp(camRot.x, deg_to_rad(-60),deg_to_rad(60)),
		camRot.y,
		camRot.z
	);
end

function _physics_process(delta: number): nil
	local newVelo = velocity;
	-- Add the gravity
	if not is_on_floor() then 
		newVelo.y += GRAVITY * delta;
	end;
	
	-- Detect landing from air
	if is_on_floor() and not wasOnFloor then
		-- MARK: TODO;
	end
	
	-- Update floor state tracker
	wasOnFloor = is_on_floor();
	
	-- Handle jump
	if Input.is_action_just_pressed("jump") and is_on_floor() then
		newVelo.y = jumpVelocity;
	end
	
	-- Handle sprint
	if Input.is_action_pressed("sprint") then
		speed = sprintSpeed;
	else
		speed = walkSpeed;
	end
	
	local inputDir: Vector3 = Input.get_vector("left", "right", "forward", "backward");
	local moveDir: Vector3 = cameraPivot.transform.basis * Vector3(inputDir.x, 0, inputDir.y);
	moveDir = moveDir:normalized();
	
	if is_on_floor() then
		if not moveDir:is_zero_approx() then
			newVelo.x = moveDir.x * speed;
			newVelo.z = moveDir.z * speed;
		else
			newVelo.x = lerp(newVelo.x, moveDir.x * speed, delta * inertiaGround);
			newVelo.z = lerp(newVelo.z, moveDir.z * speed, delta * inertiaGround);
		end
	else
		newVelo.x = lerp(newVelo.x, moveDir.x * speed, delta * inertiaAir);
		newVelo.z = lerp(newVelo.z, moveDir.z * speed, delta * inertiaAir);
	end
	
	velocity = newVelo;
	move_and_slide();
end
