--- @extends Node2D

function _ready()
	GameState.ScoreChanged:Connect(OnScoreChanged);
	GameState:AddScore(10);
	GameState:AddScore(25);
	GameState:AddScore(65);
	print(`[autoload] final score={GameState.Score}`);
	GameState:Reset();
end

function OnScoreChanged(score: number)
	print(`[autoload] score changed: {score}`);
end
