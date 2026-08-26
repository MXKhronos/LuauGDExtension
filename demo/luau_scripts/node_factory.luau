--- @extends Node2D

--- @export_range(1, 20)
SpawnCount = 6 :: number;

local palette = {
	Color(0.9, 0.3, 0.35),
	Color(0.3, 0.65, 0.95),
	Color(0.95, 0.75, 0.25),
	Color(0.4, 0.8, 0.45),
};

function _ready()
	for i = 1, SpawnCount do
		local pos = Vector2(randf() * 420 + 90, randf() * 220 + 140);
		SpawnStar(pos, i);
	end
end

function SpawnStar(pos: Vector2, index: number): Polygon2D
	local star = Polygon2D{
		Name = `Star{index}`;
		Position = pos;
		Polygon = StarPoints(randf() * 18 + 22);
		Color = palette[(index % #palette) + 1];
	};
	AddChild(star);
	Animate(star);
	return star;
end

function StarPoints(radius: number): PackedVector2Array
	local points = {};
	for i = 0, 9 do
		local angle = degToRad(i * 36) - degToRad(90);
		local r = if i % 2 == 0 then radius else radius * 0.45;
		table.insert(points, Vector2(cos(angle), sin(angle)) * r);
	end
	return PackedVector2Array(points);
end

function Animate(star: Polygon2D)
	local step: number = 0.05;
	local elapsed: number = 0;
	while elapsed < 5 do
		await(step);
		elapsed += step;
		star.Rotation = star.Rotation + degToRad(180) * step;
		local p = star.Position;
		p.Y -= 24 * step;
		star.Position = p;
	end
	star.QueueFree();
end
