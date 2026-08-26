--- @extends Node2D

OnPing = signal("OnPing");

local pings: number = 0;

function _ready()
	OnPing:Connect(_onPing);
	SendPing();
end

function SendPing()
	pings += 1;
	OnPing:Emit(pings);
end

function _onPing(count: number)
	print(`[signals] ping #{count}`);
	if count < 3 then
		SendPing();
	end
end
