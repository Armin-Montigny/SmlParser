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
// transfer.cpp
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


#include "transfer.hpp"

// -------------------------------------------------------------------------------------------------------------------------------------
// 1. Wrapper functions for transmitting data via class AsynchronousDataWriterWithSelfDestruct

	// This functions will always be used to transmit the data
	// So far we will not use and instance of AsynchronousDataWriterWithSelfDestruct explicetely
	
	
	// Functions to write data from different source
	

	// From a string
	void writeDataAsynchronous(const std::string &sourceString, const Handle h)
	{
		//lint --e{429}  "Custodial pointer 'Symbol' (Location) has not been freed or returned".   Yes, intended
		TransferInternal::AsynchronousDataWriterWithSelfDestruct *const adw = new TransferInternal::AsynchronousDataWriterWithSelfDestruct(sourceString,h);
		(*adw)();
	}
	
	// From a ostringstream
	void writeDataAsynchronous(const std::ostringstream &s, const Handle h)
	{
		//lint --e{429}  "Custodial pointer 'Symbol' (Location) has not been freed or returned".   Yes, intended
		TransferInternal::AsynchronousDataWriterWithSelfDestruct *const adw = new TransferInternal::AsynchronousDataWriterWithSelfDestruct(s,h);
		(*adw)();
	}
	
	
namespace TransferInternal
{
// -----------------------------------------------------------
// 2. Event Handler for Proactor

	// Call back for asynchronous function. We will do nothing
	// Just forward to synchronous event handler
	//lint -e{1961}    1961 virtual member function 'Symbol' could be made const
	AsynchronousCompletionEvent::Action AsynchronousDataWriterWithSelfDestruct::handleCompletionEventAsync(ACT *)
	{
		return AsynchronousCompletionEvent::CallSynchronousEventhandler;
	}
	
	// This object and function will be destroyed synchronously
	//lint -e{952} "Parameter 'Symbol' (Location) could be declared const"   No. Cannot.
	EventProcessing::Action  AsynchronousDataWriterWithSelfDestruct::handleCompletionEventSync(ACT *actl) 
	{ 
		// ACT for this operation is no longer in use
		actl->actBusySynchronously = false;	
		// We will delete "this"
		destroy(); 
		// Continue with main event loop of Reactor
		return EventProcessing::Continue; 	
	}
}

 

