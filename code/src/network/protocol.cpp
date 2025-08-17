#include "protocol.hpp"
#include <debug/assert.hpp>

using namespace boost::asio::ip;
using boost::asio::buffer;

Socket::Socket(std::optional<std::string> ip, int port, unsigned int bufferSize):
	localPort(port),
	socket(ioService, udp::v4(), port),
	ipToListenTo(ip),
	recievedBuffer(malloc(bufferSize)),
	bufferLen(bufferSize)
{
	Assert(recievedBuffer);
	
	//socket.open(udp::v4());
	
	if (ipToListenTo) {
		socket.bind(udp::endpoint(address::from_string("192.168.0.1"), port));
	}

}

void Socket::Send(std::string address, int port, const void* data, unsigned int nBytes) {
	socket.send_to(buffer(data, nBytes), udp::endpoint(address::from_string(address), port));
}

Socket::Packet Socket::Recieve() {

	if (ipToListenTo) {
		boost::system::error_code error;
		unsigned len = socket.receive(buffer(recievedBuffer, bufferLen), 0, error);
		Assert(!error);
		return Packet{ recievedBuffer, len, *ipToListenTo };
	}
	else {
		udp::endpoint origin;
		boost::system::error_code error;
		unsigned len = socket.receive_from(buffer(recievedBuffer, bufferLen), origin, 0, error);
		Assert(!error);

		std::string origin_ip = origin.address().to_string();

		return Packet{ recievedBuffer, len, origin_ip };
	}
	
}

Socket::~Socket() {
	socket.close();
	free(recievedBuffer);
}