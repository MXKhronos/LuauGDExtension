--- @extends CharacterBody2D

local speed: number = 300;
local vecZero: Vector2 = Vector2.ZERO;

function _physics_process(delta: number): nil
	local dir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down");
	
	if not dir:is_zero_approx() then
		velocity = dir * speed;

	else
		velocity = velocity:move_toward(vecZero, speed);

	end
	
	move_and_slide();
end
