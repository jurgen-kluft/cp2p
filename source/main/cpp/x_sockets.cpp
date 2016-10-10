#include "xp2p/private/x_sockets.h"

#ifdef PLATFORM_PC
#include <winsock.h>         // For socket(), connect(), send(), and recv()
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

#ifdef PLATFORM_PC
static bool sWSAinitialized = false;
#endif



namespace xcore
{
	namespace xnet
	{

		// Function to fill in address structure given an address and port
		static int fillAddr(const char* address, u16 port, sockaddr_in &addr)
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

		socket_base::socket_base(int type, int protocol)
		{
#ifdef PLATFORM_PC
			if (!sWSAinitialized)
			{
				WORD wVersionRequested;
				WSADATA wsaData;

				wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
				if (WSAStartup(wVersionRequested, &wsaData) != 0)
				{  // Load WinSock DLL
				// "Unable to load WinSock DLL");
				}
				else
				{
					sWSAinitialized = true;
				}
			}
#endif

			// Make a new socket
			if ((sockDesc = ::socket(PF_INET, type, protocol)) < 0)
			{
				// "socket_base creation failed (socket())", true);
			}
		}

		socket_base::socket_base(int sockDesc)
		{
			this->sockDesc = sockDesc;
		}

		socket_base::~socket_base()
		{
#ifdef PLATFORM_PC
			::closesocket(sockDesc);
#else
			::close(sockDesc);
#endif
			sockDesc = -1;
		}

		void socket_base::getLocalAddress(char* address, s32 len)
		{
			sockaddr_in addr;
			unsigned int addr_len = sizeof(addr);

			if (getsockname(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// "Fetch of local address failed (getsockname())", true);
			}
			const char* str = inet_ntoa(addr.sin_addr);
			strcpy_s(address, len, str);
		}

		u16 socket_base::getLocalPort()
		{
			sockaddr_in addr;
			unsigned int addr_len = sizeof(addr);

			if (getsockname(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// "Fetch of local port failed (getsockname())", true);
			}
			return ntohs(addr.sin_port);
		}

		void socket_base::setLocalPort(u16 localPort)
		{
			// Bind the socket to its port
			sockaddr_in localAddr;
			memset(&localAddr, 0, sizeof(localAddr));
			localAddr.sin_family = AF_INET;
			localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
			localAddr.sin_port = htons(localPort);

			if (bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0)
			{
				// "Set of local port failed (bind())", true);
			}
		}

		void socket_base::setLocalAddressAndPort(const char* localAddress, u16 localPort)
		{
			// Get the address of the requested host
			sockaddr_in localAddr;
			fillAddr(localAddress, localPort, localAddr);

			if (bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0)
			{
				// "Set of local address and port failed (bind())", true);
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

		socket::socket(s32 type, s32 protocol) : socket_base(type, protocol)
		{
		}

		socket::socket(s32 newConnSD) : socket_base(newConnSD)
		{
		}

		void socket::connect(const char* foreignAddress, u16 foreignPort)
		{
			// Get the address of the requested host
			sockaddr_in destAddr;
			fillAddr(foreignAddress, foreignPort, destAddr);

			// Try to connect to the given port
			if (::connect(sockDesc, (sockaddr *)&destAddr, sizeof(destAddr)) < 0)
			{
				// "Connect failed (connect())", true);
			}
		}

		void socket::send(const void *buffer, s32 bufferLen)
		{
			if (::send(sockDesc, (raw_type *)buffer, bufferLen, 0) < 0)
			{
				// "Send failed (send())", true);
			}
		}

		s32 socket::recv(void *buffer, s32 bufferLen)
		{
			s32 rtn;
			if ((rtn = ::recv(sockDesc, (raw_type *)buffer, bufferLen, 0)) < 0)
			{
				// "Received failed (recv())", true);
			}

			return rtn;
		}

		void socket::getForeignAddress(char* address, s32 len)
		{
			sockaddr_in addr;
			u32 addr_len = sizeof(addr);

			if (getpeername(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// "Fetch of foreign address failed (getpeername())", true);
			}
			const char* str = inet_ntoa(addr.sin_addr);
			strcpy_s(address, len, str);
		}

		u16 socket::getForeignPort()
		{
			sockaddr_in addr;
			u32 addr_len = sizeof(addr);

			if (getpeername(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)
			{
				// "Fetch of foreign port failed (getpeername())", true);
			}
			return ntohs(addr.sin_port);
		}

		// tcpsocket Code

		tcpsocket::tcpsocket() : socket(SOCK_STREAM, IPPROTO_TCP)
		{
		}

		tcpsocket::tcpsocket(const char* foreignAddress, u16 foreignPort) : socket(SOCK_STREAM, IPPROTO_TCP)
		{
			connect(foreignAddress, foreignPort);
		}

		tcpsocket::tcpsocket(s32 newConnSD) : socket(newConnSD)
		{
		}

		// tcpserversocket Code

		tcpserversocket::tcpserversocket(u16 localPort, s32 queueLen) : socket_base(SOCK_STREAM, IPPROTO_TCP)
		{
			setLocalPort(localPort);
			setListen(queueLen);
		}

		tcpserversocket::tcpserversocket(const char* localAddress, u16 localPort, s32 queueLen) : socket_base(SOCK_STREAM, IPPROTO_TCP)
		{
			setLocalAddressAndPort(localAddress, localPort);
			setListen(queueLen);
		}

		tcpsocket *tcpserversocket::accept()
		{
			s32 newConnSD;
			if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0)
			{
				// "Accept failed (accept())", true);
			}

			return new tcpsocket(newConnSD);
		}

		void tcpserversocket::setListen(s32 queueLen)
		{
			if (listen(sockDesc, queueLen) < 0)
			{
				// "Set listening socket failed (listen())", true);
			}
		}

		// udpsocket Code

		udpsocket::udpsocket() : socket(SOCK_DGRAM, IPPROTO_UDP)
		{
			setBroadcast();
		}

		udpsocket::udpsocket(u16 localPort) : socket(SOCK_DGRAM, IPPROTO_UDP)
		{
			setLocalPort(localPort);
			setBroadcast();
		}

		udpsocket::udpsocket(const char* localAddress, u16 localPort) : socket(SOCK_DGRAM, IPPROTO_UDP)
		{
			setLocalAddressAndPort(localAddress, localPort);
			setBroadcast();
		}

		void udpsocket::setBroadcast()
		{
			// If this fails, we'll hear about it when we try to send.  This will allow 
			// system that cannot broadcast to continue if they don't plan to broadcast
			s32 broadcastPermission = 1;
			setsockopt(sockDesc, SOL_SOCKET, SO_BROADCAST,
				(raw_type *)&broadcastPermission, sizeof(broadcastPermission));
		}

		void udpsocket::disconnect()
		{
			sockaddr_in nullAddr;
			memset(&nullAddr, 0, sizeof(nullAddr));
			nullAddr.sin_family = AF_UNSPEC;

			// Try to disconnect
			if (::connect(sockDesc, (sockaddr *)&nullAddr, sizeof(nullAddr)) < 0)
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
			if (sendto(sockDesc, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&destAddr, sizeof(destAddr)) != bufferLen)
			{
				// "Send failed (sendto())", true);
			}
		}

		s32 udpsocket::recvFrom(void *buffer, s32 bufferLen, char* sourceAddress, u16 &sourcePort)
		{
			sockaddr_in clntAddr;
			socklen_t addrLen = sizeof(clntAddr);
			s32 rtn;
			if ((rtn = recvfrom(sockDesc, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&clntAddr, (socklen_t *)&addrLen)) < 0)
			{
				// "Receive failed (recvfrom())", true);
			}
			sourceAddress = inet_ntoa(clntAddr.sin_addr);
			sourcePort = ntohs(clntAddr.sin_port);

			return rtn;
		}

		void udpsocket::setMulticastTTL(unsigned char multicastTTL)
		{
			if (setsockopt(sockDesc, IPPROTO_IP, IP_MULTICAST_TTL, (raw_type *)&multicastTTL, sizeof(multicastTTL)) < 0)
			{
				// "Multicast TTL set failed (setsockopt())", true);
			}
		}

		void udpsocket::joinGroup(const char* multicastGroup)
		{
			ip_mreq multicastRequest;

			multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
			multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(sockDesc, IPPROTO_IP, IP_ADD_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
			{
				// "Multicast group join failed (setsockopt())", true);
			}
		}

		void udpsocket::leaveGroup(const char* multicastGroup)
		{
			ip_mreq multicastRequest;

			multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
			multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(sockDesc, IPPROTO_IP, IP_DROP_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
			{
				// "Multicast group leave failed (setsockopt())", true);
			}
		}

	}
}