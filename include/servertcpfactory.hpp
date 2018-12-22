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



#ifndef SERVERTCPFACTORY_HPP
#define SERVERTCPFACTORY_HPP

#include "mytypes.hpp"
#include "factory.hpp"

#include "acceptorconnector.hpp"



#include <set>
#include <string>

 
	
	// We will use a set to make sure that there is only one unique Port name or number in the list



	
	// -----------------------------------------------------------------------
	// 1.2 Factory Class for creating TCP connection based on the Port number
	
	// Factory for Creating Tcp Connection classes
	class TcpConnectionFactoryServerForEhzSystemData : public FactoryWithConstructorParameter<std::string, TcpConnectionBase, Handle>
	{
		//lint --e{1925,1540}
		public:
			// Constructor will fill the map of Ports and associated creator functions
			explicit TcpConnectionFactoryServerForEhzSystemData(EhzSystem *const ehzSystemP);
			
			// Destructor does nothing
			virtual ~TcpConnectionFactoryServerForEhzSystemData(void) {}
			
			// With this function we get all the port number strings which are available
			// The server can use this strings to start listening on this ports
			// It makes sense to listen only on a port where a TCP connection is defined . . .
			virtual void GetPortNamesOrNumbers(std::set<const std::string *> &portList);
			// The source of data that we want to serve
			EhzSystem *ehzSystem;			
		protected:
		

		private:
			TcpConnectionFactoryServerForEhzSystemData(void) : FactoryWithConstructorParameter<std::string, TcpConnectionBase, Handle>(), ehzSystem(null<EhzSystem *>()) {}
	};


#endif
