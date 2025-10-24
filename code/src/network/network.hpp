#pragma once
#include "client.hpp"
#include "events/event.hpp"
#include "protocol.hpp"

#include <glm/vec3.hpp>



// Describes what the process is doing networkwise - hosting a server? being connected to one? or neither?
enum class NetworkStatus {
	Server, // the networking engine is hosting a server. 
	Client, // the networking engine is connected to a server.
	ClientConnecting, // the networking engine is attempting to connect to a server.
	Offline // networking engine is currently neither hosting nor being connected to a server. IT MAY STILL be communicating over the internet (i.e. to matchmaking servers).
};

enum class ConnectionFailureReason {
	NoNetwork, // no network to connect to/communicate over
	TimedOut, // server did not respond to connection attempt, perhaps because the ip/port was wrong or because the timeout was too low.
	ServerRejected, // we successfully connected to the server, but they didn't want us :(
};

// Struct for recieving user-specified data over the network.
struct NetworkUserdata {
	std::vector<uint8_t> data;
	bool reliable;
};




class GameObject;
class TransformComponent;

std::pair<bool, std::optional<std::string>> DefaultConnectionRequestHandler(std::string ipAddress, int port);

// This singleton class is in charge of multiplayer stuff.
// Everyone is either a server or a client (or offline).
// Also, you can just directly use the sockets defined in protocol.hpp if your use case requires a less generic, fine-tuned networking method.
	// This is primarily for networking specifically chosen gameobjects.
class NetworkingEngine {
public:
	float RESEND_PACKET_TIME = 2.0f;
	float TIMEOUT_TIME = 30.0f;

	const std::vector<std::shared_ptr<Client>>& GetClientList();

	static NetworkingEngine& Get();

	// returns the current network status - whether we're the server, the client, or offline.
	NetworkStatus GetStatus();

	// Begins hosting a server on the given port and sets network status to Server.
	// Aborts if current network status is Server or Client - can't host if you're already hosting/connected to a server.
	void Host(int port = 49000);

	// Stops hosting a server, calling Kick() for any currently connected clients. Must be Server.
	// Automatically called by destructor if Server.
	void Unhost();

	// Kicks the specified client from the server. Must be Server.
	void Kick(std::shared_ptr<Client>& client, std::string reason);

	// Disconnects from the current server, changing network status to offline. Must be Client or ClientConnecting (which would cancel the connection attempt).
	void Disconnect();

	// Sets network status to ClientConnecting and asynchronously tries to connect to the given server ip/port. 
	// Aborts if already connected or hosting.
	// Will fire OnConnectionAttemptComplete on completion regardless of success.
		// If successful, network status will change to Client, otherwise it will return to Offline. 
		// If successful, server will begin syncing stuff with the client.
	void Connect(std::string address, int serverPort = 49000, int localPort = 49001, float timeoutPerTry = 0.5f, unsigned maxTries = 10);

	// Call every frame. Dispatches and recieves/handles network events.
	// dt should be time since Update() was last called.
	void Update(float dt);

	std::shared_ptr<Client> GetLocalMachine();
	std::shared_ptr<Client> GetServer();

	// Returns nullptr if no connected client with given info
	std::shared_ptr<Client> GetClient(std::string address, int port);

	struct ConnectionAttemptResult {
		bool successful; // true if the networking engine is now successfully connected to the requested server.
		std::optional<ConnectionFailureReason> failureReason;
		std::optional<std::string> failureMessage; // extra failure information, such as a message from the server if the failure reason was ServerRejected.
	};

	std::shared_ptr<Event<ConnectionAttemptResult>> onConnectionAttemptComplete;

	// Fired on both client and server.
	//std::shared_ptr < Event<std::shared_ptr<Client>>> onNewClient;

	// fired on client when server has finished syncing all of its stuff with the client.
	std::shared_ptr<Event<>> onInitialSyncComplete;

	std::shared_ptr<Event<std::shared_ptr<Client>>> onNewClient;

	// fired on client when data from SendData()/SendDataReliable() arrives
	std::shared_ptr<Event<NetworkUserdata>> onUserdataRecieved;

	// A function that the server will call to decide whether to let a potential client connect. Should return <true, nullopt> if the client is allowed to join, or <false, reason> if they are not. 
	// the returned reason must be less than ~400 characters because i don't want to have to use multiple packets for this.
	// By default, it will let anyone connect.
	std::function<std::pair<bool, std::optional<std::string>>(std::string ipAddress, int port)> connectionRequestHandler = DefaultConnectionRequestHandler;

	void SendDataReliable(void* data, size_t nBytes, std::shared_ptr<Client>& destination);

	// Sends data to server. Client only.
	void SendDataReliable(void* data, size_t nBytes);

	// nBytes must be <=500
	void SendData(void* data, size_t nBytes, std::shared_ptr<Client>& destination);

	// Sends data to server. Client only.
	void SendData(void* data, size_t nBytes);

	// Informs the networking engine to sync this gameobject's transform using the given sync id.
		// Defaults to Server being the owner.
	void SyncObjectTransform(std::shared_ptr<GameObject> obj, SyncId name, bool ackSnapshots = true);

	// Sets the network owner of a given gameobject's transform to the given client (which could be the server).
	// Gameobject must have a transform and a rigidbody.
	// Server only.
	void SetNetworkOwner(std::shared_ptr<GameObject> obj, std::shared_ptr<Client> client);

private:
	// Used for specifying an order to packets when sending them. Doesn't correspond to the ticks of recieved packets.
	NetworkTickId currentTick;

	void ImplSendDataReliable(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata);

	// nBytes must be <=500
	void ImplSendData(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata);

	void HandleAck(Socket::Packet& packet);

	// Also times out connections.
	void ResendUnackedMessages();
	
	void AckMessages();

	void ProcessAckArray(std::shared_ptr<Client>& client, AckId* acks, unsigned nAcks);
	void ProcessShortMessage(std::shared_ptr<Client>& client, uint8_t* data, unsigned nBytes, bool isUserdata);
	void ProcessShortMessageReliable(std::shared_ptr<Client>& client, AckId ackId, uint8_t* data, unsigned nBytes, bool isUserdata);
	void ProcessLongMessageFragment(std::shared_ptr<Client>& client, AckId firstAckId, uint16_t idOffset, uint16_t nPackets, uint8_t* data, unsigned nBytes,  bool isUserdata);

	void SyncGameobjects();

	void HandleTransformSyncPacket(std::shared_ptr<Client>& client, PacketStructs::TransformSyncPacket*, unsigned nSnapshots);
	void HandleRigidbodySyncPacket(std::shared_ptr<Client>& client, PacketStructs::RigidbodySyncPacket*, unsigned nSnapshots);

	std::vector<std::shared_ptr<Client>> clients;

	

	float timeUntilConnectionAttemptTimeout = -1.0f;
	float connectionAttemptTimeout = -1.0f;
	int connectionAttemptsRemaining = -1;
	std::string targetConnectionIp = "";

	NetworkStatus status;
	
	// client <-> server socket.
	std::optional<Socket> serverSocket;

	NetworkingEngine();
};