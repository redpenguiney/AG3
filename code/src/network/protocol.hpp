#pragma once
#include <string>
#include <optional>
#include <tuple>
#include <cmath>
#include <vector>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define WINDOWS
#define WIN32_LEAN_AND_MEAN 
#endif

#ifdef WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#error unsupported platform
#endif

class SocketConnectionException : public std::exception {
public:
	SocketConnectionException(std::string error) : std::exception(error.c_str()) {}
};

class SocketException : public std::exception {
public:
	SocketException(std::string error) : std::exception(error.c_str()) {}
};

// Portable socket wrapper used to transfer data over the network via UDP.
class Socket {
public:

	// the recieving port on this machine.
	const int localPort;

	// if no ipToListenTo, will listen to all incoming ips (what you would want for a server).
	// port is assigned to localPort.
	// bufferSize is how much data this socket can hold before Recieve() must be called to avoid dropping data.
	Socket(std::optional<std::string> ipToListenTo, int port = 49000, unsigned int bufferSize = pow(2, 20));
	~Socket();
	Socket(const Socket&) = delete;

	// max number of bytes we can put in a single udp packet.
	const static inline unsigned int MAX_PACKET_SIZE = 512;

	// nBytes must not exceed MAX_PACKET_SIZE.
	// Literally uses raw UDP to send the requested packet over the network. Absolutely no guarantee if the packet will reach its destination.
	void Send(std::string address, int port, void* data, unsigned int nBytes);

	struct Packet {
		const void* data;
		unsigned int dataNBytes;
		std::string ipOfOrigin;
	};

	// Returns a pointer + length in bytes to whatever data the socket has recieved since Recieve() was last called. NOTE: this must be called frequently, or excess data will be discarded.
	// If size is 0, no data has been recieved.
	// The returned packet's data becomes invalid after the next time Recieve() is called or upon socket destruction.
	std::vector<Packet> Recieve();

private:

#ifdef WINDOWS
	SOCKET winsocket;
#endif

	class SocketInitializer {
	public:
		static void EnsureSocketsInitialized();

		SocketInitializer();
		~SocketInitializer();

	private:
		WSADATA wsaData;
	};

	std::optional<std::string> ipToListenTo;
	uint8_t* recievedBuffer;
	const unsigned int bufferLen;
};

