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
// Implementation of Oberver or Publisher Subscriber Pattern

// Normally I would call this pattern Observer, because the Event Channel is missing and other typical
// characteristics. By using templates and lose coupling, I tend to name it Publisher Subscriber.
// Anyway. The concept is in the end the same. All other discussions are more or less religious.

// There are 2 main classes:
// 1. Subscriber. The subscriber is interested in something from the publisher. It provides an update
// function. The Publisher will call this update function in case there are news
// 2. The Publisher. The publisher has information to distribute. Subscribers will register a subscription 
// with the Publisher. In case that important information is available, the publisher notifies all registered
// suppliers. 
//
// The publisher holds a list of subscribers. 
//
// Please note: Internally, in the design of the pattern, care must be taken because an update function, 
// called by notify, may unregister a subscriber. This would modify the container while an iteration runs over it.
// And that's generally a problem.
//
// Therefore a std::list will be used as container. In contrast to a std::vector the std::list iterators will not
// be invalidated by erasing an element from the list. Anyway, we will take special precaution for deleting
// elements. Elements will not be erased immediately. A flag will be set to mark a subscriber as inactive.
// During the next notify operation, all inactive elements will be removed
//
// Additionally we will prevent adding more than one identical element.
//

#ifndef OBSERVER_HPP
#define OBSERVER_HPP

#include "mytypes.hpp"
//#include "userinterface.hpp"

#include <list>
#include <algorithm>
	
 
 
	
	
	
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Subscriber class

	// If any class wants to be a subscriber to another class, it should derive from this class and 
	// add the class type of the other class (the publisher) as template parameter.
	// If you want to subscribe to more publishers,	then simply derive from more Subscriber Classes
	// with the appropriate template parameter

	template <class T>
	class Subscriber
	{
		public:
			Subscriber(void) {}	// Empty standard constructor
			virtual ~Subscriber(void) {}	// Empty standard destructor
			// The update function. Will be called by Publisher in case of notification
			virtual void update(T *publisher) = 0; 
	};


	
	
	
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Publisher Class

	// --------------------------------------------------------------------
	// 2.1 Publisher class definition


	// This class can notify other classes on any state change. 
	// Subscriber classes can add a subscription (or remove it). All subscribers can be notified 
	// be calling the notify function. The notify function will call all update functions from the subscribers.

	// If a class shall be a publisher, then please derive this class from Publisher with the template
	// parameter being the name of the -not yet- defined class.
	// This is called "curiously recurring template pattern" (CRTP).
	template <class T>
	class Publisher
	{
		public:
			Publisher(void) : subscriberList() {} // Standard constructor. Will initialize the subscriber list to empty
			virtual ~Publisher(void) {} // Empty standard destructor
			// Main interface
			virtual void addSubscription(Subscriber<T> *subscriber);  // Add a subscriber to the publisher
			virtual void removeSubscription(Subscriber<T> *subscriber);  // Remove a subscriber from the publisher

		protected:
		
			// Notify all registered subscribers
			virtual void notifySubscribers(void); 
			
			// As mentioned in above description. We will not immediately erase elements from the subscriber list
			// during the removal of a subscriber. Will will reset the active flag and then erase it later
			// For this reason we will not only store Pointer to T (subscriber) but also an active flag.
			// This flag will be set to true by default
			struct SubscriberListElement
			{
				// Constructor: Store subscriber and set the active flag to true
				SubscriberListElement(Subscriber<T> *subscriber) : subscriber(subscriber), active(true) {}
				// Operator for comparison. We will only compare the "subscriber" part
				boolean operator == (const SubscriberListElement &sle) {return (subscriber == sle.subscriber);	}
				
				Subscriber<T> *subscriber;	// The subscriber
				boolean active;				// The active flag. Only active subscribers will be notified.

			};
			// The list of registered subscribers
			std::list<SubscriberListElement> subscriberList;

			// Functor for deactivating a subscriber
			struct SubscriberDeactivator
			{
				// Constructor. Initialize with subscriber that shall be deactiviated
				SubscriberDeactivator(Subscriber<T> *subscriber) : subscriber(subscriber) {}
				boolean operator() (SubscriberListElement &subscriberListElement); // Functor function
				Subscriber<T> *subscriber;	// Subscriber to deactivate
			};
	};

	
	// --------------------------------------------------------------------
	// 2.2 Publisher class member functions
	
		// --------------------------------------------------------------------
		// 2.2.1 Functor operator for deactivating a subscriber
		
		// Deactivated subscribers will not be notified
		// Meaning, their update function will not be called

		// Functors function call operator for removal or inactivating the subscriber
		// Predicate for find_if
		template <class T>
		boolean Publisher<T>::SubscriberDeactivator::operator() (SubscriberListElement &subscriberListElement)
		{
			// Per default, we assume that we did not yet find the subscriber
			boolean rc = false;
			// Could we find the subscriber?
			if (subscriber == subscriberListElement.subscriber)
			{
				// If so. set the active flag to false
				subscriberListElement.active = false;
				rc = true;  // Indicate that we found the element
			}
			return rc;
		}

		
		// --------------------------------------------------------------------
		// 2.2.2 Add a subscriber to the publisher
		
		template <class T>
		void Publisher<T>::addSubscription(Subscriber<T> *subscriber)
		{	
			// Subscribers shall be unique. So, first check, if the given subscriber is already in the list
			// Try to find the subscriber
			typename std::list<SubscriberListElement>::iterator iter = std::find(subscriberList.begin(),subscriberList.end(),subscriber);
			// If the subscriber could not be found, then we will add it to the list
			if (iter == subscriberList.end())
			{
				// Create a subscriber List Element
				SubscriberListElement sle(subscriber);
				// And copy the data into the list
				subscriberList.push_back(sle);
			}
			else
			{
				//ui.msgf("Subscriber already exists. Active = %s\n",(*iter).active?"True":"False");
				// In any case. Activate it again. Even if already active
				(*iter).active = true;
			}
		}

		

		// --------------------------------------------------------------------
		// 2.2.3 Remove subscriber
		
		// As explained above. Will simply deactivate the Subscriber
		template <class T>
		void Publisher<T>::removeSubscription(Subscriber<T> *subscriber)
		{
			// Instantiate deactivator functor
			SubscriberDeactivator subscriberDeactivator(subscriber);
			// Search the element and deactivate it.
			std::find_if(subscriberList.begin(),subscriberList.end(),subscriberDeactivator);
		}

		// --------------------------------------------------------------------
		// 2.2.4 Notify Subscribers
		
		// The Publisher has maintains a list of subscribers. This notification operation invokes
		// the update function of all registered subscribers
		// While iterating over the subscribers, all inactive subscribers will be erased
		template <class T>
		void Publisher<T>::notifySubscribers(void)
		{
			// Define an iterator over the subscriber list of this publisher instance and set it to the first element
			typename std::list<SubscriberListElement>::iterator subscriberListElementIterator = subscriberList.begin();
			// Iterate. For loop will not work, because we will not always increment the iterator
			while(subscriberListElementIterator != subscriberList.end())
			{
				// Check if subscriber is still active
				if ((*subscriberListElementIterator).active)
				{

					// Yes, it is acive. Call subscribers update function
					(*subscriberListElementIterator).subscriber->update(static_cast<T *>(this));
					// Goto next
					++subscriberListElementIterator;
				}
				else
				{
					// No, subscriber is no longer active. Erase it. Set iterator to next element auf list
					subscriberListElementIterator = subscriberList.erase(subscriberListElementIterator);
				}
			}
		}

 

#endif

