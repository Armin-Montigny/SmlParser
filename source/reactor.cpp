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
// This module implements the “Reactor” design pattern intended for synchronous event handling. The basis 
// for the implementation is the work from Douglas C Schmidt already published 1995 in the course of 
// developing high performance web servers. For a description of Schmidt’s work, please refer to 
// https://www.dre.vanderbilt.edu/~schmidt/PDF/Reactor.pdf .
//
// Please note: There is a second design pattern for high load web servers using asynchronous operations: 
// The so called “Proactor” pattern.
//
// The implementation is close to the description of Schmidt. The main elements are:
//
//
// 1.Handle
//
// The “Handle” is simply an already open file descriptor. The “SynchronousEventDemultiplexor” checks, if an 
// “Event” on that descriptor occurred.
//
//
// 2. EventHandler
//
// Owner of a “Handle”. If there is an “Event” for the associated “Handle”, then the “EventHandler” is 
// invoked and deals with that event. The “EventHandler” is implemented as an abstract base class. The 
// main interface consists of:
//    Handle getHandle(void) = 0;
//    EventProcessing::Action handleEvent(const EventType eventType) = 0;
// The “handleEvent” function is called by the “InitiationDispatcher” through the “SyncronousEventDemultiplexor”.
// This is implemented using a call back mechanism.
//
//
// 3. ConcreteEventHandler
//
// The “ConcreteEventHandler” is derived from the abstract “EventHandler”. It finally provides the 
// functionality needed. An example is the “CommunicationEndPoint” class which is itself an abstract 
// base class for specific communication instances using serial ports or sockets. All of them have to 
// implemented the “handleEvent” function and do the appropriate things for the “Event”.
//
//
// 4. InitiationDispatcher
//
// The “InitiationDispatcher” is a container for “EventHandler”s. “EventHandler”s can be registered and
// unregistered with the “InitiationDispatcher”. The “InitiationDispatcher” will handle the events through
// the “SyncronousEventDemultiplexor”. It is providing the call back functionality. The main interface is:
//    void registerHandler(EventHandler *ev, EventType et);
//    void unregisterHandler(EventHandler *ev);
//    EventProcessing::Action handleEvents(void);
// Interested components will register their “EventHandler”s. The “InitiationDispatcher” will then take care 
// for handling all upcoming “Events”. Eventually the call back function along with the found “Events” will 
// be called. Of course “EventHandlers” that are not needed any longer can be unregistered.
//
//
// 5. SynchronousEventDemultiplexor
// 
// The “SynchronousEventDemultiplexor” uses operation system support to detect activities on “Handle”s. 
// It is a blocking entity and waits for something to happen. Then the Event is de-multiplexed and assigned
// to the corresponding “EventHandler”. 
//
// Since operating systems have different mechanism to implement such functionality we use a static Strategy 
// Pattern to define and use the most fitting solution. Many operating systems have the function “select” 
// for that purpose. Windows uses “WaitForMultipleObjects” and I selected the Linux compatible “poll” function.
// If some other implementation is wished then you should derive a class from 
// “SynchronousEventDemultiplexorImplementation” and do your own implementation. 
//
//
// 6. Reactor
// 
// All needed and before defined functionalities are wrapped in one class. The Reactor.
// This class serves as the main interface to the outer world.
//
//
// The reactor will be used to listen on raw sockets for network connections, to get data from serial ports or
// other data sources, like pipes or fifos. 



#include "userinterface.hpp"

#include "reactor.hpp"

#include <errno.h>
#include <string.h>

#include <algorithm>
#include <cxxabi.h>

namespace ReactorInternal
{

// ------------------------------------------------------------------------------------------------------------------------------
// 1. Initialization

	// ctor for InitiationDispatcher
	// It sets an internal reference (pointer) to the used "SynchronousEventDemultiplexor", because we will use its
	// handleEvent function. Additionally the flag for updating the registered descriptor list is set to true.
	// So in the main event loop of the "SynchronousEventDemultiplexor" the descriptor list will be rebuild before
	// the blocking call to wait for events
	InitiationDispatcher::InitiationDispatcher(SynchronousEventDemultiplexorImplementation *const sedi) : registrationDataContainer(), 
																									updateHandler(true), 
																									synchronousEventDemultiplexor(sedi)
	{
	}


	// Default ctor for InitiationDispatcher
	InitiationDispatcher::InitiationDispatcher(void) : registrationDataContainer(), 
														updateHandler(false), 
														synchronousEventDemultiplexor(null<SynchronousEventDemultiplexorImplementation *>())
	{
	}

	
	
	
// ------------------------------------------------------------------------------------------------------------------------------
// 2. Event Handling, general
	

	// 2.1 -------------------
	// Main Event Handler
	
	// Handle Events function of the "InitiationDispatcher" will hand over control to the main event loop of
	// the "SynchronousEventDemultiplexor". Additionally a rebuild of the descriptor list may be requested
	// This function will usually not return until the end of the program
	EventProcessing::Action InitiationDispatcher::handleEvents(void)
	{
		EventProcessing::Action rc = EventProcessing::Error;
		// NULL Pointer prevention
		if (null<SynchronousEventDemultiplexorImplementation *>() != synchronousEventDemultiplexor)
		{
			// Wait for events to happen (synchronously), demultiplex AND dispatch them (call the handlers)
			// The updateHandler flage indicates the a new handler has been registered or an existing
			// handler has been unregistered. In this case the SynchronousEventDemultiplexor mus
			// update its internal structures
			rc = synchronousEventDemultiplexor->handleEvents(registrationDataContainer, updateHandler);
		}
		return rc;
	}

	
	
	// ----------------
	// 2.2 Register Handler

	
	// Register a EventHandler. A pointer to the class will be stored here. The pointer is of the type
	// of the abstract base class. With that, all derived classed can be handled here
	// The pointer to the class and the event that should be monitored is given as a parameter
	void InitiationDispatcher::registerHandler(EventHandler *const ev,  const EventType et)
	{
		registrationDataContainer[ev] = et; 
		// Because we made modifications to the "EventHandler" container, the
		// SynchronousEventDemultiplexor needs to rebuild the descriptor list
		updateHandler = true;
		
		//Maybe the synchronousEventDemultiplexor wants to know
		synchronousEventDemultiplexor->registerHandlerInfo(ev, et);

	}


	
	
	// ------------------
	// 2.3 Unregister Handler

	
	// Unregister an EventHandler. Look if EventHandler is in the container. If
	// so, then remove it
	void InitiationDispatcher::unregisterHandler( EventHandler *const ev)
	{
		const RegistrationDataContainer::size_type eraseResult = registrationDataContainer.erase(ev);
		if  (null<RegistrationDataContainer::size_type>()  < eraseResult)
		{
			updateHandler = true;
			//Maybe the synchronousEventDemultiplexor wants to know
			synchronousEventDemultiplexor->unregisterHandlerInfo(ev);
		}
		else
		{
		/*
			int status;
			char * demangled = abi::__cxa_demangle(typeid(*ev).name(),0,0,&status);
			ui << demangled <<":   Double or unknown eventhandler unregister" << std::endl;
			free(demangled);
			*/
		}
	}


// ------------------------------------------------------------------------------------------------------------------------------
// 3. PPOLL  -  SynchronousEventDemultiplexor

	// --------------
	// 3.1 Initialization

	// Concrete Implementation of "SynchronousEventDemultiplexor"
	// Linux function ppoll will be used here. The ppoll function needs a list of file descriptors and events that
	// shall be monitored. Additionally a timeout may be used
	//lint -e{915}   915 Implicit conversion (Context) Type to Type
	SynchronousEventDemultiplexorImplementationUsingPoll::SynchronousEventDemultiplexorImplementationUsingPoll(void) :
																									SynchronousEventDemultiplexorImplementation(),
																									pollHandleProperty(null<PollHandleProperty *>()),
																									pollTimeSpeccification() 
	{
		//Set time out time
		pollTimeSpeccification.tv_sec = 60L;
		pollTimeSpeccification.tv_nsec = 0L;
	}

	
	
	// ---------------------------------------------
	// 3.2 Specific function for PPOLL Set up properties
	
	// Concrete Implementation of "SynchronousEventDemultiplexor"
	// Linux function ppoll will be used here. The ppoll function needs a list of file descriptors and events that
	// shall be monitored. This function builds the necessary arrays for the ppoll function.
	void SynchronousEventDemultiplexorImplementationUsingPoll::updatePollHandlerPropertyList(RegistrationDataContainer &rdc, boolean &updateHandler)
	{
		// If there is something to do (So if there was a new registration / unregistration of an EventHandler) . . .
		if (updateHandler)
		{
			//ui << "Begin delete [] pollHandleProperty;" << std::endl;
		
			// then delete the old list
			delete [] pollHandleProperty;
			// Get the number of registered event handlers

			const uint numberOfElements = rdc.size();

			//ui << "const uint numberOfElements = rdc.size();  " <<  numberOfElements << std::endl;

			
			// And build an empty new array for the descriptors and event handlers
			//lint -e{9035}    //9035   Variable length array declared 
			pollHandleProperty = new PollHandleProperty[numberOfElements];
			
			// Now fill the array and assign values to Elements
			uint i = null<uint>();
			//lint -e{1702}  //1702 operator 'Name' is both an ordinary function 'String' and a member function 'String'
			for (RdcIterator rdci = rdc.begin(); rdci != rdc.end(); ++rdci)
			{
				pollHandleProperty[i].fd = (*rdci).first->getHandle();    //lint !e1960
				// Set event that shall be monitored
				pollHandleProperty[i].events = (*rdci).second;	//lint !e1960
				++i;
			}
			
			// Update done. No need to update the next time (as long as there is no change in the
			// list of registered handlers)
			updateHandler = false;
		}
	}

	
	
	// -----------------------------
	// 3.3 Eventhandler using PPOLL
	
	// Concrete Implementation of "SynchronousEventDemultiplexor"
	// Linux function ppoll will be used here.
	//
	// Main event Loop
	//
	// Tasks to do here:
	//
	// In a loop
	// -Build some internal structures with handles and events
	// -Wait for an event to happen
	// -Evaluate Event (Demultiplx)
	// -Call event handler
	// -If event handler requests, terminate loop
	EventProcessing::Action SynchronousEventDemultiplexorImplementationUsingPoll::handleEvents(RegistrationDataContainer &rdc, boolean &updateHandler)
	{
		sint rcPoll = null<sint>();
			
		EventProcessing::Action resultEventHandlerCall = EventProcessing::Continue;
		
		
		// Main Event loop
		while(EventProcessing::Continue == resultEventHandlerCall)
		{
			// Update descriptor list for poll function if necessary
			updatePollHandlerPropertyList(rdc, updateHandler);
			
			// Get number of registered Elements
			const uint numberOfElements = rdc.size();
			//ui << "Handler list updated. Number of Elements " << numberOfElements << std::endl;
			// Synchronous Wait for Event
			// Wait for something to happen on a descriptor or for a time out or a signal or an error
			// Blocking and synchronous wait
			//lint -e{921}     921 Cast from Type to Type
			rcPoll = ppoll(pollHandleProperty,
						   static_cast<nfds_t>(numberOfElements),
						   &pollTimeSpeccification,
						   null<__sigset_t *>());
			
			// Poll finished and found something

			if (rcPoll > null<sint>())
			{
				// Demultiplexor / Dispatcher
				// If an event happened
				// Go through the list of file descriptors and check for occurred events
		
				uint i = null<uint>();
				//lint -e{1702}   1702 operator 'Name' is both an ordinary function 'String' and a member function 'String' --
				for (RdcIterator rdci = rdc.begin(); (rdci != rdc.end()) && (!updateHandler); ++rdci)
				{
					// Demultiplex
					// Was there an event on this descriptor?
					const EventType ev = pollHandleProperty[i].revents;  //lint !e1960
					// If there was any event
					//lint -e{911,9007,1960,}
					//9007   side effects on right hand of logical operato
					//911 Implicit expression promotion from Type to Type
					if ((null<EventType>() != ev) && (null<sint>() != pollHandleProperty[i].fd))   
					{
						// Call "EventHandler"s call back functions
						// First check, if still existing. An eventhandler might have unregistered
						// a pending event
						EventHandler *const eh =(*rdci).first;
						//lint -e{917}   917 Prototype coercion (Context) Type to Type
						//if (eh != static_cast<EventHandler *>(NULL))
						{	
							//ui << "Call Eventhandler " << i << std::endl;
							// Dispatch
							//lint -e{917}   917 Prototype coercion (Context) Type to Type
							// Now act on the received event
							resultEventHandlerCall = eh->handleEvent(ev);
						} 

						//else
						//{
						//	resultEventHandlerCall = EventProcessing::Error;
						//}

						} // end if
					++i;
				} // end for 
			}
			else if (null<sint>() == rcPoll)
			{
				// Poll reported a time out
				//lint -e{956}       // Non const, non volatile static or external variable 'Symbol'
				static sint counter = 1;
				ui << "Timeout Counter: " << counter << std::endl;
				++counter;
				// x s timeout
			}
			else
			{
				// Poll reported an error
				// Halt and report error
				ui << "Poll Error" << std::endl;
				// error
				resultEventHandlerCall = EventProcessing::Error;
			}
		} // end while
		return resultEventHandlerCall;
	}

	
	
	
// ------------------------------------------------------------------------------------------------------------------------------
// 4. EPOLL  -  SynchronousEventDemultiplexor
	
	// ----------------------------
	// 4.1 Initialization 
	
	
	// Constructor for EPOLL version of SynchronousEventDemultiplexor
	SynchronousEventDemultiplexorImplementationUsingEPoll::SynchronousEventDemultiplexorImplementationUsingEPoll(void) : SynchronousEventDemultiplexorImplementation(), ePollHandle(null<Handle>()), epollEventData()
	{
		// Create and enable Linux EPOLL functionality. Get a handle for the EPOLL function
		ePollHandle = epoll_create(1);  // 1 is something other than 0. Please read man for epoll create
		
		// Check, if it worked
		if (ePollHandle < null<Handle>()) 
		{
			// If not, then set main function handler to 0. Do nothing
			ePollHandle = null<Handle>();
		}
	}
	
	
	// ----------------------------
	// 4.2 Deinitialization 
	
	// Destructor for EPOLL version of SynchronousEventDemultiplexor
	SynchronousEventDemultiplexorImplementationUsingEPoll::~SynchronousEventDemultiplexorImplementationUsingEPoll(void)
	{
		try
		{
			// Close the handle to the EPOLL functionality (if it is open)
			if (ePollHandle != null<Handle>())
			{
				close (ePollHandle);
			}
		}
		catch (...)
		{
		}
	}
	
	// ---------------------------------
	// 4.3 Specific Function for EPOLL
	
	// Register handler
	// EPOLL uses special functions for regsitering event handler specific data: epoll_ctl
	// See man for more info
	void SynchronousEventDemultiplexorImplementationUsingEPoll::registerHandlerInfo(EventHandler *const eh, const EventType et)  
	{
		//lint -e{921,571}  921 Cast from Type to Type,    571 Suspicious Cast
		// The event for what we are waiting
		epollEventData.events = static_cast<uint32_t>(et);  
		// The event handler
		epollEventData.data.ptr = eh;	
		//lint -e{534} 534 Ignoring return value of function 'Symbol'
		// Set data
		epoll_ctl(ePollHandle, EPOLL_CTL_ADD, eh->getHandle(),&epollEventData);
	}
	
	
	// --------------------------------
	// 4.4 Specific Function for EPOLL
	// Unregister handler
	// EPOLL uses special functions for regsitering event handler specific data: epoll_ctl
	// See man for more info
	void SynchronousEventDemultiplexorImplementationUsingEPoll::unregisterHandlerInfo( EventHandler *const eh) 
	{
		// We do not want to wait for any event
		epollEventData.events = null<uint32_t>();
		// The event handler that we want to remove
		epollEventData.data.ptr = eh;	
		// Remove handler
		if (-1 == epoll_ctl(ePollHandle, EPOLL_CTL_DEL, eh->getHandle(),&epollEventData))
		{
			ui << "\nEPOLL_CTL DEL " <<  eh->getHandle() << " --> " << errno << "  " << strerror(errno) << std::endl;
		}
	}

	
	// --------------------------------
	// 4.5 Event Handler Using EPOLL
	
	EventProcessing::Action SynchronousEventDemultiplexorImplementationUsingEPoll::handleEvents(RegistrationDataContainer &rdc, boolean &updateHandler)
	{
		// Magic numbers. Are used for c10k test
		const sint maxEventsToRead = 1024;
		const sint timeoutInMs = 30000;
		
		// EPOLL Events will be read in chunks
		// This is on chunk
		EpollEventData eventsRead[maxEventsToRead];
		
		// Basic assumption. Stay in event loop until something extra ordinary happens	
		// Function return codes
		EventProcessing::Action resultEventHandlerCall = EventProcessing::Continue;
		
		// Main Event loop
		while(EventProcessing::Continue == resultEventHandlerCall)
		{
			// Wait for Event to happen
			const sint numberOfEventsRead = epoll_wait(ePollHandle, &eventsRead[0], maxEventsToRead, timeoutInMs);
			//ui << "Number of Events Read by EPoll:  " << numberOfEventsRead << std::endl;

			// Did we receive an event?
			if (numberOfEventsRead > null<sint>())
			{
				updateHandler = false;
				
				// Iterate through all reported events
				for (sint i = null<sint>(); (i<numberOfEventsRead) && (EventProcessing::Continue == resultEventHandlerCall); ++i)
				{
					// Geht the associated event handler
					//lint -e{925}   925 Cast from pointer to pointer
					EventHandler *const eh = static_cast<EventHandler *>(eventsRead[i].data.ptr);
					// ANd, if this is existing (sanity check)
					if (null<EventHandler *>() != eh)
					{
						// Again sanity check. Handler could have been removed meanwhile
						const RegistrationDataContainer::size_type countOfEntries = rdc.count(eh);
						if (null<RegistrationDataContainer::size_type>() != countOfEntries)
						{
							// Eventhandler is still existing
							//ui <<"Call eventhandler " << eh << " with event " << eventsRead[i].events << std::endl;
							//lint -e{921}     921 Cast from Type to Type
							// Call the eventhandler
							resultEventHandlerCall = eh->handleEvent(static_cast<EventType>(eventsRead[i].events));
						}
					}
					else
					{
						resultEventHandlerCall = EventProcessing::Error;	// Should never happen. Error
						ui << "Error: Event Handler Pointer was NULL" << std::endl;
					}
				} // end for 
			}
			else if (null<sint>() == numberOfEventsRead)
			{
				// EPoll reported a time out
				//lint -e{956}    921 Cast from Type to Type
				static sint counter = 1;
				ui << "Timeout Counter: " << counter << std::endl;
				++counter;
			}
			else
			{
				// EPoll reported an error. Halt and report error
				ui << "Poll Error"  <<  " Error Number: " << errno<< " "<< strerror(errno) <<std::endl;
				//resultEventHandlerCall = EventProcessing::Error;
			}		
		}
		return resultEventHandlerCall;
	}

}
	
