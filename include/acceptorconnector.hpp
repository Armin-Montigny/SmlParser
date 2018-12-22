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
//
// acceptorconnector.hpp
//
// Some classes to implement the "acceptor connector" design pattern
// 
// For setting up network connections we need normally 2 parts
// 1. A server that accepts connection requests
// 2. A client that tries to connect to a server
//
// Here we will build the acceptor and the connector part.
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

#ifndef ACCEPTORCONNECTOR_HPP
#define ACCEPTORCONNECTOR_HPP

#include "observer.hpp"
#include "tcpconnection.hpp"
#include "reactor.hpp"

#include <arpa/inet.h>
#include <netdb.h>



	namespace AcceptorConnectorInternal
	{
	
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Helper Classes for Acceptor / Connector
	
	
	// -----------------------------------------------------------------------
	// 1.1 Typedefs for NetworkAddressInfo
	
	
		// We will use a more meaningful name  for the struct addrinfo
		typedef struct addrinfo NetworkAddressInfo;
		// And create a shortcut for the related iterator
		typedef std::vector<NetworkAddressInfo *>::iterator NAII;
		// For default initializing purposes
		const std::string StrEmpty;

		
	// -----------------------------------------------------------------------
	// 1.2 Network Data

		// For Client/Server Acceptor/Connector functionality we need the internet address info of the machine, 
		// where the server/acceptor/connector is running on, meaning: This machine or the peer to connect to.
		// This class gets the relevant info in a most compatible way, taking even  IPV6 address into account. 
		// Please note. This is only a helper class for further usage by accptor/connector classes
		class NetworkData
		{
			public:
				// Explicit Constructor that must be used. The Constructor will automatically create the address info
				explicit NetworkData(const std::string &PortNumberString,  const std::string &IpAddressOrHostName = StrEmpty);
				// Destructor will free the recursive addrinfo structure
				virtual ~NetworkData(void);
				
				// Worker horse. Get the address
				void createMachineNetworkAddressInfo(void);		// Get the network address info
				 
				// Holds the port number as a string
				//lint -e{1725,1925} Yes, we want to have a public reference
				const std::string &portNumberString;
				// Holds the IP Address od hostname of the machine to connect to
				// NULL in case that we want to use the address of our own machine
				//lint -e{1725,1925}  Yes, we want ot have a public reference
				const std::string &ipAddressOrHostName;
				
				// Wrapper around linked list from OS
				//lint -e{1925} Yes, we want ot have a public member
				std::vector<NetworkAddressInfo *>addressInfo;

			protected:
				// Pointer to the retrieved linked list addrinfo structure
				NetworkAddressInfo *networkAddressInfo;

			private:
				// Default Constructor. No to be used. Set everything to 0
				NetworkData(void) : portNumberString(StrEmpty),
									ipAddressOrHostName(StrEmpty),
									addressInfo(),
									networkAddressInfo(null<NetworkAddressInfo *>()){}
		};


	// -----------------------------------------------------------------------
	// 1.2 Common Base Class for Acceptor and Connector
	
	// Both use common functionality

		// Abstract base class for Acceptor and Connector
		class NetworkConnectionCreation : public CommunicationEndPoint
		{
			public: 

				// Explicit constructor with port name and IP Address
				// For the acceptor the IpAddressOrHostName will be empty
				explicit NetworkConnectionCreation(const std::string &portNameOrNumber, const std::string &IpAddressOrHostName = StrEmpty);
				// Set activated to false
				virtual ~NetworkConnectionCreation(void) { activated = false; }	
				
				// Start (either) acceptor or connector. Call its activate function and inform on result
				//lint -e{1933}	// 1933 Call to unqualified virtual function 'Symbol' from non-static member function // Yes. We want this
				virtual void start(void) { activate(); }
				
				virtual void stop(void) { CommunicationEndPoint::stop(); activated = false;}
				
				virtual boolean isActive(void) const { return NetworkConnectionCreation::activated;  }
				
				// Event handler for Reactor. Connect/Accept and create stream class. This functions is a factory
				virtual EventProcessing::Action handleEvent(const EventType et) = 0;
				
			protected:

			
				// Set up and start the complete connection creation functionality
				virtual void activate(void) = 0;

			
				// Storage for network adddress info
				NetworkData machineNetworkAddressInfo;
				
				boolean activated;
				boolean activationOngoing;
			private:
				// Standard Constructor
				NetworkConnectionCreation(void) : CommunicationEndPoint(), machineNetworkAddressInfo(StrEmpty,StrEmpty), activated(false), activationOngoing(false) {}
				
		};


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Connector


		// A Connector for connecting to network peers
		class Connector : public NetworkConnectionCreation, 
						  public Publisher<Connector>
		{
			public:
				// Explicit. Store a port string and start. Wait for connection request
				explicit Connector(const std::string &portNameOrNumber, const std::string &IpAddressOrHostName)  : NetworkConnectionCreation(portNameOrNumber, IpAddressOrHostName), Publisher<Connector>(){}

				// Empty DTor
				virtual ~Connector(void);
				// Event Handler for Connection has been established
				virtual EventProcessing::Action handleEvent(const EventType et);
			
			protected:

				// Set up and start the complete connect functionality
				virtual void activate(void) ;
				
				void finalizeNetworkConnection(void);
			private:
					// Default constructor. Before using this class, set the port string
				Connector(void) : NetworkConnectionCreation(StrEmpty,StrEmpty), Publisher<Connector>() {}
		};




// ------------------------------------------------------------------------------------------------------------------------------------------------
// 3. Acceptor

	// -----------------------------------------------------------------------
	// 3.1 Acceptor Class Definition
	
	
		// The Acceptor is an implementation of Design Pattern Acceptor / Connector using a Reactor 
		// as the Event Handler. It will get this-machines internet address, open a socket, bind this
		// machine to the socket and then listens on this socket for an incoming connection request.
		// After the connection request has been received, an instance of a raw Tcp stream connection is created
		// and again registered with the reactor. So the acceptor realizes a factory.
		template<class TcpConnectionFactory>
		class Acceptor : public NetworkConnectionCreation, 
						 public Subscriber<TcpConnectionBase>
		{
			public:
				// Explicit. Store a port string and start. Wait for connection request
				explicit Acceptor(const std::string &portNameOrNumber, TcpConnectionFactory *tcpConnectionFactoryP);
				// Terminate / Destroy internally stored TCP Connections
				virtual ~Acceptor(void);
				

				// Event Handler: Somebody wants to establish a connection and we will accept it
				virtual EventProcessing::Action handleEvent(const EventType et);
				
				
				// Information from TCP COnnection. Connection closed
				virtual void update(TcpConnectionBase *publisher);				
			protected:
				// Set up and start the complete accept functionality
				virtual void activate(void) ;
				


				// STore a new established TCP connection in our internal list and remove garbage
				void storeNewTcpConnection(TcpConnectionBase *tcb);
				// GArbage collector. Remove closed TCP connections from our internal list
				void removeAndDeleteClosedTcpConnections(void);
				
				// Depending on the port, we will create a dedicated TCP connection.
				// This is the pointer to the factory that will create the requested TCP connection
				TcpConnectionFactory *tcpConnectionFactory;

				
				// Container for established TCP connections after a connection request has been accepted
				// The algorithms used here demand a std::list as the container, because we will
				// iterate through the list and erase list elements. For other containers
				// the iterator would be invalidated.
				std::list<TcpConnectionBase *> tcpConnection;
				// Indicates that there is an inactive TCP connection that needs to be removed from our internal list
				uint requestForCloseConnectionCounter;
				
					
				// Functor for Stopping (close handle) and deleting an instance of an open TCP COnnection
				struct AcceptorTcpConnectionStopper
				{
					explicit AcceptorTcpConnectionStopper(Acceptor *acceptor) : acceptor(acceptor) {}
					void operator() (TcpConnectionBase *tcpConnectionBase);
					Acceptor *acceptor;
				};		
			
				
			private:
				// Default constructor. Do not use it
				Acceptor(void) : NetworkConnectionCreation(StrEmpty,StrEmpty), Publisher<Acceptor>() {}
		};

	// ---------------------------------------------------------------------------
	// 3.2 Acceptor Class member functions. Defined in Header because of Template
		

		// ---------------------------------------------------------------------------
		// 3.2.1 Acceptor Constructor
		
		// Constructor
		// Gets the portname and the related tcpConnectionFactory.
		template<class TcpConnectionFactory>
		Acceptor<TcpConnectionFactory>::Acceptor(const std::string &portNameOrNumber, TcpConnectionFactory *tcpConnectionFactoryP) : 
				NetworkConnectionCreation(portNameOrNumber), 
				Subscriber<TcpConnectionBase>(),
				tcpConnectionFactory(tcpConnectionFactoryP),
				tcpConnection(),
				requestForCloseConnectionCounter(null<uint>())
		{
		}		

		// ---------------------------------------------------------------------------
		// 3.2.2 Acceptor Destructor
		
		// Will stop all internally stored TCP connections, delete them and remove them from the list
		template<class TcpConnectionFactory>
		Acceptor<TcpConnectionFactory>::~Acceptor(void)
		{
			// Functor for stopping (close and set handle to 0) the stored TCP connections
			AcceptorTcpConnectionStopper acceptorTcpConnectionStopper(this);
			// STop everything
			std::for_each(tcpConnection.begin(),tcpConnection.end(),acceptorTcpConnectionStopper);
			// Delete and remove it
			removeAndDeleteClosedTcpConnections();
		}	

		// ---------------------------------------------------------------------------
		// 3.2.3 Remove and delete closed TCP COnnections
		
		// The acceptor has an internal list, where it stores all accepted connection requests 
		// and set up TCP COnnections. This function removes closed TCP connections
		// A closed TCP connection is identified by a handle equal to 0
		// The peer or ourself can close a TCP connection
		template<class TcpConnectionFactory>
		inline void Acceptor<TcpConnectionFactory>::removeAndDeleteClosedTcpConnections(void)
		{
			// This algorithm demands a std::list as the container, because we will
			// iterate through the list and erase list elements. For other containers
			// the iterator would be invalidated.
			
			// Get head of list
			std::list<TcpConnectionBase *>::iterator tcbi = tcpConnection.begin();
			
			// As long as there are still Elements in the list to work on
			while(tcbi != tcpConnection.end())
			{
				// If the TCP connection has been closed and should be removed
				if (null<Handle>() == (*tcbi)->getHandle())
				{
					// Delete the instance of the TCP connection. So delete the Element stored in the list
					delete (*tcbi);
					// End now erase the pointer to the (previously) deleted Element from the list
					// The iterator will be set to the following element in the list
					tcbi = tcpConnection.erase(tcbi);
				}
				else
				{
					// There was no deletion request for this element
					// Go and check the next element in the list
					++tcbi;
				}
			}
		}
		
		// ---------------------------------------------------------------------------
		// 3.2.4 Stop TCP Connection
		
		// Functor operator for stopping a TCP connection that is stored in the internal list of acceptor
		template<class TcpConnectionFactory>
		inline void Acceptor<TcpConnectionFactory>::AcceptorTcpConnectionStopper::operator() (TcpConnectionBase *tcpConnectionBase)
		{
			// We do not want to handle events from this handle any longer
			reactorUnRegisterEventHandler(tcpConnectionBase);
			// Close the handle (If not already done) and set handle to 0 to indicate that it is closed
			// This is important for later garbage collection.
			tcpConnectionBase->stop();
			// This acceptor does not want to receive and updates from this TCP connection any longer
			tcpConnectionBase->removeSubscription(acceptor);	
		}

		
		// ---------------------------------------------------------------------------
		// 3.2.5 Stop TCP connection. It has been terminated
		
		// Update means: TCP connection is finished. Error or Hangup or Closed
		// This function is called from a TcpConnection class, when the TCP connection has been closed
		// The acceptor should nomally now delete this instance of the TCP connection
		// But becuase the instance of the TCP connection is calling this function, we cannot do that
		// Otherwise we would delete ourselves and continue the code. This would crash the system.
		// So we stop the TCP connection, close the handle (if not already closed) and set the handle to 0,
		// to indicate, that it can be destructed in another module. 
		// This will be done in the destructor of the acceptor or
		// everytime when we add a new connection and the requestForCloseConnectionCounter > certain thresholf
		// This is like a garbage collection
		template<class TcpConnectionFactory>
		void Acceptor<TcpConnectionFactory>::update(TcpConnectionBase *publisher)
		{
			// Instantiate functor to stop "this" TCP connection
			AcceptorTcpConnectionStopper acceptorTcpConnectionStopper(this);
			// Call functor. Stop TCP connection. Close it. Set handle to 0
			acceptorTcpConnectionStopper(publisher);
			// Register request to delete the instance of the TCP connection
			// when we call the storeNewTcpConnection next time
			++requestForCloseConnectionCounter;
		}


		// ---------------------------------------------------------------------------
		// 3.2.6 Store a new TCP COnnection in internal container
		
		// Store a new TCP connection in the internal list of the Acceptor
		// And, if necessary, do some garbage collection for old and already closed TCP COnnections
		// in our internal list
		template<class TcpConnectionFactory>
		void Acceptor<TcpConnectionFactory>::storeNewTcpConnection(TcpConnectionBase *tcb)
		{
			// Garbage collection of closed TCP connections in accepotrs internal list
			// If there are more dead connections then half the size of the list
			if (requestForCloseConnectionCounter > ((tcpConnection.size())>>2))
			{
				// Then we remove the old stuff
				// Reset the counter, so that we will not call the garbage collector immediately again
				requestForCloseConnectionCounter = null<uint>();
				// Kick out old connections
				removeAndDeleteClosedTcpConnections();
			}
			
			// And now we execute the real intention of this function and add a new TCP connection
			// to the internal list in our acceptor
			tcpConnection.push_back(tcb);
		}





		// ---------------------------------------------------------------------------
		// 3.2.7 Start Acception functionality

		// This function will prepare a network socket for accepting connection requests by other network entities
		template<class TcpConnectionFactory>
		void Acceptor<TcpConnectionFactory>::activate(void)
		{
			// If already active, do nothing
			// If there is already a address available
			
			//ui.msgf("Activate Acceptor portnumber string: %s\n",machineNetworkAddressInfo.portNumberString.c_str());
			if (!isActive() && (!activationOngoing))
			{
				// Reentrance protection
				activationOngoing = true;
			
			
				// Additional Loop run flag. Will be set to false in case of error or if we could find a usable address
				boolean doIterate = true;   
				
				// Now iterate through the linked list of addresses found by sub class NetworkData
				for (NAII addressIterator = machineNetworkAddressInfo.addressInfo.begin();
						(addressIterator != machineNetworkAddressInfo.addressInfo.end()) && doIterate;
						++addressIterator)
				{
					// Open a socket for the found address
					handle = socket((*addressIterator)->ai_family, (*addressIterator)->ai_socktype,(*addressIterator)->ai_protocol);
					// If everything OK
					if (-1 != handle)
					{
						//ui << "Socket created"  << std::endl;
						// Function setsockoptions needs a pointer to the option value, so prepare a storage location
						const sint sintTrue = 1; 
						// if the socket could be opened then set options for the socket. 
						// SO_REUSEADDR allows immediate reuse of socket., meaning, we can accept lots of connection requests 
						// a in short time frame
						sint returnValue = setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &sintTrue, sizeof(sint));
						// If this worked
						if (null<sint>() == returnValue)
						{	
							// Now bind the opened socket to our own network address (IP)
							returnValue = bind(handle, (*addressIterator)->ai_addr, (*addressIterator)->ai_addrlen);
							if (-1 != returnValue) // Check if it worked
							{
								//ui << "Socket bound"  << std::endl;

								// No need to try any other address
								doIterate = false;
							}
							else
							{
								// no, could not bind.
								// Then close the socket and try it with the next address. So continue to iterate
								stop();
							}
						}
						else
						{
							// setsockopt reported an error. Stop.
							stop();
							doIterate = false;
						}
					} // no else. If socket could not be opened,then we will simply continue with the next address
				} // end of for
				
				// If we have a socket to work with
				if ((null<Handle>() != handle) )
				{
					// Socket open and bound
					// Start listening
					if (-1 == listen(handle,10))
					{
						// Could not start listening error: Stop
						stop();
					}
					else
					{
						// Start of listen succeeded
						ui << "Acceptor Activated. Listening started on Port: "  << machineNetworkAddressInfo.portNumberString << std::endl;
						activated = true;
					}
				}
				activationOngoing = false;			
			} // endif
		}

		
		
		// ---------------------------------------------------------------------------
		// 3.2.7 Event handler for incomming connection requests
	
		template<class TcpConnectionFactory>
		EventProcessing::Action Acceptor<TcpConnectionFactory>::handleEvent(const EventType et) 	
		{
			// Event handler for reactor.
			// We are listening on a socket and waiting for a connection request
			// Now we got something
			
			
			// Standard initialisation. Assume Problem
			EventProcessing::Action rc = EventProcessing::Stop;
			
			switch (static_cast<sint>(et))
			{
				case EventTypeIn:
					{
						// The following structures are used in order to be independent from the internet address families
						// e.g. IPV4 or IPV6.  The basic original functions have been designed for IPV4 only. 
						// But now we are in the age of IPV6. And we need to have means to deal with both address types
						// So we have one Data type, that can hold both IPV4 and IPV6 (because it has the length of the 
						// larger IPV6 address). The pointer of this structure can be casted to the original data type
						// that the functions always expected.
						// The first field in the structures denotes the IP Address Family
						
						// This is the big storage that can hold either a IPV4 or a IPV6 address
						typedef struct sockaddr_storage SocketAddressStorage;
						
						// This Type can hold the length of the IP Address container
						typedef socklen_t SocketAddressStorageLength;
						
						
						// This type is the Socket Address that OS function expect. We will cast the pointer of the big
						// data type to this one
						typedef struct sockaddr SocketAddress;
						
						// The next 2 are specific address containers for either IPV4 or IPV6. 
						// One of them will be a part of the big "struct sockaddr_storage"
						typedef struct sockaddr_in SocketAddressInternetIPV4;
						typedef struct sockaddr_in6 SocketAddressInternetIPV6;
						
						
						// We use the big structure that can hold an IPV4 and IPV6 address
						// because we do not know, who is contacting this node
						SocketAddressStorage addressOfCommunicationPartner;
						// Get the length of the above define data structure
						SocketAddressStorageLength socketAddressStorageLength = sizeof(addressOfCommunicationPartner);
						
						
						// Accept the connection request from a client
						// handle is the filedescriptor bound to this node and listening for connection requests
						// The function will return a new file descriptor for the connected socket. This is a specific socket
						// for the just established connection. The handle will continue to listen for more connection requests
						// So this is a factory. We are listening for connection requests and if we get one, we create a new
						// file descriptor for the specific communication purposes
						// The information of the foreign node will be put in the "addressOfCommunicationPartner"
						
						// Accept the connection request from a client
						//lint -e{740,929,1924}
						const Handle connectionHandle = accept(handle, reinterpret_cast<SocketAddress *>(&addressOfCommunicationPartner), &socketAddressStorageLength);
						// Check, if connection could be established and we have a valid file descriptor
						if (connectionHandle > null<Handle>())
						{
							// Now we want to get the IP address of the partner. Can be IPv4 or IPv6
							// The following old style C String can hold both IPv4 and IPv6 address strings
							mchar ipAddressCString[INET6_ADDRSTRLEN+1];
							
							// This is a pointer into the address structure of the communication partner
							// It points either to sin_addr for IPv4 or sin6_addr for IPv6
							const void *ipAddressPEitherV4orV6;

							// This will contain the IP Version as a string
							std::string ipVersion;
							
							// Now check, what family, what type of IP adress we have
							//lint -e{911,1960}
							if (AF_INET == addressOfCommunicationPartner.ss_family)
							{
								// So, it is IPv4. Remember that
								ipVersion = "IPv4";

								// Get a pointer to the appropriate element of the struct, which contains IP address info. And this depending on the IP Family/Type
								//lint --e{740,925,929}     Yes indeed, an unusual pointer cast
								ipAddressPEitherV4orV6 = static_cast<const void *>(  &((reinterpret_cast<const SocketAddressInternetIPV4 *const>(&addressOfCommunicationPartner))->sin_addr)  );
							}
							else
							{
								// It is IPv6. Remember that
								ipVersion = "IPv6";						

								// Get a pointer to the appropriate element of the struct, which contains IP address info. And this depending on the IP Family/Type
								//lint --e{740,925,929}  Yes indeed, an unusual pointer cast
								ipAddressPEitherV4orV6 = static_cast<const void *>(  &((reinterpret_cast<const SocketAddressInternetIPV6 *const>(&addressOfCommunicationPartner))->sin6_addr)  );
							}
							
							// Convert native IP address format to readable C-String
							//lint -e{917,1960}
							if (null<mchar *>() == inet_ntop(addressOfCommunicationPartner.ss_family, ipAddressPEitherV4orV6, ipAddressCString, sizeof(ipAddressCString)))
							{
								// If this did not work then we will not show any IP Address. We can live with that
								ipAddressCString[0] = '\x0';
							}
							
							
							// Debug Output
							{
								static long i=1;
								ui << "Connection accepted " << i << " " << ipVersion << " " << ipAddressCString << " " << machineNetworkAddressInfo.portNumberString << std::endl;
								i++;
							}
							
							
							
							// So. The connection request was established. We gathered all information
							
							// Create a new TCP connection
							TcpConnectionBase *tcpConnectionBase =tcpConnectionFactory->createInstance(machineNetworkAddressInfo.portNumberString, connectionHandle);
							// And put a pointer to it in our internal list of Tcp Connection
							tcpConnection.push_back(tcpConnectionBase);
							
							
							TcpConnectionEhzDataServer *tceds = dynamic_cast<TcpConnectionEhzDataServer *>(tcpConnectionBase);
							if (null<TcpConnectionEhzDataServer *>() != tceds)
							{
								tceds->setEhzSystemDataPointer(tcpConnectionFactory->ehzSystem);
								tceds->setPeerAddressData(ipAddressCString,machineNetworkAddressInfo.portNumberString);
							}
							
							
							// We want to know, when the TCP connection is closed. Then we will remove it from out internal list
							tcpConnectionBase->addSubscription(this);
							
							// Register the new TCP connection in the Reactor. The TCP connection class will then handle all events
							reactorRegisterEventHandler(tcpConnectionBase,EventTypeIn);
							
							rc = EventProcessing::Continue;
						}
						else
						{
							ui << "Could not accept   " << machineNetworkAddressInfo.portNumberString << std::endl;
							waitForKeyPress();

						}
					}
					break;
				default:
					ui << "Handle Event for Acceptor: received event: " << et << std::endl;
					waitForKeyPress();
					rc = EventProcessing::Continue;
					break;
			}
			return rc; 
		}

	}
	

#endif
