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
#include "MainPanel.h"
#include "base/ASLTypes.h"

#include <enet/enet.h>
#include <expected>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <set>
#include <string_view>
#include <thread>
#include <unordered_map>



class Sprite;
namespace Network {
	void Init();
	bool NetworkingAvailable();

	enum class EventType : std::uint8_t {
		TEXT               = 0x00,
		UPDATE_INPUT_STATE = 0x01,
		PUSH_SPRITE        = 0x02,
		SET_DRAWABLE       = 0x03,
		FREE_DRAWABLES     = 0x04,
		INVALID            = 0xff
	};

	struct Event {
		EventType                  Type = EventType::INVALID;
		std::vector<unsigned char> Data;
	};

	struct OutgoingEvent {
		Event event;
		bool  reliable = true;
	};

	Event                  SerializeCommand(const Command &command);
	std::optional<Command> DeserializeCommand(const Event &event);

	class NetworkInterface {
	protected:
		NetworkInterface() = default;

	public:
		virtual ~NetworkInterface();

		NetworkInterface(const NetworkInterface &)            = delete;
		NetworkInterface &operator=(const NetworkInterface &) = delete;

		bool                 SendEvent(const Event &event, bool reliable = true);
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

		ENetAddress                                address{};
		std::unique_ptr<ENetHost, ENetHostDeleter> network_interface = nullptr;

		std::thread       EventLoop;
		std::atomic<bool> Running{false};

		std::mutex                EventMutex;
		std::queue<OutgoingEvent> OutgoingEvents;
		std::queue<Event>         IncomingEvents;
	};


	struct BodyInfoFlagGroup {
		const asl::uint8 Group;
		explicit consteval BodyInfoFlagGroup(const asl::uint8 group) : Group(group)
		{
			if(group > 7)
				throw std::invalid_argument("Flag group must be less than 8");
		}
	};
	struct BodyInfo {
		bool                 Active = false;
		std::size_t          Index  = 0;
		[[=BodyInfoFlagGroup(0)]] const Sprite        *sprite = nullptr;
		[[=BodyInfoFlagGroup(1)]] asl::float32         frame  = 0;
		[[=BodyInfoFlagGroup(2)]] asl::float32         xpos = 0, ypos = 0;
		[[=BodyInfoFlagGroup(2)]] asl::float32         xvel = 0, yvel = 0;
		[[=BodyInfoFlagGroup(3)]] asl::int32           angle = 0;
		[[=BodyInfoFlagGroup(4)]] asl::float32         alpha = 0;
		[[=BodyInfoFlagGroup(5)]] std::array<bool, 16> swizzle{}; // We assume a simple swizzle matrix for sake of bandwidth
		[[=BodyInfoFlagGroup(6)]] asl::float32         xscale = 0;
		[[=BodyInfoFlagGroup(6)]] asl::float32         yscale = 0;
	};

	class Server final : public NetworkInterface {
		Server() = default;

	public:
		struct PeerState {
			int     Identifier = -1;
			Command CurrentInput;
		};

		~Server() override;

		static std::expected<std::unique_ptr<Server>, std::string_view> CreateServer();

		std::vector<PeerState> GetPeerStates() const;
		void                   SendSprite(const Sprite *sprite);

		static asl::uint16 PackSwizzle(const BodyInfo &info);
		static Event       GenerateBodyEvent(asl::uint64 spriteID, const BodyInfo &info, asl::uint8 mask);

		asl::uint64 PushSprite(const Sprite *sprite);
		void        MarkUnused();
		void        PushBody(const Body &body, Point position, double cloak, int step);
		void        FreeUnused();

	protected:
		void OnPeerConnected(ENetPeer *connectedPeer) override;
		void OnPeerDisconnected(ENetPeer *disconnectedPeer) override;

		bool InterceptEvent(ENetPeer *peer, const Event &event) override;

	private:
		void RunEventLoop();

		mutable std::mutex                        PeersMutex;
		std::unordered_map<ENetPeer *, PeerState> Peers;

		std::unordered_map<const Sprite *, asl::uint64> SpriteIDs;
		std::vector<const Sprite *>                     SpriteByID;

		std::unordered_map<const Body *, BodyInfo> Bodies;
		std::set<std::size_t>                      FreeIndices;
		std::size_t                                NextIndex = 0;
	};

	class Client final : public NetworkInterface {
		Client();

	public:
		~Client() override;

		static std::expected<std::unique_ptr<Client>, std::string_view> CreateClient();
		std::optional<std::string_view>                                 Connect(std::string_view address);

		void AddGhosts(DrawList &draws);

	protected:
		struct ConnectCommand {
			std::string                                   address;
			std::promise<std::optional<std::string_view>> result;
		};

		void RunEventLoop();

		void OnPeerConnected(ENetPeer *connectedPeer) override;
		void OnPeerDisconnected(ENetPeer *disconnectedPeer) override;

		bool InterceptEvent(ENetPeer *peer, const Event &event) override;

	private:
		// Accessed only by the event-loop thread.
		ENetPeer *peer = nullptr;

		std::mutex                                  CommandMutex;
		std::queue<std::unique_ptr<ConnectCommand>> Commands;

		std::mutex                  BodyMutex;
		std::vector<const Sprite *> SpriteByID;
		std::vector<BodyInfo>       Bodies;
		std::vector<Swizzle>        Swizzles;
	};
} // namespace Network



#endif //ENDLESS_SKY_NETWORK_H
