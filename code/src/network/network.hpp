#pragma once
#include "client.hpp"
#include "events/event.hpp"
#include "protocol.hpp"

// Describes what the process is doing networkwise - hosting a server? being connected to one? or neither?
enum class NetworkStatus {
	Server, // the networking engine is hosting a server. 
	Client, // the networking engine is connected/connecting to a server.
	Offline // networking engine is currently neither hosting nor being connected to a server. IT MAY STILL be communicating over the internet (i.e. to matchmaking servers).
};

enum class ConnectionFailureReason {
	NoNetwork, // no network to connect to/communicate over
	TimedOut, // server did not respond to connection attempt, perhaps because the ip/port was wrong or because the timeout was too low.
	ServerRejected, // we successfully connected to the server, but they didn't want us :(
};

// This singleton class is in charge of multiplayer stuff.
// Everyone is either a server or a client (or offline).
// Also, you can just directly use the sockets defined in protocol.hpp if your use case requires a less generic, fine-tuned networking method.
class NetworkingEngine {
	public:

	const std::vector<Client>& GetClientList();

	static NetworkingEngine& Get();

	// returns the current network status - whether we're the server, the client, or offline.
	NetworkStatus GetStatus();

	// Begins hosting a server on the given port and sets network status to Server.
	// Aborts if current network status is Server or Client - can't host if you're already hosting/connected to a server.
	void Host(int port = 49000);

	// Kicks the specified client from the server. Must be hosting.
	void Kick(const Client& client);

	// Disconnects from the current server, changing network status to offline. Must be client.
	void Disconnect();

	// Sets network status to Client and asynchronously tries to connect to the given server ip/port. 
	// Aborts if already connected or hosting.
	// Will fire OnConnectionAttemptComplete on completion regardless of success.
		// If successful, network status will remain Client, otherwise it will return to Offline. 
		// If successful, server will begin syncing stuff with the client.
	void Connect(std::string ipAddress, int port = 49000, float timeout = 4.0f);

	// Call every frame (when not offline). Dispatches and recieves/handles network events.
	void Update();

	struct ConnectionAttemptResult {
		bool successful; // true if the networking engine is now successfully connected to the requested server.
		std::optional<ConnectionFailureReason> failureReason;
		std::optional<std::string> failureMessage; // extra failure information, such as a message from the server if the failure reason was ServerRejected.
	};

	std::shared_ptr<Event<ConnectionAttemptResult>> onConnectionAttemptComplete;

	// fired on client when server has finished syncing all of its stuff with the client.
	std::shared_ptr<Event<>> onInitialSyncComplete;

	// A function that the server will call to decide whether to let a potential client connect. Should return <true, nullopt> if the client is allowed to join, or <false, reason> if they are not. 
	// the returned reason must be less than ~400 characters because i don't want to have to use multiple packets for this.
	// By default, it will let anyone connect.
	std::optional<std::function<std::pair<bool, std::optional<std::string>>(std::string ipAddress, int port)>> connectionRequestHandler;

	private:

	NetworkStatus status;

	// empty on client.
	// on server: all the client <-> server connections
	std::vector < std::pair<Client, Socket> > activeConnections;

	// on client: client <-> server socket.
	// on server: socket used for letting newcomers in.
	std::optional< Socket > serverSocket;

	NetworkingEngine();
};