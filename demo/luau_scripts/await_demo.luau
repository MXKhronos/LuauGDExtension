--- @extends Node2D

CountdownFinished = signal("CountdownFinished");

function _ready()
	Run();
end

function Run()
	print("[await] waiting 1 second... tick=",tick());
	await(1.0);
	print("[await] waited 1 second... tick=",tick());

	print("[await] waiting 3 frames...");
	for i = 1, 3 do
		await();
		print(`[await] frame {i}`);
	end

	print("[await] waiting for CountdownFinished...");
	FinishCountdown();
	await(CountdownFinished);
	print("[await] countdown finished!");
end

function FinishCountdown()
	for i = 3, 1, -1 do
		print(`[await] countdown {i}`);
		await(1.0);
	end
	CountdownFinished:Emit();
end
