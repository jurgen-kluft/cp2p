#ifndef __XP2P_PRACTICALSOCKET_INCLUDED__
#define __XP2P_PRACTICALSOCKET_INCLUDED__
#include "xbase\x_target.h"
#ifdef USE_PRAGMA_ONCE 
#pragma once 
#endif

#include "xp2p\x_types.h"

namespace xcore
{
	namespace xsocket
	{
		class xinit
		{
		public:
			inline		xinit() : m_initialized(false) {}

			bool		startUp();

			/**
			*   If WinSock, unload the WinSock DLLs; otherwise do nothing.  We ignore
			*   this in our sample client code but include it in the library for
			*   completeness.  If you are running on Windows and you are concerned
			*   about DLL resource consumption, call this after you are done with all
			*   Socket instances.  If you execute this on Windows while some instance of
			*   Socket exists, you are toast.  For portability of client code, this is
			*   an empty function on non-Windows platforms so you can always include it.
			*   @param buffer buffer to receive the data
			*   @param bufferLen maximum number of bytes to read into buffer
			*   @return number of bytes read, 0 for EOF, and -1 for error
			*/
			bool		cleanUp();

		protected:
			bool		m_initialized;
		};

		/**
		 *   Base class representing basic communication endpoint
		 */
		class xudp
		{
		public:
			enum
			{
				INVALID_SOCKET_DESCRIPTOR = -1,
			};

			xudp();

			/**
			 *   Close and deallocate this socket
			 */
			~xudp();

			/**
			*   Get the local address
			*   @return true if socket has valid descriptor
			*/
			bool valid() const;

			/**
			 *   Get the local address
			 *   @return local address of socket
			 */
			s32 getLocalAddress(char* address, s32 len);

			/**
			 *   Get the local port
			 *   @return local port of socket
			 */
			u16 getLocalPort();

			/**
			 *   Set the local port to the specified port and the local address
			 *   to any interface
			 *   @param localPort local port
			 */
			void setLocalPort(u16 localPort);

			/**
			 *   Set the local port to the specified port and the local address
			 *   to the specified address.  If you omit the port, a random port
			 *   will be selected.
			 *   @param localAddress local address
			 *   @param localPort local port
			 */
			void setLocalAddressAndPort(const char* localAddress, u16 localPort = 0);


			/**
			*   Construct a UDP socket with the given local port
			*   @param localPort local port
			*/
			void open(u16 localPort);

			/**
			*   Construct a UDP socket with the given local port and address
			*   @param localAddress local address
			*   @param localPort local port
			*/
			void open(const char* localAddress, u16 localPort);

			/**
			*   Unset foreign address and port
			*   @return true if disassociation is successful
			*/
			void disconnect();

			/**
			*   Send the given buffer as a UDP datagram to the specified address/port
			*   @param buffer buffer to be written
			*   @param bufferLen number of bytes to write
			*   @param foreignAddress address (IP address or name) to send to
			*   @param foreignPort port number to send to
			*   @return true if send is successful
			*/
			void sendTo(const void *buffer, s32 bufferLen, const char* foreignAddress, u16 foreignPort);

			/**
			*   Read read up to bufferLen bytes data from this socket.
			The given buffer is where the data will be placed.
			*   @param buffer buffer to receive data
			*   @param bufferLen maximum number of bytes to receive
			*   @param foreignAddress address of datagram source
			*   @param foreignPort port of data source
			*   @return number of bytes received and -1 for error
			*/
			int recvFrom(void *buffer, s32 bufferLen, char* foreignAddress, u16 &foreignPort);

			/**
			*   Set the multicast TTL
			*   @param multicastTTL multicast TTL
			*/
			void setMulticastTTL(u8 multicastTTL);

			/**
			*   Join the specified multicast group
			*   @param multicastGroup multicast group address to join
			*/
			void joinGroup(const char* multicastGroup);

			/**
			*   Leave the specified multicast group
			*   @param multicastGroup multicast group address to leave
			*/
			void leaveGroup(const char* multicastGroup);



		private:
			// Prevent the user from trying to use value semantics on this object
					xudp(const xudp &sock);
			void	operator=(const xudp &sock);

		protected:
			s32 mSocketDescriptor;

					xudp(s32 descriptor);

			bool open_socket(s32 type, s32 protocol);
			void setBroadcast();
		};



	}
}

#endif
