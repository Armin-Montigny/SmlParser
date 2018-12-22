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

#ifndef REACTOR_HPP
#define REACTOR_HPP

#include "mytypes.hpp"
#include "singleton.hpp"
#include "eventhandler.hpp"



#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <map>
#include <vector>
 
 

namespace ReactorInternal
{

// ------------------------------------------------------------------------------------------------------------------------------
// 1. General




	// The "Reactor" pattern registers information about "EventHandler"s and corresponding events.
	// Here we define a  data structure as a helper class for more convenient functionality
	// Definition of ContainerType that will be used to store/register the EventHandler Data
	typedef std::map<EventHandler *,EventType> RegistrationDataContainer;
	typedef std::map<EventHandler *,EventType>::iterator  RdcIterator;


	// The InitiationDispatcher initiates the dispatching of the "EventHandler" and "Events".
	// So here we will register / unregister the list of "EventHandler"s that shall be monitored
	// for certain Events. This is a container class for "EventHandler"s
	// The main function is of course "handleEvents" function which will be implemented by the
	// SynchronousEventDemultiplexor. 
	//
	// As defined by the Reactor pattern their is a strong cooperation
	// between the InitiationDispatcher and the SynchronousEventDemultiplexor. Therefore this class holds
	// a pointer to the SynchronousEventDemultiplexorImplementation (which is a abstract base class)
	// Meaning: This class will work with all different kinds of implementations for the SynchronousEventDemultiplexor.
	// We are using the "Bridge"-design pattern to realize that

	class SynchronousEventDemultiplexorImplementation; // Forward Declaration

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Initiation Dispatcher

	class InitiationDispatcher
	{
		public:
			InitiationDispatcher(void);
			// Ctor needs concrete implementation of SynchronousEventDemultiplexor 
			explicit InitiationDispatcher(SynchronousEventDemultiplexorImplementation *sedi);
			//lint --e{1551,1540}
			//1540 pointer member 'Symbol' (Location) neither freed nor zero'ed by destructor
			//1551 function 'Symbol' may throw an exception in destructor 'Symbol'
			~InitiationDispatcher(void) { synchronousEventDemultiplexor = null<SynchronousEventDemultiplexorImplementation *>(); }
			
			// Register an EventHandler. So add it to this class' container 
			void registerHandler( EventHandler *const ev,  const EventType et);
			// Remove EventHandler From container
			void unregisterHandler( EventHandler *const ev);
			
			// Handle events using the SynchronousEventDemultiplexor
			// According to the standard reactor pattern the initiation dispatcher has to dispatch the events
			// Since we want to dcouple functionalities und increase the independence from the OS, we ask
			// the Synchronous Event Demultiplexor and here the specific OS dependent implementation
			// to additionally do the dispatching task. So the Synchronous Event Demultiplexor waits
			// for events to happen, demultiplexs them AND dispatches the event (Call the event handlers) 
			EventProcessing::Action handleEvents(void);

		protected:
			// The container for the EventHandler. Container can be implemented as a vector or a list
			RegistrationDataContainer registrationDataContainer; 
			// After a new "EventHandler" has been added or removed, the SynchronousEventDemultiplexor
			// must be informed about that so that it can build a new "Handle" (descriptor) list
			boolean updateHandler;
			
			// Pointer to the implementation of the SynchronousEventDemultiplexor
			// Bridge Pattern
			SynchronousEventDemultiplexorImplementation *synchronousEventDemultiplexor;
	};


// ------------------------------------------------------------------------------------------------------------------------------
// 3. Synchronous Event Demultiplexor
	

	// -----------------------------------------------------------
	// 3.1 General Base Class for Synchronous Event Demultiplexors
	

	// Abstract base class for any kind of implementation of the SynchronousEventDemultiplexor
	// As explained above. Different operating systems offer different system calls for a synchronous
	// wait. Since we want to decouple OS specific operations from this function we use the
	// bridge pattern to be able to select appropriate or preferred specific implementations.
	// This base class describes the main public interface for the class
	class SynchronousEventDemultiplexorImplementation
	{
		public:
			SynchronousEventDemultiplexorImplementation(void)  {}			// Empty ctor
			virtual ~SynchronousEventDemultiplexorImplementation(void) {}	// Empty dtor
			// pure virtual interface function to the SynchronousEventDemultiplexor
			// Inform on registered "EventHandler"s and if the container has been updated.
			// Then run the main event loop
			virtual EventProcessing::Action handleEvents(RegistrationDataContainer &rdc, boolean &updateHandler) = 0;
			
			// In case that the SynchronousEventDemultiplexor is interested in these activities
			//lint --e{1961}  //1961 virtual member function 'Symbol' could be made const
			virtual void registerHandlerInfo(EventHandler *const,  const EventType) {}
			virtual void unregisterHandlerInfo( EventHandler *const) {}
	};

	// ---------------------------------------------------------------
	// 3.2 Synchronous Event Demultiplexors using Linux function epoll

	class SynchronousEventDemultiplexorImplementationUsingEPoll : public SynchronousEventDemultiplexorImplementation
	{
		public:
		
			typedef struct epoll_event EpollEventData;
			
			// Initialization
			SynchronousEventDemultiplexorImplementationUsingEPoll(void);
			virtual ~SynchronousEventDemultiplexorImplementationUsingEPoll(void);
			
			// Register / Unregister Eventhandlers
			virtual void registerHandlerInfo(EventHandler *const eh,  const EventType et) ; 
			virtual void unregisterHandlerInfo( EventHandler *const eh); 
			// Event handler
			virtual EventProcessing::Action handleEvents(RegistrationDataContainer &rdc, boolean &updateHandler);
			
		protected:
			// Handle to the EPOLL functionality itself
			Handle ePollHandle;
			// Persistent data
			EpollEventData epollEventData;
	};


	// ---------------------------------------------------------------
	// 3.3 Synchronous Event Demultiplexors using Linux function ppoll

	// Concrete implementation of a SynchronousEventDemultiplexor using the Linux ppoll function
	// Decision to use ppoll because ease of use and some advantages over the classical approach with select

	class SynchronousEventDemultiplexorImplementationUsingPoll : public SynchronousEventDemultiplexorImplementation
	{
		public:
			SynchronousEventDemultiplexorImplementationUsingPoll(void);
			//lint -e{1740,1551}
			//1740 pointer member 'Symbol' (Location) not directly freed or zero'ed by destructor
			//1551 function 'Symbol' may throw an exception in destructor 'Symbol'
			virtual ~SynchronousEventDemultiplexorImplementationUsingPoll(void) { delete [] pollHandleProperty; pollHandleProperty = null<PollHandleProperty *>(); }
			
			// Concrete Implementation of the main "handleEvent" functions
			virtual EventProcessing::Action handleEvents(RegistrationDataContainer &rdc, boolean &updateHandler);
			
		protected:
			typedef struct pollfd 		PollHandleProperty;	    // struct of file descriptors and event
			typedef struct timespec 	PollTimeSpeccification; // Time-out definition (ppoll specific)
					
			PollHandleProperty *pollHandleProperty;				// Container of descriptors and events
			PollTimeSpeccification pollTimeSpeccification;		// specific time out
			
			// Helper function. After (un)registration of EventHandlers the container of descriptors and events
			// needs to be rebuild
			// updateHandler is an indicator, that the list has to be updated
			void updatePollHandlerPropertyList(RegistrationDataContainer &rdc, boolean &updateHandler);
	};
			

// ------------------------------------------------------------------------------------------------------------------------------
// 4. Reactor
			
			
	// The Reactor
	//
	// The Reactor class is implemented as an Template. The template Parameter is the implementation method
	// of the "SynchronousEventDemultiplexor".
	//
	// The "Reactor" contains the InitiationDispatcher and the "SynchronousEventDemultiplexor" as well as the
	//    handleEvents, registerHandler, unregisterHandler
	// 
	// We use the Facade Pattern here to wrap every other functionality just in one class.
	//
	// Additionally: The Reactor is implemented using a Singleton 

	template <class SynchronousEventDemultiplexorImplementationUsingMethod>
	class ReactorBase
	{
		public:
			~ReactorBase(void) {}
			ReactorBase<SynchronousEventDemultiplexorImplementationUsingMethod>(void) : 
													synchronousEventDemultiplexor(), 
													initiationDispatcher(&synchronousEventDemultiplexor) {}
													
			// Wrapper functions for initiationDispatcher
			EventProcessing::Action handleEvents(void) { return initiationDispatcher.handleEvents(); }
			void registerHandler( EventHandler *const ev,  const EventType et) { initiationDispatcher.registerHandler(ev, et); }
			void unregisterHandler( EventHandler *const ev) { initiationDispatcher.unregisterHandler(ev); }
			
			// Singleton, generates one and only pointer to this class
			SINGLETON_FOR_CLASS(ReactorBase<SynchronousEventDemultiplexorImplementationUsingMethod>)
			
		protected:
			SynchronousEventDemultiplexorImplementationUsingMethod synchronousEventDemultiplexor;
			InitiationDispatcher initiationDispatcher;
	};

}


	// Select implementation and create a short name

	//typedef ReactorInternal::ReactorBase<ReactorInternal::SynchronousEventDemultiplexorImplementationUsingPoll> Reactor;
	typedef ReactorInternal::ReactorBase<ReactorInternal::SynchronousEventDemultiplexorImplementationUsingEPoll> Reactor;


	// Shortcut functions
	inline void reactorRegisterEventHandler(EventHandler *const eh, const EventType et) {Reactor::getInstance()->registerHandler(eh,et);} 
	inline void reactorUnRegisterEventHandler(EventHandler *const eh) {Reactor::getInstance()->unregisterHandler(eh);} 


#endif
