#include "protocol.hpp"
#include <debug/assert.hpp>
#include <utility/utility.hpp>

using namespace boost::asio;

Socket::SocketGlobals& Socket::SocketGlobals::Get() {
	static SocketGlobals globals;
	return globals;
}

boost::asio::ip::udp::endpoint Socket::SocketGlobals::GetEndpoint(std::string address, int port)
{
	ip::udp::resolver::query query(address, std::to_string(port).c_str());
	try {
		auto results = ioResolver.resolve(query);
		return *results;
	}
	catch (boost::system::system_error& err) {
		throw SocketAddressResolvingException(std::string(err.what()));
	}
}

Socket::SocketGlobals::SocketGlobals():
ioService(),
ioResolver(ioService)
{
	
}

Socket::SocketGlobals::~SocketGlobals() {

}



Socket::Socket(std::string ip, int destPort, int localPort, unsigned int recvBufferCapacity) :
	localPort(localPort),
	ipToListenTo(ip),
	portToListenTo(destPort),
	recievedBuffer(static_cast<uint8_t*>(malloc(recvBufferCapacity))),
	bufferCapacity(recvBufferCapacity),
	socket(SocketGlobals::Get().ioService)
{
	socket.open(ip::udp::v4());
	socket.bind(ip::udp::endpoint(ip::udp::v4(), localPort));
	socket.connect(SocketGlobals::Get().GetEndpoint(ip, destPort));
	Assert(recievedBuffer);
	socket.non_blocking(true);

	
}

Socket::Socket(int localPort, unsigned int recvBufferCapacity):
	localPort(localPort),
	ipToListenTo(std::nullopt),
	portToListenTo(std::nullopt),
	recievedBuffer(static_cast<uint8_t*>(malloc(recvBufferCapacity))),
	bufferCapacity(recvBufferCapacity),
	socket(SocketGlobals::Get().ioService) // TODO: non-ipv4 support?
{
	socket.open(ip::udp::v4());
	socket.bind(ip::udp::endpoint(ip::udp::v4(), localPort));

	Assert(recievedBuffer);
	socket.non_blocking(true);

}


void Socket::Send(std::string address, int port, void* data, unsigned int nBytes) {
	try {
		DebugLogInfo("Sending () ", nBytes);
		socket.send_to(buffer(data, nBytes), SocketGlobals::Get().GetEndpoint(address, port), 0);
	}
	catch (std::exception& e) {
		DebugLogInfo("Boost socket error ", e.what());
		Assert(false);
	}
}

void Socket::Send(void* data, unsigned int nBytes) {
	try {
		DebugLogInfo("Sending ", nBytes);
		socket.send(buffer(data, nBytes), 0);
	}
	catch (std::exception& e) {
		DebugLogInfo("Boost socket error ", e.what());
		Assert(false);
	}
}

// TODO: ENDIANNESS

std::vector<Socket::Packet> Socket::Recieve() {

	std::vector<Packet> packets;

	auto timestamp = Time();

	while (true) {
		ip::udp::endpoint sender_endpoint;

		boost::system::error_code err;
		size_t len = socket.receive_from(buffer(recievedBuffer, bufferCapacity), sender_endpoint, 0, err);
		if (err.value() == boost::system::errc::success) { // success
			Packet newPacket;
			newPacket.originAddress = sender_endpoint.address().to_string();
			newPacket.originPort = sender_endpoint.port();
			newPacket.data.assign(recievedBuffer, recievedBuffer + len);
			newPacket.timestamp = timestamp;
			packets.push_back(newPacket);
		}
		else if (err.value() == boost::system::errc::operation_would_block || err.value() == 10035) { // then no more data to recieve at the moment
			return packets;
		}
		else if (err.value() == 10054) { // IDK, https://stackoverflow.com/questions/34242622/windows-udp-sockets-recvfrom-fails-with-error-10054
			DebugLogInfo("WARNING 10054");
			return packets;
		}
		else {
			throw SocketException("boost socket recieve_from returned error code " + std::to_string(err.value()));
		}
	}	
}


Socket::~Socket() {
	socket.close();
	//free(recievedBuffer);
}

void InitSocketGlobals() {
	Socket::SocketGlobals::Get();
}
