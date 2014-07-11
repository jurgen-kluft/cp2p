#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

#include "xp2p\x_p2p.h"
#include "xp2p\x_peer.h"
#include "xp2p\private\x_dictionary.h"
#include "xp2p\private\x_allocator.h"

namespace xcore
{
	namespace xp2p
	{
		// "Peer ID - Address" Dictionary
		class PeerDictionary : public Dictionary
		{
		public:
									PeerDictionary(iallocator* allocator);
			virtual					~PeerDictionary() {}

			virtual IPeer*			RegisterPeer(PeerID, NetIP4);
			virtual IPeer*			FindPeerByID(PeerID) const;

			virtual bool			UnregisterPeer(PeerID);

			XCORE_CLASS_PLACEMENT_NEW_DELETE

		private:
			iallocator*				mAllocator;
			s32						mNumPeers;
			IPeer*					mPeers[1024];
		};

		PeerDictionary::PeerDictionary(iallocator* allocator)
			: mAllocator(allocator)
			, mNumPeers(0)
		{
		
		}

		IPeer*			PeerDictionary::RegisterPeer(PeerID _id, NetIP4 _netip)
		{
			IPeer* peer = FindPeerByID(_id);
			// Replace NetIP4 ?
			return peer;
		}

		IPeer*			PeerDictionary::FindPeerByID(PeerID _id) const
		{
			for (s32 i = 0; i < mNumPeers; ++i)
			{
				if (mPeers[i]->GetID() == _id)
					return mPeers[i];
			}
			return NULL;
		}

		bool			PeerDictionary::UnregisterPeer(PeerID _id)
		{
			for (s32 i = 0; i < mNumPeers; ++i)
			{
				if (mPeers[i]->GetID() == _id)
				{
					mPeers[i] = mPeers[mNumPeers - 1];
					--mNumPeers;
					return true;
				}
			}
			return false;
		}


		Dictionary*		gCreateDictionary(iallocator* allocator)
		{
			void* mem = allocator->alloc(sizeof(PeerDictionary), sizeof(void*));
			PeerDictionary* dict = new (mem)PeerDictionary(allocator);
			return dict;
		}


	}
}