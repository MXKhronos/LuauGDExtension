--- @extends Node3D
-- Translated from GodotFPS-Template-main/common/spawner/spawner.gd

item = nil :: PackedScene;
waitTime = 2.0 :: number;
spawnTime = 5.0 :: number;
timeIncrement = 0.1 :: number;
maxSpawnTime = 10.0 :: number;
minSpawnTime = 10.0 :: number;
maxItems = 1 :: number;
spawnRange = 2.0 :: number;

local spawnPoint: Marker3D = nil;
local spawnTimer: Timer = nil;
local currentItemCount: number = 0;

function _ready(): nil
	spawnPoint = GetNode("SpawnPoint");
	spawnTimer = GetNode("SpawnTimer");

	spawnTimer:Start(waitTime);
	spawnTimer:Connect("timeout", OnSpawnTimerTimeout);
end

function OnSpawnTimerTimeout()
	if spawnTime <= maxSpawnTime and spawnTime >= minSpawnTime then
		spawnTime -= timeIncrement;
	end
	spawnTimer:Start(spawnTime);

	-- Only spawn if below max items limit
	if currentItemCount >= maxItems then
		return;
	end

	local instance = item:Instantiate();

	-- Calculate spawn position with random offset on x and z plane
	local randomOffset = Vector3(
		randfRange(-spawnRange, spawnRange),
		0.0,
		randfRange(-spawnRange, spawnRange)
	);
	local spawnPosition = GlobalPosition + randomOffset;

	-- Track when the item is removed from the scene
	instance:Connect("tree_exited", OnItemRemoved);
	currentItemCount += 1;

	GetParent():CallDeferred("add_child", instance);
	instance:SetDeferred("global_position", spawnPosition);
end

function OnItemRemoved()
	currentItemCount -= 1;
end
