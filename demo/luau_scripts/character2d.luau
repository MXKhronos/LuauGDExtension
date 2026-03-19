--- @extends CharacterBody2D

local VEC2ZERO = Vector2(0, 0);
local speed: number = 300;

function _physics_process(delta: number): nil
	--print("Input", Input, Input.get_vector, "Input typeof", typeof(Input));
	local dir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down");
	
	if dir ~= VEC2ZERO then
		velocity = dir * speed;
	else
		velocity = velocity.move_toward(VEC2ZERO, speed)
	end
	
	move_and_slide();
end
