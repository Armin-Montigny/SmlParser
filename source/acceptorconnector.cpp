// -----------------------------------------------------------------------------------------------------
//
// SML Parser
//
// Copyright (C) 2018  Armin Montigny
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------------------------------
//
// acceptorconnector.cpp
//
// Some classes to implement the "acceptor connector" design pattern
//
// Member Functions
// 
// For setting up network connections we need normally 2 parts
// 1. A server that accepts connection requests
// 2. A client that tries to connect to a server
//
// Her we eill build the cacceptor and the connector part.
//
// The acceptor does the following
//
// - It finds out its network address information
// - Then it opens a socket 
// - sets options for the socket
// - And binds the address to the open socket
// - FInally listen is started and the port waits for incoming connection requests
//
// Server functions register the socket handler with the reactor
// If there is some connection request the reactor will call the acceptors
// handleEvent function. Then the acceptor will create a TCP connection.
// So, the acceptor is a factory for TCP Connections. Depending on the
// given port, the related type of TCP connection will be created.
//
// The acceptor internally holds a list of established TCP connections
// and handles all necessary administrative stuff for them.
//
// Acceptor is a template with a factory class for TCP Connections as template parameter
// An instance of a factory class is stored in the acceptor (bridge pattern)
//
//
//
//
// The connector is a little bit simpler.
// As a parameter ist receives the IP address of the peer to which it shall connect.
// Then a socket is opened in none blocking mode.
// If the connection cannot be established immediately, the socket is registered 
// with the reactor with event type out. If the socket is writable the connection
// will be established. This functionality is split ito several functions.
// After a connection has been established subscribers of the connector will be notified
// Then they will in turn create a TCP connection for this socket.
//
// Interestingly, the acceptor and connector share common functionality.
// This will be defined in NetworkConnectionCreation. Both acceptor and
// connector derive from this class.
//
//
#include "acceptorconnector.hpp"

#include <fcntl.h>
#include <errno.h>


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Helper Class member functions for Acceptor / Connector


	namespace AcceptorConnectorInternal
	{
	// -----------------------------------------------------------------------
	// 1.1 Network Data Hekp class member function
	
		// ----------------------------------------------------
		// 1.1.1 COnstructor
		// Standard constructor. Initialize the port number string, like for example "8765"
		NetworkData::NetworkData(const std::string &PortNumberString, const std::string &IpAddressOrHostName) : 
					portNumberString(PortNumberString), 
					ipAddressOrHostName(IpAddressOrHostName),
					addressInfo(),
					networkAddressInfo(null<NetworkAddressInfo *>())
		{
			// Get the internet address of this machine/port and remember result
			createMachineNetworkAddressInfo();
		}

		
		// ----------------------------------------------------
		// 1.1.2 Destructor
		// Destructor will free the allocated memory of the linked list with addresses
		//lint -e{1740}  "pointer member 'Symbol' (Location) not directly freed or zero'ed by destructor --". yes it is
		NetworkData::~NetworkData(void)
		{
			try
			{
				if ( null<NetworkAddressInfo *>() !=networkAddressInfo)
				{
					//Free the allocated memory of the linked list with addresses
					freeaddrinfo(networkAddressInfo);
				}
			}
			catch(...)
			{
			}
		}
		
		// ----------------------------------------------------
		// 1.1.3 Wrapper for getaddrinfo
		
		// A wrapper with extended functionality for "getaddrinfo". See
		// http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
		// This function will actually get machine relevant address information for the given port
		// Both Acceptor an Connector use this functionality
		void NetworkData::createMachineNetworkAddressInfo(void)
		{
			
			// Hints (selection criteria) to get the correct address for the socket function
			NetworkAddressInfo networkAddressInfoCriteriaHints;
			
			// First fill the hints for the selecting criteria for getting the desired network address info
			// Initialize the structure
			memset(&networkAddressInfoCriteriaHints, null<sint>(), sizeof(networkAddressInfoCriteriaHints));
			
			// Tell system that we can deal with IPV4 and IPV6 addresses
			networkAddressInfoCriteriaHints.ai_family = AF_UNSPEC;
			// We want to use a reliable 2 way connection. We want to use "streams" sockets
			//lint -e{911,641}
			// 911 Implicit expression promotion from Type to Type		// Library Function
			// 641 Converting enum to int								// Library Function
			networkAddressInfoCriteriaHints.ai_socktype = SOCK_STREAM;

			const mchar *node;
			
			
			// Different behaviour for acceptor and connector (server and client)
			// Test for acceptor
			if (StrEmpty == ipAddressOrHostName)
			{
				// We are a server (or acceptor)
				// We want to bind a socket and accept a connection. The following will fill in our own IP
				networkAddressInfoCriteriaHints.ai_flags = AI_PASSIVE; 
				node = null<mchar *>();
			} 
			else
			{
				// We are a client (or connector)
				// Do not set passive flag, because we want to connect
				// We want to connect to a node with the given IP address
				node = ipAddressOrHostName.c_str();
			}
			
			
			// Now get the address (or more addresses)
			// "NULL" for the "node" name together with "AI_PASSIVE" in the "flags" will get our own internet address (IP)
			// If ipAddressOrHostName is not NULL, we will get the address of the other side. To whom we want to connect
			if (null<sint>() == getaddrinfo(node, portNumberString.c_str(), &networkAddressInfoCriteriaHints, &networkAddressInfo))
			{
				// First clear the vector. It has to be empty because we want to add now a net array of elements
				addressInfo.clear();
				// Store all found network addresses in a vector for std::  compliant access
				for (NetworkAddressInfo *aIterator = networkAddressInfo; null<NetworkAddressInfo *>() != aIterator; aIterator = aIterator->ai_next)
				{
					addressInfo.push_back(aIterator);
				}
			}
		}

	// -----------------------------------------------------------------------
	// 1.2 Common Base CLass for Acceptor and Connector
	
		// ----------------------------------------------------
		// 1.2.1 Constructor
	
		// Explicit constructor with port name and IP Address
		NetworkConnectionCreation::NetworkConnectionCreation(const std::string &portNameOrNumber, const std::string &IpAddressOrHostName) :
																			CommunicationEndPoint(),
																			// Set the machines internet address and the selected port
																			machineNetworkAddressInfo(portNameOrNumber, IpAddressOrHostName),
																			activated(false),
																			activationOngoing(false)
		{}

		// ----------------------------------------------------
		// 1.2.2 Destructor
		// Destructor for Connector
		
		Connector::~Connector(void)
		{
			try
			{
				// close open socket handle
				Connector::stop();	
			}
			catch(...)
			{
			}
		}


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Connector Class member FUnctions
		
	// -----------------------------------------------------------------------
	// 2.1 Handle Event for COnnector (Connection established)
	
		// This function will prepare a network socket for accepting connection requests by other network entities
		// The connector is trying to make a connection in non blocking mode
		// If the connection could not be done immediately, an eventhandler is registered
		// in our reactor. When the connection is ready then a write event is available
		// We will check this here
		EventProcessing::Action Connector::handleEvent(const EventType et) 
		{ 
						//ui << "Connected handle Event 1" << std::endl;
			
			// Default is to continue the event loop
			const EventProcessing::Action rc = EventProcessing::Continue;
			//lint -e{921}	// 921 Cast from Type to Type	// OK, Short to int
			switch (static_cast<sint>(et))
			{
				case EventTypeOut:
					// Write event is here
					// Check for error
					{
						//ui << "Connected handle Event 2"  << std::endl;
						boolean success = false;
						sint result;
						socklen_t result_len = sizeof(result);
						// Check for error
						if (getsockopt(handle, SOL_SOCKET, SO_ERROR, &result, &result_len) >= null<sint>()) 
						{
							if (null<sint>() == result) 
							{
								// No error
								// Finalize connection, create TCP Connection
								finalizeNetworkConnection();
								success = true;
							}
							else
							{
								ui << "getsockopt returned error"  << std::endl;
							}
							// Else error, no success
						}
						else
						{
							ui << "getsockopt call error: " << errno << " " << strerror(errno) << std::endl;
	
						}
						// Else error, no success
						if (!success)
						{
							reactorUnRegisterEventHandler(this);	
							Connector::stop();				
						}
					}
					
					break;
					
				// In case that connection could not be established
				case EventTypeOut + EventTypeError + EventTypeHangup:
					ui << "Could not connect to " << machineNetworkAddressInfo.ipAddressOrHostName << ':' << machineNetworkAddressInfo.portNumberString << std::endl;
					reactorUnRegisterEventHandler(this);	
					Connector::stop();							
					break;
					
				default:
					// Unexpected Event
					{
						static sint i=null<sint>();
						ui << "Connector::handleEvent: " << i << "  Unexpected Event: " << static_cast<sint>(et) << std::endl;
						++i;
					}
					reactorUnRegisterEventHandler(this);	
					Connector::stop();		
					
					break;
			}
			
			// In any case. The activiation routine is not active any more
			activationOngoing = false;
			
			return rc; 
		} 	

	// -----------------------------------------------------------------------
	// 2.2 Finalize Network Connection creation


		// The OS system connect function succeded. Now we finalize the creation of the connection
		void Connector::finalizeNetworkConnection(void)
		{

			static sint i =1;
			ui << "Connected " << i << "  Port Number " << machineNetworkAddressInfo.ipAddressOrHostName << ":" << machineNetworkAddressInfo.portNumberString << std::endl;
			++i;


			// Set flag to indicate that the connection is active
			activated = true;
			
			// Remove event handler that was started for the OS system "connect"-function
			// This may or may not exist. Depends on, if the Connector could make
			// a connection immediately or not. But no Problem, the unregister function has
			// no issues with that
			// We need to do that. Otherwise POLLOUT events will be received continously.
			reactorUnRegisterEventHandler(this);	
			
			// Inform interested parties about new TCP connection
			//ui << "Establish Network Connection. Call Notify" << std::endl;
			Connector::notifySubscribers();
			activationOngoing = false;
		}



	// -----------------------------------------------------------------------
	// 2.3 Start Connecting


		void Connector::activate(void) 
		{
			// Be pessimistic. There may be even no network address in the vector. Assume a problem
			boolean thereIsAProblem = true;
			
			// If there is not already a connection available and we have not yet been activated
			if ((!Connector::isActive()) && (!activationOngoing))
			{
				ui << "Connector activate for  " << machineNetworkAddressInfo.ipAddressOrHostName << ":" << machineNetworkAddressInfo.portNumberString <<std::endl;
			
				activationOngoing = true;
				
				// Now iterate through the linked list of addresses found by sub class NetworkData
				// As long there is a network address available and as long as there is a problem of seom sort
				for (NAII addressIterator = machineNetworkAddressInfo.addressInfo.begin();
						(addressIterator != machineNetworkAddressInfo.addressInfo.end()) && thereIsAProblem;
						++addressIterator)
				
				{
					// Open a socket for the found address
					handle = socket((*addressIterator)->ai_family, (*addressIterator)->ai_socktype,(*addressIterator)->ai_protocol);
					// If everything OK
					if (-1 != handle)
					{
						// We have a valid handle. The socket could be opened
						// Set handle to non blocking mode
						// We do not want to block the system while waiting for the connection to be established
						//lint -e{1960,9001}  "Octal constant used"   OK, Lib Function
						if (-1 == fcntl(handle, F_SETFL , O_NONBLOCK))
						{
							// Could not set non blocking mode
							ui << "Socket Set-to-none-blocking-mode error:  " << errno << " --> " << strerror(errno) << std::endl;
							// Error, so close socket handle and try next address
							Connector::stop();
						}
						else
						{
							// No error. Socket open and set to none blocking mode
							// Ask OS to connect to other side
							const sint rc = connect(handle, (*addressIterator)->ai_addr, (*addressIterator)->ai_addrlen);
							if (null<sint>() == rc)
							{
								// The connection could immediately be made. Connect succeeded
								ui << "Connected immediately" << std::endl;
								
								// It worked, create a tcp connection with this handle
								finalizeNetworkConnection();
								// And done

							}
							else
							{
								// Connect returned with error
								// Because of non blocking mode, this is very likely
								switch (errno)
								{

									// We will check the error number and react accordingly
									// This should not happen and is basically a logical programming error,
									// because the active and activation Ongoing flags should prevent that
									case EALREADY:		// Connection request on this socket was already made
										// do nothing
										ui << "Connector connect error EALREADY:  " << errno << " --> " << strerror(errno) << std::endl;

										// And no, despite of error, for our functionthere is no error. We just need to wait for
										// the event coming back from the reactor
										thereIsAProblem = false;
										break;



										
									// Still trying to establish connection. This will happen most likely, because we opened the
									// handle in noneblocking mode
									case EINPROGRESS:
										// Then we will install an event handler for the socket handle that informs us when the connection is done
										reactorRegisterEventHandler(this, EventTypeOut);
										// This is not a problem at all. This is normal
										thereIsAProblem = false;
										break;


										// Connection already existing	
										// Should never happen. Flag activated should already be true
									case EISCONN:
										activated = true;
										// And no, despite of error, for our function there is no error. The connection is already existing
										thereIsAProblem = false;
										ui << "Should never happen. Programming error. Connector connect error EISCONN:  " << errno << " --> " << strerror(errno) << std::endl;
										break;
										
									// Any kind of other error
									default:
										ui << "Connector connect error:  " << errno << " --> " << strerror(errno) << std::endl;
										// Close this handle
										Connector::stop();
										// Try next address in list
										break;
										
								} // end switch errno
							} // end else if 0 == rc  (connect failed)
						} // end if (-1 == fcntl(handle, F_SETFL , O_NONBLOCK))
					} 
					else
					{
						// Since we could not open it, there is no need to close the socket handle and to call stop
						// But since the handle is now -1, we will set it to 0 again
						handle = null<Handle>();
						// Debug info
						ui << "Connector open socket error:  " << errno << " --> " << strerror(errno) << std::endl;
						// Continue with next IP address in our IP address vector
					}
				} // end for
			} // end  if (!isActive() && (!activationOngoing))
			
			// If there is still a problem, we mark the activation routine as inactive
			if (thereIsAProblem)
			{
				activationOngoing = false;
			}
		}

	}

 
	
	
