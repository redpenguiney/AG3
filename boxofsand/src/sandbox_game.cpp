#include "debug/log.hpp"
#include <conglomerates/basic_renderer.hpp>
#include <graphics/gengine.hpp>
#include <network/network.hpp>

//#include "WinSock2.h"
#include "boost/asio.hpp"
#include <tests/graphics_test.hpp>


void GameInit(std::vector<std::string> args) {
	DebugLogInfo("SANDBOXING? MORE LIKE SANDBAGGING!");

	bool is_server = true;
	bool headless = false;
	for (auto& arg : args) {
		if (arg == "--headless")
			headless = true;
		else if (arg == "--client")
			is_server = false;
	}

	if (!headless) {
		BasicRenderer::Setup();
		GraphicsEngine::Get().SetDebugFreecamEnabled(true);
	}

	//is_server = !is_server;

	if (is_server) {
		NetworkingEngine::Get().Host();

		NetworkingEngine::Get().onUserdataRecieved->Connect([](NetworkUserdata userdata) {
			std::string messagestr((const char*)userdata.data.data(), userdata.data.size());
			DebugLogInfo("RECIEVED FROM CLIENT (", userdata.reliable, " ", userdata.data.size(), "): first ", (int)messagestr[0], " ", messagestr);
		});

		//TestGraphics();

		/*Socket* server = new Socket(49000);


		GraphicsEngine::Get().preRenderEvent->Connect([server](float dt) {
			auto packets = server->Recieve();
			DebugLogInfo("GOT ", packets.size());
		});		*/	
	}
	else {
		NetworkingEngine::Get().Connect("127.0.0.1");

		NetworkingEngine::Get().onConnectionAttemptComplete->Connect([](NetworkingEngine::ConnectionAttemptResult result) {
			Assert(result.successful);
			//char str[] = "POOPLESNOOT";

			//char str[] = ""
			std::ifstream file("../resources/bee_move_script.txt");
			std::stringstream buffer;
			buffer << file.rdbuf();
			std::string str = buffer.str();
			Assert(str.size() > 0);
			NetworkingEngine::Get().SendDataReliable(str.data(), str.size());
			DebugLogInfo("SENT ", str.size());
		});

		/*Socket* client = new Socket("127.0.0.1", 49000, 49001);
		GraphicsEngine::Get().preRenderEvent->Connect([client](float dt) {
			char data[3] = "HI";
			client->Send(data, 3);
		});*/
	}
	//using namespace boost::asio;

	//io_service ioService;

	//ip::udp::resolver ioResolver(ioService);
	//ip::udp::resolver::query localhostQuery(ip::udp::v4(), "127.0.0.1", "49001");
	//ip::udp::endpoint result = *ioResolver.resolve(localhostQuery);

	//ip::udp::socket socket1(ioService, ip::udp::endpoint(ip::udp::v4(), 49000));
	//ip::udp::socket socket2(ioService, ip::udp::endpoint(ip::udp::v4(), 49001));
	//socket2.non_blocking(true);

	//boost::system::error_code err;
	//socket1.send_to(buffer("HI!"), result, 0, err);

	//std::this_thread::sleep_for(std::chrono::milliseconds(500));
	//
	//char recv_buffer[1000] = { '\0' };
	////recv_buffer[999] = '\0';
	//ip::udp::endpoint sender;
	//boost::system::error_code err2;
	//socket2.receive_from(buffer(recv_buffer, 999), sender, 0, err2);

	//DebugLogInfo("RECIEVED ", recv_buffer)

	//Socket server(49000);
	//Socket client("127.0.0.1", 49000, 49001);


	//for (int i = 0; i < 5; i++) {
	//	char data[3] = "HI";
	//	//server.Send("127.0.0.1", 49001, data, 3);
	//	client.Send(data, 3);
	//	DebugLogInfo("SENT");
	//	std::this_thread::sleep_for(std::chrono::seconds(1));
	//	//auto packets = client.Recieve();
	//	auto packets = server.Recieve();
	//	DebugLogInfo("GOT ", packets.size());
	//}
}

void GameClose() {}