--- @extends Node


--MARK: properties
DebugMode = false :: boolean;
EnemiesKilled = 0 :: number;
IsInVehicle = false :: boolean;

Player = nil :: CharacterBody3D;

--MARK: signals
OnAimModeChanged = signal("OnAimModeChanged");
OnFireInput = signal("OnFireInput");
OnReloadInput = signal("OnReloadInput");
OnWeaponSwitchInput = signal("OnWeaponSwitchInput");
OnActivatePressed = signal("OnActivatePressed");
