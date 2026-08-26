--- @extends Node2D

Message = "Hello from Luau!" :: string;

local elapsed: number = 0;
local frames: number = 0;

function _init()
	print(`[hello_world] _init self={self} typeof={typeof(self)}`);
end

function _enter_tree()
	print(`[hello_world] _enter_tree name={Name}`);
end

function _ready()
	print(`[hello_world] _ready message={Message}`);
	print(`[hello_world] children={GetChildCount()} position={Position}`);
end

function _process(delta: number)
	elapsed += delta;
	frames += 1;
	if frames % 60 == 0 then
		print(`[hello_world] uptime={string.format("%.1f", elapsed)}s frames={frames}`);
	end
end
