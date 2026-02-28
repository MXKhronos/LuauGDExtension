--- @extends Sprite2D

local totalDelta: number = 0;
local red: Color = Color.RED;
print("red", red);
local blue: Color = Color.BLUE;
print("blue", blue);
green = Color.GREEN;
print("green", green, "rgba", green.r, green.g, green.b, green.a);
green.r = 0.5;
print("new green", green, Color.GREEN);
local newColor = Color(0.1, 0.2, 0.3);
print("newColor", newColor);

local i = 0;
local newDelta = 12;
local testString: string = `Hello wrold?`;
local helloTable = {Hello="World"};

print("test self", self)
print("test Color variant", type(Color));
print("test Color variant2", typeof(Color));
print("test Color variant3", type(red));
print("test Color variant4", typeof(red));
print("test Color hex", Color.hex(0x008b8bff));
print("v2", Vector2(32, 64));
print("v2i", Vector2i(4, 8));

function _init()
	print("init", helloTable);
	print("PackedStringArray", PackedStringArray{
		"Hello";
		"World!";
	});
end

function _ready()
	print("testNode2D Ready self:", self, `type: {type(self)} typeof: {typeof(self)}`, name, global_position);
	local s = 0.5;
	print(`[info{i}]`, `red`, red, "lerp", red.lerp, "blue", blue, "s", s); i +=1;

	print(`[info{i}]`, "assigning red.lerp", red.lerp); i +=1;
	local redlerp = red.lerp;
	print(`[info{i}]`, "assigned red.lerp", redlerp); i +=1;

	print(`[info{i}]`, "red.lerp", redlerp); i +=1;
	print(`[info{i}]`, "red.lerp type", type(redlerp)); i +=1;
	print(`[info{i}]`, "red.lerp typeof", typeof(redlerp)); i +=1;
	print(`[info{i}]`, "red.lerp call", red:lerp(blue, s)); i +=1;

	local colorlerp = Color.lerp;
	print(`[info{i}]`, "color.lerp", colorlerp); i +=1;
	print(`[info{i}]`, "color.lerp type", type(colorlerp)); i +=1;
	print(`[info{i}]`, "color.lerp typeof", typeof(colorlerp)); i +=1;
	
	test();
	testObject();
end

function _process(self: Sprite2D, delta)
	rotate(0.02);

	totalDelta = totalDelta + delta;

	local s = (sin(totalDelta)+1)/2;
	modulate = red:lerp(blue, s);
end

function test()
	print(`[info{i}]`, "test"); i += 1;
end

function testObject()
	print(`[info{i}]`, "testObject start"); i +=1;
	
	print(`[info{i}]`, "Node3D", Sprite2D);
	print(`[info{i}]`, "typeof Node3D", typeof(Sprite2D));
	print(`[info{i}]`, `get metatable`, getmetatable(Sprite2D));
	local newSprite2D = Sprite2D{
		name = "NewSprite2D";
		offset = Vector2(0, 100);
	};
	
	print(`[info{i}]`, "newSprite2D", newSprite2D, "self.add_child", add_child);
	add_child(newSprite2D);
	print(`[info{i}]`, "testObject end");
end
