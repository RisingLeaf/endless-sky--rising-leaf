/* Network.h
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

#ifndef ENDLESS_SKY_NETWORK_H
#define ENDLESS_SKY_NETWORK_H

#include <enet/enet.h>
#include <expected>
#include <string_view>
#include <utility>


namespace Network {
	void Init();
	bool NetworkingAvailable();

	class Server {
	public:
		~Server();

		static std::expected<Server, std::string_view> CreateServer();

		Server(const Server &other) = delete;
		Server(Server &&other) noexcept : address(other.address), server(std::exchange(other.server, nullptr))
		{
		}
		Server &operator=(const Server &other) = delete;
		Server &operator=(Server &&other) noexcept
		{
			if(this == &other)
				return *this;
			if(server)
				enet_host_destroy(server);
			address = other.address;
			server = other.server;
			other.server = nullptr;
			return *this;
		}

	private:
		Server();

		ENetAddress address{};
		ENetHost *server = nullptr;
	};
} // namespace Network



#endif //ENDLESS_SKY_NETWORK_H
