---@extends Node

-- hello world
ACONST = 123;
local BCONST = 345;
acount = 1;
local bcount: number = 0;

function _init(self: Node)
	print("[luau] init!", self);
	print("[luau] init ACONST=", ACONST);
	print("[luau] init BCONST=", BCONST);
	print("[luau] init acount=", acount);
	print("[luau] init bcount=", bcount);
	hello();
end

function _ready(self: Node)
	print("[luau] ready!", self);
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

function _enter_tree(self: Node): nil
	print("[luau] enter tree");
	return;
end

function _process(self: Node, delta: number): nil
	return;
end

function hello()
	print(`[luau] world! {BCONST}`);
end