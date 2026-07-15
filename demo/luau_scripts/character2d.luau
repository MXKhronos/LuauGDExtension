--- @extends CharacterBody2D

local speed: number = 300;

local camera: Camera = self:GetNode("Camera2D");

function _ready(): nil
end

function _physics_process(delta: number): nil
	local moveDir: Vector2 = Input.GetVector("ui_left", "ui_right", "ui_up", "ui_down");
	
	if not moveDir:IsZeroApprox() then
		self.Velocity = moveDir * speed;

	else
		self.Velocity = self.Velocity:MoveToward(Vector2.ZERO, speed);

	end
	
	self:MoveAndSlide();
end
