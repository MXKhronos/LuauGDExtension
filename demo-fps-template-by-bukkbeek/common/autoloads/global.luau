--- @extends Node


--MARK: properties
DebugMode = false;
EnemiesKilled = 0;
InVehicle = false;

Player = (nil :: CharacterBody3D);

--MARK: signals
OnAimModeChanged = signal("OnAimModeChanged");

print("OnAimModeChanged", OnAimModeChanged, type(OnAimModeChanged), typeof(OnAimModeChanged))
