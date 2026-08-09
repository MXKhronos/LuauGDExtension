--- @extends Node2D

-- hello world
print(self, typeof(self), "meta=", getmetatable(self));
print(getfenv(), typeof(getfenv()), "meta=", getmetatable(getfenv()))
print("ScriptName=", self.ScriptName, ScriptName);
print("Node2D.Transform=", self.Transform, Transform);
print("Node2D:ApplyScale=", self.ApplyScale, typeof(self.ApplyScale), ApplyScale);
--print("Const Node.NOTIFICATION_READY", Node.NOTIFICATION_READY);
ACONST = 123;
local BCONST = 345;
acount = 1;
local bcount: number = 0;

function _init()
	print("[luau] init!", self, self.Name, typeof(self.Name));
	print("[luau] init ACONST=", ACONST);
	print("[luau] init BCONST=", BCONST);
	print("[luau] init acount=", acount);
	print("[luau] init bcount=", bcount);
	hello();
	
	print("Before ApplyScale=", Scale, self.Scale);
	ApplyScale(Vector2(2, 4))
	print("After ApplyScale=", Scale, self.Scale);
end

function _ready()
	print("[luau] ready!", self, self.Name, typeof(self.Name));
	while true do
		print("[luau] hello world! bcount=", bcount);
		bcount = bcount +1;
		if bcount > 3 then
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

function hello()
	print(`[luau] world! {BCONST}`);
end
