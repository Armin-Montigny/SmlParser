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
//
// This module implements basically the Proactor Design Pattern used for handling the completion of
// asynchronous operations. It is a event handler, similar to a Reactor, but it does wait for the
// completion of an operation. In contrast, the reactor waits for a handle/descriptor to become active 
// and then perform an operation. The Proactor relies on Asynchronous Operations provided by an 
// Operating system. This allows for multiple asynchronous operations in a single threaded environment
// (or in a multi thread environment in ONE common thread) without the need to take care for task
// synchronisation and avoiding all pitfalls of working with a multi threaded environment.
//
// The Proactor Pattern was introduced by Douglas C. Schmidt in 1998 for high load web servers.
// http://www.cs.wustl.edu/~schmidt/PDF/proactor.pdf
//
// This implementation follows Schmidt's ideas but it enhances the functionality by making the Proactor
// an Event Handler of a Reactor and thus combine the advantages of both algorithms.
// There is also a functionality available that makes it possible to handle the completion of
// an asynchronous operation purely asynchronous (like in a separate thread) or synchronous using
// the event handler of a Reactor or both.
//
// Additionally an adapted and enhanced version of the Asynchronous Completion Token Pattern is used
// (like a Mediator Pattern) to transport user data information between all collaborators of the Proactor
//
// The basic "Asynchronous Completion Token Design Pattern" has also been developed by Douglas C. Schmidt 
// in 1998 for alleviating the demultiplexing and dispatching of events to the designated handler.
// https://www.dre.vanderbilt.edu/~schmidt/PDF/ACT.pdf
//
// In this implementation the Asynchronous Completion Token Design Pattern has been adapted to the needs
// of the combined Reactor and Proactor functionalities.
//
// The main entities in this implementation of the Proactor Design Pattern are:
// 
// 1. Proactor
//
// The Proactor is responsible for demultiplexing and dispatching events to the associated completion
// handler. It is implemented as a singleton and basically not visible to the outside world. As mentioned
// above the Proactor is an event handler for a Reactor. It uses an intra process/thread self pipe
// mechanism to inform the reactor for the completion of an event and to eliminate the need for a separate
// event demultiplexer/dispatcher using functions like aio_suspend (in Linux) or WaitForMultipleObjects
// in (Windows)
//
//
// 2. Asynchronous Operation Processor (AOP)
//
// The asynchronous operation processor is usually the operation system. Since we want to keep this implementation
// more general, we use a static bridge pattern with a template to isolate OS specific functions in one 
// place. So the AsynchronousOperationProcessor class is an interface class only. An OS specific class
// has to be derived from the base class with all the concrete implementations of the needed functions.
// In this example we use Linux type "aio" functions.
//
//
// 3. Asynchronous Operation (AO)
//
// Public class to be used to execute Asynchronous functions. Template class with the Asynchronous Operation 
// Processor specific implementation as a template parameter and thus acting as a static bridge.
// Via a typedef the Asynchronous Operation is bound to the specific implementation of the AOP.
//
//
//
// 4. Completion Handler
//
// The completion handler class has 2 main functions:
// a.) Asynchronous completion via direct callback
// b.) Synchronous completion through a Reactor
// Return codes of the functions determine further behaviour. If the Asynchronous Completion Handler 
// returns a value > 0 then the synchronous functions will be called (otherwise not). So it can
// be selected what to do. If only synchronous completion handling is desired, then a
// Asynchronous Completion Handler that simply returns 1 should be implemented.
// For the Asynchronous Completion Handler in the Linux environment not all operating system calls
// are allowed. Please refer to the aio documentation for further details.
//
// Since it is often the case that a completion handler uses asynchronous functions itself, a derived
// class is provided that already contains an Asynchronous Operation (AO) and a Asynchronous Completion 
// Token (ACT)
//
//
//
// 5. Asynchronous Completion Token (ACT)
//
// The Asynchronous Completion Token class is basically a POD and acts as a mediator for
// transferring information between all participating classes. Especially important are contained pointers
// to the completion handlers. With that the Proactor can easily demultiplex and dispatch completion events.
// Note: A pointer to the Proactor itself is also available. Additionally we carry information about
// the started asynchronous function and a free usable void pointer to any needed user data information.
// There is more information in the ACT as in the common implementations found in literature. (AO, handle, etc.)
// See also: https://www.dre.vanderbilt.edu/~schmidt/PDF/ACT.pdf//
// ------------------------
//
// Please note. This function does not enqueue asynchronous operations. So the read/write buffers, the ACT
// and the AO should be persistent or exist as least as long until the complete operation is done!
//
//

#include "proactor.hpp"
#include "reactor.hpp"

#include <fcntl.h>

#include <algorithm>







// ------------------------------------------------------------------------------------------------------------------------------
// 1. Dispatchers / Callbacks

	// --------------------
	// 1.1 STatic Call Back
	
	// Static call back function
	// The Linux aio functions cannot have a class member function as a callback function. Therefore we use this
	// wrapper function which will simply forward the call to the Proactor's dispatcher
	//lint -e{1746}  Signature requested by library
	void AsynchronousOperationProcessorUsingLinuxAio::dispatchCompletionEventStatic(const AioSignalValue aioSignalValue)
	{
		// sival_ptr is actually an ACT*
		ACT *const act = static_cast<ACT *>(aioSignalValue.sival_ptr);  //lint !e925  //Cast from pointer to pointer
		// Call Proactor's call back functions and pass the ACT again as a parameter
		act->proactor->dispatchCompletionEvent(act);
	}

	// ----------------------------------------
	// 1.1 Dynamic Call back for specific Class

	// Class specific call back function. Called by static function
	void Proactor::dispatchCompletionEvent(ACT *const act) const
	{
		// Call asynchronous callback function
		const AsynchronousCompletionEvent::Action rc = act->ch->handleCompletionEventAsync(act);
		// Asynchronous handling done
		act->actBusyAsynchronously = false;
		// If also synchronous handling is desired
		if (AsynchronousCompletionEvent::CallSynchronousEventhandler == rc)
		{
			// The write file handle of the pipe has been opened in none blocking mode
			// The the function will always return.
			// This is intended, because this function will be called from an asynchronous callback. It must not block.
			
			// However. If the write does not succeed, we will loose the synchronous event
			// The strategy here is, to ignore that event  --> Bad!!!!!!!
			write(notificationPipeDescriptors[pipeWriteIndex],&act, sizeof(ACT *));  // Check return value!!!!!!
		}
		else
		{
			// No only asynchronous handling was desired
			// Therefore we can also reset the synchronous busy flag
			act->actBusySynchronously = false;
		}
	}


	// The Proactor's event handler for the Reactor. 
	// Repeat: This is for synchronous event handling in the Reactor
	// With self piping we send an event from the asynchronous event handler to the reactor
	//lint -e{1961}   // virtual member function 'Symbol' could be made const --
	EventProcessing::Action Proactor::handleEvent(const EventType)
	{
		EventProcessing::Action rc = EventProcessing::Stop;
		ACT *act;   // Through the pipe we receive a pointer to an ACT

		const sint bytesTransferred = read(notificationPipeDescriptors[pipeReadIndex],&act,sizeof(ACT *));  // Read it
		//lint -e{921}   Cast from Type to Type
		if (static_cast<sint>(sizeof(ACT *)) == bytesTransferred)
		{
			// act read, call synchronous event handler
			 rc = act->ch->handleCompletionEventSync(act); // Handle the completion event synchronously
		}
		else
		{
			// Should never happen, internal error
			// Stop event loop and program
		}
		// In any case, synchronous handling done
		act->actBusySynchronously = false;

		return rc;
	}

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Initialization


	// Ctor for Proactor
	Proactor::Proactor(void) : EventHandler()
	{
		sint rc = -1;
		sint pipeRc;
		// Open a pipe for communication between Proactor and Reactor
		//lint -e{1960}   call system function
		pipeRc = pipe(notificationPipeDescriptors);
		if (null<sint>() == pipeRc) 
		{
			// Set the handle for writing to a none blocking mode
			//lint -e{9001,1960}   Note 9001: Octal constant used
			if (-1 !=fcntl(notificationPipeDescriptors[pipeWriteIndex], F_SETFL , O_NONBLOCK))
			{
				rc = null<sint>(); // Everything OK
			}
		} 
		// In case of error
		if (-1 == rc)
		{
			// In case of error
			notificationPipeDescriptors[pipeReadIndex] = null<Handle>();
			notificationPipeDescriptors[pipeWriteIndex] = null<Handle>();
		}
		
		reactorRegisterEventHandler(this,EventTypeIn);		
	}


	// CTor for OS specific AsynchronousOperationProcessor
	AsynchronousOperationProcessorUsingLinuxAio::AsynchronousOperationProcessorUsingLinuxAio(void) : 
															AsynchronousOperationProcessor(), asynchronousIoControlBlock()
	{
		initStaticAsynchronousIoControlBlockComponents(); // Initialize invariant values for AIOCB
	}


	// Initialize invariant values for AIOCB for the AsynchronousOperationProcessor
	void AsynchronousOperationProcessorUsingLinuxAio::initStaticAsynchronousIoControlBlockComponents(void)
	{
		asynchronousIoControlBlock.aio_offset = null<off_t>();  		// Preinitialize to 0. This may change	
		asynchronousIoControlBlock.aio_reqprio = null<sint>();			// Task Prio
		asynchronousIoControlBlock.aio_sigevent.sigev_notify = SIGEV_THREAD;		// We will use Call Through dispatching with a call back
		asynchronousIoControlBlock.aio_sigevent.sigev_notify_attributes = null<pthread_attr_t *>();	// No attributes needed
	}


	
// ------------------------------------------------------------------------------------------------------------------------------
// 3. Asynchonous Operation
	
	
	// The actual OS specific Write function
	sint AsynchronousOperationProcessorUsingLinuxAio::aWrite(byte *const writeBuffer, const uint numberOfBytes, ACT *const act, const s32 offset)
	{
		// Setup the aiocb
		asynchronousIoControlBlock.aio_offset = offset;
		asynchronousIoControlBlock.aio_sigevent.sigev_notify_function = &dispatchCompletionEventStatic;
		asynchronousIoControlBlock.aio_sigevent.sigev_value.sival_ptr = act; 
		asynchronousIoControlBlock.aio_fildes = *(act->ao->getHandle());
		asynchronousIoControlBlock.aio_buf = writeBuffer;
		asynchronousIoControlBlock.aio_nbytes = numberOfBytes;
		// Call the corresponding system function
		return aio_write(&asynchronousIoControlBlock);
	}


	// The actual OS specific Read function
	sint AsynchronousOperationProcessorUsingLinuxAio::aRead(byte *const readBuffer, const uint numberOfBytes, ACT *const act, const s32 offset)
	{
		// Setup the aiocb
		asynchronousIoControlBlock.aio_offset = offset;
		asynchronousIoControlBlock.aio_sigevent.sigev_notify_function = &dispatchCompletionEventStatic;
		asynchronousIoControlBlock.aio_sigevent.sigev_value.sival_ptr = act; 
		asynchronousIoControlBlock.aio_fildes = *(act->ao->getHandle());
		asynchronousIoControlBlock.aio_buf = readBuffer;
		asynchronousIoControlBlock.aio_nbytes = numberOfBytes;
		
		// Call the corresponding system function
		return aio_read(&asynchronousIoControlBlock);
	}


// ------------------------------------------------------------------------------------------------------------------------------
// 4. Deinitialization

	// Delete all ACTs in Completion Handler
	CompletionHandlerWithAo::~CompletionHandlerWithAo(void)
	{ 
		try
		{
			for (std::vector<ACT *>::iterator actIterator = act.begin(); actIterator != act.end(); ++actIterator)
			{
				delete *actIterator;
			}
		}
		catch(...)
		{
		}
	}


// ------------------------------------------------------------------------------------------------------------------------------
// 5. ACT Help Function

	// Find a free unused ACT that can be used with an asynchronous operation
	static bool isNotBusy(const ACT *const act) { return !(act->isBusy()); }

	ACT *CompletionHandlerWithAo::getFreeAct(void)
	{
		ACT *actResult = &dummyAct;
		
		const std::vector<ACT *>::iterator actIterator = find_if(act.begin(), act.end(), &isNotBusy);
		if (actIterator == act.end())
		{
			// No, all is busy
			// Build new ACT
			actResult = new ACT(this,&ao);
			// Store new pointer
			act.push_back(actResult);
		}
		else
		{
			// Yes, we found something. Return that
			actResult = *actIterator;
		}
		// Set some default value for user defined identifier
		//lint -e{1702} // operator 'Name' is both an ordinary function 'String' and a member function 'String'
		actResult->userDefinedActIdentifier = actIterator-act.begin();
		// Set it to busy by default, so that the next call to this function will not return the same ACT
		actResult->setBusy();
		
		return actResult;
	}
