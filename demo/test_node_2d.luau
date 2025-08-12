--- @extends Node2D

local totalDelta: number = 0;
local red: Color = Color.RED;
local blue: Color = Color.BLUE;
local green = Color.GREEN;
local newDelta = 12;
local testString: string = `Hello wrold?`;
local helloTable = {Hello="World"};

function _init()
	print("init", helloTable, Color.Red);
end

function _ready()
	print("testNode2D Ready", self, name, global_position);
	print("testNode2D Red", red, type(red), `typeof:{typeof(red)}`);
end

function _process(self, delta)
	rotate(0.02);
	
	totalDelta = totalDelta + delta;
	
	local s = (sin(totalDelta)+1)/2;
	--modulate = red:lerp(blue, s);
end

function test()
	print("test");
end
