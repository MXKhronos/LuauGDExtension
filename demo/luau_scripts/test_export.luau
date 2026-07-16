--- @extends Node2D

Name = "HelloWorld" :: string;

--- @export_group("Movement")
--- @export_range(0, 1000)
Speed = 300 :: number;

--- @export_range(0, 100, 1)
Health = 100 :: number;

--- @export_group("Combat")
--- @export_enum("Sword", "Shield", "Bow")
Weapon = "Sword" :: string;

--- @export_flags("Fire", "Ice", "Poison")
Resistances = 0 :: number;

--- @export_file("*.png")
Icon = "" :: string;

--- @export_multiline
Description = "" :: string;

function _ready(): nil
	print("Speed", self.Speed, "Health", self.Health, "Weapon", self.Weapon, "Resistances", self.Resistances);
end
