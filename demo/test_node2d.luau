--- @extends Node2D

local totalDelta: number = 0;
local red: Color = Color.RED;
local blue: Color = Color.BLUE;

function _ready(self)
	print("testNode2D Ready", self, self.name, self.global_position);
	print("testNode2D Red", red);
	print_warn("testNode2D Blue", blue);
end

function _process(self, delta)
	rotate(0.02);
	
	totalDelta = totalDelta + delta;

    local s = (sin(totalDelta)+1)/2;
    modulate = red:lerp(blue, s);
end
