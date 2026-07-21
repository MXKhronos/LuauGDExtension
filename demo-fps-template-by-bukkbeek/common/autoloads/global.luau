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

print("OnAimModeChanged", OnAimModeChanged, typeof(OnAimModeChanged));
print("raw self.OnAimModeChanged", self.OnAimModeChanged, typeof(self.OnAimModeChanged));
function _ready()
	print("ready self typeof", typeof(self))
	print("ready self", self)
	print("ready self varToStr", varToStr(self))
	print("ready OnAimModeChanged", OnAimModeChanged, typeof(OnAimModeChanged));
	print("ready self.OnAimModeChanged", self.OnAimModeChanged, typeof(self.OnAimModeChanged));
end
-- print("self.OnAimModeChanged typeof", typeof(self.OnAimModeChanged));
--print("aim_mode_changed", aim_mode_changed, typeof(aim_mode_changed));
--print("self.aim_mode_changed", self.aim_mode_changed, typeof(self.aim_mode_changed));
