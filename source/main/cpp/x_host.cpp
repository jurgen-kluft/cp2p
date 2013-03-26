#include "xbase\x_target.h"
#include "xbase\x_allocator.h"
#include "xp2p\x_p2p.h"

namespace xcore
{
	namespace xp2p
	{
		static x_iallocator*	gAllocator = NULL;

		class Host : public Peer
		{
		public:
			inline				Host(x_iallocator* a) : mAllocator(a) {}
			virtual				~Host() {}

			virtual void		AddChannel(IChannel* channel);
			
			virtual void		Start(PeerID tracker);
			virtual void		Stop();
			virtual void		WaitUntilExit();

			virtual PeerID		GetId() const;

			virtual void		ConnectTo(PeerID peerId);
			virtual u32			NumConnections() const;
			virtual void		GetConnections(PeerID* outPeerList, u32 sizePeerList, u32& outPeerCnt);
			virtual void		DisconnectFrom(PeerID peerId);

			XCORE_CLASS_PLACEMENT_NEW_DELETE
		private:
			x_iallocator*		mAllocator;

			u32					mNumChannels;
			IChannel*			mChannels[4];

			PeerID				mPeerID;
			PeerID				mTrackerID;
			
			s32					mNumConnections;
			PeerID				mConnections[64];
		};


		void		Host::AddChannel(IChannel* channel)
		{
			mChannels[mNumChannels++] = channel;
			// Register channel at network system
		}

		void		Host::Start(PeerID tracker)
		{
			mTrackerID = tracker;
			// Start our network system
			// Have network system connect to tracker
		}

		void		Host::Stop()
		{
			// Stop our network system
		}

		void		Host::WaitUntilExit()
		{
			// Wait until our network system has terminated
		}

		PeerID		Host::GetId() const
		{
			return mPeerID;
		}

		void		Host::ConnectTo(PeerID peer)
		{
			// Get address details using gFindAddressOfPeer
			// Have network system connect to peer
		}

		u32			Host::NumConnections() const
		{
			return mNumConnections;
		}

		void		Host::GetConnections(PeerID* outPeerList, u32 sizePeerList, u32& outPeerCnt)
		{
			s32 i = mNumConnections - 1;
			while (i>=0)
			{
				outPeerList[i] = mConnections[i];
			}
			outPeerCnt = mNumConnections;
		}

		void		Host::DisconnectFrom(PeerID peer)
		{
			// Have network system disconnect from peer
		}



		
		Peer*				gCreateHost(PeerID id)
		{
			void* host_mem = gAllocator->allocate(sizeof(Host), 4);
			Host* host = new (host_mem) Host(gAllocator);

			return host;
		}

		void				gDestroyHost(Peer* peer)
		{
			Host* host = (Host*)peer;
			host->~Host();
			gAllocator->deallocate(peer);
		}


	}
}