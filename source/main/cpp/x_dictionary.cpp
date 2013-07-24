#include "xbase\x_target.h"
#include "xbase\x_map.h"
#include "xbase\x_set.h"

#include "xp2p\x_p2p.h"
#include "xp2p\private\x_dictionary.h"

namespace xcore
{
	namespace xp2p
	{
		// 
		struct PeerAddress
		{
			char		mAddress[256];
		};

		struct PeerID2NetAddress : xmapnode
		{
			PeerID		mPeerID;
			NetAddress	mAddress;
		};
		typedef		xmap_derive_strategy<PeerID, PeerID2NetAddress*>	PeerID2NetAddressMapStrategy;
		typedef		xmap<PeerID, PeerID2NetAddress*, xstd::xcompare<PeerID>, PeerID2NetAddressMapStrategy >	PeerID2NetAddressMap;

		struct NetAddress2PeerID : xmapnode
		{
			NetAddress	mAddress;
			PeerID		mPeerID;
		};
		typedef		xmap_derive_strategy<NetAddress, NetAddress2PeerID*>	NetAddress2PeerIDMapStrategy;
		typedef		xmap<NetAddress, NetAddress2PeerID*, xstd::xcompare<NetAddress>, NetAddress2PeerIDMapStrategy >	NetAddress2PeerIDMap;

		// "Peer ID - Address" Dictionary
		class PeerDictionary : public Dictionary
		{
		public:
									PeerDictionary(MemoryAllocator* allocator);
			virtual					~PeerDictionary() {}

			virtual NetAddress		RegisterAddress(const char* address_str);
			virtual void			RegisterPeerID(PeerID, NetAddress);

			virtual NetAddress		FindAddressByPeerID(PeerID) const;
			virtual PeerID			FindPeerIDByAddress(NetAddress) const;

			virtual void			UnregisterPeer(PeerID);	

		private:
			MemoryAllocator*				mAllocator;

			PeerID2NetAddressMapStrategy	mPeerID2NetAddressMapStrategy;
			PeerID2NetAddressMap			mPeerID2NetAddressMap;
			NetAddress2PeerIDMapStrategy	mNetAddress2PeerIDMapStrategy;
			NetAddress2PeerIDMap			mNetAddress2PeerIDMap;
		};

		PeerDictionary::PeerDictionary(MemoryAllocator* allocator)
			: mPeerID2NetAddressMapStrategy(X_OFFSET_OF(PeerID2NetAddress, mPeerID), X_OFFSET_OF(PeerID2NetAddress, mAddress), X_OFFSET_OF(PeerID2NetAddress, mNode))
			, mPeerID2NetAddressMap(mPeerID2NetAddressMapStrategy)
			, mNetAddress2PeerIDMapStrategy(X_OFFSET_OF(PeerID2NetAddress, mAddress), X_OFFSET_OF(PeerID2NetAddress, mPeerID), X_OFFSET_OF(PeerID2NetAddress, mNode))
			, mNetAddress2PeerIDMap(mNetAddress2PeerIDMapStrategy)
		{


		}

		NetAddress		PeerDictionary::RegisterAddress(const char* address_str)
		{

			return 0;
		}

		void			PeerDictionary::RegisterPeerID(PeerID, NetAddress)
		{

		}

		NetAddress		PeerDictionary::FindAddressByPeerID(PeerID) const
		{

			return 0;
		}

		PeerID			PeerDictionary::FindPeerIDByAddress(NetAddress) const
		{

			return 0;
		}

		void			PeerDictionary::UnregisterPeer(PeerID)
		{

		}



		extern Dictionary*			gCreateDictionary(MemoryAllocator* allocator);


	}
}