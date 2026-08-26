--- @extends Control
-- Translated from GodotFPS-Template-main/common/hud/damage_display.gd

local DAMGE_COLOR = Color(0.5, 0, 0, 0.5);
local HEAL_COLOR = Color(0, 0.5, 0, 0.5);

local clawRect: TextureRect = nil;
local damageOverlay: ColorRect = nil;

local clawRectCurrentPos: Vector2 = Vector2(0, 0);

function _ready(): nil
	clawRect = GetNode("ClawRect");
	damageOverlay = GetNode("DamageOverlay");

	clawRect.Visible = false;
	damageOverlay.Visible = false;
	clawRectCurrentPos = Position;
	Global.OnPlayerHealthChanged:Connect(OnPlayerHealthChanged);
end

function OnPlayerHealthChanged(healthChange: number, _currentHealth: number, _maxHealth: number, isEnemyDamage: boolean)
	if healthChange < 0 then

		-- Only show claw marks for enemy damage
		if isEnemyDamage then
			-- Random rotation for scratch mark effect (full 360 degrees)
			clawRect.RotationDegrees = randfRange(0, 360);

			-- Random offset from center (15-40 pixels in any direction)
			local randomOffsetX = randfRange(-40, 40);
			local randomOffsetY = randfRange(-40, 40);

			-- Ensure offset is at least 15 pixels from center
			local minOffset = 15.0;
			if math.abs(randomOffsetX) < minOffset then
				if randomOffsetX >= 0 then randomOffsetX = minOffset; else randomOffsetX = -minOffset; end
			end
			if math.abs(randomOffsetY) < minOffset then
				if randomOffsetY >= 0 then randomOffsetY = minOffset; else randomOffsetY = -minOffset; end
			end

			-- Position around center with random offset
			local centerX = Size.X / 2.0;
			local centerY = Size.Y / 2.0;
			clawRect.Position = Vector2(centerX + randomOffsetX, centerY + randomOffsetY);

			-- Random scale for varied scratch marks (0.7-1.5 for more variation)
			clawRect.Scale = Vector2(randfRange(0.7, 1.5), randfRange(0.7, 1.5));

			clawRect.Visible = true;
		end

		damageOverlay.Visible = true

		await(0.2);
		local m = damageOverlay.Modulate
		damageOverlay.Modulate = Color(m.R, m.G, m.B, 0.75)

		await(0.2);
		m = damageOverlay.Modulate
		damageOverlay.Modulate = Color(m.R, m.G, m.B, 0.50)

		await(0.2);
		m = damageOverlay.Modulate
		damageOverlay.Modulate = Color(m.R, m.G, m.B, 0.25)

		await(0.2);
		m = damageOverlay.Modulate
		damageOverlay.Modulate = Color(m.R, m.G, m.B, 0.00)

		await(1.6);

		clawRect.Visible = false
		damageOverlay.Visible = false
	end
end
