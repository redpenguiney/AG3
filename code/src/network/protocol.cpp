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

void Socket::Send(std::string address, int port, const void* data, unsigned int nBytes) {
#ifdef WINDOWS
	sockaddr_in dest{};
	dest.sin_port = port;
	dest.sin_family = AF_INET;
	Assert(inet_pton(AF_INET, address.c_str(), &dest.sin_addr));
	int bytesSent = sendto(winsocket, static_cast<const char*>(data), nBytes, 0, (sockaddr*)&dest, sizeof(dest));
	
	if (bytesSent < 0) {
		int errorCode = WSAGetLastError();
		throw SocketException(std::string("send failure: ") + std::to_string(errorCode));

	}
	Assert(bytesSent == nBytes);
#endif
}

std::vector<Socket::Packet> Socket::Recieve() {

	std::vector<Packet> packets;
	uint8_t* currentBufferPos = recievedBuffer;
	int bytesLeft = bufferLen;
#ifdef WINDOWS
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