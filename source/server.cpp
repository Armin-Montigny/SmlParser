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
// In this module we define on which ports we can operate and what
// type of TCP connection is associated with what port.
//




 
#include "server.hpp"
#include "servertcpfactory.hpp"



// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Factory for creating TCP connections

	// -----------------------------------------------------------------------
	// 1.1 Definition of the create functions for the factory

	//lint -save -e957   Note 957: Function 'xxx' defined without a prototype in scope
	
	//TcpConnectionBase* createTcpConnectionFor5678(Handle h) { return new TcpConnectionWithAsync(h); }
	//TcpConnectionBase* createTcpConnectionFor3456Ehz(Handle h) { return new TcpConnectionForEhzData(h); }

	TcpConnectionBase* createTcpConnectionSimpleHtmlAnswer(const Handle h) { return new TcpConnectionSimpleHtmlAnswer(h); }
	TcpConnectionBase* createTcpConnectionSimpleHtmlAnswer2(const Handle h) { return new TcpConnectionSimpleHtmlAnswer(h); }
	TcpConnectionBase* createTcpConnectionEhzDataServer(const Handle h) { return new TcpConnectionEhzDataServer(h); }
	TcpConnectionBase* createTcpConnectionEhzPowerStateServer(const Handle h) { return new TcpConnectionEhzPowerStateServer(h); }
	TcpConnectionBase* createTcpConnectionSimpleHtmlAnswerPowerState(const Handle h) { return new TcpConnectionSimpleHtmlAnswerPowerState(h); }

	//lint -restore
	
	// -----------------------------------------------------------------------
	// 1.2 Constructor
	
	// Build the the selector - creator map
	// For each port that the server wants to listen on, we will create a TCP Connection of dedicated type
	TcpConnectionFactoryServerForEhzSystemData::TcpConnectionFactoryServerForEhzSystemData(EhzSystem *const ehzSystemP) : FactoryWithConstructorParameter<std::string, TcpConnectionBase, Handle>(), ehzSystem(ehzSystemP)
	{
		//lint --e{1901,1911}
		// Note 1901: Creating a temporary of type 'const std::basic_string<char>'
		// Note 1911: Implicit call of constructor 'std::basic_string<char>::basic_string(const char *, const std::allocator<char> &)' (see text)
		//choice["80"] = &createTcpConnectionSimpleHtmlAnswer2;	
		choice["5678"] = &createTcpConnectionEhzDataServer;
		choice["3456"] = &createTcpConnectionEhzPowerStateServer;
		choice["9876"] = &createTcpConnectionSimpleHtmlAnswer;
		choice["3457"] = &createTcpConnectionSimpleHtmlAnswerPowerState;
	}

	// -----------------------------------------------------------------------
	// 1.2 Build list of valid port numbers for the server to listen on
	
	void TcpConnectionFactoryServerForEhzSystemData::GetPortNamesOrNumbers(NetworkPortStringSet &portList) 
	{
		//lint --e{1901,1911,1702}
		//Note 1901: Creating a temporary of type 'std::_Rb_tree_const_iterator<std::pair<const std::basic_string<char>,TcpConnectionBase *(*)(int)>>'
		//Note 1911: Implicit call of constructor 'std::_Rb_tree_const_iterator<std::pair<const std::basic_string<char>,TcpConnectionBase *(*)(int)>>::_Rb_tree_const_iterator(const std::_Rb_tree_iterator<std::pair<const std::basic_string<char>,TcpConnectionBase *(*)(int)>> &)' (see text)
		//Info 1702: operator 'operator!=' is both an ordinary function 'std::operator!=(const std::pair<<1>,<2>> &, const std::pair<<1>,<2>> &)' and a member function
		//'std::_Rb_tree_const_iterator<std::pair<const std::basic_string<char>,TcpConnectionBase *(*)(int)>>::operator!=(const std::_Rb_tree_const_iterator<std::pair<const
		//std::basic_string<char>,TcpConnectionBase *(*)(int)>> &) const'
		
				
		for (std::map<std::string, ElementCreator>::const_iterator it = choice.begin(); it != choice.end(); ++it)
		{
			// If it is already in the set, then fine. No error. Therefore we do not check for the return value.
			//ui.msgf("GetPortNamesOrNumbers:  Add '%s' from map to set\n",it->first.c_str());
			//lint -e{534}    534 Ignoring return value of function 'Symbol
			portList.insert(&(it->first));
		}
	}



