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
#include "userinterface.hpp"


#include "server.hpp"
#include "ehz.hpp"
#include "servertcpfactory.hpp"

// Main 

// Handles top most Program 

// There is a client and server version. This is decided via program parameter
// Main calls then either runAsServer or runAsClient
// Both of those functions initialize some variables and then start the main event loop.
// Everything is handled via the main event loop.


// ---------------------------------------------------------------------------------------------------------
// Globals 

// This initializes the complete User Interface
//lint --e(1935,1937)
NCursesUserinterface ui;

//lint -e{641,911}
sint globalDebugMode = DebugModeError;

namespace MainInternal
{

	// Function return codes of Main
	const sint MainReturnCode_OK = 0;
	const sint MainReturnCode_WrongProgramInvocationParameter = -1;
	const sint MainReturnCode_ErrorInEventloop = -2;

	// Protoytpes:
	EventProcessing::Action runMainEventLoop(void);
	sint runAsServer(void);
	sint runAsClient(void);
	boolean checkProgramParameter(const sint argc, mchar *const argv[], boolean &isServer);
		
	// ---------------------------------------------------------------------------------------------------------
	// Main event loop of the whole program
	//
	// This program is fully event triggered
	// There is no proactive function call
	// There are 2 major sources for events:
	// 1. Data is coming from a serial port from an EHZ
	// 2. Activities on network sockets.
	// The event handlers of the respective classes do all the necessary work 
		

	EventProcessing::Action runMainEventLoop(void)
	{
		// Checks the file descriptor on standard input, if data is avaliable and then calls the eventhandler
		//lint --e{1788}
		StandardInputSimple standardInput;	
		
		ui << "Main event loop started" << std::endl;

		EventProcessing::Action returnCodeEventHandler =  EventProcessing::Continue;
		while (returnCodeEventHandler == EventProcessing::Continue)	
		{
			// Call Main Eventhandler, until error or user stop request
			returnCodeEventHandler = Reactor::getInstance()->handleEvents();
		}
		return returnCodeEventHandler;
	}

		
	// ---------------------------------------------------------------------------------------------------------
	// Server functionality
	//
	// Instantiate and initialize needed Classes
	// Start up the functionality
	// Call the main event loop
	// Stop all necessary classes again
		
	sint runAsServer(void)
	{
		// Assume that everything is ok
		sint returnCode = MainReturnCode_OK;

		// Defintion and initilization of event loop return code. 
		EventProcessing::Action returnCodeEventHandler;
		
		const std::vector<EhzConfigDefinition> myVecEhzConfigDefinition(&EhzInternal::myEhzConfigDefinition[0], &EhzInternal::myEhzConfigDefinition[EhzInternal::MyNumberOfEhz]);
		EhzSystem ehzSystem(myVecEhzConfigDefinition);
		// Define a factory class for TCP connection. Depending on the port number
		// a TCP class will be created and run
		TcpConnectionFactoryServerForEhzSystemData tfss(&ehzSystem);
		// Define the server
		Server<TcpConnectionFactoryServerForEhzSystemData> server(&tfss);

		// Reveive bytes from EHZ, Parse them and process the resulting data
		ehzSystem.start();
		// Start the server functionality
		server.start();	
	
		// And run the main event loop 
		returnCodeEventHandler = runMainEventLoop();



		// In case of error, inform calling function
		if (EventProcessing::Error  == returnCodeEventHandler)
		{
			returnCode = MainReturnCode_ErrorInEventloop;
		}
		return returnCode;
	}
		
	// ---------------------------------------------------------------------------------------------------------
	// Client functionality
	//
	// Instantiate and initialize needed Classes
	// Start up the functionality
	// Call the main event loop
	// Stop all necessary classes again

	sint runAsClient(void)
	{
		// We will call this port an ip address
		const std::string portNumber3456("3456");
		const std::string rpi2("192.168.40.150");
//		const std::string rpi2("192.168.40.199");
		
		// Assume that everything is ok
		sint returnCode = MainReturnCode_OK;
		// Defintion and initilization of event loop return code. 
		EventProcessing::Action returnCodeEventHandler;
		
		// Instantiate the client
		ClientWithAutoReconnect<TcpConnectionGetEhzPowerStateClient> client(portNumber3456,rpi2);
		// And start it
		client.start();
		
		// Run the main event loop
		returnCodeEventHandler = runMainEventLoop();
		
		// In case of error, inform calling function
		if (EventProcessing::Error  == returnCodeEventHandler)
		{
			returnCode = MainReturnCode_ErrorInEventloop;
		}
		return returnCode;
	}

	// ---------------------------------------------------------------------------------------------------------
	// Check program invocation options
	//
	// Program may be called  with either
	// 	ehz server
	//or
	// 	ehz client
	// Check this and additionally return the evaluated paramter: isServer

	boolean checkProgramParameter(const sint argc, mchar *const argv[], boolean &isServer)
	{
		// CHeck number of program parameters
		boolean programParameterOK = (2 == argc);
		// If that is OK then
		if (programParameterOK)
		{
			// Look if we called the function with 'server' or client'
			std::string parameter(argv[1]);
			if (parameter == "server")
			{
				ui << "Server Modus" << std::endl;
				isServer = true;
			}
			else if (parameter == "client")
			{
				ui << "Client Modus" << std::endl;
				isServer = false;		}
			else
			{
				programParameterOK = false;
			}
		}
		// If called with wrong parameters, inform user
		if (!programParameterOK)
		{
				ui << "Wrong program parameter\nCall Program with  'ehz  server | client'\nPress key to end" << std::endl;
				waitForKeyPress();	
		}
		return programParameterOK;
	}
}


	
	
// ---------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------

// main

// ---------------------------------------------------------------------------------------------------------

sint main(const sint argc, mchar *const argv[])
{
	sint mainRc = MainInternal::MainReturnCode_OK;
	boolean isServer = false;
	
	ui << "START\n" << cls <<  SetPos(0,0) << "Hello World" << std::endl;	

	// Check parameter
	const boolean programParameterOK = MainInternal::checkProgramParameter(argc,argv,isServer);
	if (!programParameterOK)
	{
		mainRc = MainInternal::MainReturnCode_WrongProgramInvocationParameter;
	}
	else
	{
		//Run program

		if (isServer)
		{
			mainRc = MainInternal::runAsServer();
		}
		else
		{
			mainRc = MainInternal::runAsClient();
		}
	
		ui << "Press key to end" << std::endl;
		waitForKeyPress();
	}
	return mainRc;
}




