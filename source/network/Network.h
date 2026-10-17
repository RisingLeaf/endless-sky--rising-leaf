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

#include "SDL3/SDL_events.h"

#include <enet/enet.h>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string_view>
#include <thread>
#include <utility>



class Command;
namespace Network {
	void Init();
	bool NetworkingAvailable();

	enum class EventType : std::uint8_t { TEXT = 0x00, COMMAND = 0x01, INVALID = 0xff };

	struct Event {
		EventType Type = EventType::INVALID;
		std::vector<unsigned char> Data;
	};
	struct OutgoingEvent {
		Event event;
		bool reliable = true;
	};

	enum class InputEventType : std::uint8_t {
		KEY_DOWN,
		KEY_UP,
		MAX_ENUM
	};
	Event SerializeCommand(const Command &command);
	std::optional<Command> DeserializeCommand(const Event &event);

	class Server {
		Server();

	public:
		~Server();

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

		static std::expected<std::unique_ptr<Server>, std::string_view> CreateServer();
		void RunEventLoop();

		bool SendEvent(const Event &event, bool reliable = true);
		std::optional<Event> PollEvent();

	private:
		ENetAddress address{};
		ENetHost *server = nullptr;

		std::thread EventLoop;
		std::atomic<bool> Running;

		std::mutex EventMutex;
		std::queue<OutgoingEvent> OutgoingEvents;
		std::queue<Event> IncomingEvents;
	};


	class Client {
		Client();

	public:
		~Client();

		Client(const Client &other) = delete;
		Client(Client &&other) noexcept : client(std::exchange(other.client, nullptr))
		{
		}
		Client &operator=(const Client &other) = delete;
		Client &operator=(Client &&other) noexcept
		{
			if(this == &other)
				return *this;
			if(client)
				enet_host_destroy(client);
			client = other.client;
			other.client = nullptr;
			return *this;
		}

		static std::expected<std::unique_ptr<Client>, std::string_view> CreateClient();
		std::optional<std::string_view> Connect(std::string_view address);

		bool SendEvent(const Event &event, bool reliable = true);
		std::optional<Event> PollEvent();

	private:
		struct ConnectCommand {
			std::string address;
			std::promise<std::optional<std::string_view>> result;
		};

		void RunEventLoop();

		ENetHost *client = nullptr;

		// Accessed only by the event-loop thread.
		ENetPeer *peer = nullptr;

		std::atomic<bool> Running{false};
		std::thread EventLoop;

		std::mutex CommandMutex;
		std::queue<std::unique_ptr<ConnectCommand>> Commands;

		std::mutex EventMutex;
		std::queue<OutgoingEvent> OutgoingEvents;
		std::queue<Event> IncomingEvents;
	};
} // namespace Network



#endif //ENDLESS_SKY_NETWORK_H
