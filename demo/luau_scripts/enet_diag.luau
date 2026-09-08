--- @extends Node2D

function _ready()
	print("diag: class =", ENetMultiplayerPeer);

	local ok, peer = pcall(function() return ENetMultiplayerPeer() end);
	print("diag: direct call pcall =", ok, peer);

	if peer ~= nil then
		print("diag: typeof =", typeof(peer));
		print("diag: GetClass =", peer:GetClass());
		print("diag: IsClass =", peer:IsClass("ENetMultiplayerPeer"));
		print("diag: GetInstanceId =", peer:GetInstanceId());
		local err = peer:CreateServer(12345, 4);
		print("diag: CreateServer err =", err);
		print("diag: connection_status =", peer:GetConnectionStatus());
	end
end
