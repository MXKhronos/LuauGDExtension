--- @extends CharacterBody2D

function _ready(): nil
	print("1 velocity", self.Velocity);
	self.Velocity.x = 10;
	print("2 velocity.x", self.Velocity.x);
	self.Velocity = Vector3(1, 2, 3);
	print("3 velocity", self.Velocity);
end
