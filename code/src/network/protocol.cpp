#include "protocol.hpp"
#include <debug/assert.hpp>

void Socket::SocketInitializer::EnsureSocketsInitialized() {
	static SocketInitializer initializer;
}

Socket::SocketInitializer::SocketInitializer() {
#ifdef WINDOWS
	// Initialize Winsock
	int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	Assert(error == 0);
#endif
}

Socket::SocketInitializer::~SocketInitializer() {
	WSACleanup();
}



Socket::Socket(std::optional<std::string> ip, int port, unsigned int bufferSize):
	localPort(port),
	ipToListenTo(ip),
	recievedBuffer(static_cast<uint8_t*>(malloc(bufferSize))),
	bufferLen(bufferSize)
{
	SocketInitializer::EnsureSocketsInitialized();

	Assert(recievedBuffer);
	


#ifdef WINDOWS
	//socket.open(udp::v4());
	addrinfo hints{};
	addrinfo* result = nullptr;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (ipToListenTo)
		hints.ai_flags = AI_PASSIVE;

	int error = getaddrinfo(ip ? ip->c_str() : nullptr, std::to_string(port).c_str(), &hints, &result);
	if (error) {
		throw SocketConnectionException("getaddrinfo failure");
	}

	winsocket = INVALID_SOCKET;
	winsocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (winsocket == INVALID_SOCKET) {
		freeaddrinfo(result);
		throw SocketConnectionException("socket creation failure");
		// TODO: getaddrinfo actually returns a linked list and we should try all of them
	}

	if (ipToListenTo) {
		int error = connect(winsocket, result->ai_addr, (int)result->ai_addrlen);
		if (error == SOCKET_ERROR) {
			closesocket(winsocket);
			throw SocketConnectionException("socket connection failure");
		}
	}
	else {
		int error = bind(winsocket, result->ai_addr, (int)result->ai_addrlen);
		if (error == SOCKET_ERROR) {
			freeaddrinfo(result);
			closesocket(winsocket);
			throw SocketConnectionException("socket binding failure");
		}

		// TCP ONLY
		//int error = listen(winsocket, 1024);
		//if (error == SOCKET_ERROR) {
			//freeaddrinfo(result);
			//closesocket(winsocket);
			//throw SocketConnectionException("socket binding failure");
		//}
	}

	// once connected/bound we don't need this anymore
	freeaddrinfo(result);

	// set nonblocking mode so if we ask for data and there isn't any it doesn't wait
	unsigned long NOBLOCK = 1;
	int error2 = ioctlsocket(winsocket, FIONBIO, &NOBLOCK);
	if (error2 == SOCKET_ERROR) {
		closesocket(winsocket);
		throw SocketConnectionException("failure to set nonblocking mode on socket");
	}
	

#endif

}

void Socket::Send(std::string address, int port, void* data, unsigned int nBytes) {
#ifdef WINDOWS
	Assert(WSAGetLastError() == NO_ERROR);


//#define _WINSOCK_DEPRECATED_NO_WARNINGS  1

	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	//dest.sin_addr.s_addr = inet_addr(address.c_str());
	int addressError = inet_pton(AF_INET, address.c_str(), &dest.sin_addr.s_addr);
	dest.sin_port = htons(port);

	//dest.sin_addr.s_addr = inet_addr(address.c_str());
	//Assert(inet_pton(AF_INET, address.c_str(), &(dest.sin_addr)));

	char testaddress[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &dest.sin_addr, testaddress, INET_ADDRSTRLEN);
	std::string adrstr(testaddress);
	Assert(adrstr == address);

	Assert(addressError == 1);
	Assert(WSAGetLastError() == NO_ERROR);

	int bytesSent = sendto(winsocket, reinterpret_cast<const char*>(data), nBytes, 0, (sockaddr*)(&dest), sizeof(sockaddr_in));
	
	if (bytesSent < 0) {
		int errorCode = WSAGetLastError();
		//DebugLogInfo("Fail");
		throw SocketException(std::string("send failure: ") + std::to_string(errorCode));

	}
	//Assert(bytesSent == nBytes);

	WSASetLastError(0);
#endif
}

std::vector<Socket::Packet> Socket::Recieve() {

	std::vector<Packet> packets;
	uint8_t* currentBufferPos = recievedBuffer;
	int bytesLeft = bufferLen;
#ifdef WINDOWS

	Assert(WSAGetLastError() == NO_ERROR);

	while (true) {
		int nBytes;
		std::string ip;
		if (ipToListenTo) {
			ip = *ipToListenTo;
			nBytes = recv(winsocket, (char*)currentBufferPos, bytesLeft, 0);
		}
		else {
			struct sockaddr_in senderAddress;
			int sizeOfType = sizeof(senderAddress);
			nBytes = recvfrom(winsocket, (char*)currentBufferPos, bytesLeft, 0, (SOCKADDR*)&senderAddress, &sizeOfType);
				
			if (nBytes > 0) {
				char ipBuffer[128];
				ip = std::string(inet_ntop(AF_INET, &senderAddress.sin_addr, ipBuffer, 128)); // TODO won't work with IPv6
			}
		}
		if (nBytes == 0) {
			break;
		}
		else if (nBytes > 0) {
			packets.push_back(Packet{ .data = currentBufferPos, .dataNBytes = (unsigned)nBytes, .ipOfOrigin = *ipToListenTo });
		}
		else {
			int errorCode = WSAGetLastError();
			WSASetLastError(0);
			if (errorCode == WSAEWOULDBLOCK)
				break; // this just means there's no more data to extract

			throw SocketException(std::string("recv failure: ") + std::to_string(errorCode));
		}
	}
#endif
	return packets;
	
}

Socket::~Socket() {
#ifdef WINDOWS
	closesocket(winsocket);
#endif
	free(recievedBuffer);
}