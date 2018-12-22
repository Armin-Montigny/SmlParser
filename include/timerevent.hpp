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
// timerevent.hpp
//
// This application uses a reactor for event handling
// The reactor listens on open handles/file descriptors
//
// Here we define a simple timer functionality that works with file descriptors
//
// The publisher subscriber mechanism will be used to inform about expired timers
//
// So, any class can create a timer, register it with the reactor and 
// subscribe to it.
//



#ifndef TIMEREVENT_HPP
#define TIMEREVENT_HPP

#include "eventhandler.hpp"
#include "observer.hpp"
#include "reactor.hpp"

#include <unistd.h>
#include <sys/timerfd.h>

#include <string.h>

 
 



// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Definition of TimerClass

	class EventTimer : 	public EventHandlerBasic,
								public Publisher<EventTimer>
	{
		public:
			// Constructor takes the time period value in ms as parameter
			explicit EventTimer(const u32 periodInMs);
			// Destructor: Stop the timer and close the handle
			virtual ~EventTimer(void);

			// Handle Timer Event. Notify Subscribers
			virtual EventProcessing::Action handleEvent(EventType);

			// Start the timer in mode "periodic"
			virtual void startTimerPeriodic(void);
			// Start the timer in mode "One Shot"
			virtual void startTimerOneShot(void);

			// Stop the Timer
			virtual void stopTimer(void);

			// Set new timer values
			virtual void setTimerValues(const u32 periodInMs);
			
							   
		protected:
			// itimerspec is given in library. Give more readable name
			typedef struct itimerspec IntervalTimerSpec;
			// Define 3 predefined timer values for the 3 different modes.
			struct ITS
			{
				// In the beginning ste everything to 0. That means "stop"
				ITS(void) {memset(&periodic,0,sizeof(periodic));memset(&oneShot,0,sizeof(oneShot));memset(&stop,0,sizeof(stop));}
				IntervalTimerSpec periodic;
				IntervalTimerSpec oneShot;
				IntervalTimerSpec stop; // Will never be changed
			};
			ITS its;
		private:
			// Do not use default constructor
			EventTimer(void) : EventHandlerBasic(), Publisher<EventTimer>(), its() {}
	};


// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Timer CLass member functions


	// -----------------------------------------------------------------------
	// 2.1 Constructor and Destructor

	
	// Constructor sets timer values and opens the handle
	inline EventTimer::EventTimer(const u32 periodInMs) : EventHandlerBasic(), Publisher<EventTimer>(), its()
	{
		// Set timer values for OneShot and Periodic Timer
		setTimerValues(periodInMs);
		// Open a file descriptor for the timer
		handle = timerfd_create(CLOCK_MONOTONIC, 0);
	}

	// Destructor: Stop timer and close handle
	inline EventTimer::~EventTimer(void)
	{
		//stopTimer();
		if (null<Handle>() < handle) close(handle);
	}


	// -----------------------------------------------------------------------
	// 2.2 Set the timer values
	inline void EventTimer::setTimerValues(const u32 periodInMs)
	{
		// Calculate the internally needed timer values from the parameter given in ms
		// Second part
		time_t seconds = static_cast<time_t>(periodInMs / 1000UL);
		// Nano second part
		s32 nanoSeconds = static_cast<s32>((static_cast<u64>(periodInMs) * 1000ULL) - (static_cast<u64>(periodInMs / 1000UL) * 1000000ULL));

		// Set values for periodic and One Shot Timer
		// For the one shot timer the it_interval part stays 0. That defines the one shot characteristic
		its.periodic.it_interval.tv_sec = its.periodic.it_value.tv_sec = its.oneShot.it_value.tv_sec = seconds;
		its.periodic.it_interval.tv_nsec = its.periodic.it_value.tv_nsec = its.oneShot.it_value.tv_nsec = nanoSeconds;

	}

	// -----------------------------------------------------------------------
	// 2.3 Handle the timer event.
	
	// The timer fires. Either one shot or periodic
	// The reactor recognizes this on the file descriptor and calls the this event handler
	inline EventProcessing::Action EventTimer::handleEvent(EventType)
	{
		// We will continue the Main Event Loop
		EventProcessing::Action eventProcessingAction = EventProcessing::Continue;
		
		// We need to read from the handle/file descriptor and get back the number of expirations of the timer
		u64 numberOfExpirations;	
		if (read(handle, &numberOfExpirations, sizeof(numberOfExpirations)) > 0)
		{
			// If we could read the timer then inform all subscribers
			notifySubscribers();
		}
		else
		{
			// In case of error, do nothing
		}
		return  eventProcessingAction;
	}

	// -----------------------------------------------------------------------
	// 2.4 Timer start

	inline void EventTimer::startTimerPeriodic(void) 
	{
		// Start the timer with the periodic argument
		timerfd_settime(handle, 0, &(its.periodic), null<struct itimerspec *>());
		// Arm the reactors event handler
		reactorRegisterEventHandler(this,EventTypeIn);
	}
	
	inline void EventTimer::startTimerOneShot(void) 
	{
		// Start the timer with the oneShot argument
		timerfd_settime(handle, 0, &(its.oneShot), null<struct itimerspec *>());
		// arm the reactors event handler
		reactorRegisterEventHandler(this,EventTypeIn);
	}
	
	// -----------------------------------------------------------------------
	// 2.4 Timer stop
	
	inline void EventTimer::stopTimer(void) 
	{
		// No more event handling for timer
		reactorUnRegisterEventHandler(this);	
		// Set timer values to 0
		timerfd_settime(handle, 0, &(its.stop), null<struct itimerspec *>());
	}

 

#endif