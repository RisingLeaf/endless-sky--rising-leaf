/* Network.cpp
Copyright (c) 2026 by RisingLeaf

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Network.h"

#include "Command.h"
#include "GameData.h"
#include "Logger.h"
#include "Messages.h"
#include "image/Sprite.h"
#include "image/SpriteSet.h"

#include <cmath>


namespace {
	std::atomic NETWORKING_AVAILABLE{false};

	constexpr std::size_t MAX_CLIENTS  = 4;
	constexpr std::size_t MAX_CHANNELS = 2;

	constexpr enet_uint32 EVENT_TIMEOUT_MS = 50;
	constexpr enet_uint16 DEFAULT_PORT     = 55542;

	constexpr std::size_t MAX_PACKET_SIZE   = 4096;
	constexpr std::size_t MAX_QUEUED_EVENTS = 4096;
	constexpr std::size_t MAX_DRAWABLES     = 2048;

	constexpr asl::uint8 ALL_FIELD_MASK = ~0;

	void LogMessage(const std::string_view text)
	{
		Messages::Add(Message(text.data(), GameData::MessageCategories().Get("high")));

		Logger::Log(text, Logger::Level::INFO);
	}

	bool IsValidEventType(unsigned char value)
	{
		switch(static_cast<Network::EventType>(value))
		{
		case Network::EventType::TEXT:
		case Network::EventType::UPDATE_INPUT_STATE:
		case Network::EventType::PUSH_SPRITE:
		case Network::EventType::SET_DRAWABLE:
		case Network::EventType::FREE_DRAWABLES:     return true;
		case Network::EventType::INVALID:            return false;
		}

		return false;
	}

	std::optional<std::vector<unsigned char>> EncodeEvent(const Network::Event &event)
	{
		if(event.Data.size() >= MAX_PACKET_SIZE)
			return std::nullopt;

		std::vector<unsigned char> encoded;
		encoded.emplace_back(static_cast<unsigned char>(event.Type));
		encoded.append_range(event.Data);
		return encoded;
	}

	std::optional<Network::Event> DecodeEvent(const unsigned char *data, const std::size_t size)
	{
		if(data == nullptr || size == 0 || size > MAX_PACKET_SIZE)
			return std::nullopt;

		if(!IsValidEventType(data[0]))
			return std::nullopt;

		Network::Event result;
		result.Type = static_cast<Network::EventType>(data[0]);
		result.Data.assign(data + 1, data + size);

		return result;
	}

	bool SendPacket(ENetPeer *peer, const Network::Event &event, const bool reliable)
	{
		if(peer == nullptr || peer->state != ENET_PEER_STATE_CONNECTED)
			return false;

		const auto encoded = EncodeEvent(event);
		if(!encoded)
			return false;

		const enet_uint32 flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;

		ENetPacket *packet = enet_packet_create(encoded->data(), encoded->size(), flags);
		if(!packet)
			return false;

		if(enet_peer_send(peer, 0, packet) < 0)
		{
			enet_packet_destroy(packet);
			return false;
		}

		return true;
	}

	template <std::size_t N>
	using uint_of_size = std::conditional_t<
	    N == 1,
	    std::uint8_t,
	    std::conditional_t<N == 2, std::uint16_t, std::conditional_t<N == 4, std::uint32_t, std::uint64_t>>>;

	template <typename T>
	constexpr bool IsSupportedSerializedType =
	    (std::is_integral_v<T> && !std::is_same_v<T, bool> &&
		 (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8)) ||
	    (std::is_floating_point_v<T> && (sizeof(T) == 4 || sizeof(T) == 8));

	template <typename Type>
	    requires IsSupportedSerializedType<Type>
	void WriteSerialized(std::vector<unsigned char> &out, Type value)
	{
		using U      = uint_of_size<sizeof(Type)>;
		const U bits = std::bit_cast<U>(value);

		for(std::size_t i = 0; i < sizeof(Type); ++i)
			out.emplace_back(static_cast<unsigned char>((bits >> (i * 8)) & U{0xff}));
	}

	template <typename Type>
	    requires IsSupportedSerializedType<Type>
	bool ReadSerialized(const std::vector<unsigned char> &data, std::size_t &offset, Type &value)
	{
		using U = uint_of_size<sizeof(Type)>;

		if(offset > data.size() || sizeof(Type) > data.size() - offset)
			return false;

		U bits = 0;
		for(std::size_t i = 0; i < sizeof(Type); ++i)
			bits |= static_cast<U>(static_cast<U>(data[offset + i]) << (i * 8));

		value = std::bit_cast<Type>(bits);
		offset += sizeof(Type);

		return true;
	}

	// Applies an operation for every entry in BodyInfo that has a flag group annotation
	template <class F>
	void OperateOnBodyEntries(F &&rule)
	{
		template for(constexpr auto entry : asl::nonstatic_members<Network::BodyInfo>())
		{
			if constexpr(asl::has_annotation<Network::BodyInfoFlagGroup, entry>())
			{
				constexpr auto flag_group = asl::unique_annotation_of_type<Network::BodyInfoFlagGroup, entry>().Group;
				rule.template  operator()<entry>(flag_group);
			}
		}
	}
} // namespace


void Network::Init()
{
	if(NETWORKING_AVAILABLE.load())
		return;

	if(enet_initialize() != 0)
	{
		Logger::Log("An error occurred while initializing ENet.", Logger::Level::ERROR);
		return;
	}

	std::atexit(enet_deinitialize);
	NETWORKING_AVAILABLE.store(true);
}

bool Network::NetworkingAvailable()
{
	return NETWORKING_AVAILABLE.load();
}

Network::Event Network::SerializeCommand(const Command &command)
{
	Event result;
	result.Type = EventType::UPDATE_INPUT_STATE;
	WriteSerialized(result.Data, command.State());
	WriteSerialized(result.Data, command.Turn());

	return result;
}

std::optional<Command> Network::DeserializeCommand(const Event &event)
{
	constexpr static std::size_t EXPECTED_SIZE = sizeof(std::uint64_t) + sizeof(double);

	if(event.Type != EventType::UPDATE_INPUT_STATE)
		return std::nullopt;
	if(event.Data.size() != EXPECTED_SIZE)
		return std::nullopt;


	std::uint64_t state{};
	double        turn{};

	std::size_t offset = 0;
	if(!ReadSerialized(event.Data, offset, state))
		return std::nullopt;
	if(!ReadSerialized(event.Data, offset, turn))
		return std::nullopt;
	if(offset != event.Data.size() || !std::isfinite(turn))
		return std::nullopt;

	return Command::CreateSubstituteCommand(state, turn);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// NetworkInterface
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Network::NetworkInterface::~NetworkInterface()
{
	Stop();
	network_interface.reset();
}

void Network::NetworkInterface::Stop()
{
	Running.store(false);

	if(EventLoop.joinable())
	{
		if(EventLoop.get_id() == std::this_thread::get_id())
			std::terminate();

		EventLoop.join();
	}
}

bool Network::NetworkInterface::SendEvent(const Event &event, const bool reliable)
{
	if(!Running.load())
		return false;

	if(event.Type == EventType::INVALID)
		return false;

	if(event.Data.size() + 1 > MAX_PACKET_SIZE)
		return false;

	std::lock_guard lock(EventMutex);

	if(!Running.load() || OutgoingEvents.size() >= MAX_QUEUED_EVENTS)
		return false;

	OutgoingEvents.push(OutgoingEvent{event, reliable});
	return true;
}

std::optional<Network::Event> Network::NetworkInterface::PollEvent()
{
	std::lock_guard lock(EventMutex);

	if(IncomingEvents.empty())
		return std::nullopt;

	Event event = std::move(IncomingEvents.front());
	IncomingEvents.pop();

	return event;
}

void Network::NetworkInterface::OnPeerConnected(ENetPeer *peer)
{
	char ip[64]{};

	if(enet_address_get_host_ip(&peer->address, ip, sizeof(ip)) < 0)
		LogMessage("A client connected.");
	else
		LogMessage(std::format("Client connected from {}:{}", ip, peer->address.port));
}

void Network::NetworkInterface::OnPeerDisconnected(ENetPeer *peer)
{
	LogMessage("A peer disconnected.");
}

bool Network::NetworkInterface::ProcessEvents()
{
	if(!network_interface)
		return false;

	std::queue<OutgoingEvent> outgoing;
	{
		std::lock_guard lock(EventMutex);
		std::swap(outgoing, OutgoingEvents);
	}

	while(!outgoing.empty())
	{
		auto [event, reliable] = std::move(outgoing.front());
		outgoing.pop();

		for(std::size_t i = 0; i < network_interface->peerCount; ++i)
		{
			ENetPeer *target = &network_interface->peers[i];

			if(target->state != ENET_PEER_STATE_CONNECTED)
				continue;

			if(!SendPacket(target, event, reliable))
				Logger::Log("Failed to queue event for a peer.", Logger::Level::WARNING);
		}
	}

	enet_host_flush(network_interface.get());

	ENetEvent event{};

	const int result = enet_host_service(network_interface.get(), &event, EVENT_TIMEOUT_MS);
	if(result < 0)
	{
		Logger::Log("ENet host service failed.", Logger::Level::ERROR);
		return false;
	}
	if(result == 0)
		return true;

	switch(event.type)
	{
	case ENET_EVENT_TYPE_CONNECT: OnPeerConnected(event.peer); break;

	case ENET_EVENT_TYPE_RECEIVE: {
		const std::unique_ptr<ENetPacket, decltype(&enet_packet_destroy)> packet(event.packet, &enet_packet_destroy);

		if(packet->dataLength == 0 || packet->dataLength > MAX_PACKET_SIZE)
		{
			Logger::Log("Received an empty or oversized packet.", Logger::Level::WARNING);
			break;
		}

		auto decoded = DecodeEvent(packet->data, packet->dataLength);
		if(!decoded)
		{
			Logger::Log("Received an invalid event packet.", Logger::Level::WARNING);
			break;
		}

		if(InterceptEvent(event.peer, *decoded))
			break;

		std::lock_guard lock(EventMutex);
		if(IncomingEvents.size() >= MAX_QUEUED_EVENTS)
		{
			Logger::Log("Incoming event queue is full; dropping packet.", Logger::Level::WARNING);
			break;
		}

		IncomingEvents.push(std::move(*decoded));
		break;
	}

	case ENET_EVENT_TYPE_DISCONNECT:
		OnPeerDisconnected(event.peer);
		event.peer->data = nullptr;
		break;

	case ENET_EVENT_TYPE_NONE: break;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Server
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Network::Server::~Server()
{
	Stop();
}

std::expected<std::unique_ptr<Network::Server>, std::string_view> Network::Server::CreateServer()
{
	if(!NetworkingAvailable())
		return std::unexpected{"Networking has not been initialized."};

	auto instance = std::unique_ptr<Server>(new Server);

	instance->address.host = ENET_HOST_ANY;
	instance->address.port = DEFAULT_PORT;

	instance->network_interface.reset(enet_host_create(&instance->address, MAX_CLIENTS, MAX_CHANNELS, 0, 0));

	if(!instance->network_interface)
		return std::unexpected{"Failed to create ENet server host."};

	instance->Running.store(true);

	try
	{
		instance->EventLoop = std::thread([ptr = instance.get()] {
			ptr->RunEventLoop();
		});
	} catch(...)
	{
		instance->Running.store(false);

		return std::unexpected{"Failed to start server event loop."};
	}

	return instance;
}

std::vector<Network::Server::PeerState> Network::Server::GetPeerStates() const
{
	std::vector<PeerState> states;

	std::lock_guard guard(PeersMutex);

	states.reserve(Peers.size());

	for(const auto &state : Peers | std::views::values)
		states.emplace_back(state);

	return states;
}

void Network::Server::SendSprite(const Sprite *const sprite)
{
	Event pushSpriteEvent;
	pushSpriteEvent.Type = EventType::PUSH_SPRITE;
	pushSpriteEvent.Data.reserve(sprite->Name().size());
	for(const char c : sprite->Name())
		WriteSerialized(pushSpriteEvent.Data, c);

	SendEvent(pushSpriteEvent, true);
}

asl::uint64 Network::Server::PushSprite(const Sprite *const sprite)
{
	const asl::uint64 spriteID = SpriteIDs.size();
	SpriteIDs[sprite]          = spriteID;

	SpriteByID.emplace_back(sprite);

	SendSprite(sprite);

	LogMessage(std::format("Pushing new sprite: {}, total: {}", sprite->Name(), spriteID));

	return spriteID;
}

void Network::Server::MarkUnused()
{
	for(auto &info : Bodies | std::views::values)
		info.Active = false;

	BodyCount = 0;
	BodyDataToSend.clear();
}

asl::uint16 Network::Server::PackSwizzle(const BodyInfo &info)
{
	asl::uint16 swizzlePacked = 0;
	for(int i = 0; i < 16; i++)
		swizzlePacked |= info.swizzle[i] ? 1 << i : 0;
	return swizzlePacked;
}

void Network::Server::GenerateBodyEvent(const asl::uint64 spriteID, const BodyInfo &info, const asl::uint8 mask)
{
	if(mask == 0)
		return;
	WriteSerialized(BodyDataToSend, mask);
	WriteSerialized(BodyDataToSend, info.Index);

	OperateOnBodyEntries([&]<auto entry>(const asl::uint8 flag_group) {
		if(mask & (1 << flag_group))
		{
			if constexpr(entry == ^^BodyInfo::sprite)
				WriteSerialized(BodyDataToSend, spriteID);
			else if constexpr(entry == ^^BodyInfo::swizzle)
				WriteSerialized(BodyDataToSend, PackSwizzle(info));
			else
				WriteSerialized(BodyDataToSend, info.[:entry:]);
		}
	});

	++BodyCount;
}

void Network::Server::PushBody(const Body &body, Point position, double cloak, const int step)
{
	const auto sprite = body.GetSprite();
	if(!sprite)
		return;

	asl::uint64 spriteID = 0;
	if(const auto it = SpriteIDs.find(sprite); it == SpriteIDs.end())
		spriteID = PushSprite(sprite);
	else
		spriteID = it->second;

	BodyInfo info{.Active = true,
	              .sprite = body.GetSprite(),
	              .frame  = body.GetFrame(step),
	              .xpos   = static_cast<asl::float32>(position.X()),
	              .ypos   = static_cast<asl::float32>(position.Y()),
	              .xvel   = static_cast<asl::float32>(body.Velocity().X()),
	              .yvel   = static_cast<asl::float32>(body.Velocity().Y()),
	              .angle  = body.Facing().Raw(),
	              .alpha  = static_cast<float>(1.0 - cloak),
	              .xscale = static_cast<float>(0.5 * body.Zoom() * body.Scale().X()),
	              .yscale = static_cast<float>(0.5 * body.Zoom() * body.Scale().Y())};
	for(int i = 0; i < 16; i++)
		info.swizzle[i] = body.GetSwizzle()->MatrixPtr()[i] != 0.0;

	if(const auto it = Bodies.find(&body); it != Bodies.end())
	{
		// Compute Difference Event
		Event updateBodyEvent;
		updateBodyEvent.Type = EventType::SET_DRAWABLE;

		const auto existing = it->second;
		info.Index          = existing.Index;

		// Reserve mask space upfront
		asl::uint8 mask = 0;

		OperateOnBodyEntries([&]<auto entry>(const asl::uint8 flag_group) {
			if(info.[:entry:] != existing.[:entry:])
				mask |= 1 << flag_group;
		});

		GenerateBodyEvent(spriteID, info, mask);

		it->second = info;
	}
	else
	{
		if(!FreeIndices.empty())
			info.Index = FreeIndices.extract(FreeIndices.begin()).value();
		else
			info.Index = NextIndex++;

		Bodies[&body] = info;

		GenerateBodyEvent(spriteID, info, ALL_FIELD_MASK);
	}
}

void Network::Server::SendToClient()
{
	Event sendBodies;
	sendBodies.Type = EventType::SET_DRAWABLE;
	WriteSerialized(sendBodies.Data, BodyCount);
	sendBodies.Data.append_range(BodyDataToSend);
	SendEvent(sendBodies);

	Event freeUnusedBodies;
	freeUnusedBodies.Type = EventType::FREE_DRAWABLES;

	std::vector<std::size_t> indices;
	std::erase_if(Bodies, [&](const auto &body) -> bool {
		if(!body.second.Active)
		{
			FreeIndices.insert(body.second.Index);
			indices.emplace_back(body.second.Index);
			return true;
		}
		return false;
	});

	WriteSerialized(freeUnusedBodies.Data, indices.size());
	for(const std::size_t index : indices)
		WriteSerialized(freeUnusedBodies.Data, index);

	SendEvent(freeUnusedBodies, true);
}

void Network::Server::OnPeerConnected(ENetPeer *connectedPeer)
{
	char ip[64]{};

	if(enet_address_get_host_ip(&connectedPeer->address, ip, sizeof(ip)) < 0)
		LogMessage("A client connected.");
	else
		LogMessage(std::format("Client connected from {}:{}", ip, connectedPeer->address.port));

	{
		std::lock_guard guard(PeersMutex);
		Peers[connectedPeer] = PeerState{.Identifier = static_cast<int>(Peers.size())};
	}

	for(const auto sprite : SpriteByID)
		SendSprite(sprite);

	for(const auto &info : Bodies | std::views::values)
		GenerateBodyEvent(SpriteIDs[info.sprite], info, ALL_FIELD_MASK);
}

void Network::Server::OnPeerDisconnected(ENetPeer *disconnectedPeer)
{
	{
		std::lock_guard guard(PeersMutex);
		Peers.erase(disconnectedPeer);
	}
	NetworkInterface::OnPeerDisconnected(disconnectedPeer);
}

bool Network::Server::InterceptEvent(ENetPeer *peer, const Event &event)
{
	if(event.Type == EventType::UPDATE_INPUT_STATE)
	{
		std::lock_guard guard(PeersMutex);
		if(const auto it = Peers.find(peer); it != Peers.end())
		{
			if(const auto deserialized = DeserializeCommand(event); deserialized)
				it->second.CurrentInput = *deserialized;
		}

		return true;
	}
	return false;
}

void Network::Server::RunEventLoop()
{
	try
	{
		while(Running.load())
		{
			if(!ProcessEvents())
				break;
		}
	} catch(const std::exception &e)
	{
		Logger::Log(std::format("Server event loop failed: {}", e.what()), Logger::Level::ERROR);
	} catch(...)
	{
		Logger::Log("Server event loop failed unexpectedly.", Logger::Level::ERROR);
	}

	Running.store(false);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Client
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Network::Client::Client() = default;

Network::Client::~Client()
{
	Stop();

	std::lock_guard lock(CommandMutex);
	while(!Commands.empty())
	{
		Commands.front()->result.set_value("Client is shutting down.");
		Commands.pop();
	}
}

std::expected<std::unique_ptr<Network::Client>, std::string_view> Network::Client::CreateClient()
{
	if(!NetworkingAvailable())
		return std::unexpected{"Networking has not been initialized."};

	auto instance = std::unique_ptr<Client>(new Client);

	instance->network_interface.reset(enet_host_create(nullptr, 1, MAX_CHANNELS, 0, 0));

	if(!instance->network_interface)
		return std::unexpected{"Failed to create ENet client host."};

	instance->Running.store(true);

	try
	{
		instance->EventLoop = std::thread([ptr = instance.get()] {
			ptr->RunEventLoop();
		});
	} catch(...)
	{
		instance->Running.store(false);

		return std::unexpected{"Failed to start client event loop."};
	}

	return instance;
}

std::optional<std::string_view> Network::Client::Connect(const std::string_view address)
{
	if(!Running.load())
		return "Client is not running.";

	if(address.empty())
		return "Address cannot be empty.";

	if(address.size() > 253)
		return "Address is too long.";

	auto command     = std::make_unique<ConnectCommand>();
	command->address = std::string(address);

	auto future = command->result.get_future();
	{
		std::lock_guard lock(CommandMutex);

		if(!Running.load())
			return "Client is not running.";

		static constexpr std::size_t MAX_PENDING_CONNECTS = 16;

		if(Commands.size() >= MAX_PENDING_CONNECTS)
			return "Too many pending connection requests.";

		Commands.push(std::move(command));
	}

	return future.get();
}

void Network::Client::AddGhosts(DrawList &draws, const int engine_step)
{
	CurrentStep = engine_step;
	std::lock_guard guard(BodyMutex);

	for(const auto &body : Bodies)
	{
		if(!body.Active)
			continue;

		draws.PushRaw(body.sprite,
		              body.frame,
		              body.xpos + static_cast<asl::float32>(CurrentStep - body.BaseStep) * body.xvel,
		              body.ypos + static_cast<asl::float32>(CurrentStep - body.BaseStep) * body.yvel,
		              Angle::FromRaw(body.angle),
		              body.alpha * 0.1f,
		              Point(body.xscale, body.yscale),
		              &Swizzles[body.Index]);
	}
}

void Network::Client::OnPeerConnected(ENetPeer *connectedPeer)
{
	if(peer != connectedPeer)
		peer = connectedPeer;

	NetworkInterface::OnPeerConnected(connectedPeer);
}

void Network::Client::OnPeerDisconnected(ENetPeer *disconnectedPeer)
{
	if(peer == disconnectedPeer)
		peer = nullptr;

	LogMessage("Server disconnected.");
}

bool Network::Client::InterceptEvent(ENetPeer *, const Event &event)
{
	if(event.Type == EventType::PUSH_SPRITE)
	{
		std::string name;
		std::size_t offset = 0;
		char        c;
		while(ReadSerialized(event.Data, offset, c))
			name += c;

		{
			std::lock_guard guard(BodyMutex);
			SpriteByID.emplace_back(SpriteSet::Get(name));
		}

		LogMessage(std::format("Received new sprite: {}, total: {}", name, SpriteByID.size() - 1));

		return true;
	}
	if(event.Type == EventType::SET_DRAWABLE)
	{
		std::size_t offset = 0;

		std::size_t count = 0;

		if(!ReadSerialized(event.Data, offset, count))
		{
			LogMessage("Malformed drawable update header.");
			return true;
		}

		for(std::size_t i = 0; i < count; i++)
		{
			asl::uint8  mask  = 0;
			asl::uint64 index = 0;

			if(!ReadSerialized(event.Data, offset, mask) || !ReadSerialized(event.Data, offset, index))
			{
				LogMessage("Malformed drawable update header.");
				return true;
			}

			if(mask == 0)
			{
				LogMessage(std::format("Invalid drawable update mask, {}", static_cast<int>(mask)));
				return true;
			}

			// Decode the fields before acquiring the mutex.
			BodyInfo info{.Active = true};

			asl::uint64 spriteID      = 0;
			asl::uint16 swizzlePacked = 0;

			bool success = true;
			OperateOnBodyEntries([&]<auto entry>(const asl::uint8 flag_group) {
				if(mask & (1 << flag_group))
				{
					if constexpr(entry == ^^BodyInfo::sprite)
					{
						if(!ReadSerialized(event.Data, offset, spriteID) || spriteID >= SpriteByID.size())
							success = false;
						else
							info.sprite = SpriteByID[spriteID];
					}
					else if constexpr(entry == ^^BodyInfo::swizzle)
					{
						if(!ReadSerialized(event.Data, offset, swizzlePacked))
							success = false;
						else
							for(int j = 0; j < 16; ++j)
								info.swizzle[j] = (swizzlePacked & (asl::uint16{1} << j)) != 0;
					}
					else
					{
						if(!ReadSerialized(event.Data, offset, info.[:entry:]))
							success = false;
					}
				}
			});

			if(!success)
				return true;


			{
				std::lock_guard guard(BodyMutex);

				if(index > MAX_DRAWABLES)
				{
					LogMessage("Drawable index is out of range.");
					continue;
				}

				if(index >= Bodies.size())
				{
					Bodies.resize(index, {.Active = false});
					Swizzles.resize(index);
				}

				if(index == Bodies.size())
				{
					Bodies.emplace_back(BodyInfo{.Active = false});
					Swizzles.emplace_back();
				}

				auto &body = Bodies[index];

				if(!body.Active)
				{
					if((mask & ALL_FIELD_MASK) != ALL_FIELD_MASK)
					{
						LogMessage("Incomplete initial drawable state.");
						continue;
					}

					if(spriteID >= SpriteByID.size())
					{
						LogMessage("Drawable references an unknown sprite.");
						continue;
					}

					body = info;

					body.BaseStep = CurrentStep;

					Swizzles[index] = Swizzle(body.swizzle);

					body.Active = true;
					continue;
				}


				OperateOnBodyEntries([&]<auto entry>(const asl::uint8 flag_group) {
					if(mask & (1 << flag_group))
					{
						body.[:entry:] = info.[:entry:];
						if constexpr(entry == ^^BodyInfo::swizzle)
							Swizzles[index] = Swizzle(body.swizzle);
						if constexpr(entry == ^^BodyInfo::xpos || entry == ^^BodyInfo::ypos)
							body.BaseStep = CurrentStep;
					}
				});
			}
		}

		return true;
	}
	if(event.Type == EventType::FREE_DRAWABLES)
	{
		std::size_t offset = 0;
		std::size_t count;

		if(!ReadSerialized(event.Data, offset, count))
			return true;

		for(std::size_t i = 0; i < count; i++)
		{
			std::size_t indexToFree = 0;
			if(!ReadSerialized(event.Data, offset, indexToFree))
				return true;
			if(indexToFree >= Bodies.size())
				continue;
			Bodies[indexToFree].Active = false;
		}

		return true;
	}

	return false;
}

void Network::Client::RunEventLoop()
{
	std::queue<std::unique_ptr<ConnectCommand>> pending;

	constexpr auto FailQueue = [](auto &commands, const std::string_view reason) {
		while(!commands.empty())
		{
			commands.front()->result.set_value(reason);
			commands.pop();
		}
	};

	try
	{
		while(Running.load())
		{
			{
				std::lock_guard lock(CommandMutex);
				std::swap(pending, Commands);
			}

			while(!pending.empty())
			{
				const auto command = std::move(pending.front());
				pending.pop();

				if(peer != nullptr)
				{
					command->result.set_value("Client is already connecting or connected.");
					continue;
				}

				ENetAddress target{};
				target.port = DEFAULT_PORT;

				if(enet_address_set_host(&target, command->address.c_str()) < 0)
				{
					command->result.set_value("Failed to resolve server address.");
					continue;
				}

				peer = enet_host_connect(network_interface.get(), &target, MAX_CHANNELS, 0);
				if(peer == nullptr)
				{
					command->result.set_value("Failed to initiate connection.");
					continue;
				}

				command->result.set_value(std::nullopt);
			}

			if(!ProcessEvents())
				break;
		}
	} catch(const std::exception &e)
	{
		Logger::Log(std::format("Client event loop failed: {}", e.what()), Logger::Level::ERROR);
	} catch(...)
	{
		Logger::Log("Client event loop failed unexpectedly.", Logger::Level::ERROR);
	}

	Running.store(false);

	FailQueue(pending, "Client event loop stopped.");
	{
		std::lock_guard lock(CommandMutex);
		FailQueue(Commands, "Client event loop stopped.");
	}
}
