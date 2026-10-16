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

#include "GameData.h"
#include "Logger.h"
#include "Messages.h"


namespace {
	std::atomic NETWORKING_AVAILABLE = false;

	constexpr int MAX_CLIENTS = 4;
	constexpr int MAX_CHANNELS = 2;

	constexpr enet_uint32 EVENT_TIMEOUT_MS = 50;

	constexpr int DEFAULT_PORT = 55542;

	void LogMessage(const std::string_view text)
	{
		Messages::Add(Message(text.data(), GameData::MessageCategories().Get("high")));

		Logger::Log(text, Logger::Level::INFO);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// Server
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Network::Server::Server() = default;


Network::Server::~Server()
{
	Running.store(false);
	if(EventLoop.joinable())
		EventLoop.join();
	if(server)
		enet_host_destroy(server);
}


std::expected<std::unique_ptr<Network::Server>, std::string_view> Network::Server::CreateServer()
{
	if(!NETWORKING_AVAILABLE.load())
		return std::unexpected{"Networking has not been initialized."};

	auto instance = std::unique_ptr<Server>(new Server);

	instance->address.host = ENET_HOST_ANY;
	instance->address.port = DEFAULT_PORT;

	instance->server = enet_host_create(&instance->address, MAX_CLIENTS, MAX_CHANNELS, 0, 0);

	if(!instance->server)
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
		enet_host_destroy(instance->server);
		instance->server = nullptr;
		throw;
	}

	return instance;
}


void Network::Server::RunEventLoop()
{
	while(Running.load())
	{
		ENetEvent event{};

		const int result = enet_host_service(server, &event, EVENT_TIMEOUT_MS);
		if(result < 0)
		{
			Logger::Log("ENet server service failed.", Logger::Level::ERROR);
			break;
		}
		if(result == 0)
			continue;

		switch(event.type)
		{
		case ENET_EVENT_TYPE_CONNECT: {
			char ip[64]{};
			enet_address_get_host_ip(&event.peer->address, ip, sizeof(ip));
			const std::string message = std::format("Client connected from {}:{}", ip, event.peer->address.port);

			LogMessage(message);
			break;
		}
		case ENET_EVENT_TYPE_RECEIVE: {
			const std::string message(reinterpret_cast<const char *>(event.packet->data), event.packet->dataLength);

			Logger::Log(std::format("Received {} bytes from a client.", event.packet->dataLength), Logger::Level::INFO);
			enet_packet_destroy(event.packet);
			break;
		}
		case ENET_EVENT_TYPE_DISCONNECT:
			LogMessage("A client disconnected.");
			event.peer->data = nullptr;
			break;
		case ENET_EVENT_TYPE_NONE:
			break;
		}
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
	Running.store(false);

	{
		std::lock_guard lock(CommandMutex);

		while(!Commands.empty())
		{
			Commands.front()->result.set_value("Client is shutting down.");
			Commands.pop();
		}
	}

	if(EventLoop.joinable())
		EventLoop.join();

	if(client)
	{
		enet_host_destroy(client);
		client = nullptr;
	}
}

std::expected<std::unique_ptr<Network::Client>, std::string_view> Network::Client::CreateClient()
{
	if(!NETWORKING_AVAILABLE.load())
		return std::unexpected{"Networking has not been initialized."};

	auto instance = std::unique_ptr<Client>(new Client);

	instance->client = enet_host_create(nullptr, 1, MAX_CHANNELS, 0, 0);

	if(!instance->client)
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
		enet_host_destroy(instance->client);
		instance->client = nullptr;
		throw;
	}

	return instance;
}

std::optional<std::string_view> Network::Client::Connect(const std::string_view address)
{
	if(!Running.load())
		return "Client is not running.";

	if(address.empty())
		return "Address cannot be empty.";

	auto command = std::make_unique<ConnectCommand>();
	command->address = std::string(address);

	auto future = command->result.get_future();
	{
		std::lock_guard lock(CommandMutex);

		if(!Running.load())
			return "Client is not running.";

		Commands.push(std::move(command));
	}

	return future.get();
}

void Network::Client::RunEventLoop()
{
	while(Running.load())
	{
		std::queue<std::unique_ptr<ConnectCommand>> pending;
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

			ENetAddress address{};
			address.port = DEFAULT_PORT;

			if(enet_address_set_host(&address, command->address.c_str()) < 0)
			{
				command->result.set_value("Malformed address to connect to.");
				continue;
			}

			peer = enet_host_connect(client, &address, MAX_CHANNELS, 0);
			if(!peer)
			{
				command->result.set_value("No available peers for initiating connection.");
				continue;
			}

			command->result.set_value(std::nullopt);
		}

		ENetEvent event{};
		const int result = enet_host_service(client, &event, EVENT_TIMEOUT_MS);
		if(result < 0)
		{
			Logger::Log("ENet client service failed.", Logger::Level::ERROR);
			break;
		}

		if(result == 0)
			continue;

		switch(event.type)
		{
		case ENET_EVENT_TYPE_CONNECT:
			Logger::Log("Connected to the server.", Logger::Level::INFO);
			break;
		case ENET_EVENT_TYPE_RECEIVE: {
			const std::string message(reinterpret_cast<const char *>(event.packet->data), event.packet->dataLength);

			Logger::Log(std::format("Received {} bytes from the server.", event.packet->dataLength),
			            Logger::Level::INFO);

			// TODO

			enet_packet_destroy(event.packet);
			break;
		}
		case ENET_EVENT_TYPE_DISCONNECT: {
			Logger::Log("Disconnected from the server.", Logger::Level::INFO);
			peer = nullptr;
			break;
		}
		case ENET_EVENT_TYPE_NONE:
			break;
		}
	}

	Running.store(false);

	std::lock_guard lock(CommandMutex);
	while(!Commands.empty())
	{
		Commands.front()->result.set_value("Client event loop stopped.");
		Commands.pop();
	}
}
