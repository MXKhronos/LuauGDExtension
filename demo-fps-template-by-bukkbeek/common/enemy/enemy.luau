--- @extends CharacterBody3D
-- Translated from GodotFPS-Template-main/common/enemy/enemy.gd


-- MARK: exports (node connections)
characterRoot = nil :: Node3D;
collisionShape = nil :: CollisionShape3D;
animationTree = nil :: AnimationTree;
damageBody = nil :: PhysicalBoneSimulator3D;

-- MARK: exports (settings)
moveSpeed = 1.0 :: number;
rotateSpeed = 2.0 :: number;
maxHealth = 100.0 :: number;
showHealthBar = true :: boolean;
canOpenDoors = false :: boolean;

-- MARK: exports (attack)
attackDamage = 10.0 :: number;
attackRange = 2.0 :: number;
visionRange = 10.0 :: number;
chaseTime = 5.0 :: number;

-- MARK: exports (sfx)
spawnSfx = nil :: AudioStreamPlayer3D;
idleSfx = nil :: AudioStreamPlayer3D;
alertSfx = nil :: AudioStreamPlayer3D;
hitSfx = nil :: AudioStreamPlayer3D;
attackSfx = nil :: AudioStreamPlayer3D;
deathSfx = nil :: AudioStreamPlayer3D;

-- MARK: exports (vfx)
spawnVfx = nil :: Node3D;
attackVfx = nil :: Node3D;
hitVfx = nil :: Node3D;
deathVfx = nil :: Node3D;

local GRAVITY: number = -20.0;
local isAlive: boolean = true;
local player = nil;

local navAgent: NavigationAgent3D = nil;
local debugLabel: Label3D = nil;
local chaseTimer: Timer = nil;
local healthBar: MeshInstance3D = nil;

local stateMachine = nil;
local currentState = nil;
local previousState = nil;
local isChasing: boolean = false;
local currentHealth: number = 100.0;

function _ready(): nil
	navAgent = GetNode("NavigationAgent3D");
	debugLabel = GetNode("DebugLabel");
	chaseTimer = GetNode("ChaseTimer");
	healthBar = GetNode("HealthBar");

	chaseTimer:Connect("timeout", OnChaseTimerTimeout);
	currentHealth = maxHealth;
	healthBar.Visible = showHealthBar;

	-- Make the health bar material unique for this instance
	if healthBar:GetSurfaceOverrideMaterialCount() > 0 then
		healthBar:SetSurfaceOverrideMaterial(0, healthBar:GetActiveMaterial(0):Duplicate());
	end

	UpdateHealthBar();

	if canOpenDoors then
		AddToGroup("door_access");
	end
	debugLabel.Visible = Global.DebugMode;
	stateMachine = animationTree:Get("parameters/playback");

	local boneArray = damageBody:GetChildren();
	for _, colBone in boneArray do
		colBone.OnUpdateDamage:Connect(GetDamage);
		colBone.CollisionLayer = 2;
		colBone.CollisionMask = 2;
	end

	PlaySfx(spawnSfx);
	PlayVfx(spawnVfx);
end

function _physics_process(delta: number): nil
	-- Read once: `velocity` resolves through the property bridge, so every
	-- access returns a temporary copy. Mutate a local and write it back.
	local vel = velocity;

	if not IsOnFloor() then
		vel.Y += GRAVITY * delta;
	end

	if player == nil then
		player = Global.Player;
	end

	if player == nil or not isAlive then
		return;
	end

	-- Determine current state
	if not isAlive then
		currentState = "dead";
	elseif PlayerInAttackRange() then
		currentState = "attack";
	elseif ChasingPlayer() then
		currentState = "move";
	else
		currentState = "idle";
	end

	-- Only update animation tree when state changes
	if currentState ~= previousState then
		ChangeState(currentState);
		previousState = currentState;
	end

	local animNode = stateMachine:GetCurrentNode();
	if animNode == "idle" then
		vel = Vector3(0, 0, 0);

	elseif animNode == "walk" then
		navAgent:SetTargetPosition(player.GlobalPosition);
		local nextNavPoint = navAgent:GetNextPathPosition();
		vel = (nextNavPoint - GlobalTransform.Origin):Normalized() * moveSpeed;
		LookAtTarget(nextNavPoint, delta);

	elseif animNode == "attack" then
		vel = Vector3(0, 0, 0);
		LookAtTarget(player.GlobalPosition, delta);

	elseif animNode == "dead" then
	end

	velocity = vel;
	MoveAndSlide();
end

function ChangeState(newState: string)
	-- Reset all conditions
	animationTree:Set("parameters/conditions/idle", false);
	animationTree:Set("parameters/conditions/move", false);
	animationTree:Set("parameters/conditions/attack", false);
	animationTree:Set("parameters/conditions/dead", false);

	-- Set the new state condition and update debug label
	if newState == "idle" then
		animationTree:Set("parameters/conditions/idle", true);
		PlaySfx(idleSfx);
		debugLabel.Text = "IDLE";
	elseif newState == "move" then
		animationTree:Set("parameters/conditions/move", true);
		PlaySfx(alertSfx);
		debugLabel.Text = "MOVE";
	elseif newState == "attack" then
		PlaySfx(attackSfx);
		PlayVfx(attackVfx);
		animationTree:Set("parameters/conditions/attack", true);
		debugLabel.Text = "ATTACK";
	elseif newState == "dead" then
		animationTree:Set("parameters/conditions/dead", true);
		debugLabel.Text = "DEAD";
	end
end

function LookAtTarget(target: Vector3, delta: number)
	local dir = target - GlobalPosition;
	dir.Y = 0;
	dir = dir:Normalized();

	local targetYaw = atan2(-dir.X, -dir.Z);
	characterRoot.Rotation = Vector3(
		characterRoot.Rotation.X,
		lerpAngle(characterRoot.Rotation.Y, targetYaw, rotateSpeed * delta),
		characterRoot.Rotation.Z
	);
end

function PlayerInAttackRange(): boolean
	return GlobalPosition:DistanceTo(player.GlobalPosition) < attackRange;
end

function ChasingPlayer(): boolean
	if PlayerInVisionRange() then
		isChasing = true;
		chaseTimer:Start(chaseTime);
	end

	return isChasing;
end

function PlayerInVisionRange(): boolean
	return GlobalPosition:DistanceTo(player.GlobalPosition) < visionRange;
end

function OnChaseTimerTimeout()
	isChasing = false;
end

function DealDamage()
	if PlayerInAttackRange() then
		local direction = GlobalPosition:DirectionTo(player.GlobalPosition);
		player:GetDamage(attackDamage, direction, true);
	end
end

function GetDamage(damage: number, _direction: Vector3)
	currentHealth -= damage;
	UpdateHealthBar();
	PlaySfx(hitSfx);
	PlayVfx(hitVfx);

	if currentHealth <= 0 then
		isAlive = false;
		Die();
	end
end

function UpdateHealthBar()
	local healthMat = healthBar:GetActiveMaterial(0);
	local uv = healthMat.Uv1Offset;
	uv.X = -((currentHealth / maxHealth) - 1) / 2;
	healthMat.Uv1Offset = uv;
end

function Die()
	Global.OnEnemyKilled:Emit();

	PlayVfx(deathVfx);
	PlaySfx(deathSfx);

	characterRoot.Visible = false;
	collisionShape.Disabled = true;
	healthBar.Visible = false;
	debugLabel.Visible = false;

	await(5.0);
	QueueFree();
end

function PlaySfx(sfx: AudioStreamPlayer3D)
	if sfx ~= nil then
		sfx:Play();
	end
end

function PlayVfx(vfx: Node3D)
	if vfx ~= nil then
		if vfx:IsClass("GPUParticles3D") then
			vfx:Restart();
		elseif vfx:HasMethod("activate") then
			vfx:Activate();
		end
	end
end
