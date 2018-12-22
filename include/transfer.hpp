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
// transfer.hpp
//
// Send data asynchronously to a stream
//
// Helper functions for asynchronous data transmission. Mainly used to send data via TCP.
// in most cases, handles of file descriptors point to sockets
//
// A class is defined for transmitting data over a socket asynchronously and with
// no handshake or whatsoever. Meaning, we don't care, if the function succeeds or not.
// The class instance is dynamically created on the heap and self destructs after it
// completes its work.
//


#ifndef TRANSFER_HPP
#define TRANSFER_HPP


#include "proactor.hpp"
#include <sstream>


// -------------------------------------------------------------------------------------------------------------------------------------
// 1. Wrapper functions for transmitting data via class AsynchronousDataWriterWithSelfDestruct

	// -----------------------------------------------------
	// 1.1 Declaration of functions defined in transfer.cpp
	
	// This functions will always be used to transmit the data
	// So far we will not use and instance of AsynchronousDataWriterWithSelfDestruct explicetely
	
		extern void writeDataAsynchronous(const std::string &sourceString, const Handle h);
		extern void writeDataAsynchronous(const std::ostringstream &s, const Handle h);

	namespace TransferInternal
	{

	// -----------------------------------------------------
	// 1.2 Asynchronous Data Transmission class

		// Typically this class will be created / instantiated on the heap
		// The function operator () will be called.
		// After the work is finsihed, the object will self destruct via delete this
	
		class AsynchronousDataWriterWithSelfDestruct : public CompletionHandlerWithAo
		{
			public:
				// The constructors will work with different types of sources. Handle (file descriptor) to socket will always be given

				// Take a string as source
				explicit AsynchronousDataWriterWithSelfDestruct(const std::string &sourceString, const Handle h);
				// Take a stringstream as source
				explicit AsynchronousDataWriterWithSelfDestruct(const std::ostringstream &s, const Handle h);
				// Self destruct
				AsynchronousDataWriterWithSelfDestruct(void);
				
				// The only interface of this class
				void operator ()(void);
				
				// Eventhandler for proactor
				virtual AsynchronousCompletionEvent::Action handleCompletionEventAsync(ACT *actl);
				virtual EventProcessing::Action handleCompletionEventSync(ACT *actl);			
			protected:
				//lint -e{9008}    Bug in Lint
				void destroy(void) { delete this;}
				// We do not want to have called the destructor explicetly				
				virtual ~AsynchronousDataWriterWithSelfDestruct(void){}
				
				
		
				// Handle or file desciptor of (normally) an open TCP socket
				Handle handle;
				// And the output string. All data given in the contructors will be copied into this variable
				// The ourputString is a persistent buffer and exists until all AIO functions are completed
				// and the class finally self destructs
				std::string outputString;
		};

	// -----------------------------------------------------------
	// 1.3 Asynchronous Data Transmission class inline functions
		

		// -----------------------------------------------------------
		// 1.3.1 Constructors
		// All constructors copy the given data into the internal data buffer outputString
		
			
			// Source is string
			inline AsynchronousDataWriterWithSelfDestruct::AsynchronousDataWriterWithSelfDestruct(const std::string &sourceString, const Handle h) : 
				CompletionHandlerWithAo(&handle),
				handle(h),
				outputString(sourceString)
			{
			}

			// Source is ostringstream
			inline AsynchronousDataWriterWithSelfDestruct::AsynchronousDataWriterWithSelfDestruct(const std::ostringstream &s, const Handle h) : 
				CompletionHandlerWithAo(&handle),
				handle(h),
				outputString(s.str())
			{
			}
			
			// Default constructor will not be called. Sets everything to null and empty
			inline AsynchronousDataWriterWithSelfDestruct::AsynchronousDataWriterWithSelfDestruct(void) : 
				CompletionHandlerWithAo(&handle),
				handle(null<Handle>()),
				outputString()
			{
			}



		// -----------------------------------------------------------
		// 1.3.2 Write the data

			inline void AsynchronousDataWriterWithSelfDestruct::operator () (void)
			{
				//lint -e(1960,925,9005,534,926)
				ao.aWrite(reinterpret_cast<byte* const>(const_cast<mchar *>(outputString.c_str())), outputString.length(), getFreeAct());
			}
	}

 

	
#endif
