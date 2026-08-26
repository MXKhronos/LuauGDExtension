--- @extends Node2D

Title = "Exports" :: string;

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

--- @export_multiline
Description = "Select this node and edit these values in the Inspector." :: string;

function _ready()
	print(`[exports] title={Title} speed={Speed} health={Health}`);
	print(`[exports] weapon={Weapon} resistances={Resistances}`);
	print(`[exports] description={Description}`);
end
