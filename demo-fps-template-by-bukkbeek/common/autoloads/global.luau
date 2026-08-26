--- @extends Node
-- Translated from GodotFPS-Template-main/common/autoloads/global.gd

DebugMode = false :: boolean;
EnemiesKilled = 0 :: number;
IsInVehicle = false :: boolean;

Player = nil :: CharacterBody3D;

--MARK: signals
OnEnemyKilled = signal("OnEnemyKilled");
OnEnemyCountChanged = signal("OnEnemyCountChanged");
OnAimModeChanged = signal("OnAimModeChanged");
OnActiveCameraFovChanged = signal("OnActiveCameraFovChanged");
OnFireInput = signal("OnFireInput");
OnReloadInput = signal("OnReloadInput");
OnWeaponSwitchInput = signal("OnWeaponSwitchInput"); -- carries +1/-1 (port adaptation of weapon_switch_next/prev)
OnBulletsChanged = signal("OnBulletsChanged");
OnCameraShake = signal("OnCameraShake");
OnPlayerVelocityChanged = signal("OnPlayerVelocityChanged");
OnPlayerHealthChanged = signal("OnPlayerHealthChanged");
OnPlayerDead = signal("OnPlayerDead");
OnPlayerSprintingChanged = signal("OnPlayerSprintingChanged");
OnActivatePressed = signal("OnActivatePressed");

function _ready()
	OnEnemyKilled:Connect(OnEnemyKilledFired);
end

function OnEnemyKilledFired()
	EnemiesKilled += 1;
	OnEnemyCountChanged:Emit(EnemiesKilled);
end
