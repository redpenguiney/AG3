#pragma once
#include <string>
#include <optional>
#include <boost/asio.hpp>
#include <tuple>
#include <cmath>

// Portable socket wrapper used to transfer data over the network.
class Socket {
	public:
	// the recieving port on this machine.
	const int localPort;

	// if no ipToListenTo, will listen to all incoming ips (what you would want for a server).
	// port is assigned to localPort.
	// bufferSize is how much data this socket can hold before Recieve() must be called to avoid dropping data.
	Socket(std::optional<std::string> ipToListenTo, int port = 49000, unsigned int bufferSize = pow(2, 16));
	~Socket();
	Socket(const Socket&) = delete;

	// max number of bytes we can put in a single udp packet.
	const static inline unsigned int MAX_PACKET_SIZE = 512;

	// nBytes must not exceed MAX_PACKET_SIZE.
	// Literally uses raw UDP to send the requested packet over the network. Absolutely no guarantee if the packet will reach its destination.
	void Send(std::string address, int port, const void* data, unsigned int nBytes);

	// Returns a pointer + length in bytes to whatever data the socket has recieved since Recieve() was last called. NOTE: this must be called frequently, or excess data will be discarded.
	// If size is 0, no data has been recieved.
	// Do not use the returned pointer after the socket's destruction.
	std::pair<const void*, unsigned int> Recieve();

	private:

	static boost::asio::io_service ioService;

	std::optional<std::string> ipToListenTo;
	boost::asio::ip::udp::socket socket;
	void* recievedBuffer;
	const unsigned int bufferLen;
};

