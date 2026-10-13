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

#include "Logger.h"
#include "system/Log.h"


namespace {
	bool NETWORKING_AVAILABLE = false;

	constexpr int MAX_CLIENTS = 4;
	constexpr int MAX_CHANNELS = 2;

	constexpr int DEFAULT_PORT = 55542;
} // namespace

void Network::Init()
{
	if(NETWORKING_AVAILABLE)
		return;

	if(enet_initialize() != 0)
	{
		Logger::Log("An error occurred while initializing ENet.", Logger::Level::ERROR);
		return;
	}
	atexit(enet_deinitialize);
	NETWORKING_AVAILABLE = true;
}

bool Network::NetworkingAvailable()
{
	return NETWORKING_AVAILABLE;
}

std::expected<Network::Server, std::string_view> Network::Server::CreateServer()
{
	if(!NETWORKING_AVAILABLE)
		return std::unexpected{"Unexpected request to create a server when networking was marked as unavailable"};

	Server server;

	server.address.host = ENET_HOST_ANY;
	server.address.port = DEFAULT_PORT;

	server.server = enet_host_create(&server.address, MAX_CLIENTS, MAX_CHANNELS, 0, 0);
	if(!server.server)
	{
		Logger::Log("An error occurred while trying to create an ENet server host.", Logger::Level::ERROR);
		return std::unexpected{"Failed to create ENet server host"};
	}

	return server;
}

Network::Server::Server() = default;

Network::Server::~Server()
{
	if(server)
		enet_host_destroy(server);
}
