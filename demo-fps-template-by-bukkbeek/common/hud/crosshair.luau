--- @extends Control
-- Translated from GodotFPS-Template-main/common/hud/crosshair.gd

local crosshairTop: ColorRect = nil;
local crosshairBottom: ColorRect = nil;
local crosshairLeft: ColorRect = nil;
local crosshairRight: ColorRect = nil;
local crosshairCenter: ColorRect = nil;

-- Crosshair settings
local LINE_WIDTH: number = 3.0;
local LINE_LENGTH: number = 10.0;
local CENTER_DOT_SIZE: number = 2.0;

local MIN_SPREAD: number = 10.0;  -- Minimum distance from center when aiming
local MAX_SPREAD: number = 50.0;  -- Maximum distance from center when moving fast
local BASE_SPREAD: number = 25.0; -- Default spread

local currentSpread: number = BASE_SPREAD;
local targetSpread: number = BASE_SPREAD;
local isAiming: boolean = false;

function _ready(): nil
	crosshairTop = GetNode("CrosshairTop");
	crosshairBottom = GetNode("CrosshairBottom");
	crosshairLeft = GetNode("CrosshairLeft");
	crosshairRight = GetNode("CrosshairRight");
	crosshairCenter = GetNode("CrosshairCenter");

	Global.OnAimModeChanged:Connect(OnAimModeChanged);
	Global.OnPlayerVelocityChanged:Connect(OnPlayerVelocityChanged);
	UpdateCrosshairPositions();
end

function _process(delta: number): nil
	-- Smoothly interpolate to target spread
	currentSpread = lerp(currentSpread, targetSpread, delta * 10.0);
	UpdateCrosshairPositions();
end

function OnPlayerVelocityChanged(velocity: Vector3)
	if isAiming then
		return;
	end

	-- Calculate horizontal velocity magnitude
	local horizontalVelocity = Vector2(velocity.X, velocity.Z):Length();

	-- Map velocity to spread (0-10 units -> BASE to MAX spread)
	local velocityFactor = clamp(horizontalVelocity / 10.0, 0.0, 1.0);
	targetSpread = lerp(BASE_SPREAD, MAX_SPREAD, velocityFactor);
end

function UpdateCrosshairPositions()
	local centerX = Size.X / 2.0;
	local centerY = Size.Y / 2.0;

	-- Top line
	crosshairTop.Position = Vector2(centerX - LINE_WIDTH / 2.0, centerY - currentSpread - LINE_LENGTH);
	crosshairTop.Size = Vector2(LINE_WIDTH, LINE_LENGTH);

	-- Bottom line
	crosshairBottom.Position = Vector2(centerX - LINE_WIDTH / 2.0, centerY + currentSpread);
	crosshairBottom.Size = Vector2(LINE_WIDTH, LINE_LENGTH);

	-- Left line
	crosshairLeft.Position = Vector2(centerX - currentSpread - LINE_LENGTH, centerY - LINE_WIDTH / 2.0);
	crosshairLeft.Size = Vector2(LINE_LENGTH, LINE_WIDTH);

	-- Right line
	crosshairRight.Position = Vector2(centerX + currentSpread, centerY - LINE_WIDTH / 2.0);
	crosshairRight.Size = Vector2(LINE_LENGTH, LINE_WIDTH);

	-- Center dot
	crosshairCenter.Position = Vector2(centerX - CENTER_DOT_SIZE / 2.0, centerY - CENTER_DOT_SIZE / 2.0);
	crosshairCenter.Size = Vector2(CENTER_DOT_SIZE, CENTER_DOT_SIZE);
end

function OnAimModeChanged(aimMode: boolean)
	isAiming = aimMode;
	if aimMode then
		-- Hide crosshair when aiming down sights
		crosshairTop.Visible = false;
		crosshairBottom.Visible = false;
		crosshairLeft.Visible = false;
		crosshairRight.Visible = false;
		crosshairCenter.Visible = false;
	else
		crosshairTop.Visible = true;
		crosshairBottom.Visible = true;
		crosshairLeft.Visible = true;
		crosshairRight.Visible = true;
		crosshairCenter.Visible = true;
	end
end
