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
// ehz.hpp
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

#ifndef EHZ_HPP
#define EHZ_HPP


#include "parser.hpp"
#include "serial.hpp"
#include "timerevent.hpp"
#include "database.hpp"
#include "parsetreevisitor.hpp"
#include "ehzconfig.hpp"


namespace EhzInternal
{
	const std::string StrEmpty;
	
	
// ------------------------------------------------------------------------------------------------------------------------------
// 1. Definition of the EHZ

	class Ehz : public Publisher<Ehz>, 
				public Subscriber<SerialInternal::EhzSerialPort>
	{

		public:
			// Constructor gets the specific configuration data for this EHZ instance
			explicit Ehz(const EhzConfigDefinition &ecd);
			// The destrutor removes the subscription from the serial port
			virtual ~Ehz(void);
			
			// Open and start serial port
			void start(void);  
			// Detach from Event Handler and close the serial port
			void stop(void); 
			
			// We are a subscriber to EhzSerialPort. If the serial ports receives data, then it
			// notifies this Ehz and calls this update function. The EHZ will parse the raw data
			//lint --e(1735)
			virtual void update(SerialInternal::EhzSerialPort *const publisher); 

			// EhzSystem and other functions are interested in the results of the parse activity
			// This functions gives access to the results
			const AllMeasuredValuesForOneEhz &getAllMeasuredDataForOneEhz(void) const { return allMeasuredValuesForOneEhz; }    
			
			// Get index of this Ehz (index in Ehz system)
			const uint &getEhzIndex(void) const { return ehzConfigDefinition.index; }
			
		protected:		
			// The properties of this specific Ehz
			//lint --e(1725)		class member 'Symbol' is a reference
			const EhzConfigDefinition &ehzConfigDefinition;  // The specific configuration for this instance of the EHZ

			// The data from the Ehz are read via a serial port. This is the related port for this Ehz
			SerialInternal::EhzSerialPort ehzSerialPort;
			
			

			// After a telegram (a SmlFile) from the Ehz has been received, it will be parsed. Then
			// the below class will retrieve the data from the parse tree using the visitor pattern
			ParserInternal::SmlListEntryEvaluation smlListEntryEvaluation; 

		
			// This is the parser for the raw data received from the Ehz via the serial port.
			// It contains a lexer and helper classes. The parser will create a parse tree for
			// each successfully received SmlFile
			Parser parser;
			
			// The measured results contained in the SML File will be stored here. This will
			// be done by smlListEntryEvaluation
			// The EhzSystem class will use this data for further processing
			AllMeasuredValuesForOneEhz allMeasuredValuesForOneEhz;	
			
		private:
			// Hidden default constructor. Must not be used
			//lint --e(1704)  --e(1938)
			Ehz(void) : Publisher<Ehz>(), 
						Subscriber<SerialInternal::EhzSerialPort>(),
						ehzConfigDefinition(EhzInternal::ehzConfigDefinitionNULL), 
						ehzSerialPort(StrEmpty), 
						smlListEntryEvaluation(EhzInternal::ehzConfigDefinitionNULL,&emdaDummy), 
						parser(), 
						allMeasuredValuesForOneEhz()    {}
		
	};

} // end of namespace	


// ------------------------------------------------------------------------------------------------------------------------------
// 2. Definition of the EHZ System. A container of one or more EHZ




class EhzSystem : 	public Subscriber<EhzInternal::Ehz>,
					public Subscriber<EventTimer>
{
		const std::vector<EhzConfigDefinition> vecEhzConfigDefinitionNULL;

	public:
		
		// Explicit constructor for the EhzSystem. Will create the the Ehz's as given 
		// by configuration a no of Ehz parameter. Dynamic allocation
		         
		//explicit EhzSystem(const EhzConfigDefinition *const pecd, const sint numberOfEhz);
		explicit EhzSystem(const std::vector<EhzConfigDefinition> &vecd);
        
        
        // Will remove the subscription and delete the dynamically allocated Ehz
		virtual ~EhzSystem(void);

		// Send a start command to each Ehz and register the EhzEventHandler with the reactor
		// Will start reading bytes from the serial port, parse them and get the results
		void start(void);
		// Stop all Ehz activities. Remove Event Handler from reactor
		void stop(void);

		// Update function of Observer
		// Specifically: After the Ehz has done a successfull parse of a SmlFile
		// and stored the resulting data, this update function will be called.
		// The Ehz System can then do something with the resulting data
		//lint --e(1735)
		virtual void update(EhzInternal::Ehz *const publisher);
		//lint --e(955)
		virtual void update(EventTimer *const);

		
		// Check if the EhzSystem was initialized and contains plausible data
		boolean isInitialized(void) const;
		
		
		const AllMeasuredValuesForAllEhz &getEhzSystemResult(void) const { return allMeasuredValuesForAllEhz; }
		
		friend std::ostream& operator<< (std::ostream &out, const EhzSystem &ehzSystemP);


		
	protected:
	
		// The final result
		// Container for Measured values
		AllMeasuredValuesForAllEhz allMeasuredValuesForAllEhz;	
	
        // The configuration of the EhzSystem. Defines, how many Ehz are in the System
		// and especially their properties. The Configuration is a global constant
		// that will be copied by the constructor and will be used by this instance
		// of EhzSystem
		//lint --e(1725)		class member 'Symbol' is a reference		
		const std::vector<EhzConfigDefinition> &vehzConfigDefinition;
        
        // This is the container, where all Ehz instances will be stored. The number 
		// of Ehz with their properties are derived from the configuration.
		// The Ehz instances will be allocated dynamically
		std::vector<EhzInternal::Ehz *> vehz;
		
		EventTimer ehzSystemTimer;
		DatabaseInternal::EhzDataBase *ehzDataBase;
        
	private:

		// Hidden default constructor. Must not be used
		//lint --e(1704)
		EhzSystem(void) : 	Subscriber<EhzInternal::Ehz>(), 
							Subscriber<EventTimer>(),
							vecEhzConfigDefinitionNULL(),
							allMeasuredValuesForAllEhz(),
							vehzConfigDefinition(vecEhzConfigDefinitionNULL), 
							vehz(), 
							ehzSystemTimer(null<u32>()),
							ehzDataBase(null<DatabaseInternal::EhzDataBase *>()) 
		{  }
		
		// Hidden copy constructor
		//lint --e(1529) --e(1704) --e(1738)
		EhzSystem(const EhzSystem &) :  Subscriber<EhzInternal::Ehz>(), 
										Subscriber<EventTimer>(),
										vecEhzConfigDefinitionNULL(),
										allMeasuredValuesForAllEhz(),
										vehzConfigDefinition(vecEhzConfigDefinitionNULL), 
										vehz(),
										ehzSystemTimer(null<u32>()), 
										ehzDataBase(null<DatabaseInternal::EhzDataBase *>())
		{ }
		
		// Hidden assignment operator
		//lint --e(1704) --e(1941) --e(1529) --e(1745)
		EhzSystem &operator =(const EhzSystem &) {return *this;} 
};

	
	
// The EHZ System is one of the main Objects of the whole application and defined as a global
//lint --e(956,1929)
//extern EhzSystem &ehzSystem(void);




 



#endif
