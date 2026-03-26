--- @extends CharacterBody2D

local speed: number = 300;

function _physics_process(delta: number): nil
	local moveDir: Vector2 = Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down");
	
	if not moveDir:is_zero_approx() then
		velocity = moveDir * speed;

	else
		velocity = velocity:move_toward(Vector2.ZERO, speed);

	end
	
	move_and_slide();
end
