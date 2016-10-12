#include "xbase/x_target.h"
#include "xbase/x_string_ascii.h"
#include "xp2p/private/x_sockets.h"

#ifdef PLATFORM_PC
#include <winsock2.h>         // For socket(), connect(), send(), and recv()
#include <ws2tcpip.h>
typedef int socklen_t;
typedef char raw_type;       // Type used for raw data on this platform
#else
#include <sys/types.h>       // For data types
#include <sys/socket.h>      // For socket(), connect(), send(), and recv()
#include <netdb.h>           // For gethostbyname()
#include <arpa/inet.h>       // For inet_addr()
#include <unistd.h>          // For close()
#include <netinet/in.h>      // For sockaddr_in
#include <cstring>	       // For memset()
#include <cstdlib>	       // For atoi()
typedef void raw_type;       // Type used for raw data on this platform
#endif

#include <errno.h>             // For errno

namespace xcore
{
	namespace xnet
	{
#ifdef PLATFORM_PC
		static bool sWSAinitialized = false;
#endif

		class endpoint
		{
		public:
#ifdef PLATFORM_PC
			s32				construct(const char* addr)
			{
				struct addrinfo hints;
				struct addrinfo *result = NULL;
				struct addrinfo *ptr = NULL;

				//--------------------------------
				// Setup the hints address info structure which is passed to the getaddrinfo() function
				ZeroMemory(&hints, sizeof(hints));
				hints.ai_flags = AI_NUMERICHOST;
				hints.ai_family = AF_UNSPEC;
				//    hints.ai_socktype = SOCK_STREAM;
				//    hints.ai_protocol = IPPROTO_TCP;

				//--------------------------------
				// Call getaddrinfo(). If the call succeeds, the result variable will hold a linked list
				// of addrinfo structures containing response information
				DWORD dwRetval = getaddrinfo(addr, NULL, &hints, &result);
				if (dwRetval != 0) 
				{
					//printf("getaddrinfo failed with error: %d\n", dwRetval);
					return -1;
				}

				mProtocol = 0;
				ZeroMemory(&mIPv6[0], sizeof(mIPv6));
				ZeroMemory(&mIPv4[0], sizeof(mIPv4));

				// Retrieve each address and print out the hex bytes
				for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
				{
					u8* ipaddr = NULL;
					if (ptr->ai_family == AF_INET6)
					{
						ipaddr = mIPv6;	// IPv6
					}
					else if (ptr->ai_family == AF_INET)
					{
						ipaddr = mIPv4;	// IPv4
					}

					if (ipaddr != NULL)
					{
						if (ptr->ai_socktype == SOCK_STREAM && ptr->ai_protocol == IPPROTO_TCP)
						{
							write((u8 const*)ptr->ai_addr, ptr->ai_addrlen, ipaddr);
							mProtocol = 1;
							break;
						}
						else if (ptr->ai_socktype == SOCK_DGRAM && ptr->ai_protocol == IPPROTO_UDP)
						{
							write((u8 const*)ptr->ai_addr, ptr->ai_addrlen, ipaddr);
							mProtocol = 2;	// UDP
							break;
						}
					}
				}

				freeaddrinfo(result);
			}

			bool			get_ipv4(sockaddr_in*& addr)
			{
				addr = (sockaddr_in*)mIPv4;
				return false;
			}

			bool			get_ipv6(sockaddr_in6*& addr)
			{
				addr = (sockaddr_in6*)mIPv6;
				return false;
			}

#endif


		protected:
			void		write(u8 const* data, u32 len, u8*& buffer)
			{
				for (s32 i = 0; i < len; i++)
				{
					*buffer++ = data[i];
				}
			}

			u32			mProtocol;
			u8			mIPv6[32];
			u8			mIPv4[16];
		};


		// Function to fill in address structure given an address and port
		static s32 fillAddr(const char* address, u16 port, sockaddr_in &addr)
		{
			memset(&addr, 0, sizeof(addr));  // Zero out address structure
			addr.sin_family = AF_INET;       // Internet address

			hostent *host;  // Resolve name
			if ((host = gethostbyname(address)) == NULL)
			{
				// strerror() will not work for gethostbyname() and hstrerror() 
				// is supposedly obsolete
				// "Failed to resolve name (gethostbyname())"
				return -1;
			}
			addr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
			addr.sin_port = htons(port);     // Assign port in network byte order
			return 0;
		}
		
		// socket_base Code

		socket_base::socket_base(s32 socketDescriptor)
			: mSocketDescriptor(socketDescriptor)
		{
		}

		socket_base::~socket_base()
		{
#ifdef PLATFORM_PC
			::closesocket(mSocketDescriptor);
#else
			::close(mSocketDescriptor);
#endif
			mSocketDescriptor = -1;
		}

		bool socket_base::valid() const
		{
			return mSocketDescriptor != INVALID_SOCKET_DESCRIPTOR;
		}

		bool socket_base::open_socket(s32 type, s32 protocol)
		{
			bool result = false;
#ifdef PLATFORM_PC
			if (!sWSAinitialized)
			{
				WORD wVersionRequested;
				WSADATA wsaData;

				wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
				if (WSAStartup(wVersionRequested, &wsaData) != 0)	// Load WinSock DLL
				{	// "Unable to load WinSock DLL");
					result = false;
					sWSAinitialized = false;
				}
				else
				{
					sWSAinitialized = true;
				}
			}

			if (sWSAinitialized)
#endif
			{
				if (mSocketDescriptor == INVALID_SOCKET_DESCRIPTOR)
				{
					// Make a new socket
					if ((mSocketDescriptor = ::socket(PF_INET, type, protocol)) < 0)
					{
						// socket_base creation failed (socket())
						mSocketDescriptor = INVALID_SOCKET_DESCRIPTOR;
					}
				}
				result = mSocketDescriptor != INVALID_SOCKET_DESCRIPTOR;
			}
			return result;
		}

		s32 socket_base::getLocalAddress(char* address, s32 len)
		{
			if (!valid())
				return -1;

			sockaddr_in addr;
			unsigned int addr_len = sizeof(addr);

			if (getsockname(mSocketDescriptor, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// Fetch of local address failed (getsockname())
				return -1;
			}
			const char* addrStr = inet_ntoa(addr.sin_addr);
			s32 addrStrLen = StrLen(addrStr);
			if (address != NULL)
				strcpy_s(address, len, addrStr);
			return addrStrLen;
		}

		u16 socket_base::getLocalPort()
		{
			sockaddr_in addr;
			unsigned int addr_len = sizeof(addr);
			if (!valid() || getsockname(mSocketDescriptor, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// Fetch of local port failed (getsockname())
				return 0;
			}
			return ntohs(addr.sin_port);
		}

		void socket_base::setLocalPort(u16 localPort)
		{
			if (valid())
			{
				// Bind the socket to its port
				sockaddr_in localAddr;
				memset(&localAddr, 0, sizeof(localAddr));
				localAddr.sin_family = AF_INET;
				localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
				localAddr.sin_port = htons(localPort);

				if (bind(mSocketDescriptor, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0)
				{
					// "Set of local port failed (bind())", true);
				}
			}
		}

		void socket_base::setLocalAddressAndPort(const char* localAddress, u16 localPort)
		{
			if (valid())
			{
				// Get the address of the requested host
				sockaddr_in localAddr;
				fillAddr(localAddress, localPort, localAddr);

				if (bind(mSocketDescriptor, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0)
				{
					// "Set of local address and port failed (bind())", true);
				}
			}
		}

		void socket_base::cleanUp()
		{
#ifdef PLATFORM_PC
			if (WSACleanup() != 0)
			{
				// "WSACleanup() failed");
			}
#endif
		}

		u16 socket_base::resolveService(const char* service, const char* protocol)
		{
			struct servent *serv;        /* Structure containing service information */

			if ((serv = getservbyname(service, protocol)) == NULL)
				return atoi(service);			/* Service is port number */
			else
				return ntohs(serv->s_port);		/* Found port (network byte order) by name */
		}

		// socket Code

		socket::socket(s32 descriptor) : socket_base(descriptor)
		{

		}

		void socket::connect(const char* foreignAddress, u16 foreignPort)
		{
			// Get the address of the requested host
			sockaddr_in destAddr;
			fillAddr(foreignAddress, foreignPort, destAddr);

			// Try to connect to the given port
			if (::connect(mSocketDescriptor, (sockaddr *)&destAddr, sizeof(destAddr)) < 0)
			{
				// "Connect failed (connect())", true);
			}
		}

		s32 socket::send(const void *buffer, s32 bufferLen)
		{
			s32 const res = ::send(mSocketDescriptor, (raw_type *)buffer, bufferLen, 0);
			if (res < 0)
			{
				// Send failed (send())
				return -1;
			}
			return res;
		}

		s32 socket::recv(void *buffer, s32 bufferLen)
		{
			s32 const rtn = ::recv(mSocketDescriptor, (raw_type *)buffer, bufferLen, 0);
			if (rtn < 0)
			{
				// Received failed (recv())
				return -1;
			}
			return rtn;
		}

		s32 socket::getForeignAddress(char* address, s32 len)
		{
			sockaddr_in addr;
			u32 addr_len = sizeof(addr);
			if (getpeername(mSocketDescriptor, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// Fetch of foreign address failed getpeername()
				return -1;
			}
			const char* addrStr = inet_ntoa(addr.sin_addr);
			s32 addrStrLen = StrLen(addrStr);
			if (address != NULL)
				strcpy_s(address, len, addrStr);

			return addrStrLen;
		}

		u16 socket::getForeignPort()
		{
			sockaddr_in addr;
			u32 addr_len = sizeof(addr);
			if (getpeername(mSocketDescriptor, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// "Fetch of foreign port failed (getpeername())", true);
				return 0;
			}
			return ntohs(addr.sin_port);
		}

		// tcpsocket Code

		tcpsocket::tcpsocket() : socket()
		{
		}

		void tcpsocket::open()
		{
			open_socket(SOCK_STREAM, IPPROTO_TCP);
		}

		void tcpsocket::connect(const char* foreignAddress, u16 foreignPort)
		{
			socket::connect(foreignAddress, foreignPort);
		}

		// tcpserversocket Code
		tcpserversocket::tcpserversocket() : socket_base()
		{
		}

		void tcpserversocket::open(u16 localPort, s32 queueLen)
		{
			open_socket(SOCK_STREAM, IPPROTO_TCP);
			setLocalPort(localPort);
			setListen(queueLen);
		}

		void tcpserversocket::open(const char* localAddress, u16 localPort, s32 queueLen)
		{
			open_socket(SOCK_STREAM, IPPROTO_TCP);
			setLocalAddressAndPort(localAddress, localPort);
			setListen(queueLen);
		}

		s32 tcpserversocket::accept(tcpsocket* newSocket)
		{
			s32 descriptor;
			if ((descriptor = ::accept(mSocketDescriptor, NULL, 0)) < 0)
			{
				// Accept failed (accept())
				return -1;
			}
			newSocket->use(descriptor);
			return 0;
		}

		void tcpserversocket::setListen(s32 queueLen)
		{
			if (listen(mSocketDescriptor, queueLen) < 0)
			{
				// Set listening socket failed (listen())
			}
		}

		// udpsocket Code

		udpsocket::udpsocket() : socket()
		{
		}

		void udpsocket::open(u16 localPort)
		{
			open_socket(SOCK_DGRAM, IPPROTO_UDP);
			setLocalPort(localPort);
			setBroadcast();
		}

		void udpsocket::open(const char* localAddress, u16 localPort)
		{
			open_socket(SOCK_DGRAM, IPPROTO_UDP);
			setLocalAddressAndPort(localAddress, localPort);
			setBroadcast();
		}

		void udpsocket::setBroadcast()
		{
			// If this fails, we'll hear about it when we try to send.  This will allow 
			// system that cannot broadcast to continue if they don't plan to broadcast
			s32 broadcastPermission = 1;
			setsockopt(mSocketDescriptor, SOL_SOCKET, SO_BROADCAST, (raw_type *)&broadcastPermission, sizeof(broadcastPermission));
		}

		void udpsocket::disconnect()
		{
			sockaddr_in nullAddr;
			memset(&nullAddr, 0, sizeof(nullAddr));
			nullAddr.sin_family = AF_UNSPEC;

			// Try to disconnect
			if (::connect(mSocketDescriptor, (sockaddr *)&nullAddr, sizeof(nullAddr)) < 0)
			{
#ifdef PLATFORM_PC
				if (errno != WSAEAFNOSUPPORT)
				{
#else
				if (errno != EAFNOSUPPORT)
				{
#endif
					// "Disconnect failed (connect())", true);
				}
			}
		}

		void udpsocket::sendTo(const void *buffer, s32 bufferLen, const char* foreignAddress, u16 foreignPort)
		{
			sockaddr_in destAddr;
			fillAddr(foreignAddress, foreignPort, destAddr);

			// Write out the whole buffer as a single message.
			if (sendto(mSocketDescriptor, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&destAddr, sizeof(destAddr)) != bufferLen)
			{
				// "Send failed (sendto())", true);
			}
		}

		s32 udpsocket::recvFrom(void *buffer, s32 bufferLen, char* sourceAddress, u16 &sourcePort)
		{
			sockaddr_in clntAddr;
			socklen_t addrLen = sizeof(clntAddr);
			s32 rtn;
			if ((rtn = recvfrom(mSocketDescriptor, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&clntAddr, (socklen_t *)&addrLen)) < 0)
			{
				// "Receive failed (recvfrom())", true);
			}
			sourceAddress = inet_ntoa(clntAddr.sin_addr);
			sourcePort = ntohs(clntAddr.sin_port);

			return rtn;
		}

		void udpsocket::setMulticastTTL(unsigned char multicastTTL)
		{
			if (setsockopt(mSocketDescriptor, IPPROTO_IP, IP_MULTICAST_TTL, (raw_type *)&multicastTTL, sizeof(multicastTTL)) < 0)
			{
				// "Multicast TTL set failed (setsockopt())", true);
			}
		}

		void udpsocket::joinGroup(const char* multicastGroup)
		{
			ip_mreq multicastRequest;

			multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
			multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(mSocketDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
			{
				// "Multicast group join failed (setsockopt())", true);
			}
		}

		void udpsocket::leaveGroup(const char* multicastGroup)
		{
			ip_mreq multicastRequest;

			multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
			multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(mSocketDescriptor, IPPROTO_IP, IP_DROP_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
			{
				// "Multicast group leave failed (setsockopt())", true);
			}
		}

	}
}