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
// eventhandler.hpp
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



#ifndef EVENTHANDLER_HPP
#define EVENTHANDLER_HPP

#include "mytypes.hpp"

#include <unistd.h>
       #include <sys/signalfd.h>
       #include <signal.h>
 
// ------------------------------------------------------------------------------------------------------------------------------
// 1. General Definitions

	// Return codes from a Eventthandler for the reactor
	// Depending on this result, the reactor will either continue with its event handling loop, or terminate
	
	struct EventProcessing
	{
		enum Action
		{
			Stop,		// Stop event handling loop
			Continue,	// Continue event hanling loop
			Error		// Stop event handling loop with Error
		};
	};


// ------------------------------------------------------------------------------------------------------------------------------
// 2. The Event Handler root class

	// The "EventHandler". This is a abstract base class for all kind of EventHandlers. It implements the basic
	// functionalities. It provides the "Handle" and the call back functions.
	// In this implementation we register a pointer to the "complete EventHandler (Other implementations only set
	// a call back function). WIth this approach we want to be more flexible

	class EventHandler
	{
		public:
			EventHandler(void) {};
			virtual ~EventHandler(void) {};
			// pure virtual functions to be implemented be derived classes
			virtual Handle getHandle(void) const = 0;					// return associated handle
			virtual EventProcessing::Action handleEvent(const EventType eventType) = 0;  // event handle function that will be called back
	};


	
// ------------------------------------------------------------------------------------------------------------------------------
// 3. The Event Handler with storage place for the handle and minimum functionality
	
	class EventHandlerBasic : public EventHandler
	{
		public:
			EventHandlerBasic(void) : EventHandler(), handle(null<Handle>()) {}
			virtual ~EventHandlerBasic(void) {}
			// Functions necessary for reactor
			virtual Handle getHandle(void) const { return handle; } // Get the handle
			// Default call back function (does nothing). Must be overwritten
			virtual EventProcessing::Action handleEvent(EventType et) = 0;
		protected:
			Handle handle;
	};


// ------------------------------------------------------------------------------------------------------------------------------
// 4. Event Handler with even more functionality
	
	
	// Helper

	// Base class for a Communication end point. A communication end point can basically open and close files
	// and acts as an Event handler for a reactor. On destruction, open files are closed. 
	class CommunicationEndPoint : public EventHandlerBasic
	{
		public:
			// Default and standard constructor
			// Some classes will open and close the handle. So in the beginning the handle is always 0
			CommunicationEndPoint(void) : EventHandlerBasic() {}
			
			// Other functions may use an already open handle. For this we will provide an explicit constructor
			explicit CommunicationEndPoint(const Handle h) : EventHandlerBasic() {handle=h;}
			
			
			// Standard Destructor. This will close the file (handle)
			virtual ~CommunicationEndPoint(void);
			
			// Open the file (handle) and start up any necessary activities
			// Pure Virtual Function. Needs to be implemented according to file type
			virtual void start(void) = 0;
			// Close the file.
			virtual void stop(void);

			// Default call back function (does nothing). Should be overwritten
			//lint -e{1961}		//Note 1961: virtual member function 'CommunicationEndPoint::handleEvent(short)' could be made const --- Eff. C++ 3rd Ed. item 3
			virtual EventProcessing::Action handleEvent(EventType) { return  EventProcessing::Continue; } 

	};



// ------------------------------------------------------------------------------------------------------------------------------
// 5. Simple Event Handler for standard Input

	// Standard Input Event handler. Events will be caught by Reactor
	class StandardInputSimple : public CommunicationEndPoint
	{
		public: 
			StandardInputSimple(void);
			virtual ~StandardInputSimple(void);

			// No start needed. STDIn is always present
			//lint --e{1961}
			virtual void start(void) {}  	// Do nothing. Always OK
			// Overwrite base class' stop function. We will not close stdin
			virtual void stop(void) {}						// Do nothing
			// Event Handler for Standard input
			virtual EventProcessing::Action handleEvent(const EventType et);
			
		protected:
	};


// ------------------------------------------------------------------------------------------------------------------------------
// 6. Eventhandler for User Interface SIGWINCH events. Meaning. The change of size of our user interface window

class EventHandlerSIGWINCH : public EventHandlerBasic
{
	public:
		EventHandlerSIGWINCH(void);
		~EventHandlerSIGWINCH(void);
		//lint -e{1961}		//Note 1961: virtual member function 'CommunicationEndPoint::handleEvent(short)' could be made const --- Eff. C++ 3rd Ed. item 3
		virtual EventProcessing::Action handleEvent(EventType);	
	
	protected:
	
		typedef sigset_t SignalMaskSet;
		typedef signalfd_siginfo SignalInfoForFileDescriptor;
		
		static const size_t  sizeOfSignalInfoForFileDescriptor = sizeof(SignalInfoForFileDescriptor);
		
		SignalInfoForFileDescriptor signalInfoForFileDescriptor;
		
};


#endif
















