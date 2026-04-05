--- @extends CharacterBody2D

function _ready(): nil
	print("1 velocity", velocity);
	velocity.x = 10;
	print("2 velocity.x", velocity.x);
	velocity = Vector3(1, 2, 3);
	print("3 velocity", velocity);
end
