--- @extends Node


--MARK: properties
DebugMode = false;
EnemiesKilled = 0;
IsInVehicle = false;

Player = nil :: CharacterBody3D;

--MARK: signals
OnAimModeChanged = signal("OnAimModeChanged");
OnFireInput = signal("OnFireInput");
OnReloadInput = signal("OnReloadInput");
OnWeaponSwitchInput = signal("OnWeaponSwitchInput");
OnActivatePressed = signal("OnActivatePressed");
