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
// This module is a part of an application tor read the data from Electronic Meters for Electric Power
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



#include "tcpconnection.hpp"
#include "ehz.hpp"
#include "reactor.hpp"
#include "transfer.hpp"
#include "timerevent.hpp"

#include <sys/socket.h>
#include <errno.h>

#include <sstream>


// ------------------------------------------------------------------------------------------------------------------------------
// 1. General Definitions

	const size_t EhzIndexForEhzPowerState = 1U;
	const size_t EhzValueIndexForEhaPowerState = 3U;

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Generic Base class for all TCP Connections
			

	// ------------------------------------------------------------------------
	// 2.1 Event handler for TCP connection. The reactor got an read event
	
		EventProcessing::Action TcpConnectionBase::handleEvent(const EventType et)
		{
			// Standard eventhandler for TCP connections
			// We assume that everything will be OK and that we will continue the main event loop of the reactor
			EventProcessing::Action rc = EventProcessing::Continue;
			//lint -e{911}   Note 911: Implicit expression promotion from short to int
			// Check returned Event type. We asked for EventTypeIn  (POLLIN)
			if (EventTypeIn == et)
			{
				// Data should be available. Read the data. Of course we have a top cap with buffer size.
				// Reading is done buffered in chunks
				const sint bytesRead = read(handle, &receivedRawData[0], MaxSizeReceiveBuffer);
				// Did we read something
				if (bytesRead > null<sint>())
				{
					//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function
					// Process the read data. Overloaded Function
					// Every derived class can have a different handling of the read data
					rc = handleReadData(bytesRead);
				}
				else if (null<sint>() == bytesRead)
				{	
					//lint -e{911}   Note 911: Implicit expression promotion from short to int

					// Zero bytes read. This means: The peer closed the connection
					
					// Some debug stuff
					static volatile sint i=1;
					
					ui << "TCP Connection " << i << " closed: " << getPeerIPAddress() << ':' <<  getPeerIPAddressPort() << std::endl;
					++i;
					
					
					// Inform others on this event. CLose the TCP connection
					//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function
					notifySubscribers();
				}
				else
				{
					// Read return value was negative
					
					// This can happen if we got the info that an POLLIN event is available
					// but meanwhile the connection has been closed.
					// In an implementation with epoll, this could of course happen if there are
					// tons of pending events but only few events processed in one run
					if (ECONNRESET != errno)
					{
						// If the read function shows a different error then we will terminate the main event loop
						rc = EventProcessing::Error;
					}
					ui << "TCP Connection  Error Data Server: "<< bytesRead << " Error Number: " << errno<< " "<< strerror(errno);
					// In any case. Stop this connection
					//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function
					notifySubscribers();
				}
			}
			else
			{
				// For Events like POLLERR or POLLHUP or POLLRDHUP or something
				// In any case. Stop and close this connection

				// Debug messages	
				sint result;
				socklen_t result_len = sizeof(result);
				// Check for error
				const sint getsockoptResult = getsockopt(handle, SOL_SOCKET, SO_ERROR, &result, &result_len);
				if (getsockoptResult >= null<sint>())
				{
					static volatile sint i = null<sint>();
					ui << "------------------TCP Connection " << i << this << et << result << strerror(result) << std::endl;
					
					++i;
				}
				else
				{
					ui << "------------------Get Sock Option Error" << std::endl;
				}

				// Inform others on this event. Close the TCP connection
				//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function
				notifySubscribers();
			}
			return rc; 	
		}
	
	
	
// ------------------------------------------------------------------------------------------------------------------------------
// 3. TCP Connection Server

	// -------------------------------------------------------------------------------
	// 3.1. TCP Connection Server for transmitting all measured values from all EHZ
		
		// Handle the data read by handleEvent
		EventProcessing::Action TcpConnectionEhzDataServer::handleReadData(const sint )
		{
			//lint -e{911}   /Note 911: Implicit expression promotion from char to int
			//ui << "TcpConnectionEhzDataServer::handleReadData   " <<receivedRawData[0] << std::endl;
			// Wiat for a one digit request command
			//lint -e{911,1960,917}   /
			//Note 911: Implicit expression promotion from char to int
			//Note 1960: Violates MISRA C++ 2008 Required Rule 5-0-4, Implicit conversion changes signedness
			//Note 917: Prototype coercion (arg. no. 1) int to unsigned int
			if (tcpConnectionGetEhzDataCommand[0] == receivedRawData[0])
			{
				//lint -e{1933} Note 1933: Call to unqualified virtual function 'TcpConnectionEhzDataServer::buildOutputData(void)' from non-static member function			
				// Received request command
				// Build the data that we want to send back
				buildOutputData();
				// And send back the data asynchronously
				writeDataAsynchronous(outputData,handle);
			}
			// Continue main ecent loop of reactor
			return EventProcessing::Continue;
		}
		
		// Build the output data for an EhzDataServer
		// Return the information of the complete EHZ system
		void TcpConnectionEhzDataServer::buildOutputData(void)
		{
			std::ostringstream oss;
			// Will call all child functions. Output all EhzSystem data in a raw format
			// STX data1 US data2 US ... dataN US ETX
			if (null<EhzSystem *>() == ehzSystem)
			{
				ui << "NULL Pointer derefencing in void TcpConnectionEhzDataServer::buildOutputData(void)" << std::endl;
			}
			else
			{
				oss << *ehzSystem;
			}
			outputData = oss.str();
		}

	// -------------------------------------------------------------------------------
	// 3.2. TCP Connection Server for transmitting the overall Power state


		// Inform other side, how many KW the complete system is using
		// This is important for an avaluation, if solar power is available for usage or not.
		void TcpConnectionEhzPowerStateServer::buildOutputData(void)
		{
			std::ostringstream oss;
			//lint -e{1963,1950,9050}
			// This is completely specific for my own system. For others it has to be adapted.
			if (null<EhzSystem *>() == ehzSystem)
			{
				ui << "NULL Pointer derefencing in void TcpConnectionEhzPowerStateServer::buildOutputData(void)" << std::endl;
			}
			else
			{
				oss << charSTX << (ehzSystem->getEhzSystemResult()[EhzIndexForEhzPowerState].measuredValueForOneEhz[EhzValueIndexForEhaPowerState].doubleValue ) << charUS << charETX; 
			}
			outputData = oss.str();
		}




	// -------------------------------------------------------------------------------------
	// 3.3. TCP Connection Server for transmitting all measured values from all EHZ in HTML

		// Build an answer (without the HTML header) for a HTML GET request
		// Build HTML for all EHZ System data
		void TcpConnectionSimpleHtmlAnswer::buildOutputData(void)
		{
			//This will hold the complete HTML output page
			std::ostringstream htmlOut;
			
			if (null<EhzSystem *>() == ehzSystem)
			{
				ui << "Dereferencing null pointer in void TcpConnectionSimpleHtmlAnswer::buildOutputData(void)"  << std::endl;
			}
			else
			{
				
				
				//lint -e{1963,1950,9050}
				// Oh Oh. We misuse HTML tables for formatting
				htmlOut << "<html><body><table border=""0"">";
				
				// For all EHZ in the EHZ system
				for (uint noEhz = null<uint>(); noEhz < EhzInternal::MyNumberOfEhz; ++noEhz)
				{
					//lint -e{1963,1950,9050}
					// Start HTML table and insert Name of the EHZ
					htmlOut << "<tr><td><br><b>" << EhzInternal::myEhzConfigDefinition[noEhz].EhzName << "</b></td><td> </td><td> </td><td> </td></tr>";
					// Now for each value for one of the EHZ in the EHZ system
					for (uint noemd = null<uint>(); noemd< NumberOfEhzMeasuredData; ++noemd)
					{
						// Get the type of the value stored in the EHZ. Either Double or Text, or NULL (Nothing)
						const EhzMeasuredDataType::Type emdt = EhzInternal::myEhzConfigDefinition[noEhz].ehzMeasuredDataType[noemd];
						// If there is an associated value
						if (EhzMeasuredDataType::Null != emdt)
						{
							//lint -e{1963,1950,9050}
							htmlOut << "<tr><td>" << EhzInternal::myEhzConfigDefinition[noEhz].ehzDataValueDefinition[noemd].NameForDataValue << ":</td><td> </td><td>";
							// Check the type of the value. Is either Number (Double) or Text
							// and then define the type of the corresponding field
							// Basically SQLITE doesnt care so much about types, but anyway. Lets assign the right type
							if (EhzMeasuredDataType::Number == emdt)
							{
								// Type: Number / DOUBLE / Float
								//lint -e{1963,1950,9050}
								htmlOut << ehzSystem->getEhzSystemResult()[noEhz].measuredValueForOneEhz[noemd].doubleValue;
							}
							else
							{
								// Type: Text
								//lint -e{1963,1950,9050}
								htmlOut << ehzSystem->getEhzSystemResult()[noEhz].measuredValueForOneEhz[noemd].smlByteString.c_str();
							}
							//lint -e{1963,1950,9050}
							htmlOut << " " << ehzSystem->getEhzSystemResult()[noEhz].measuredValueForOneEhz[noemd].unit.c_str() << "</td><td> </td></tr>";
						}
					}
				}
				//lint -e{1963,1950,9050}
				htmlOut << "</table></body></html>";
			}
			// And copy to internal buffer
			outputData = htmlOut.str();
			

		}

	// -------------------------------------------------------------------------------------
	// 3.4. Web Server for EHZ System specific data
	
		
		// Handle read date given by handleEvent from the reactor
		// This will implement a web server
		// Wait for HTML GET header. Parse it. Send requested data back
		EventProcessing::Action TcpConnectionSimpleHtmlAnswer::handleReadData(const sint bytesRead)
		{
			// For all bytes in the buffer read by handleEvent
			for (sint byteIndex = null<sint>(); byteIndex < bytesRead; ++byteIndex)
			{	
				// Get the current byte out of the buffer
				const mchar currentByte = receivedRawData[byteIndex];
				// Ignore \r
				//lint -e{911}   /Note 911: Implicit expression promotion from char to int
				// Skip \r
				if ('\r' != currentByte) 
				{
					// So. It was no \r. This is a small state machine for parsing the HTML GET string
					// We expect: "GET <url> \n\n"
					switch (state)
					{
						case StateWaitForGet:
							{
								// In the beginning we wait for the wort "GET "
								const mchar gc[] = "GET ";
								// Did we find the expected character
								//lint -e{911}   /Note 911: Implicit expression promotion from char to int
								if (gc[matchIndex] == currentByte)
								{
									// Then wait for next
									++matchIndex;
									const uint sz = (sizeof(gc)/sizeof(gc[0]))-1U;
									// did we find all expected characters?
									if (sz == matchIndex)
									{
										// If so, then reset internal variables for next try and goto next state
										matchIndex = null<uint>();
										// Now we want to read the URL
										state = StateWaitForUrl;
										// Reset URL to empty
										url = "";
									}
								} 
								else
								{
									// No, we did not find the expected character. 
									// We will wait for the first character of the search string "GET " again
									matchIndex = null<uint>();
								}
							}
							break;
							
						// So. We read "GET ". Now we want to read the url	
						case StateWaitForUrl:
							//lint -e{911}   /Note 911: Implicit expression promotion from char to int
							// The blank is the separator and shows the end of the url
							if (' ' == currentByte)
							{
								// End of url string found. Go and wait for the end of the header (\n\n)
								state = WaitForHeaderEnd;
								// The headerLengthCounter is used as a safeguard. 
								// If there are too many bytes, we will terminate the functionality
								headerLengthCounter = null<sint>();
								// This will count the newlines. We are waiting for 2
								newLineCounter = null<sint>();
							}
							else
							{
								// Strill reading url. Add next byte to url string
								url += currentByte;
							}
							break;
							
						// We have read the url and a blank. Now we are waiting for 2 \n as in indicator for end of header	
						case WaitForHeaderEnd:
							//lint -e{911}   /Note 911: Implicit expression promotion from char to int
							// \n found?
							if ('\n' == currentByte)
							{	
								++newLineCounter;
								// If we found 2 \n in a row. The header is complete
								if (2 == newLineCounter)
								{
									//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function

									// Build the HTML reply string
									buildOutputData();
									// Get the length (necessary for the header)
									const std::string::size_type length = outputData.length();
									
									// Build the header for the reply string
									std::ostringstream htmlOut;
									//lint -e{1963,1950,9050}
									htmlOut << "HTTP/1.1 200 OK\r\nContent-Length: " << length << "\r\n\r\n" << outputData;	

									// Send complete reply to peer
									writeDataAsynchronous(htmlOut,handle);
									
									// Wait for next GET command
									state = StateWaitForGet;
								}
							}
							else
							{
								// No 2 \n in a row
								// Reset the counter for the next try
								newLineCounter = null<sint>();
								// Safeguard counter.
								// If there are 2 many bytes without an end of header, we will stop parsing
								++headerLengthCounter;
								if (headerLengthCounter > maxHeaderLength)
								{
									// Wait for next get
									state = StateWaitForGet;   
								}
							}
							break;
							
						default:
							// Should not happen
							matchIndex = null<uint>();
							state = StateWaitForGet;
							break;
					}
				}
			}
			return EventProcessing::Continue;
		}

	// -------------------------------------------------------------------------------------
	// 3.5. TCP Connection Server for transmitting the Powerstate of the EHZ System in HTML

		// Inform other side, how many KW the complete system is using
		// This is important for an evaluation, if solar power is available for usage or not.
		void TcpConnectionSimpleHtmlAnswerPowerState::buildOutputData(void)
		{
			std::ostringstream htmlOut;
			
			if (null<EhzSystem *>() == ehzSystem)
			{
				ui << "Dereferencing NULL pointer in void TcpConnectionSimpleHtmlAnswerPowerState::buildOutputData(void)"  << std::endl;
			}
			else
			{
				//lint -e{1963,1950,9050}
				htmlOut << "<html><body>Gesamtleistung: ";
				//lint -e{1963,1950,9050}	
				htmlOut << ehzSystem->getEhzSystemResult()[EhzIndexForEhzPowerState].measuredValueForOneEhz[EhzValueIndexForEhaPowerState].doubleValue;
				//lint -e{1963,1950,9050}
				htmlOut << ehzSystem->getEhzSystemResult()[EhzIndexForEhzPowerState].measuredValueForOneEhz[EhzValueIndexForEhaPowerState].unit.c_str();
				//lint -e{1963,1950,9050}	
				htmlOut << "</body></html>";
			}
			outputData = htmlOut.str();
		}


		
		
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	
// ------------------------------------------------------------------------------------------------------------------------------
// 4. TCP Connection Clients

	// --------------------------------------------------------------------------------------------------------------------------
	// 4.1 TCP Connection Client: Get EHZ System data from connected Server
	
	
		// Get periodically data from a connected server

		// ---------------------
		// 4.1.1 Administration
		
			// Explicit constructor
			TcpConnectionGetEhzDataClient::TcpConnectionGetEhzDataClient(const Handle connectionHandle, const u32 pollPeriod) : 	
					TcpConnectionBase(connectionHandle),
					Subscriber<EventTimer>(),
					vEMDA(EhzInternal::MyNumberOfEhz),
					requestCommand(&tcpConnectionGetEhzDataCommand[0]),
					pollTimer(pollPeriod),
					receivedDataAsStrings(),
					convertedDataPart(),
					state(StateWaitForStart)
			{
		
			}

			// private constructor. Must not be used
			TcpConnectionGetEhzDataClient::TcpConnectionGetEhzDataClient(void) : 	
					TcpConnectionBase(null<Handle>()),
					Subscriber<EventTimer>(),
					vEMDA(),
					requestCommand(&tcpConnectionGetEhzDataCommand[0]),
					pollTimer(null<u32>()),
					receivedDataAsStrings(),
					convertedDataPart(),
					state()
			{
			}

			// Destructor
			TcpConnectionGetEhzDataClient::~TcpConnectionGetEhzDataClient(void)
			{
				try
				{
					// STop the Timer
					TcpConnectionGetEhzDataClient::stopPoll();
				}
				catch(...)
				{
				}
			}

			
			// Start the poll timer and add handler to reactor
			 void TcpConnectionGetEhzDataClient::startPoll(void)  
			 {
				pollTimer.addSubscription(this); 
				pollTimer.startTimerPeriodic();
			}

			 // Stop timer
			 void TcpConnectionGetEhzDataClient::stopPoll(void)  
			 {
				
				pollTimer.stopTimer();
				pollTimer.removeSubscription(this);
			}
			
			 // Override of Publisher function 
			 // In case of stop of TCP connection we will first stop the poll timer
			 void TcpConnectionGetEhzDataClient::notifySubscribers(void)
			 {
				//lint -e{1933}     Note 1933: Call to unqualified virtual function 'TcpConnectionGetEhzDataClient::stopPoll(void)' from non-static member function
				stopPoll();
				//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function
				TcpConnectionBase::notifySubscribers();
			 }
			
			
		// -------------------------------------
		// 4.1.2 Update function for timer Event
		
			// The Client starts a timer and wants to get data from the connected peer for all x seconds
			// Wen the timer fires the we handle this event here
			// We will send one byte (the request command) in raw mode to the peer

			void TcpConnectionGetEhzDataClient::update(EventTimer *const)
			{
				//ui << "TcpConnectionGetEhzDataClient::update(EventTimer  "  << std::endl;
				if (null<Handle>() != handle)
				{
					//lint -e{925}    925 Cast from pointer to pointer
					// Send the request. The server shall send data
					ui << "TCP Client: Sending Get Request"  << std::endl;
					
					writeDataAsynchronous(requestCommand, handle);
				}
				
			}



		// -------------------------------------
		// 4.1.3 Store received data

			// The server sends the data in the format:
			// STX data1 US data2 US ... dataN US ETX
			// The data will be parsed an the result will be stored into a vector of strings
			// This will be stored in the correct part for an EhzInternal::AllMeasuredValuesForOneEhz
			void TcpConnectionGetEhzDataClient::setValuesFromStrings(std::vector<std::string>::iterator &iter)
			{
				// For the system
				for (std::vector<EhzInternal::AllMeasuredValuesForOneEhz>::iterator emdaI = vEMDA.begin(); emdaI != vEMDA.end(); ++emdaI)
				{
					// For a single EHZ
					(emdaI)->setValuesFromStrings(iter);
				}
			}



		// ---------------------------------------------------
		// 4.1.3 Handle data receive from handleEvent function
		
			EventProcessing::Action TcpConnectionGetEhzDataClient::handleReadData(const sint bytesRead)
			{
				// Assume that everything is OK
				EventProcessing::Action rc = EventProcessing::Continue;
				
				// Check all bytes in the buffer
				// Parse the sring received by the Server in the format
				// STX data1 US data2 US ... dataN US ETX
				
				for (sint i = null<sint>(); i<bytesRead; i++)
				{
					switch (state)
					{
						// Wait for STX
						case StateWaitForStart:
							//lint -e{911}   /Note 911: Implicit expression promotion from char to int
							if (charSTX == receivedRawData[i])
							{
								// If we saw an ETX, then we will read strings in the next step
								state = StateDoConversion;
								// Clear result string from one parse step
								convertedDataPart.clear();
							}
							break;
							
						// Now we will read strings, separated by US	
						case StateDoConversion:
							// Is the current byte an US
							//lint -e{911}   /Note 911: Implicit expression promotion from char to int
							if (charUS == receivedRawData[i])
							{
								// If so, store the result in a vector of strings. Yes, copy data
								receivedDataAsStrings.push_back(convertedDataPart);
								// And reset container for next run
								convertedDataPart.clear();
							}
							// Check for end of transmission
							//lint -e{911}   /Note 911: Implicit expression promotion from char to int
							else if (charETX == receivedRawData[i])
							{
								// ETX found. Stop function and reset state for next run
								state = StateWaitForStart;
								// And now store the read strings 
								std::vector<std::string>::iterator iter = receivedDataAsStrings.begin();
								// If there are strings at all
								if (iter != receivedDataAsStrings.end())
								{
									//lint -e{1933}    Note 1933: Call to unqualified virtual function 'TcpConnectionBase::handleReadData(int)' from non-static member function
									// The derived classes know where to store the strings
									setValuesFromStrings(iter);
									
								}
								// Make ready for next run. Reste internal variables
								receivedDataAsStrings.clear();
								
								//Publisher<TcpConnectionGetEhzDataClient>::notifySubscribers(static_cast<vo*>(&vEMDA));
							}
							else
							{
								// Add next received byte to internal string
								convertedDataPart += receivedRawData[i];
							}
							break;
							
						default:
							// Should not happen
							ui << "Conversion Error"  << std::endl;
							// STop reactor Event loop
							rc = EventProcessing::Error;
							// Close TCP connection
							TcpConnectionBase::notifySubscribers();
							break;
					}
				}
				return rc;
			}




