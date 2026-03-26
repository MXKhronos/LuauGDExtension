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
print("Vector2.ZERO", Vector2.ZERO, "Vector2.INF", Vector2.INF);
print("Vector3.ZERO", Vector3.ZERO, "Vector3i.MAX", Vector3i.MAX);
print("Projection.IDENTITY", Projection.IDENTITY);

local i = 0;
local newDelta = 12;
local testString: string = `Hello wrold?`;
local helloTable = {Hello="World"};

print("self", self);
print("Color variant", type(Color)); -- table
print("Color variant2", typeof(Color)); -- Dictionary
print("Color variant3", type(red)); -- userdata
print("Color variant4", typeof(red)); -- Color
print("Color hex", Color.hex(0x008b8bff)); -- hex (0.0, 0.5451, 0.5451, 1.0)
print("Vector2", Vector2(32, 64)); -- (32.0, 64.0)
print("Vector2i", Vector2i(4, 8)); -- (4, 8)

function _init()
	print("init", helloTable);
	print("[luau] init!", self, self.name, typeof(self.name));
	print("PackedStringArray", PackedStringArray{
		"Hello";
		"World!";
	});
	print("NOTIFICATION_POSTINITIALIZE", Object.NOTIFICATION_POSTINITIALIZE);
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

function _process(delta: number)
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
	newSprite2D.texture = self.texture;
	
	print(`[info{i}]`, "newSprite2D", newSprite2D, "self.add_child", add_child);
	add_child(newSprite2D);
	print(`[info{i}]`, "testObject end");
end
