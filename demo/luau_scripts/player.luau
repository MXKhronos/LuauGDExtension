--- @extends CharacterBody2D

--- @export_range(50, 1000)
Speed = 300 :: number;

function _physics_process(delta: number)
	local dir: Vector2 = Input:GetVector("ui_left", "ui_right", "ui_up", "ui_down");
	if not dir:IsZeroApprox() then
		Velocity = dir * Speed;
	else
		Velocity = Velocity:MoveToward(Vector2.ZERO, Speed * 8 * delta);
	end
	MoveAndSlide();
end
