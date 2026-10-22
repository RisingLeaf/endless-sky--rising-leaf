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

#include "../Command.h"
#include "Body.h"
#include "base/ASLTypes.h"

#include <enet/enet.h>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string_view>
#include <thread>
#include <unordered_map>



class Sprite;
namespace Network {
	void Init();
	bool NetworkingAvailable();

	enum class EventType : std::uint8_t {
		TEXT = 0x00,
		UPDATE_INPUT_STATE = 0x01,
		PUSH_SPRITE = 0x02,
		SET_DRAWABLE = 0x03,
		FREE_DRAWABLE = 0x04,
		INVALID = 0xff
	};

	struct Event {
		EventType Type = EventType::INVALID;
		std::vector<unsigned char> Data;
	};

	struct OutgoingEvent {
		Event event;
		bool reliable = true;
	};

	Event SerializeCommand(const Command &command);
	std::optional<Command> DeserializeCommand(const Event &event);

	class NetworkInterface {
	protected:
		NetworkInterface() = default;

	public:
		virtual ~NetworkInterface();

		NetworkInterface(const NetworkInterface &) = delete;
		NetworkInterface &operator=(const NetworkInterface &) = delete;

		bool SendEvent(const Event &event, bool reliable = true);
		std::optional<Event> PollEvent();

	protected:
		void Stop();
		bool ProcessEvents();

		virtual void OnPeerConnected(ENetPeer *peer);
		virtual void OnPeerDisconnected(ENetPeer *peer);

		virtual bool InterceptEvent(ENetPeer *peer, const Event &)
		{
			return false;
		}

		struct ENetHostDeleter {
			void operator()(ENetHost *host) const noexcept
			{
				if(host)
					enet_host_destroy(host);
			}
		};

		ENetAddress address{};
		std::unique_ptr<ENetHost, ENetHostDeleter> network_interface = nullptr;

		std::thread EventLoop;
		std::atomic<bool> Running{false};

		std::mutex EventMutex;
		std::queue<OutgoingEvent> OutgoingEvents;
		std::queue<Event> IncomingEvents;
	};

	class Server final : public NetworkInterface {
		Server() = default;

	public:
		struct PeerState {
			int Identifier = -1;
			Command CurrentInput;
		};

		~Server() override;

		static std::expected<std::unique_ptr<Server>, std::string_view> CreateServer();

		std::vector<PeerState> GetPeerStates() const;
		void SendSprite(const Sprite *sprite);

		asl::uint64 PushSprite(const Sprite *sprite);
		void PushBody(const Body &body, Point position, double cloak);

	protected:
		void OnPeerConnected(ENetPeer *connectedPeer) override;
		void OnPeerDisconnected(ENetPeer *disconnectedPeer) override;

		bool InterceptEvent(ENetPeer *peer, const Event &event) override;

	private:
		void RunEventLoop();

		mutable std::mutex PeersMutex;
		std::unordered_map<ENetPeer *, PeerState> Peers;

		std::unordered_map<const Sprite *, asl::uint64> SpriteIDs;
		std::vector<const Sprite *> SpriteByID;
	};

	class Client final : public NetworkInterface {
		Client();

	public:
		~Client() override;

		static std::expected<std::unique_ptr<Client>, std::string_view> CreateClient();
		std::optional<std::string_view> Connect(std::string_view address);

	protected:
		struct ConnectCommand {
			std::string address;
			std::promise<std::optional<std::string_view>> result;
		};

		void RunEventLoop();

		void OnPeerConnected(ENetPeer *connectedPeer) override;
		void OnPeerDisconnected(ENetPeer *disconnectedPeer) override;

		bool InterceptEvent(ENetPeer *peer, const Event &event) override;

	private:
		// Accessed only by the event-loop thread.
		ENetPeer *peer = nullptr;

		std::mutex CommandMutex;
		std::queue<std::unique_ptr<ConnectCommand>> Commands;

		std::vector<const Sprite *> SpriteByID;
	};
} // namespace Network



#endif //ENDLESS_SKY_NETWORK_H
