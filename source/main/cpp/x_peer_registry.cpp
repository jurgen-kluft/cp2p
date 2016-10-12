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
		// "Peer" Dictionary
		class peer_registry : public peer_registry
		{
		public:
									peer_registry(allocator* allocator);
			virtual					~peer_registry() {}

			virtual void			register_peer(peer* peer);
			virtual bool			unregister_peer(peer* peer);
			virtual peer*			find_peer_by_ip(netip*) const;

			virtual void			release();

			XCORE_CLASS_PLACEMENT_NEW_DELETE

		private:
			allocator*				allocator_;
			s32						num_peers_;
			peer*					peers_[1024];
		};

		peer_registry::peer_registry(allocator* allocator)
			: allocator_(allocator)
			, num_peers_(0)
		{
		
		}

		void			peer_registry::register_peer(peer* peer)
		{

			// replace netip4 ?
			
		}

		bool			peer_registry::unregister_peer(peer* peer)
		{
			for (s32 i = 0; i < num_peers_; ++i)
			{
				if (peers_[i] == peer)
				{
					peers_[i] = peers_[num_peers_ - 1];
					--num_peers_;
					return true;
				}
			}
			return false;
		}

		peer*			peer_registry::find_peer_by_ip(netip* ip) const
		{
			for (s32 i = 0; i < num_peers_; ++i)
			{
				if (peers_[i]->get_ip() == ip)
					return peers_[i];
			}
			return NULL;
		}

		void			peer_registry::release()
		{
			allocator* _allocator = allocator_;
			this->~peer_registry();
			_allocator->deallocate(this);
		}


		peer_registry*		gCreatePeerRegistry(allocator* _allocator)
		{
			void* mem = _allocator->allocate(sizeof(peer_registry), sizeof(void*));
			peer_registry* reg = new (mem) peer_registry(_allocator);
			return reg;
		}


	}
}