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


#ifndef SERIAL_HPP
#define SERIAL_HPP

#include "mytypes.hpp"

#include "eventhandler.hpp"
#include "observer.hpp"

#include <termios.h>

#include <string>



// ------------------------------------------------------------------------------------------------------------------------------
// 1. Setting Serial Port Parameters


namespace SerialInternal
{

	// --------------------------------------------------------------
	// 1.1 General


	// Helper Class for setting parameters for a serial port
	// Abstract base class
	// Handle must be present. So file is already open. FCNTL functions can be used to change behaviour
	class SerialPortParameter
	{
		public:
			// Initialize class. Copy handle
			explicit SerialPortParameter(const Handle handleSerialPort);
			// Empty ctor
			virtual ~SerialPortParameter(void) {}

			// Check if everything is OK
			boolean isOK(void) const { return(ok); }
			
		protected:
			// Set the specific data. Pure virtual function. Define specific attributes in derived class.
			virtual void setSerialPortParameter(void) = 0;	

			// Data struct needed to set attributes
			struct termios serialOptions;
			// Internal check, if everything worked well
			boolean ok;
			// Copy of file handle
			Handle handle;
			
		private:
			// Default constructors set handle to 0. Use initialize to set handle afterwards
			SerialPortParameter(void) : serialOptions() , ok(false), handle(null<Handle>())	{}
	};

	// --------------------------------------------------------------
	// 1.2 EHZ Specific
	
	
	
	// Concrete implementation for Serial Port Parameters for an EHZ
	class EhzSerialPortParameter : public SerialPortParameter
	{
		public:
			// Construct and initialize class
			//lint -e{1933,1506}
			//Note 1933: Call to unqualified virtual function 'SerialInternal::EhzSerialPortParameter::setSerialPortParameter(void)' from non-static member function
			//Warning 1506: Call to virtual function 'SerialInternal::EhzSerialPortParameter::setSerialPortParameter(void)' within a constructor or destructor [MISRA C++ Rule 12-1-1], ---    Eff. C++ 3rd Ed. item 9
			explicit EhzSerialPortParameter(const Handle xhandle) : SerialPortParameter(xhandle) { setSerialPortParameter();}
			// Empty dtor
			virtual ~EhzSerialPortParameter(void) {}

		protected:
			// Set EHZ specific serial port parameter
			virtual void setSerialPortParameter(void);		
		private:
			// Default constructor. Do not use
			EhzSerialPortParameter(void) : SerialPortParameter(0) {}
	};

	
	
	
// ------------------------------------------------------------------------------------------------------------------------------
// 2. Simple serial port class. Uses Reactor for Event Notification


	// -------------------------------------------------------------------
	// 2.2 General

	// A serial port class. Implemented as event handler for the reactor
	// Uses the device name of a serial port. Opens this device (serial port)
	// and set attributes
	class SerialPort : public CommunicationEndPoint
	{
		public:
			// Standard constructor copies the name of the serial port (device)
			explicit SerialPort(const std::string &pn) : CommunicationEndPoint(), portName(pn) {}
			// Dtor
			virtual ~SerialPort(void) { }

			// Pure virtual function. Override this to open the desired serial port in the necessary way
			virtual void start(void) = 0;
		protected: 
			// Copy of the physical port name (device)
			std::string portName;
			
		private:
			// Default ctor. Do not use
			SerialPort(void) : CommunicationEndPoint(), portName() {}
		
	};

	
	// -------------------------------------------------------------------
	// 2.3 Specific for EHZ
	
	// Has an Event Handler
	// Is notfied by the Reactor that data is present
	// Reads data from ersial port, byte by byte
	// Notifies Subscribers that data is available and 
	// provides data via one main interface function getLastReceivedByte
	
	
	// Specific call for an EHZ serial port
	class EhzSerialPort : public SerialPort, public Publisher<EhzSerialPort>
	{
		public:
			// Standard constructor. Copy name of port (device)
			explicit EhzSerialPort(const std::string &pn) : SerialPort(pn), Publisher<EhzSerialPort>(), databyte(null<EhzDatabyte>()){}		
			// Empty dtor
			virtual ~EhzSerialPort(void) {}

			// Open serial port and set parameters
			virtual void start(void);
			// Event Handler for Reactor
			virtual EventProcessing::Action handleEvent(const EventType et); 
			
			// Retrieve the last byte received by the SIO
			virtual EhzDatabyte getLastReceivedByte(void) const { return databyte; }
		protected:
			// We will read the SIO byte by byte and store here the last read byte
			EhzDatabyte databyte;
			
		private:
			// Default ctor. Do not use
			//lint -e{1901,1911}
			//Note 1901: Creating a temporary of type 'const std::basic_string<char>'
			//Note 1911: Implicit call of constructor 'std::basic_string<char>::basic_string(const char *, const std::allocator<char> &)' (see text)
			EhzSerialPort(void) : SerialPort(""), Publisher<EhzSerialPort>(), databyte(null<EhzDatabyte>()) {}
		
	};

}

 

#endif
