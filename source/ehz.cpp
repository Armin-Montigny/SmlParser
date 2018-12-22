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
// ehz.cpp
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
// In this module the properties and functionalities of one EHZ and an EHZ System
// consisting of 1 or more EHZ are desribed. Every EHZ has an associted serial port.
// All data are received thorugh the serial port. The serial port is managed via an
// Eventhandler. The EHZ parses the data from the serial port, builds a data structure
// (the parse tree) and informs others that data is available
//
// The EHZ System is configuered via one "Configuration" structure.
// It gets the data from the contained EHZ and stores it in a database.
// A timer is used for cyclic storage of data records.
// Other interested parties are informed as well abaout the availability of new datasets
// via a publisher subscriber mechanism.
// 


#include "ehz.hpp"
#include "reactor.hpp"
#include "bytestring.hpp"
#include "userinterface.hpp"

#include <set>

 

// ------------------------------------------------------------------------------------------------------------------------------
// 1. Definition of EHZ member functions


	namespace EhzInternal
	{

	
	// ---------------------------------------------
	// 1.1 Set the properties for one dedicated EHZ

		//lint -e{1901,1911}
		// Add a subscription for the serial port. So make EHZ ready to receive data
		// Constructor for Ehz
		Ehz::Ehz(const EhzConfigDefinition &ecd) : Publisher<Ehz>(), 								// Initialize base class publisher
													Subscriber<SerialInternal::EhzSerialPort>(),	// Initialize base class subscriber
													ehzConfigDefinition(ecd), 						// Store Ehz specific properties
													ehzSerialPort(ecd.EhzSerialPortName), 			// The Ehz has a serial port 
													smlListEntryEvaluation(ecd, &allMeasuredValuesForOneEhz), 	// Set reference to result values
													parser(), 										// Ehz has a parser
													allMeasuredValuesForOneEhz() 					// Ehz will hold the resulting data 
		{
			// Ehz is a subscriber to the serial port
			// Evertime when a byte arrives, we want to know and process this byte
			ehzSerialPort.addSubscription(this);
		}

	// -------------------------------
	// 1.2 Detach EHZ from Serial port
	
		// Destructor
		Ehz::~Ehz(void) 
		{ 

			// We call an external function and never know what could happen
			try 
			{ 
			
				// Remove subscription
				
				ehzSerialPort.removeSubscription(this); 
				// Debug info
			}
			catch(...)
			{
				// We don't care on result
			}
		}

	// ------------------------------------------
	// 1.3 Start receiving Data from serial port

		// Open port and register Ehz with Event Reactor
		void Ehz::start(void)  
		{ 
			// Open Serial Port
			ehzSerialPort.start();
			if(null<Handle>() != ehzSerialPort.getHandle())
			{
				// If serial port could be opened then register the 
				// instance of this serial port of this Ehz
				reactorRegisterEventHandler(&ehzSerialPort,EventTypeIn);
						
			}
		}

	// -------------------------------------------------------
	// 1.4 Stop reading from serial port and close serial port
	
		// 
		void Ehz::stop(void)   
		{ 
			if (null<Handle>() != ehzSerialPort.getHandle())
			{
				// Unregister this serial port of this Ehz from the Event Handler
				reactorUnRegisterEventHandler(&ehzSerialPort);
			
				// Close the serial port
				ehzSerialPort.stop(); 
			}
			
		}

		
	// ------------------------------------------
	// 1.5 Process data recievied via serial port
		
		// One of the major functions:
		// - Receive data stream from serial port
		// - Parse data
		// - Build a parse tree
		// - Extract needed data from parse tree
		// - Inform interested parties that a new dataset is available
		
		// The serial port called has a notification function
		// Therefore the Ehz will be updated here
		void Ehz::update(SerialInternal::EhzSerialPort *const publisher) 
		{
			// Get data from serial port
			const EhzDatabyte ehzDatabyte = publisher->getLastReceivedByte();
			//ui[ehzConfigDefinition.index] << ehzDatabyte;

			// Push the received byte into the parser. The parser
			// will build a parse tree and get a complete SML File
			// with all data
			const prCode parserResult = parser.parse(ehzDatabyte, getEhzIndex() );
			//Check output of parser function and act accordingly
			//lint -e{788}
			switch (parserResult)
			{
				case pr_PROCESSING:
					// Everything ongoing without error. Bytes from the serial port are processed 
					// normally and the SML file is not yet fully read. Simply do nothing and wait
					// until parser has completed
					// Debug output
					//ui << "--------> Parse Result SML Overall: Processing"  << std::endl; 
					break;
				case pr_DONE:
					// The parser read successfully a complete SML File. It built a parse tree and
					// all data are stored in this parse tree. We will now traverse the tree
					// with our dedicated visitor and retrieve our desired information
					smlListEntryEvaluation.clear();
					parser.traverseAndEvaluate(&smlListEntryEvaluation);

					// The Ehz informs now interested parties (subscribers) that new data is available
									
					Ehz::notifySubscribers();
					// Reset the parser and be ready for the next SML File
					parser.reset();
					break;
					
				// In case of error or any othe outcome
				case pr_ERROR:
					//FALLTHROUGH
				default: 
					{
						// Show a debug Error message
						
						// Get time of now
						std::string strNow;
						//lint -e{534}    // Do not need return value
						getNowTime(strNow);
						
						// Show error message
						ui[ehzConfigDefinition.index] << strNow; 
						//lint -e{641,1911,911}
						ui[ehzConfigDefinition.index] << "Parser Error: " << parserResult << std::endl; 
						
						// And reset the parser
						parser.reset();
					}
					break;
			}	
		}

	} // End of namespace 

// ------------------------------------------------------------------------------------------------------------------------------
// 2. Definition of EHZ System member functions
//
// The EHZ System is a collection of EHZ
// It will be automatically be configured using a configuration structure


	// ---------------------------------------
	// 2.1 Initialize and setup the EHZ System

		 
		//lint -e429    We store the pointer to Ehz in a vector
		// Constructor
		EhzSystem::EhzSystem(const std::vector<EhzConfigDefinition> &vecd)  : 	Subscriber<EhzInternal::Ehz>(), 
																				Subscriber<EventTimer>(),
																				vecEhzConfigDefinitionNULL(),
																				allMeasuredValuesForAllEhz(EhzInternal::MyNumberOfEhz), 
																				vehzConfigDefinition(vecd), 
																				vehz(), 
																				ehzSystemTimer(10000UL), 
																				ehzDataBase(null<DatabaseInternal::EhzDataBase *>())  
		{
			// Pointer to Ehz. Initialize to 0
			EhzInternal::Ehz *pehz = null<EhzInternal::Ehz *>();

			// The configuration defines the number of the EHZ's in the system that need to be created now
			for (uint ehzInstance = 0U; ehzInstance < vehzConfigDefinition.size(); ehzInstance++)
			{	
				//This will read the properties for this specific Ehz that we are going to create
				const EhzConfigDefinition &ecd = vehzConfigDefinition[ehzInstance];
				
				// Create a new Ehz
				 
				//lint -e774 -e948 --e(1938)
				pehz = new EhzInternal::Ehz(ecd);
				
				// Ehz could be created. Store result in our container
				vehz.push_back(pehz);
				// And add a subscriber in the created Ehz for us
				// Sow the EHZ system will receive information that the EHZ has new data
				pehz->addSubscription(this);
			}
		
			//lint -e{1901,1911}
			// Create a new database, where the Results of all EHZ will be stored
			ehzDataBase = new DatabaseInternal::EhzDataBase(DatabaseInternal::EhzDatabaseName);

			// And, we want ro receive a timerevent all x seconds. Then the data, stored internally in the EHZ System class,
			// will be stored in the database
			ehzSystemTimer.addSubscription(this);
		}
		 

		
	// ---------------------
	// 2.2 Clear EHZ System
		
		// Destructor. Delete Ehz and their subscriptions
		//lint -e{1740,1702,1579}
		EhzSystem::~EhzSystem(void) 
		{ 
		
			// Try catch because we call external functions and never know if they throw or not
			try
			{
				
				stop();
			
				// Stop cyclic timer for events to store datasets in the database
				//ehzSystemTimer.stopTimer();

				// We do not want to be notified any longer from the timer
				ehzSystemTimer.removeSubscription(this);

				// Close the database
				delete ehzDataBase;

				// Get number of Ehz in our Ehz System
				const uint numberOfElements = vehz.size();

				// For all Ehz in our EhzSystem
				for (uint i=0U; i<numberOfElements; i++)
				{
					// remove subscription from Ehz
					vehz[i]->removeSubscription(this);
					// and delete it . . . 
					delete vehz[i];
				}

			}
			catch(...)
			{
				// No error handling
			}
		}

		
	// --------------------------------
	// 2.3 Sanity chaeck for EHZ System
	
		// Check if the EhzSystem was initialized and contains plausible data
		// The "index" from the configuratio data will be check.
		// It must be in valid boundaries and the indices need to be unique
		
		boolean EhzSystem::isInitialized(void) const
		{
			// Assume that function returns OK
			boolean rc = true;

			// Get number of Ehz's in this EHZ System instance
			// The indices must be between 0..(max-1)
			const uint numberOfEhz = vehz.size();
			
			// Check, if Ehz are existing in our system at all
			// Of course there must be EHZ in the system. Otherwise nothing will work
			if (numberOfEhz > 0U)
			{
				// OK, at least one Ehz is existing. Check plausible indices
				
				// Iterate through existing Ehz and check indices
				// Indices must be >=0 and <max
				// Indices must be unique. We will use a c++ set to check this
				std::set<uint> Indices;
				// Result of the insert into a set function
				std::pair<std::set<uint>::iterator,bool> insertResult;

				// Iterate through all existing EHZ in the EHZ system
				// and only as long as everything is ok and no error has been found
				for (uint ehzIndex=0U; (ehzIndex <numberOfEhz) && rc; ehzIndex++)
				{
					// Read the index of the EHZ under check
					const uint indexToCheck = vehz[ehzIndex]->getEhzIndex();
					
					// Index is uint. Cannot be < 0. Check for max value
					if (indexToCheck >= numberOfEhz)
					{
						// Index number too big --> error
						rc = false;
						// Show debug message
						ui << "Ehz Index wrong" << std::endl;
					}
					else
					{
						// Check if insert is double. Try to insert the index into a temporary c++ set
						// This will fail, if index is already in
						insertResult = Indices.insert(indexToCheck);
						// Check for error
						if (!insertResult.second)
						{
							// Double index --> error
							rc = false;
							// Show debug message
							ui  << "Ehz Index wrong"  << std::endl;
						}
					}
				}
			}
			else
			{
				// No Ehz in the system --> Error
				rc = false;
				// Show debug message
				ui << "EhzSystem not initialized"  << std::endl;
			}
			return rc;
		}

	// ------------------------
	// 2.4 Start the EHZ System

		// Start all Ehz in our EHZ System
		void EhzSystem::start(void)
		{
			// If the system has been initalized and there are Ehz in our EHZ System
			if (isInitialized())
			{
				// Get number of Ehz's in our EHZ System 
				const uint numberOfEhz = vehz.size();
				
				// Iterate through existing Ehz and start them
				for (uint ehzIndex=0U; ehzIndex <numberOfEhz; ehzIndex++)
				{
					// Start Ehz
					vehz[ehzIndex]->start();
				}
				// And start the timer
				ehzSystemTimer.startTimerPeriodic();
			}
			else
			{
				ui << "EHZ System is not initialized. Could not be started" << std::endl;
			}
		}
		
		
	// -----------------------
	// 2.5 Stop the EHZ System
	
		// Stop all EHZ in our Ehz System
		void EhzSystem::stop(void)
		{
			// If the system has been initalized and there are Ehz in our EHZ System
			if (isInitialized())
			{
				// Stop the periodic timer
				ehzSystemTimer.stopTimer();
			
				// Get number of Ehz's in our EHZ System 
				const uint numberOfEhz = vehz.size();
				// Iterate through existing Ehz and stop them
				for (uint ehzIndex=0U; ehzIndex <numberOfEhz; ehzIndex++)
				{
					// Stop Ehz
					vehz[ehzIndex]->stop();
				}
			}
		}
			
	// ---------------------------------
	// 2.6 Handle EHZ data present event
			
		// After one EHZ has parsed data from the serial port data stream
		// and produced a result, it will call this function
			
		// This is the function that should do something with the results of all data
		// At this moment we simply store print the results
		void EhzSystem::update(EhzInternal::Ehz *const publisher)
		{
			// Check, which Ehz in our Ehz System sent the notification
			const uint ehzIndex = publisher->getEhzIndex();
			
			// Get the resulting values
			const EhzInternal::AllMeasuredValuesForOneEhz &allMeasuredValuesForOneEhz = publisher->getAllMeasuredDataForOneEhz();
			
			// Make a copy of EHZ's latest values
			// Yes, we want to copy data
			allMeasuredValuesForAllEhz[ehzIndex] = allMeasuredValuesForOneEhz;

			//lint -e{641,911}
			if (DebugModeObis == globalDebugMode) 
			{
				//lint -e{1702}
				// print all Obis Values:
				for (std::set<std::string>::iterator ovi = allMeasuredValuesForOneEhz.obisValues.begin(); ovi != allMeasuredValuesForOneEhz.obisValues.end(); ++ovi)
				{
					ui[ehzIndex] << *ovi << "\n";
				}
				ui[ehzIndex] << "------------" << std::endl;
			}
			
			// Print out result in debug screen
			// Show Ehz number and time, when the data had been captured
			ui(ehzIndex) << SetPos(0,0) << '(' << ehzIndex << "): " << allMeasuredValuesForOneEhz.timeWhenDataHasBeenEvaluatedString;
			
			// Go through all measured value
			for (uint i = 0U; i<NumberOfEhzMeasuredData; i++)
			{
				// What type does this value have
				const EhzMeasuredDataType::Type emdt = vehzConfigDefinition[ehzIndex].ehzMeasuredDataType[i];
				// Show status byte
				const u32 status = (static_cast<u32>(allMeasuredValuesForOneEhz.measuredValueForOneEhz[i].status));	//lint !e921
				ui(ehzIndex) << "\n(" << status << "): ";				
			
				
				// If there is a value at all, then
				if (EhzMeasuredDataType::Null != emdt)
				{
				
					// Depending on the result beeing a string or a number
					switch (emdt)
					{
						// If it is a number then print the numerical value
						case EhzMeasuredDataType::Number:
							
							ui(ehzIndex) << allMeasuredValuesForOneEhz.measuredValueForOneEhz[i].doubleValue;
							break;
							
						// If it is a string, then show that
						case EhzMeasuredDataType::String:	
							{
								// Caveat: String may contain none printable characters
								std::string smlStringTemp;
								// In this case it will be converted to ascii coded hex
								convertSmlByteStringNonePrintableCharacters(allMeasuredValuesForOneEhz.measuredValueForOneEhz[i].smlByteString, smlStringTemp);
								// And show it
								ui(ehzIndex) << smlStringTemp;
							}
							break;
						case EhzMeasuredDataType::Null:
							// Fallthrough
						default:
							// Nothing or Error: Don't do a thing
							break;
							
					}
					
					
					// Show the unit for this value
					ui(ehzIndex) << allMeasuredValuesForOneEhz.measuredValueForOneEhz[i].unit;
					
				}
			}
			
			ui(ehzIndex) << std::endl;
		}

		
	// -----------------------------------------------------
	// 2.7 Convert EHZ system data in a readable data stream
		
		// Data will be in a frame between STX and ETX
		// Datasets will be separated by US
		std::ostream& operator<< (std::ostream &out, const EhzSystem &ehzSystemP)
		{
			// Datastream will be in the form: STX data1 US data2 US .... dataN US ETX
			//lint --e{1963,1950,9050}
			
			// So first an starting STX
			out << charSTX;
			// Then we will iterate though all measured values of all EHZ and convert them
			for (std::vector<EhzInternal::AllMeasuredValuesForOneEhz>::const_iterator amvfoeI = ehzSystemP.allMeasuredValuesForAllEhz.begin(); amvfoeI != ehzSystemP.allMeasuredValuesForAllEhz.end(); ++amvfoeI)
			{
				out << *amvfoeI;
			}
			// And finalize data stream with ETX
			out << charETX;
			return out;
		}


	// ---------------------------------------------
	// 2.8 Timer Call back. Store data in database

		// Timer Callback
		void EhzSystem::update(EventTimer *const)
		{
			ehzDataBase->storeMeasuredValues(allMeasuredValuesForAllEhz);
		}






















