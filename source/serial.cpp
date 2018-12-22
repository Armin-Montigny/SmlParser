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
// serial.hpp
//
// General Description
//
// This module is a part of an application tor read the data from Electronic Meters for Electric Power
//
// In Germany they are called EHZ. Hence the name of the module an variables.
//
// The EHZ transmits data via an infrared interface. The protocoll is described in:
//
//  http://www.emsycon.de/downloads/SML_081112_103.pdf
//
// All this is done via a standard serial RS232 optical interface
// Here we have some basic classes to handle a serial port
//

#include "userinterface.hpp"

#include "serial.hpp"

#include <unistd.h>
#include <fcntl.h>



// ------------------------------------------------------------------------------------------------------------------------------
// 1. Setting Serial Port Parameters


namespace SerialInternal
{
	// ---------------------------------------------------------------------
	// 1.1 Construcor reads the current settings of an already opend port
	SerialPortParameter::SerialPortParameter(const Handle handleSerialPort) : serialOptions() , ok(false), handle(handleSerialPort)
	{
		ok = false;
		handle = handleSerialPort;
		if (handleSerialPort >= null<Handle>())
		{
			if (null<sint>() == tcgetattr(handleSerialPort, &serialOptions))
			{
				ok = true;
			}
		}
	}
	
	// ---------------------------------------------------------------------
	// 1.2 Set serial Port options to 9600N81

	void EhzSerialPortParameter::setSerialPortParameter(void)
	{

		//lint -e{917,1960,9001}
		//Note 9001: Octal constant used
		//Note 917: Prototype coercion (arg. no. 2) int to unsigned int
		if (-1 == cfsetispeed(&serialOptions, B9600))
		{
			ok = false;
		}
		if (isOK())
		{
			cfmakeraw(&serialOptions);	
			if (null<sint>() != tcsetattr(handle, TCSANOW, &serialOptions))
			{
				ok = false;
			}
		}
	}
	
// ------------------------------------------------------------------------------------------------------------------------------
// 2. Handle Serial Port
	
	// ---------------------------------------------------------------------
	// 2.1 Open and set serial port connection options / parameters
	void EhzSerialPort::start(void)
	{
		boolean rc = false;
		// if it is not open, then open serial port
		if (null<Handle>() == handle)
		{
			//lint -e{9001} Octal constant used
			handle = open(portName.c_str(),O_RDWR);	//lint !e1960
			if (handle >= null<Handle>())
			{
				// Set port parameter
				//ui.msgf("Open  %s   %d\n",  portName.c_str(), handle);
				EhzSerialPortParameter ehzSerialPortParameter(handle);
				rc = ehzSerialPortParameter.isOK();
			}
		}
		if (!rc)
		{
			ui << "Error: Could not open serial port --> " << portName << std::endl;
			//lint -e{1933}   Note 1933: Call to unqualified virtual function 'CommunicationEndPoint::stop(void)' from non-static member function
			// Close in case of problems
			stop();
		}
	}
	
	
	// ---------------------------------------------------------------------
	// 2.2 Reactor reported that data is available. Read it
	EventProcessing::Action EhzSerialPort::handleEvent(const EventType et) 	
	{ 
		//lint --e{921} 921 Cast from Type to Type --
		//ui.msgf("Serial Port EventHandler %d\n",static_cast<sint>(et));
		EventProcessing::Action rc = EventProcessing::Continue;
		switch (static_cast<sint>(et))
		{
			case EventTypeIn:
				{
					sint bytesread;
					bytesread = read(handle,&databyte,1U);
					// CHeck if have read one databyte from the serial port 
					if (1 == bytesread)
					{
						//lint -e{1933}   Note 1933: Call to unqualified virtual function 'CommunicationEndPoint::stop(void)' from non-static member function
						//ui.msgf("-(%02X)-  ",static_cast<sint>(databyte));
						notifySubscribers();
					}
				}
				break;
			default:
				// unexpected Event
				rc = EventProcessing::Stop;
				break;
			
		}
		return rc; 
	}
}		

