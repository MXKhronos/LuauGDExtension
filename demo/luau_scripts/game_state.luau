--- @extends Node

Score = 0 :: number;

ScoreChanged = signal("ScoreChanged");

function AddScore(points: number)
	Score += points;
	ScoreChanged:Emit(Score);
end

function Reset()
	Score = 0;
	ScoreChanged:Emit(Score);
end
