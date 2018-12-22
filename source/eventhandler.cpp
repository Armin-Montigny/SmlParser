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
// eventhandler.cpp
//
// We are using the Reactor pattern for general event handling.
// In module reactor.hpp an reactor class is declared.
//
// The reactor uses EventHandler as call back mechanism for found events.
// This is done with a general base class EventHandler and derived subclasses
// The eventhandler implements 2 minimum and needed functionalities:
//
// 1. GetHandle: 	Defined the handle that the reactor will work with
// 2. handleEVent:	The reactor will call handleEvent when he detects activities on the handle
//
// So the reactor does not know about handles (file descriptors) in the first place.
// We will register a class that is derived from EventHandler with the reactor 
// The reactor then call getHandle an checks for upcomining activities
// Then,, if there was an activity, it will call the EventHandler
//
// So, if any class needs eventhandling, derive it from EventHandler and implement
// the 2 needed functions
//
// We provide an additional class EventHandlerBasic that stores an handle and implements
// the getHandle function already. This maybe uses for classes that do not already have
// an implicit handle.
//
// To make things even more convienient we define a class CommunicationEndPoint
// that adds start and stop functionality, deals with a handle
// and will close the handle on destruction
//
// So, typically you would derive from this class for a standard end to end communication 
// using handles (file descriptors)
//
// And as one example, we define an EventHandler for standard input as a CommunicationEndpoint
//
//
//
// In this module only a few member functions will be defined
//

#include "eventhandler.hpp"
#include "userinterface.hpp"
#include "reactor.hpp"
 
#include <errno.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------------------------------------
// 1. CommunicationEndPoint stop and destruction

	// ---------------------------------------------
	// 1.1 Stop function for CommunicationEndPoint

		// close the handle and indicate that it is closed by setting handle to 0
		void CommunicationEndPoint::stop(void)
		{
			if (null<Handle>() != handle) 
			{
				close(handle);
			} 
			handle = null<Handle>(); 
		}
		
		
	// ---------------------------------------------
	// 1.2 Destructor. Class stop
		
		CommunicationEndPoint::~CommunicationEndPoint(void) 
		{ 
			try 
			{ 
				// close handle
				CommunicationEndPoint::stop();
			}
			catch(...)
			{
				try
				{
					// Set handle to 0 in any case
					handle = null<Handle>();
				}
				catch(...)
				{
				}
			}
		}



// ------------------------------------------------------------------------------------------------------------------------------
// 2. Event Handler for standard input
		
	// At this moment, only use to stop the whole application

		
	//lint -e{1961}
	EventProcessing::Action StandardInputSimple::handleEvent(const EventType et) 
	{ 
		// Default is to continue the mainevent loop
		
		//lint --e{921}
		EventProcessing::Action rc = EventProcessing::Continue;
		switch (static_cast<sint>(et))
		{
			case EventTypeIn:
				{	
					// Read key
					const sint ch = getch();
					// Debug message
					ui << "Standard Input. Key: "<< ch << "  '" << static_cast<mchar>(ch) << "'" << std::endl;
					
					// Depending on key press
					switch(ch)
					{
						// if somebody pressed 'q'  (for quit)
						case static_cast<sint>('q'):
							// Debug message
							ui << "EventProcessing --> Stop" << std::endl;
							// STop reactors main event loop. Terminate programm
							rc = EventProcessing::Stop;
							break;
							
						// Toggle Debug Mode
						case 'd':
							++globalDebugMode;
							//lint -e{641,911}
							if (DebugModeMax <= globalDebugMode)
							{
								globalDebugMode = 0;
							}
							ui.resizeWindows();
							break;
						default:
							// do nothing for other key
							break;
					}
				}
				break;
			default:
				// do nothing for other event
				break;
		}
		return rc; 
	}

	
// ------------------------------------------------------------------------------------------------------------------------------
// 3. Constructor / Destructor for standard input
	
	StandardInputSimple::StandardInputSimple(void) : 	CommunicationEndPoint(STDIN_FILENO) 
	{	
		reactorRegisterEventHandler(this,EventTypeIn);
	}
	
	StandardInputSimple::~StandardInputSimple(void)
	{	
		try
		{
			reactorUnRegisterEventHandler(this);
		}
		catch(...)
		{
		
		}
	}
	
	

	EventHandlerSIGWINCH::EventHandlerSIGWINCH(void) : EventHandlerBasic(),signalInfoForFileDescriptor()
	{
		SignalMaskSet signalMaskSet;
		
		// Initialiaue Mask
		sigemptyset(&signalMaskSet);	//lint !e534
		
		// We are interested in chang window size (of ther terminal) signals
		sigaddset(&signalMaskSet, SIGWINCH);	//lint !e534
		
		// Block signals so that they aren't handled according to their default dispositions
		sigprocmask(SIG_BLOCK, &signalMaskSet, null<SignalMaskSet *>());	//lint !e534
		
		// Creat fd for signal
		handle = signalfd(-1, &signalMaskSet, 0);
		if (null<Handle>() >= handle)
		{
			// Incase of error do nothing
			handle = 0;
		}
		else
		{
			// Regsiter handler with reactor
			reactorRegisterEventHandler(this,EventTypeIn);
		}
	}

	
	EventHandlerSIGWINCH::~EventHandlerSIGWINCH(void)
	{
		try
		{
			if (null<Handle>() != handle)
			{
				reactorUnRegisterEventHandler(this);
				close(handle);
			}
		}
		catch(...)
		{
		
		}
	}	
	
EventProcessing::Action EventHandlerSIGWINCH::handleEvent(const EventType et) 
{ 
	EventProcessing::Action rc = EventProcessing::Continue;
	//lint -e{911}   Note 911: Implicit expression promotion from short to int
	// Check returned Event type. We asked for EventTypeIn  (POLLIN)
	if (EventTypeIn == et)
	{
		// Data should be available. Read the data. Of course we have a top cap with buffer size.
		// Reading is done buffered in chunks
		const sint bytesRead = read(handle, &signalInfoForFileDescriptor, sizeOfSignalInfoForFileDescriptor);
		// Did we read something
		if (bytesRead > null<sint>())
		{
			//lint -e{1933,912,1960}   
			// Process the read data.
			if (SIGWINCH == signalInfoForFileDescriptor.ssi_signo)
			{
				ui.reInitialize();					
			}
		}
		else
		{
			// Read return value was 0 or negative
			rc = EventProcessing::Error;
			ui << "Read Signal SIGWINCH Error "<< bytesRead << " Error Number: " << errno<< " "<< strerror(errno);
		}
	}
	else
	{
		// For Events like POLLERR or POLLHUP or POLLRDHUP or something
		// Do nothing
	}
	return rc; 	
}
