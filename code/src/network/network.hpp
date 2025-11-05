#pragma once
#include "client.hpp"
#include "events/event.hpp"
#include "protocol.hpp"
#include "serialization.hpp"
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
	std::shared_ptr<Client> sender;
	std::vector<uint8_t> data;
	bool reliable;
};




class GameObject;
class TransformComponent;

// May not be 0 (which is used for unformatted userdata)
using UserdataFormatName = uint16_t;

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

	// fired on client when server has finished syncing all of its stuff with the client.
	std::shared_ptr<Event<>> onInitialSyncComplete;

	// Only fired on server.
	std::shared_ptr<Event<std::shared_ptr<Client>>> onNewClient;

	// fired when data from SendData()/SendDataReliable() arrives
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

	template <class ... Types>
	void SendFormattedUserdata(UserdataFormatName format, std::shared_ptr<Client> dest, bool reliable, Types...);

	// Sends data to server. Client only.
	template <class ... Types>
	void SendFormattedUserdata(UserdataFormatName format, bool reliable, Types...);

	// Informs the networking engine to sync this gameobject's transform using the given sync id.
		// Defaults to Server being the owner.
	void SyncObjectTransform(std::shared_ptr<GameObject> obj, SyncId name, bool ackSnapshots = true);

	// Sets the network owner of a given gameobject's transform to the given client (which could be the server).
	// Gameobject must have a transform and a rigidbody.
	// Server only.
	void SetNetworkOwner(std::shared_ptr<GameObject> obj, std::shared_ptr<Client> client);

	template <class ... Types>
	std::shared_ptr<Event<std::shared_ptr<Client>, Types...>> GetFormattedUserdataRecievedEvent(UserdataFormatName name);

	// It's stored via shared_ptr so the event itself won't be immediately destroyed.
	// However, calling this will immediately stop the event from being fired by recieved formatted data with the corresponding format name.
	// (Event destruction will never occur except on application exit/networking engine desturction if this method is not called!)
	// An event subsequently created with the same name/template types will be a wholly unrelated event instance which must be connected to once again. 
	void DestroyFormattedUserdataRecievedEvent(UserdataFormatName name);

private:
	// We store lambdas/functions that store and fire the events to handle the different types. 
	std::unordered_map<UserdataFormatName, std::function<void(std::shared_ptr<Client>, uint8_t*, unsigned int)>> formattedUserdataEvents;

	// Used for specifying an order to packets when sending them. Doesn't correspond to the ticks of recieved packets.
	NetworkTickId currentTick;

	void ImplSendDataReliable(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata, UserdataFormatName format = 0);

	// nBytes must be <=500
	void ImplSendData(void* data, size_t nBytes, std::shared_ptr<Client>& destination, bool isUserdata, UserdataFormatName format = 0);

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

template<class ...Types>
inline void NetworkingEngine::SendFormattedUserdata(UserdataFormatName format, std::shared_ptr<Client> dest, bool reliable, Types ... types) {
	
	size_t nBytes = (SerializedSize<Types>(types) + ...);
	void* serializedData = malloc(nBytes);
	void* current = serializedData;
	(Serialize(types, current), ...);

	if (!reliable) Assert(sizeof(serializedData) <= 500);
	
	if (reliable)
		ImplSendDataReliable(serializedData, sizeof(serializedData), dest, true, format);
	else
		ImplSendData(serializedData, sizeof(serializedData), dest, true, format);

}

template<class ...Types>
inline void NetworkingEngine::SendFormattedUserdata(UserdataFormatName format, bool reliable, Types ... data) {
	Assert(status == NetworkStatus::Client);
	for (auto& c : clients) {
		if (c->isServer) {
			SendFormattedUserdata(format, c, reliable, data...);
			return;
		}
	}
}

template <size_t I, size_t End, class ...Types>
void DeserializeToTuple(std::tuple<std::shared_ptr<Client>, Types...>& data, void*& src) {
	std::get<I>(data) = Deserialize<typename std::tuple_element<I, std::tuple<std::shared_ptr<Client>, Types...>>::type>(src);
	if constexpr (I + 1 < End) {
		DeserializeToTuple<I + 1, End, Types...>(data, src);
	}
}

template<class ...Types>
inline std::shared_ptr<Event<std::shared_ptr<Client>, Types...>> NetworkingEngine::GetFormattedUserdataRecievedEvent(UserdataFormatName name)
{
	auto event = Event<std::shared_ptr<Client>, Types...>::New();

	auto delegate = [event, name](std::shared_ptr<Client> client, uint8_t* data, unsigned nBytes) {
		//size_t expectedSize = (SerializedSize<>??? + ...);
		//if (expectedSize != nBytes) {
			//DebugLogInfo("Recieved formatted userdata ", name, ", but size was ", nBytes, " rather than ", expectedSize, " bytes.");
			//return;
		//}
		std::string o;
		for (unsigned i = 0; i < nBytes; i++) o += std::to_string((int)data[i]) + " ";
		DebugLogInfo("Deserializing input data ", o);
		

		void* voiddata = data;
		std::tuple<std::shared_ptr<Client>, Types...> formattedData;
		std::get<0>(formattedData) = client;
		DeserializeToTuple<1, sizeof...(Types)+1, Types...>(formattedData, voiddata);
		event->Fire(formattedData);
	};

	formattedUserdataEvents[name] = delegate;

	return event;
}
