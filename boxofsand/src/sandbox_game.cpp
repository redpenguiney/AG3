#include "debug/log.hpp"
#include <conglomerates/basic_renderer.hpp>
#include <graphics/gengine.hpp>
#include <network/network.hpp>

void GameInit() {
	DebugLogInfo("SANDBOXING? MORE LIKE SANDBAGGING!");

	BasicRenderer::Setup();
	GraphicsEngine::Get().SetDebugFreecamEnabled(true);

	//NetworkingEngine::Get().Host();

	Socket server(std::nullopt, 49000);
	Socket client("192.168.0.1", 49001);



	server.Send("192.168.0.1", 49001, "HI", 2);
	DebugLogInfo("SENT");
	std::this_thread::sleep_for(std::chrono::seconds(1));
	auto packets = client.Recieve();
	DebugLogInfo("GOT ", packets.size());
}

void GameClose() {}