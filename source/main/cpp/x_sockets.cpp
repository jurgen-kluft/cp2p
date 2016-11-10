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
	namespace xsocket
	{
#ifdef PLATFORM_PC
#endif

		bool	xinit::startUp()
		{
			bool result = true;
#ifdef PLATFORM_PC
			if (!m_initialized)
			{
				WORD wVersionRequested;
				WSADATA wsaData;

				wVersionRequested = MAKEWORD(2, 0);              // Request WinSock v2.0
				if (WSAStartup(wVersionRequested, &wsaData) != 0)	// Load WinSock DLL
				{	// "Unable to load WinSock DLL");
					result = false;
					m_initialized = false;
				}
				else
				{
					m_initialized = true;
				}
			}
#endif
			return result;
		}

		bool	xinit::cleanUp()
		{
			bool result = true;
#ifdef PLATFORM_PC
			if (m_initialized)
			{
				if (WSACleanup() != 0)
				{
					result = false;
				}
#endif
				m_initialized = false;
			}
			return result;
		}


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

		xudp::xudp()
			: mSocketDescriptor(xudp::INVALID_SOCKET_DESCRIPTOR)
		{
		}

		xudp::~xudp()
		{
#ifdef PLATFORM_PC
			::closesocket(mSocketDescriptor);
#else
			::close(mSocketDescriptor);
#endif
			mSocketDescriptor = -1;
		}

		bool xudp::valid() const
		{
			return mSocketDescriptor != INVALID_SOCKET_DESCRIPTOR;
		}

		bool xudp::open_socket(s32 type, s32 protocol)
		{
			bool result = false;

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

		s32 xudp::getLocalAddress(char* address, s32 len)
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

		u16 xudp::getLocalPort()
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

		void xudp::setLocalPort(u16 localPort)
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

		void xudp::setLocalAddressAndPort(const char* localAddress, u16 localPort)
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

		void xudp::open(u16 localPort)
		{
			open_socket(SOCK_DGRAM, IPPROTO_UDP);
			setLocalPort(localPort);
			setBroadcast();
		}

		void xudp::open(const char* localAddress, u16 localPort)
		{
			open_socket(SOCK_DGRAM, IPPROTO_UDP);
			setLocalAddressAndPort(localAddress, localPort);
			setBroadcast();
		}

		void xudp::setBroadcast()
		{
			// If this fails, we'll hear about it when we try to send.  This will allow 
			// system that cannot broadcast to continue if they don't plan to broadcast
			s32 broadcastPermission = 1;
			setsockopt(mSocketDescriptor, SOL_SOCKET, SO_BROADCAST, (raw_type *)&broadcastPermission, sizeof(broadcastPermission));
		}

		void xudp::disconnect()
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

		void xudp::sendTo(const void *buffer, s32 bufferLen, const char* foreignAddress, u16 foreignPort)
		{
			sockaddr_in destAddr;
			fillAddr(foreignAddress, foreignPort, destAddr);

			// Write out the whole buffer as a single message.
			if (sendto(mSocketDescriptor, (raw_type *)buffer, bufferLen, 0, (sockaddr *)&destAddr, sizeof(destAddr)) != bufferLen)
			{
				// "Send failed (sendto())", true);
			}
		}

		s32 xudp::recvFrom(void *buffer, s32 bufferLen, char* sourceAddress, u16 &sourcePort)
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

		void xudp::setMulticastTTL(unsigned char multicastTTL)
		{
			if (setsockopt(mSocketDescriptor, IPPROTO_IP, IP_MULTICAST_TTL, (raw_type *)&multicastTTL, sizeof(multicastTTL)) < 0)
			{
				// "Multicast TTL set failed (setsockopt())", true);
			}
		}

		void xudp::joinGroup(const char* multicastGroup)
		{
			ip_mreq multicastRequest;

			multicastRequest.imr_multiaddr.s_addr = inet_addr(multicastGroup);
			multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
			if (setsockopt(mSocketDescriptor, IPPROTO_IP, IP_ADD_MEMBERSHIP, (raw_type *)&multicastRequest, sizeof(multicastRequest)) < 0)
			{
				// "Multicast group join failed (setsockopt())", true);
			}
		}

		void xudp::leaveGroup(const char* multicastGroup)
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