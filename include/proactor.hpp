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
// and the AO should be persistent or exist at least as long as the complete operation is done!
//
//

#ifndef PROACTOR_HPP
#define PROACTOR_HPP



#include "mytypes.hpp"
#include "eventhandler.hpp"
#include "singleton.hpp"

#include <unistd.h>
#include <aio.h>

#include <vector>

 
 



// ------------------------------------------------------------------------------------------------------------------------------
// General declarations
	
	// Forward declaration for AsynchronousCompletionToken. We want to use a pointer in the Proactor
	struct AsynchronousCompletionToken;

	// Abbreviation. Save typing work
	typedef  AsynchronousCompletionToken ACT;

	// Return Code of functions handling an AsynchronousCompletionEvent
	struct AsynchronousCompletionEvent
	{
		enum Action
		{
			Finalize,
			CallSynchronousEventhandler
		};
	};

// ------------------------------------------------------------------------------------------------------------------------------
// 1. Proactor


	// The Proactor is responsible for completion event demultiplexing and dispatching.
	// It is itself an Event Handler for a reactor and must be registered with the respective
	// instance of the reactor. 
	// This implementation uses the Proactor as a singleton.
	// It creates a pipe to communicate with the reactor. So it is an event generator and consumer
	// at the same time.

	class Proactor : public EventHandler
	{
		public:
			Proactor(void);
			virtual ~Proactor(void) { close(notificationPipeDescriptors[pipeReadIndex]); close(notificationPipeDescriptors[pipeWriteIndex]);}
			// Demultiplex and dispatch completion events, both asynchronous and synchronous
			void dispatchCompletionEvent(ACT *const act) const;	
			// Make the Proactor a Singleton
			//lint -e{956}  // 956 Non const, non volatile static or external variable
			SINGLETON_FOR_CLASS(Proactor)		

		protected:
			// Functions for the Reactor. Get Reactor Event Handler. Reactor will listen on the pipe
			//lint -e{1768,1961}   We do not want any public usage for this function in the Proactor.
			virtual Handle getHandle(void) const { return notificationPipeDescriptors[pipeReadIndex]; }
			// Reactor Event Handler. If there is something in the pipe then read it and call the
			// synchronous completion handler
			//lint -e{1768}   We do not want any public usage for this function in the Proactor.
			virtual EventProcessing::Action handleEvent(const EventType eventType);  // event handle function that will be called back		
			// Constants for descriptor indices			
			enum {pipeReadIndex = 0, pipeWriteIndex = 1};
			// Descriptors (file handle) for the pipe
			sint notificationPipeDescriptors[2];
	};

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Asynchronous Operation Processor (AOP)

	
	// -----------------------
	// 2.1 Abstract Base class


	// Abstract Base class. Just interface definition for all available asynchronous 
	// operations that we will support
	class AsynchronousOperationProcessor
	{
		public:
			AsynchronousOperationProcessor(void) {}
			virtual ~AsynchronousOperationProcessor(void) {}
			// Read asynchronously from any file. Pure virtual function.
			virtual sint aRead(byte *const readBuffer, const uint numberOfBytes, ACT *const act, const s32 offset) = 0;
			// Write asynchronously to any file. Pure virtual function.
			virtual sint aWrite(byte *const writeBuffer, const uint numberOfBytes, ACT *const act, const s32 offset) = 0;
			// Get error from last asynchronous function call. Pure virtual function.
			virtual sint aError(void) const = 0;
			// Get function result from last asynchronous function call. Pure virtual function.
			// Basically gets the number of bytes transferred for read and write operations
			virtual sint aResult(void) = 0;
	};

	// -----------------------
	// 2.2 OS Specific

	
	// Asynchronous Operation Processor (AOP). OS specific concrete implementation using Linux aio functionality
	//
	// This is completely OS specific. It will not work for MS Windows. If you want to use
	// a different OS. Then you must create a similar class (derived from AsynchronousOperationProcessor)
	// and then change the typedef for the AsynchronousOperation
	class AsynchronousOperationProcessorUsingLinuxAio : public AsynchronousOperationProcessor
	{
		public:
			AsynchronousOperationProcessorUsingLinuxAio(void);
			virtual ~AsynchronousOperationProcessorUsingLinuxAio(void) {}
			// Read asynchronously from any file. Concrete implementation with Linux aio functions.
			virtual sint aRead(byte *const readBuffer, const uint numberOfBytes, ACT *const act, const s32 offset);
			// Write asynchronously to any file. Concrete implementation with Linux aio functions.
			virtual sint aWrite(byte *const writeBuffer, const uint numberOfBytes, ACT *const act, const s32 offset);
			// Get error from last asynchronous function call. Concrete implementation with Linux aio functions.
			virtual sint aError(void) const { return aio_error(&asynchronousIoControlBlock); }
			
			
			// Get function result from last asynchronous function call. 
			// Concrete implementation with Linux aio functions.
			// Basically gets the number of bytes transferred for read and write operations
			virtual sint aResult(void) {return aio_return(&asynchronousIoControlBlock); }
			
		protected:
			// Linux AIO specific definitions
			typedef struct aiocb AsynchronousIoControlBlock;
			typedef struct sigevent AioSignalEventHandling;
			//lint -e{1960,9018}  Union forced by library function
			typedef union sigval AioSignalValue;
			// This implementation uses call through dispatching. We will use the call back mechanism
			// provided by the Linux aio functions. Unfortunately the call back function cannot be a
			// class member. So we need a static wrapper function. This static wrapper function will be
			// used for all asynchronous functions and simply calls the Proactor's dispatcher. The
			// AioSignalValue will hold the ACT, which has all relevant information
			static void dispatchCompletionEventStatic(const AioSignalValue aioSignalValue);
			// Linux aio functions use an AsynchronousIoControlBlock aiocb to control their behaviour
			// Many components of the aiocb will not change during operations. This function is a 
			// one time initialisation for the aiocb
			void initStaticAsynchronousIoControlBlockComponents(void);
			// Instantiation for the aiocb. This must be persistent for the duration of the 
			// asynchronous operation
			AsynchronousIoControlBlock asynchronousIoControlBlock;
	};


// ------------------------------------------------------------------------------------------------------------------------------
// 3. Asynchronous Operation (AO)

	//
	// Since not all Operation Systems may support the same set of asynchronous functions, we will use
	// the bridge pattern (static, through templates) to allow for different implementations.
	// Here we map the needed asynchronous functions to a specific, OS dependent implementation.
	// With a typedef and the concrete implementation of a AsynchronousOperationProcessor we create
	// the "Asynchronous Operation (AO)" that will be used in other application parts.
	// Additionally we will store a pointer to the file descriptor (Handle) in the AO class. With that
	// we do not need to pass it as a parameter to the asynchronous functions.
	template <class AsynchronousOperationProcessorImplementation>
	class AsynchronousOperationTemplate
	{
		public:
			AsynchronousOperationTemplate(void) : aopi(), handle(null<Handle *>()) {}
			explicit AsynchronousOperationTemplate(Handle *const ph) : aopi(), handle(ph) {}
			//lint -e{1540}  // pointer member 'Symbol' (Location) neither freed nor zero'ed by destructor // Wrong message
			virtual ~AsynchronousOperationTemplate(void) { try{handle = null<Handle *>();}catch(...){} }
			// Wrapper functions
			// Call all functions of the concrete implementation class.
			sint aRead(byte *const readBuffer, const uint numberOfBytes, ACT *const act, const s32 offset = null<s32>());
			sint aWrite(byte *const writeBuffer, const uint numberOfBytes, ACT *const act, const s32 offset = null<s32>());
			sint aError(void) const { return aopi.aError(); }
			sint aResult(void) { return aopi.aResult(); }
			// Handle for all asynchronous operations is stored in this class and can be obtained from here
			const Handle *getHandle(void) const { return handle; }
			void setHandle(Handle *pHandle) { handle = pHandle; }

			
			
			
			
		protected:


			
			// The concrete implementation/instantiation of the Asynchronous Operation Processor (AOP)
			AsynchronousOperationProcessorImplementation aopi;
			// Pointer to handle. We cannot use the file descriptor number directly, because it may not yet exist		

			Handle *handle;
			

	};
	// The static bridge
	typedef AsynchronousOperationTemplate<AsynchronousOperationProcessorUsingLinuxAio> AsynchronousOperation;

// ------------------------------------------------------------------------------------------------------------------------------
// 4. Completion Handler


	// Abstract Base class. Interface definition for all kinds of completion handler
	class CompletionHandler
	{
		public:
			CompletionHandler(void) {}
			virtual ~CompletionHandler(void) {}
			
			// Call back functions after the completion of an asynchronous function
			
			// 1. Asynchronous call back function
			// Return value of this function is important!
			
			// With a return value AsynchronousCompletionEvent::CallSynchronousEventhandler the "handleCompletionEventSync" will be called additionally 
			// via synchronous event dispatching through the Reactor
			// With a return value AsynchronousCompletionEvent::Finalize no other action will be called. This will result in pure
			// asynchronous behaviour
			virtual AsynchronousCompletionEvent::Action handleCompletionEventAsync(ACT *actl) = 0;
			
			// 2. Synchronous Call Back function (through the Reactor)
			// Also here the return value of the function is of importance
			// Return value EventProcessing::Stop means: Stop main event loop. And with that most likely the whole program
			virtual EventProcessing::Action handleCompletionEventSync(ACT *actl) = 0;
	};

// ------------------------------------------------------------------------------------------------------------------------------
// 5. The Asynchronous completion  ACT

	// The Asynchronous completion token is using the following 2 patterns. 
	// a.) Mediator (partly)
	// b.) Asynchronous Completion Token as pattern by itself
	// It is used to decouple participating classes and to ease up the dispatching of completion events
	// by the Proactor. Additionally useful information is forwarded from the start of the asynchronous
	// operation to both completion handlers
	// The ACT is basically a POD. The constructor sets some default values. Especially the pointer
	// to the Proactor is set automatically the the global instance (via singleton)
	struct AsynchronousCompletionToken
	{
		public:
			explicit AsynchronousCompletionToken(CompletionHandler * const pch,
										AsynchronousOperation *const pao,
										Proactor * const pProactor=Proactor::getInstance()) : 	ch(pch),
																								ao(pao),
																								proactor(pProactor),
																								userDefinedActIdentifier(null<sint>()),
																								actBusyAsynchronously(false),
																								actBusySynchronously(false)
			{}
			CompletionHandler *ch;      // Pointer to completion handler class. With that we know the call back functions
			AsynchronousOperation *ao;  // Here we can obtain the file handle and the functions for obtaining status
			Proactor *proactor;         // The Proactor that will dispatch completion events
			sint userDefinedActIdentifier;   	// User may give any value to be able to identify the ACT later
			boolean actBusyAsynchronously;		// Indicates that the ACT is in use by asynchronous function
			boolean actBusySynchronously;		// Indicates that the ACT is in use by synchronous function
			// Set busy flags to busy
			void setBusy(void) { actBusyAsynchronously=true; actBusySynchronously=true; }
			boolean isBusy(void) const { return (actBusyAsynchronously || actBusySynchronously); }

			
		private:
			//lint -e{1704}  // 1704 Constructor 'Symbol' has private access specification 
			AsynchronousCompletionToken(void) : ch(null<CompletionHandler *>()),
												ao(null<AsynchronousOperation *>()),
												proactor(Proactor::getInstance()),
												userDefinedActIdentifier(null<sint>()),
												actBusyAsynchronously(false),
												actBusySynchronously(false)
			{}
		
	};




	template <class AsynchronousOperationProcessorImplementation>
	inline sint AsynchronousOperationTemplate<AsynchronousOperationProcessorImplementation>::aRead(byte *const readBuffer, const uint numberOfBytes, ACT *const act, const s32 offset)
	{
		act->setBusy();
		return aopi.aRead(readBuffer, numberOfBytes, act, offset);
	}
	template <class AsynchronousOperationProcessorImplementation>
	inline sint AsynchronousOperationTemplate<AsynchronousOperationProcessorImplementation>::aWrite(byte *const writeBuffer, const uint numberOfBytes, ACT *const act, const s32 offset) 
	{ 
		act->setBusy();
		return aopi.aWrite(writeBuffer, numberOfBytes, act, offset); 
	}


// ------------------------------------------------------------------------------------------------------------------------------
// 6. Combination: Completion Handler wir AO functionality

	// This is a helper class for completion handlers that will at the same time use asynchronous functions. 
	// Or Vice Versa: Classes that want to uses Asynchronous functions and at the same time handle completion events
	// The class already predefines the AsynchronousOperation "ao" and the ACT "act". Also the corresponding constructors
	// are called and set up appropriately
	// This is an abstract base class. Call back functions must be implemented by derived classes.
	class CompletionHandlerWithAo : public CompletionHandler
	{
		public:
			// CTor needs pointer to handle
			explicit CompletionHandlerWithAo(Handle *const phandle) : CompletionHandler(), ao(phandle), act(), dummyAct(this, &ao) {}
			virtual ~CompletionHandlerWithAo (void);
			// Callback functions
			virtual AsynchronousCompletionEvent::Action handleCompletionEventAsync(ACT *actl) = 0;  // Fully asynchronous
			virtual EventProcessing::Action handleCompletionEventSync(ACT *actl) = 0;   // Synchronous via reactor
			
		protected:
			AsynchronousOperation ao;   // The ao
			// ACTs for operations
			std::vector<ACT *>act;
			ACT dummyAct;
			// Use this to get a usable ACT
			ACT *getFreeAct(void);
		private:
			//lint -e{1704}  // 1704 Constructor 'Symbol' has private access specification --
			CompletionHandlerWithAo(void) :	CompletionHandler(), ao(), act(), dummyAct(this, &ao) {}
	};

	
 

#endif
