#pragma once
#include <string>
#include <optional>
#include <tuple>
#include <cmath>
#include <vector>

#include <boost/asio.hpp>



class SocketException : public std::exception {
public:
	SocketException(std::string error) : std::exception(error.c_str()) {}
};

class SocketConnectionException : public SocketException {
public:
	SocketConnectionException(std::string error) : SocketException(error.c_str()) {}
};

// Thrown if you make a socket with an invalid address.
class SocketAddressResolvingException : public SocketException {
public:
	SocketAddressResolvingException(std::string error) : SocketException(error.c_str()) {}
};

// Portable socket wrapper used to transfer data over the network via UDP.
class Socket {
public:

	// the recieving port on this machine.
	const int localPort;

	// Creates a socket bound to the given port which will only listen to and send to the given address:destPort.
	// port is assigned to localPort. 
	// bufferSize is how much data this socket can hold before Recieve() must be called to avoid dropping data.
	Socket(std::string address, int destPort, int localPort = 49000, unsigned int recvBufferCapacity = pow(2, 16));

	// Creates a socket bound to the given port which can send to and recieve from any address.
	Socket(int localPort = 49000, unsigned int recvBufferCapacity = pow(2, 16));

	~Socket();
	Socket(const Socket&) = delete;

	// max number of bytes we can put in a single udp packet.
	const static inline unsigned int MAX_PACKET_SIZE = 512;

	// nBytes must not exceed MAX_PACKET_SIZE.
	// Literally uses raw UDP to send the requested packet over the network. Absolutely no guarantee if the packet will reach its destination.
	void Send(std::string address, int port, void* data, unsigned int nBytes);
	void Send(void* data, unsigned int nBytes);

	struct Packet {
		std::vector<uint8_t> data;
		std::string originAddress;
		double timestamp = 0; // according to Time() call
		int originPort;
	};

	// Returns a pointer + length in bytes to whatever data the socket has recieved since Recieve() was last called. NOTE: this must be called frequently, or excess data will be discarded.
	// If size is 0, no data has been recieved.
	std::vector<Packet> Recieve();

private:

	class SocketGlobals {
	public:
		static SocketGlobals& Get();

		boost::asio::ip::udp::endpoint GetEndpoint(std::string address, int port);

		SocketGlobals();
		~SocketGlobals();



		boost::asio::io_service ioService;
		boost::asio::ip::udp::resolver ioResolver;

	};

	std::optional<std::string> ipToListenTo;
	std::optional<int> portToListenTo;
	uint8_t* recievedBuffer;
	const unsigned int bufferCapacity;

	boost::asio::ip::udp::socket socket;

	friend void InitSocketGlobals();
};

// Used to ensure correct destruction order for SocketGlobals compared to other singletons.
// Does nothing if already initialized.
void InitSocketGlobals();