--- @extends Control
-- Translated from GodotFPS-Template-main/common/hud/hud.gd

local enemiesLabel: Label = nil;
local bulletsLabel: Label = nil;
local healthBar: ProgressBar = nil;

local weaponsManager = nil;

function _ready(): nil
	enemiesLabel = GetNode("EnemiesLabel");
	bulletsLabel = GetNode("BulletsLabel");
	healthBar = GetNode("HealthBar");

	Global.OnBulletsChanged:Connect(OnBulletsChanged);
	Global.OnPlayerHealthChanged:Connect(OnPlayerHealthChanged);
	Global.OnEnemyCountChanged:Connect(OnEnemyCountChanged);

	-- Get weapons manager from player
	if Global.Player ~= nil then
		weaponsManager = Global.Player:GetNodeOrNull("WeaponsManager");
	end

	-- Initial display updates
	OnBulletsChanged();
	OnEnemyCountChanged(Global.EnemiesKilled);
end

function OnBulletsChanged()
	if weaponsManager == nil or weaponsManager.CurrentWeapon == nil then
		bulletsLabel.Text = "AMMO: --/--";
		return;
	end

	local currentMag = weaponsManager:GetCurrentMag();
	local currentAmmo = weaponsManager:GetCurrentAmmo();

	-- Handle infinite ammo display (-1)
	if currentAmmo == -1 then
		if weaponsManager.CurrentWeapon.ReloadRequired then
			bulletsLabel.Text = `AMMO: {currentMag}/∞`;
		else
			bulletsLabel.Text = "AMMO: ∞";
		end
	else
		if weaponsManager.CurrentWeapon.ReloadRequired then
			bulletsLabel.Text = `AMMO: {currentMag}/{currentAmmo}`;
		else
			bulletsLabel.Text = `AMMO: {currentAmmo}`;
		end
	end
end

function OnEnemyCountChanged(count: number)
	enemiesLabel.Text = `ENEMIES KILLED: {count}`;
end

function OnPlayerHealthChanged(_healthChange: number, currentHealth: number, maxHealth: number, _isEnemyDamage: boolean)
	UpdateHealthBar(currentHealth, maxHealth);
end

function UpdateHealthBar(currentHealth: number, maxHealth: number)
	healthBar.Value = currentHealth;
	healthBar.MaxValue = maxHealth;
end
