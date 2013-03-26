#include "xbase\x_target.h"
#include "xbase\x_allocator.h"

namespace xcore
{
	namespace xp2p
	{
		// Network:
		//
		// Multi-Threaded
		// Event based
		// Manages 'connections' by ID
		// Uses IOCP on Windows
		// 
		// 
		// 
		// 

		class NetServer
		{
		public:
			void			Start(u16 port);
			void			Stop();

		protected:
			static void		sAcceptThreadFunc(void* obj);
			void			AcceptThreadFunc();

			HANDLE			mThreadHandle;
			TcpSocket		mServer;
			bool			mQuit;
		};

		class NetClient
		{
		public:

		protected:
			u32				mID;
			bool			mOutgoing;
			bool			mIncoming;
			TcpSocket		mSocket;

			enum EState
			{
				CONNECTING,
				HANDSHAKE,
				CONNECTED,
				DISCONNECTING,
				DISCONNECTED,
			};
			EState			mState;
		};


		class NetHost
		{
		public:

		protected:
			NetServer		mServer;

			static void		sIOThreadFunc(void* obj);
			void			IOThreadFunc();

			u32				mNumActiveClients;
			NetClient*		mActiveClients[62];
			u32				mNumFreeClients;
			NetClient*		mFreeClients[62];
		};
	}
}