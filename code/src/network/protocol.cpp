#include "protocol.hpp"

using namespace boost::asio::ip;
using boost::asio::buffer;

//Socket::Socket(std::optional<std::string> ip, int port, unsigned int bufferSize):
//	localPort(port),
//	socket(udp::socket {ioService}),
//	ipToListenTo(ip),
//	recievedBuffer(malloc(bufferSize)),
//	bufferLen(bufferSize)
//{
//	
//	socket.open(udp::v4());
//	socket.bind(udp::endpoint(address::from_string("192.168.0.1"), port));
//
//}

void Socket::Send(std::string address, int port, const void* data, unsigned int nBytes) {
	socket.send_to(buffer(data, nBytes), udp::endpoint(address::from_string(address), port));
}

//std::pair<const void*, unsigned int> Socket::Recieve() {
//	if (ipToListenTo.has_value()) {
//		socket.receive_from(buffer(recievedBuffer, bufferLen), udp::endpoint(address::from_string(ipToListenTo.value()), localPort));
//	}
//	else {
//		socket.receive(buffer(recievedBuffer, bufferLen));
//	}
//	
//}

Socket::~Socket() {
	socket.close();
	free(recievedBuffer);
}