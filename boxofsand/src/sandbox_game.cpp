#include "debug/log.hpp"
#include <conglomerates/basic_renderer.hpp>
#include <graphics/gengine.hpp>
#include <network/network.hpp>

#include "WinSock2.h"

void GameInit() {
	DebugLogInfo("SANDBOXING? MORE LIKE SANDBAGGING!");

	BasicRenderer::Setup();
	GraphicsEngine::Get().SetDebugFreecamEnabled(true);

	//NetworkingEngine::Get().Host();

	auto ip = "127.0.0.1"; //"10.154.72.197"; // "192.168.0.1"

    sockaddr_in dest;
    sockaddr_in local;
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);

	local.sin_family = AF_INET;
    inet_pton(AF_INET, "10.154.72.96", &local.sin_addr.s_addr);
    local.sin_port = htons(49000);

    dest.sin_family = AF_INET;
    //inet_pton(AF_INET, ip, &dest.sin_addr.s_addr);
	dest.sin_addr.s_addr = INADDR_LOOPBACK;
    dest.sin_port = htons(49001);

	SOCKET s1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int bindSuccess1 = bind(s1, (sockaddr*)&local, sizeof(local));
	if (bindSuccess1 == SOCKET_ERROR) {
		DebugLogInfo("Failure1 ", WSAGetLastError());
		Assert(false);
	}

	

	//std::this_thread::sleep_for(std::chrono::milliseconds(500));

	local.sin_port = 49001;
	SOCKET s2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int bindSuccess2 = bind(s2, (sockaddr*)&local, sizeof(local));
	if (bindSuccess2 == SOCKET_ERROR) {
		DebugLogInfo("Failure2 ", WSAGetLastError());
		Assert(false);
	}

	auto pkt = "oh noes";
	auto sentCount = sendto(s1, pkt, strlen(pkt), 0, (sockaddr*)&dest, sizeof(dest));

	if (sentCount == SOCKET_ERROR) {
		DebugLogInfo("FailureSend ", WSAGetLastError());
		Assert(false);
	}
	else {
		DebugLogInfo("SENT ", sentCount);
	}

	/*unsigned long NOBLOCK = 1;
	int error2 = ioctlsocket(s2, FIONBIO, &NOBLOCK);
	if (error2 == SOCKET_ERROR) {
		DebugLogInfo("FailureNOBLOCK2 ", WSAGetLastError());
		Assert(false);
	}*/

	sockaddr_in recvaddr;
	int size = sizeof(recvaddr);
	char buffer[1000];
	auto recvCount = recvfrom(s2, buffer, 1000, 0, (sockaddr*)&recvaddr, &size);

	DebugLogInfo("CODE ", WSAGetLastError());
	Assert(recvCount != -1);
	
	closesocket(s1);
	closesocket(s2);

    WSACleanup();


	/*Socket server(std::nullopt, 49000);
	Socket client(ip, 49001);


	for (int i = 0; i < 5; i++) {
		char data[3] = "HI";
		server.Send(ip, 49001, data, 3);
		DebugLogInfo("SENT");
		std::this_thread::sleep_for(std::chrono::seconds(1));
		auto packets = client.Recieve();
		DebugLogInfo("GOT ", packets.size());
	}*/
}

void GameClose() {}