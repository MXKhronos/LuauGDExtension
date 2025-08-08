---@extends Node

-- hello world
local bcount: number = 0;

function _init()
	print("[luau] init!");
end

function _ready()
	while true do
		print("[luau] hello world!");
		bcount = bcount +1;
		if bcount > 5 then
			break;
		end
	end
	for a=1, 5 do
		print("[luau] Count", a);
	end
end

function _enter_tree(): nil
	print("[luau] enter tree");
	return;
end

function _process(delta: number): nil
	return;
end
