--- @extends Node2D

local totalDelta: number = 0;
local red: Color = Color.RED;
local blue: Color = Color.BLUE;

function _ready(self)
	print("testNode2D Ready", self, self.name, self.global_position.x, self.global_position.y);
end

function _process(self, delta)
	totalDelta = totalDelta + delta;
	self:rotate(0.02);
	self.modulate = red.lerp(blue, 0.5 + 0.5 * math.sin(totalDelta))
end
