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
// Definition of client server classes
//
// Server
//
// The server is nothing but a container for TCP connections.
// It uses the acceptor-connector defintions, define in another module
// The server uses a list of ports on which it will listen for an
// incoming connection request. All this is handled by sublasses.
// After a successfull connection establisment, a factory is used to
// create a TCP connection. Also this is done by underlying class.
// There can be tons of connections. The server simply stores all connections
// in a container. If some connection is terminated then it will be removed
// from the internal container. 
//
//
//
// Client
//
// The CLient is dedicated to one connection with a specific TCP connection.
// The type of the TCP connection is a template parameter.
// With the "start" function a connection is established and a TCP creation
// is done. All this is handled by sub classes.
//
// There is a derive client function with a autoreconnect feature.
//



#ifndef SERVER_HPP
#define SERVER_HPP

#include "mytypes.hpp"

#include "acceptorconnector.hpp"
#include "reactor.hpp"



#include <set>
#include <string>
#include <vector>
#include <algorithm> 

 
 



// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Server


	// -----------------------------------------------------------------------
	// 1.1 Definition of Type for Port Strings that the server will listen on
	
	// We will use a set to make sure that there is only one unique Port name or number in the list
	typedef std::set<const std::string *>NetworkPortStringSet;




	// -----------------------------------------------------------------------
	// 1.3 The server class

	// Depending on what port numbers are defined in a given set, the server starts an Acceptor for each of them
	// After a connection request is satisfied, the acceptor will create a TCP connection
	// For this it uses the factory class given as template parameter
	// This Server class does only create and start the acceptors. The rest is handled by sub classes
	
	// A template parameter is used to define the relation between the port and the associated Tcp Connection class
	// This is a factory. And it will be passed to the acceptor class.
	
	//lint -e{1712}
	template<class TcpConnectionFactory>
	class Server 
	{
		public:
			// Constructor takes the factory. With that this server will be completely defined
			explicit Server(TcpConnectionFactory * const tcf);
			// Destructor has to close all handles and delete all allocated memory
			virtual ~Server(void);
			// Start the server. Meaning, Crate all Acceptor for each port and start them
			virtual void start(void);
			// Stop all acceptors and existing Tcp Connections
			virtual void stop(void);
			
		protected:
			// Define a container (vector) to pointers to Acceptor classes
			typedef AcceptorConnectorInternal::Acceptor<TcpConnectionFactory> AcceptorP;
			typedef std::vector<AcceptorP *> AcceptorContainer;	
			
			// Pointers to all ports defined in the factory
			NetworkPortStringSet portsToListen;
			
			// Pointer to the listeners for each port
			AcceptorContainer vectorForPointerToAcceptor;
			
			
			// Functors for start and stop functionality
			
			// Functor for start of one acceptor
			struct ServerAcceptorStarter
			{
				ServerAcceptorStarter(void){}
				void operator() (AcceptorP *acceptor){acceptor->start();reactorRegisterEventHandler(acceptor,EventTypeIn);}
			};

			// Functor for stop of one acceptor
			struct ServerAcceptorStopper
			{
				ServerAcceptorStopper(void){}
				
				void operator() (AcceptorP *acceptor) {reactorUnRegisterEventHandler(acceptor);acceptor->stop();}
			};

			// Functor for Creation of acceptor
			//lint -e{1907}
			struct ServerAcceptorCreator
			{
				explicit ServerAcceptorCreator(AcceptorContainer &vectorForPointerToAcceptorR, TcpConnectionFactory *tcpConnectionFactoryp);
				void operator() (const std::string *portNameOrNumber);
				//lint -e{1725}     1725 class member 'Symbol' is a reference
				AcceptorContainer &vectorForPointerToAcceptor;
				TcpConnectionFactory *tcpConnectionFactory;
			};	
			// Functor for destruction of acceptor
			//lint -e{1907}
			struct ServerAcceptorDestructor
			{
				explicit ServerAcceptorDestructor(AcceptorContainer &vectorForPointerToAcceptorR) : vectorForPointerToAcceptor(vectorForPointerToAcceptorR) {}
				void operator() (AcceptorP *acceptor); 
				//lint -e{1725}    1725 class member 'Symbol' is a reference
				AcceptorContainer &vectorForPointerToAcceptor;
			};


	};
	
	// -----------------------------------------------------------------------
	// 1.4. Server class member functions
	

		// -------------------------------------------------------------------
		// 1.4.1 Constructor for Functor for creating an Acceptor
		
		// Constructor does only store its parameters in internal variable
		template<class TcpConnectionFactory>
		inline ::Server<TcpConnectionFactory>::ServerAcceptorCreator::ServerAcceptorCreator(AcceptorContainer &vectorForPointerToAcceptorR, TcpConnectionFactory *tcpConnectionFactoryp) :
				vectorForPointerToAcceptor(vectorForPointerToAcceptorR), tcpConnectionFactory(tcpConnectionFactoryp) 
		{
		}

		// -------------------------------------------------------------------
		// 1.4.2 Operator for Functor for creating an Acceptor
		
		template<class TcpConnectionFactory>
		inline void Server<TcpConnectionFactory>::ServerAcceptorCreator::operator() (const std::string *portNameOrNumber) 
		{ 
			// Create a new acceptor on the heap.
			// Pass port number and TCP creation factory as parameter
			this->vectorForPointerToAcceptor.push_back(new AcceptorConnectorInternal::Acceptor<TcpConnectionFactory> (*portNameOrNumber,this->tcpConnectionFactory));
		}

		// -------------------------------------------------------------------
		// 1.4.3 Operator for Functor for Destructing an Acceptor

		template<class TcpConnectionFactory>
		inline void Server<TcpConnectionFactory>::ServerAcceptorDestructor::operator() (AcceptorP *acceptor) 
		{
			// Define a functor for stopping the acceptor functionality
			ServerAcceptorStopper serverAcceptorStopper;
			// And stop the acceptor 
			serverAcceptorStopper(acceptor);

			// Acceptor has been stopped. Now delete the associated memory on the heap.
			delete acceptor;
		}


		// -------------------------------------------------------------------
		// 1.4.4 Constructor for Server
		
		// Gets a pointer to the factory. It checks the portnumber strings available in the factory
		// For each of those it creates an Acceptor. The Acceptor is registered with the reactor.
		// When the Acceptor establishes a connection, the event function for the Acceptor is called. 
		// The Acceptor notifies the server that it got a new Handle. The Server will then create
		// a TCP connection and store that in its internal array
		
		template<class TcpConnectionFactory>
		inline Server<TcpConnectionFactory>::Server(TcpConnectionFactory *const tcf ) : portsToListen(), vectorForPointerToAcceptor()
		{
			// The factory knows, which TcpConnections with what ports it can create
			// We need all possible port name / numbers. Get them
			tcf->GetPortNamesOrNumbers(portsToListen);
			
			// Build List with Acceptors based on the learned port name/numbers and register them
			// Iterate though portsToListen, create acceptors and add them to internal field
			ServerAcceptorCreator serverAcceptorCreator(vectorForPointerToAcceptor,tcf);
			//lint -e{534,1901,1911}
			std::for_each(portsToListen.begin(), portsToListen.end(),serverAcceptorCreator);
		}

		
		// -------------------------------------------------------------------
		// 1.4.5 Destructor for Server

		template<class TcpConnectionFactory>
		inline Server<TcpConnectionFactory>::~Server() 
		{
			try
			{
				
				// Stop and delete all acceptors
				ServerAcceptorDestructor serverAcceptorDestructor(vectorForPointerToAcceptor);
				//lint -e{534,1901,1911}
				std::for_each(vectorForPointerToAcceptor.begin(),vectorForPointerToAcceptor.end(),serverAcceptorDestructor);
			}
			catch(...)
			{
			}
		}

		
		
		// -------------------------------------------------------------------
		// 1.4.6 Start the server
		// Start each existing acceptor
		template<class TcpConnectionFactory>
		inline void Server<TcpConnectionFactory>::start(void)
		{
			//lint -e{1502}
			ServerAcceptorStarter serverAcceptorStarter;
			//lint -e{534,1901,1911}
			std::for_each(vectorForPointerToAcceptor.begin(),vectorForPointerToAcceptor.end(),serverAcceptorStarter);
		}

		// -------------------------------------------------------------------
		// 1.4.7 Stop the server
		// Stop each existing Acceptor
		template<class TcpConnectionFactory>
		inline void Server<TcpConnectionFactory>::stop(void)
		{
			//lint -e{1502}
			ServerAcceptorStopper serverAcceptorStopper;
			//lint -e{534,1901,1911}
			std::for_each(vectorForPointerToAcceptor.begin(),vectorForPointerToAcceptor.end(),serverAcceptorStopper);
		}






// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Client

	// ---------------------------------------------------------------------------
	// 2.1 A Client for connecting to a server and establishing a TCP connection

	// Establish a network connection to a given IP address on a given port
	// Then create a TCP connection of type <TcpConnectionType>
	//lint -e{1712}    Default constructor
	template<class TcpConnectionType>
	class Client : 	public Subscriber<AcceptorConnectorInternal::Connector>,
					public Subscriber<TcpConnectionBase>
	{
		public:
			// COnstrutor: Specify IP address of peer to connect to and the port
			Client(const std::string &portNameOrNumber, const std::string &IpAddressOrHostName);
			// Destructor tops any existing connection
			virtual ~Client(void);
			// Connect to the peer and establish a TCP Connection
			virtual void start(void);
			// Stop the TCP connection and disconnect from peer
			virtual void stop(void);
			
		protected:
			//lint --e{1768}   1768 Virtual function 'Symbol' has an access (String) different from the access (String) in the base class (String
			// Here we receive an information from the connector that a connection to a peer has been made
			// Then we can create the TCP connection
			virtual void update(AcceptorConnectorInternal::Connector *const publisher) ;
			
			// Information from the TCP connection class that a TCP connection has been terminated for whatever reason
			//lint -e{1960}
			virtual void update(TcpConnectionBase *const tcpConnectionBase);
			// Pointer to the dynamically created TCP connection
			TcpConnectionType *tcpConnection;
			// The Connector does the ground work
			AcceptorConnectorInternal::Connector connector;
			// Indicator for an active TCP connection
			boolean tcpConnectionActive;
	};
	
	
	// ---------------------------------------------------------------------------
	// 2.2 Client member functions


		// --------------------------
		// 2.2.1  Client Constructor

		// Take IP address of peer and the port for a desired connection
		template<class TcpConnectionType>
		inline Client<TcpConnectionType>::Client(const std::string &portNameOrNumber, const std::string &IpAddressOrHostName) : 
			Subscriber<AcceptorConnectorInternal::Connector>(),
			Subscriber<TcpConnectionBase>(),
			tcpConnection(null<TcpConnectionType *>()),
			connector(portNameOrNumber, IpAddressOrHostName), // Initialize connector. 
			tcpConnectionActive(false)
		{
		}
		
		// --------------------------
		// 2.2.2  Client Destructor
		// Stop TCP Connection and connector
		template<class TcpConnectionType>
		inline Client<TcpConnectionType>::~Client(void)
		{
			try
			{
				//lint -e{1933,1506}
				stop();
				// Deallocate dynamic memory for TCP Connection
				delete tcpConnection;
			}
			catch(...)
			{
			}

		}
		
		// --------------------------
		// 2.2.3  Start the client
		// Start the connector
		// After the connection is existing, the connector will call the update function below
		template<class TcpConnectionType>
		inline void Client<TcpConnectionType>::start(void)
		{		
			// ui << "Client::Start Client" << std::endl;
			// We want to receive messages from the connector
			connector.addSubscription(this);
			// Connect to peer
			//lint -e{534}
			connector.start();
		}

		// --------------------------
		// 2.2.4  Stop the client
		template<class TcpConnectionType>
		inline void Client<TcpConnectionType>::stop(void)
		{
			// If there is a TCP connection existing
			if (null<TcpConnectionType *>() != tcpConnection)
			{
				// No more events for TCP connections
				reactorUnRegisterEventHandler(tcpConnection);
				// Client does not want to have an messages from TCP connection
				tcpConnection->TcpConnectionBase::removeSubscription(this);
				// Stop and close the TCP Connection
				tcpConnection->stop();
			}
			else
			{
				// Nothing
			}
			// We do also not need to handle events from the connector any longer
			connector.AcceptorConnectorInternal::Connector::removeSubscription(this);
			// Stop and close the connector
			connector.stop();
			// And indicate that the connection is no longer active
			tcpConnectionActive = false;
		}


		// -----------------------------------
		// 2.2.5  Client update from connector
		
		// The connector made a connection to the peer
		// Now we want to create a TCP connection
		template<class TcpConnectionType>
		inline void Client<TcpConnectionType>::update(AcceptorConnectorInternal::Connector *const publisher)
		{	
			// Create a TCP connection class on the heap. Hand over the handle provided by the connector
			tcpConnection = new TcpConnectionType(publisher->getHandle());
			// We, this client, want to receive updates from the TCP connection. We want to know, when it is terminated
			tcpConnection->TcpConnectionBase::addSubscription(this);
			// The eventhandler shall handle events from the TCP connection. So, get data
			reactorRegisterEventHandler(tcpConnection,EventTypeIn);
			// Start the TCP connection
			tcpConnection->start();	
			tcpConnectionActive = (null<Handle>() != tcpConnection->getHandle());
		}

		// -----------------------------------------
		// 2.2.6  Client update from TCP connection
		
		// This means, the TCP connection has been terminated
		// Stop the Client
		template<class TcpConnectionType>
		inline void Client<TcpConnectionType>::update(TcpConnectionBase *const)
		{	
			ui << "Client: TCP Connection Update will call stop" << std::endl;
			//lint -e{1933}
			stop();
		}


	// ---------------------------------------------------------------------------
	// 2.3 A Client for connecting to a server and establishing a TCP connection
	// With an automatic reconnect mechanism for broken connections
	
	// Class definition. Same liek preivios class with the extension of an autoreconnect
	//lint -e{1712}    Default constructor
	template<class TcpConnectionType>
	class ClientWithAutoReconnect : public Client<TcpConnectionType>,
									public Subscriber<EventTimer>
	{
		public:
			// This constructor takes, as the base class, an IP address, a port and, additionally, a timer value in ms for the reconnect-try period
			ClientWithAutoReconnect(const ::std::string &portNameOrNumber, const ::std::string &IpAddressOrHostName, const ::u32 reconnectPeriod = 15000UL);
			// Destructor. Addtionally stops the timer
			virtual ~ClientWithAutoReconnect(void);
			// Start all Client functiality
			virtual void start(void);
			
		protected:
			// This function will be invoked when the timer fires.
			//lint -e{955,1768}
			// Note 955: Parameter name missing from prototype for function 'ClientWithAutoReconnect<<1>>::update'
			// Info 1768: Virtual function 'ClientWithAutoReconnect<<1>>::update(EventTimer *)' has an access (protected) different from the access (public) in the base class (Subscriber)
			virtual void update(::EventTimer *const);
			// And the timer
			::EventTimer reconnectTimer;
	};
	
	// ---------------------------------------------------------------------------
	// 2.4 ClientWithAutoReconnect member functions
	
		// -----------------------------------
		// 2.4.1  Simple Constructor
		
		//lint -e{1942}   // unqualified name 'EventTimer' subject to misinterpretation owing to dependent base class [MISRA C++ Rule 14-6-1]
		template<class TcpConnectionType>
		inline ClientWithAutoReconnect<TcpConnectionType>::ClientWithAutoReconnect(const ::std::string &portNameOrNumber, const ::std::string &IpAddressOrHostName, const ::u32 reconnectPeriod) :
			::Client<TcpConnectionType>::Client(portNameOrNumber, IpAddressOrHostName),
			::Subscriber<EventTimer>(),
			reconnectTimer(reconnectPeriod)
		{
		}
		
		// --------------------------------------------------------------
		// 2.1.1  Destructor. Stop Timer and then the rest of the client
		template<class TcpConnectionType>
		inline ClientWithAutoReconnect<TcpConnectionType>::~ClientWithAutoReconnect(void)
		{		
			try
			{
				//::ui << "ClientWithAutoReconnect Destructor"  << ::std::endl;
				// Stop the Timer
				reconnectTimer.stopTimer();
				// Client does not want to have updates from the Timer
				reconnectTimer.removeSubscription(this);
				
				// Calls automatically base class stop function to stop the client
			}
			catch(...)
			{
			}
		}
		
		
		// --------------------------------------------------------------
		// 2.4.2  Start Timer and rest of Client
		template<class TcpConnectionType>
		inline void ClientWithAutoReconnect<TcpConnectionType>::start(void)
		{
			// We want to have updates from Timer
			reconnectTimer.addSubscription(this);
			// Start the timer
			reconnectTimer.startTimerPeriodic();
			// Start the functionality of the Client
			//lint -e{1942}   // unqualified name 'EventTimer' subject to misinterpretation owing to dependent base class [MISRA C++ Rule 14-6-1]			
			Client<TcpConnectionType>::start();
		}
		
		// --------------------------------------------------------------
		// 2.4.3  Called, when timer fires
		template<class TcpConnectionType>
		inline void ClientWithAutoReconnect<TcpConnectionType>::update(::EventTimer *const)
		{
			// If the connection is not active
			if (! (this->tcpConnectionActive))
			{
				// Try to restart connection
				::ui << "Restart TCP Connection"  << ::std::endl;
				//lint -e{1942}   // unqualified name 'EventTimer' subject to misinterpretation owing to dependent base class [MISRA C++ Rule 14-6-1]
				Client<TcpConnectionType>::start();
			}
		}
		
 	

#endif
