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
// tcpconnection.hpp
//
//
// General Description
//
// This module is a part of an application to read the data from Electronic Meters for Electric Power
//
// In Germany they are called EHZ. Hence the name of the module an variables.
//
// The EHZ transmits data via an infrared interface. The protocoll is described in:
//
//  http://www.emsycon.de/downloads/SML_081112_103.pdf
//
// After data has been evaluated it will be provided to others for further usage.
//
// Server and Client classes for communicating data over a TCP Connection
// Socket must be open and ready to use
//
// There are generic classes for Communication and EHZ specific classses.
// Additionally a nano WebServer is present.
//
// Either all measured values from an EHZ can be shown or just the Powerstate are transmitted
// With that it is possible to see, if there is spare power that can be consumed by special devices
//


#ifndef TCPCONNECTION_HPP
#define TCPCONNECTION_HPP



#include "userinterface.hpp"

#include "observer.hpp"
#include "timerevent.hpp"
#include "ehzmeasureddata.hpp"


 
 





// ------------------------------------------------------------------------------------------------------------------------------
// 1. General Definitions
	
	const mchar tcpConnectionGetEhzDataCommand[] = "g";

	// Base class for TCP connections
	// Handles data exchange over TCP connections
	// Reads data buffered

	const size_t MaxSizeReceiveBuffer = 64U;

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Generic Base class for all TCP Connections
	
	
	class TcpConnectionBase : 	public CommunicationEndPoint, 
								public Publisher<TcpConnectionBase>
	{
		public:
			// Explicit constructor 
			// Constructor gets the connection handle (socket handle) 

			explicit TcpConnectionBase(const Handle connectionHandle) : CommunicationEndPoint(connectionHandle), Publisher<TcpConnectionBase>(), receivedRawData(), peerIPAddress(),peerIPAddressPort() {}

			// Empty virtual destructor so that we can delete derived classes via base class
			virtual ~TcpConnectionBase(void) {}		
			
			// Start does nothing. 
			//lint -e{1961}   1961 virtual member function 'Symbol' could be made const
			virtual void start(void) {}
			
			// Event handler for reactor. Handle all incoming events (data is arriving)
			virtual EventProcessing::Action handleEvent(const EventType et);
			
			virtual void setPeerAddressData(const std::string address, const std::string port) {peerIPAddress=address;peerIPAddressPort=port;}
			virtual std::string getPeerIPAddress(void) {return peerIPAddress;}
			virtual std::string getPeerIPAddressPort(void) {return peerIPAddressPort;}
		protected:
			// Because a lot of the functionality of the handleEvent function is the same
			// we use an specific function for handling the data received by handleEvent
			// So we can reuse most functionality and override this function for the specifics. Hence pure virtual
			virtual EventProcessing::Action handleReadData(const sint bytesRead) = 0;

			// // The buffer for the raw read data
			mchar receivedRawData[MaxSizeReceiveBuffer];
			
			// Some Address information of peer
			std::string peerIPAddress;
			std::string peerIPAddressPort;
		private:
			// Do not use
			// Default constructor. Handle must be set before using this class
			//lint -e{1704}     1704 Constructor 'Symbol' has private access specification
			TcpConnectionBase(void) : CommunicationEndPoint(), Publisher<TcpConnectionBase>() , receivedRawData() , peerIPAddress(),peerIPAddressPort(){ handle = null<Handle>(); }
	};

// ------------------------------------------------------------------------------------------------------------------------------
// 3. TCP Connection Server

	// -------------------------------------------------------------------------------
	// 3.1. TCP Connection Server for transmitting all measured values from all EHZ
		
		class EhzSystem;
		// A data server for rather raw connections
		// The raw contents of the data values read by the EHZ will be transmitted
		// Transmission of data will be done in ASCII. Framed bei STX and ETX and seperated by US
		class TcpConnectionEhzDataServer :	public TcpConnectionBase
		{
			public:
				// Just store the connection handle / socket handle
				explicit TcpConnectionEhzDataServer(const Handle connectionHandle) : TcpConnectionBase(connectionHandle), outputData(), ehzSystem(null<EhzSystem *>()) {}
				virtual ~TcpConnectionEhzDataServer(void) {}	//lint !e1540
				virtual void setEhzSystemDataPointer(EhzSystem *const ehzSystemP) { ehzSystem = ehzSystemP; }


			protected:
				virtual EventProcessing::Action handleReadData(const sint bytesRead);
				// Build the data that we want to send to the connected peer
				// This is the specific working horse for this functionality
				virtual void buildOutputData(void);
				// We need persistent data, becuase we will send this data via aio services
				std::string outputData;
				// Set reference data
				EhzSystem *ehzSystem;

			private:
				//lint -e{1704}      1704 Constructor 'Symbol' has private access specification
				TcpConnectionEhzDataServer(void) : TcpConnectionBase(null<Handle>()), outputData(), ehzSystem(null<EhzSystem *>()) {}
		};									

	// -------------------------------------------------------------------------------
	// 3.2. TCP Connection Server for transmitting the overall Power state

		// A data server for just transmitting the overall power (plus or minus) of the system
		// Transmission of raw data, embedded in STX ETX
		class TcpConnectionEhzPowerStateServer : public TcpConnectionEhzDataServer
		{
			public:
				// Just store the connection handle / socket handle
				explicit TcpConnectionEhzPowerStateServer(const Handle connectionHandle) : TcpConnectionEhzDataServer(connectionHandle) {}
				virtual ~TcpConnectionEhzPowerStateServer(void) {}
			protected:
				// Build specific output data. Just the power value as ascii
				virtual void buildOutputData(void);
			private:
				//lint -e{1704}     1704 Constructor 'Symbol' has private access specification
				TcpConnectionEhzPowerStateServer(void) : TcpConnectionEhzDataServer(null<Handle>()) {}
		};



	// -------------------------------------------------------------------------------------
	// 3.3. TCP Connection Server for transmitting all measured values from all EHZ in HTML

		// So this is a tiny nano Web Server

		// This class will answer to a GET request from a web browser and send the measured values from the EHZ to the connected peer
		class TcpConnectionSimpleHtmlAnswer : public TcpConnectionEhzDataServer
		{
			//lint --e{935}      935 int within struct
			public:
				// Just store the connection handle / socket handle
				explicit TcpConnectionSimpleHtmlAnswer(const Handle connectionHandle) : TcpConnectionEhzDataServer(connectionHandle), state(StateWaitForGet), 
					url(),headerLengthCounter(null<sint>()), newLineCounter(null<sint>()), matchIndex(null<uint>()) {}
					
				virtual ~TcpConnectionSimpleHtmlAnswer(void) {}
			protected:
				// Parse data that is sent from the peer
				virtual EventProcessing::Action handleReadData(const sint bytesRead);
				// Buld the reply string, so reply all EHZ data in HTML
				virtual void buildOutputData(void);
				
				// Maximum number of HTML Get header that we will wait for. Safeguard.
				enum {maxHeaderLength = 1024};  
				// States for small state machine to parse GET request
				enum {StateWaitForGet, StateWaitForUrl, WaitForHeaderEnd}; 
				// The current state of the small state machine to parese the HTML get request header
				sint state; 
				// The URL that is contained in the GET header will be stored here
				std::string url; 
				// Counter for bytes in HTML GET request header. We will only analyze a maximun number of 
				// bytes in the header. Otherwise we will ignore it and reset the parser
				sint headerLengthCounter;
				// 2 new lines separate the header from the rest. Will be used while parsing
				sint newLineCounter;
				// Will be using to parse the GET token
				uint matchIndex;
				
			private:
				// delete standard constructor
				//lint -e{1704}     1704 Constructor 'Symbol' has private access specification
				TcpConnectionSimpleHtmlAnswer(void) : TcpConnectionEhzDataServer(null<Handle>()), state(StateWaitForGet), 
					url(),headerLengthCounter(null<sint>()), newLineCounter(null<sint>()), matchIndex(null<uint>()) {}
		};

	// -------------------------------------------------------------------------------------
	// 3.4. TCP Connection Server for transmitting the Powerstate of the EHZ System in HTML

		// So this is a tiny pico Web Server
		// Simply return the overall power state of the EHZ system to a WEB browser
		class TcpConnectionSimpleHtmlAnswerPowerState : public TcpConnectionSimpleHtmlAnswer
		{
			public:
				explicit TcpConnectionSimpleHtmlAnswerPowerState(const Handle connectionHandle) : TcpConnectionSimpleHtmlAnswer(connectionHandle) {}
				virtual ~TcpConnectionSimpleHtmlAnswerPowerState(void) {}
			protected:
				// Build the reply string in HTML. Send power information
				virtual void buildOutputData(void);
			private:
				//lint -e{1704}       1704 Constructor 'Symbol' has private access specification
				TcpConnectionSimpleHtmlAnswerPowerState(void) : TcpConnectionSimpleHtmlAnswer(null<Handle>()) {}
		};




	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	
// ------------------------------------------------------------------------------------------------------------------------------
// 4. TCP Connection Clients

	// ------------------------------------------------------------------------------------------------------------------------------
	// 4.1 TCP Connection Client: Get all EHZ System data from connected Server
	
		// Send all x seconds a request to the server and get last EHZ System Data
		class TcpConnectionGetEhzDataClient :	public TcpConnectionBase,
												public Subscriber<EventTimer>
		{
			//lint --e{935}
			public:
				// Give socket handle and specify poll time
				explicit TcpConnectionGetEhzDataClient(const Handle connectionHandle, const u32 pollPeriod = 30000UL);
				virtual ~TcpConnectionGetEhzDataClient(void);
				
				// Start the communication. Start the timer and wait for notification event
				// Function is given becuase of virtual start function
				virtual void start(void) { this->startPoll();}

				// Start the poll timer and add handler to reactor
				virtual void startPoll(void);
				// Stop timer
				virtual void stopPoll(void);
				
				
			protected:
			
				// Notify all registered subscribers
				// This means that the TCP connection was closed
				virtual void notifySubscribers(void); 

				// Here we will store the result. So, the data from the other side
				std::vector<EhzInternal::AllMeasuredValuesForOneEhz> vEMDA;	
				// This function will be invoked when the timer fires. It will send a get request to the connected peer
				//lint -e{955,1768}
				virtual void update(EventTimer *const);
				// Data over TCP connection is transferred via ASCII. With beginning STX, terminating ETX and US as separator
				// From take this transmitted strings and create values out of that
				virtual void setValuesFromStrings(std::vector<std::string>::iterator &iter);
				// Handle the received data
				virtual EventProcessing::Action handleReadData(const sint bytesRead);
				// Internal state machine for conversion of received data
				enum {StateWaitForStart, StateDoConversion};
				// The request command ('g') that will be transmitted to the connected peer
				const std::string requestCommand;
				// And the timer taht we use for periodic polling data from the connected peer
				EventTimer pollTimer;
				// Here we store all received data as single string. Afterwords we will convert all stings back to the corresponding data structure
				std::vector<std::string> receivedDataAsStrings;
				// Temporary string container for byte by byte received data
				std::string convertedDataPart;
				// Current state of the internal state machine for converions of received data
				sint state;
				
			private:
				//lint -e{1704}
				TcpConnectionGetEhzDataClient(void);
		};


	// ------------------------------------------------------------------------------------------------------------------------------
	// 4.2 TCP Connection Client: Get all EHZ System Power State from a connected Server


		// Get all x seconds the Power state form the connected client
		// We want to know if we have enough solar power to consume
		class TcpConnectionGetEhzPowerStateClient : public TcpConnectionGetEhzDataClient
		{
			public:
				// Give socket handle and specify poll time
				explicit TcpConnectionGetEhzPowerStateClient(const Handle connectionHandle, const u32 pollPeriod = 5000UL) : TcpConnectionGetEhzDataClient(connectionHandle, pollPeriod), power(0.0) {}
				virtual ~TcpConnectionGetEhzPowerStateClient(void) {}
			protected:
				// Data over TCP connection is transferred via ASCII. With beginning STX, terminating ETX and US as separator
				// From take this transmitted strings and create values out of that
				virtual void setValuesFromStrings(std::vector<std::string>::iterator &iter);
				//The resulting read value
				mdouble power;
			private:
				//lint -e{1704}
				TcpConnectionGetEhzPowerStateClient(void) : TcpConnectionGetEhzDataClient(null<Handle>(), null<u32>()), power(0.0) {}
		};


	// ------------------------------------------------------------------------------------------------------------------------------
	// 4.3 TCP Connection Client: Calculate the overall power state from the received data

		// Convert the received ASCII string into a double. So we will calculate the power here
		inline void TcpConnectionGetEhzPowerStateClient::setValuesFromStrings(std::vector<std::string>::iterator &iter)
		{
		
			static sint i=0;
			power = strtod((*iter).c_str(),null<mchar **>());++iter;
			ui << "Power " << i << " " << power << std::endl;
			++i;
		}


 

#endif
