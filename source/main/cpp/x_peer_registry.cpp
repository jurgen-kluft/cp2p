#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\private\x_peer_registry.h"
#include "xp2p\private\x_allocator.h"

namespace xcore
{
	namespace xp2p
	{
		// "Peer ID - Address" Dictionary
		class peer_registry : public ipeer_registry
		{
		public:
									peer_registry(iallocator* allocator);
			virtual					~peer_registry() {}

			virtual ipeer*			register_peer(peerid, netip4);
			virtual bool			unregister_peer(peerid);
			virtual ipeer*			find_peer_by_id(peerid) const;

			virtual void			release();

			XCORE_CLASS_PLACEMENT_NEW_DELETE

		private:
			iallocator*				allocator_;
			s32						num_peers_;
			ipeer*					peers_[1024];
		};

		peer_registry::peer_registry(iallocator* allocator)
			: allocator_(allocator)
			, num_peers_(0)
		{
		
		}

		ipeer*			peer_registry::register_peer(peerid _id, netip4 _netip)
		{
			ipeer* peer = find_peer_by_id(_id);
			// replace netip4 ?
			return peer;
		}

		ipeer*			peer_registry::find_peer_by_id(peerid _id) const
		{
			for (s32 i = 0; i < num_peers_; ++i)
			{
				if (peers_[i]->get_id() == _id)
					return peers_[i];
			}
			return NULL;
		}

		bool			peer_registry::unregister_peer(peerid _id)
		{
			for (s32 i = 0; i < num_peers_; ++i)
			{
				if (peers_[i]->get_id() == _id)
				{
					peers_[i] = peers_[num_peers_ - 1];
					--num_peers_;
					return true;
				}
			}
			return false;
		}

		void			peer_registry::release()
		{
			iallocator* allocator = allocator_;
			this->~peer_registry();
			allocator->deallocate(this);
		}


		ipeer_registry*		gCreatePeerRegistry(iallocator* allocator)
		{
			void* mem = allocator->allocate(sizeof(peer_registry), sizeof(void*));
			ipeer_registry* reg = new (mem) peer_registry(allocator);
			return reg;
		}


	}
}