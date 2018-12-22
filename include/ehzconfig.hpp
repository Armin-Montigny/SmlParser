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
// ehzconfig.hpp
//
// General Description
//
// This module is a part of an application tor read the data from Electronic Meters for Electric Power
//
// In Germany they are called EHZ. Hence the name of the module
//
// The EHZ transmits data via an infrared interface. The protocoll is described in:
//
//  http://www.emsycon.de/downloads/SML_081112_103.pdf
//
// The EHZ System consists of one or more EHZ
// The static properties of the EHZ system are given in this definition file.
// All other functions refer to the definition in this file.
// So if some behaviour of this application should be changed, then make modifications here
// 
// What we define here:
//
// - The index of the EHZ in the EHZ system. Could be calculated by nasty pointer arithmetic. But given here as clear value
// - The name of the EHZ in the system. Free text
// - The Linux device name of the serial port to which the EHZ is connected
// - What values from an EHZ shall be processed. Name and type
//
// The whole data is defined as a constant
//

#ifndef EHZ_CONFIG_HPP
#define EHZ_CONFIG_HPP

#include "mytypes.hpp"
 
 

// ------------------------------------------------------------------------------------------------------------------------------
// 1. Declaration of the configuration data structure. For one EHZ



	// Definition of the properties for one Ehz. This is specific to the needs of the user
	// This is only structure definition . Data are stored in other array

	// POD
	// Must not have constructor and must not have virtual destructor

	//lint -e{1905,935}
	struct EhzConfigDefinition
	{
		//lint --e{1905}
		~EhzConfigDefinition(void) {}   //lint !e1540
		// The definition of the value. What is it? For example the "Electric Power" or "Meter ID"
		//lint -e{1905}
		struct EhzDataValueDefinition
		{
			
			~EhzDataValueDefinition(void) {}  //lint !e1540
			// In SML the possible value are encode using the OBIS scheme. 
			// These are coded numbers. Please check the internet
			const mchar *ObisForDataValue;
			// Clear description of meaning of value
			const mchar *NameForDataValue;
		};
		//lint -e{935}	
		// Index of Ehz in Ehz System. Must be a number 0..(Max-1), no double values
		const uint index;
		// Name of the EHZ, whatever we like								
		const mchar *EhzName;
		
		// Device Name of serial port where the EHZ is connected						
		const mchar *EhzSerialPortName;
		
		// Definitions for data values
		EhzDataValueDefinition ehzDataValueDefinition[NumberOfEhzMeasuredData];			
		// Type of data value
		const EhzMeasuredDataType::Type ehzMeasuredDataType[NumberOfEhzMeasuredData];	
	};


// ------------------------------------------------------------------------------------------------------------------------------
// 2. Definition of the configuration data for the whole EHZ system

	namespace EhzInternal
	{

	// ------------------------------------
	// 2.1 Const Definition for OBIS values

		// Please google for OBIS or get information from the manual of your Ehz
		// Defines an identifier tag for values in the data stream of the EHZ
		
		// Length if the identifier
		const uint ObisDataLength = 6U;

		const mchar ObisForMeterID[ObisDataLength] =  			{'\x01','\x00','\x00','\x00','\x00','\xFF'};	//lint !e743
		const mchar ObisForEnergyConsumption[ObisDataLength] =	{'\x01','\x00','\x01','\x08','\x01','\xFF'};	//lint !e743
		const mchar ObisForEnergyProduction[ObisDataLength] =	{'\x01','\x00','\x02','\x08','\x01','\xFF'};	//lint !e743
		const mchar ObisForPower[ObisDataLength] = 				{'\x01','\x00','\x01','\x07','\x01','\xFF'};	//lint !e743
		
	// ------------------------------------------------------------------------------------------
	// 2.1 Const Definition array of struct of EHZ Definitions. Defines the EHZ system properties

		
		// This is the definition of my Ehz Setup. Yours will be different
		const EhzConfigDefinition myEhzConfigDefinition[] =
		{
			//lint --e{1901,1911,915}
			{
				0U,
				"Einlieger",			// Name of the EHZ, whatever we like	
				"/dev/ttyUSB1",			// Device Name of serial port where the EHZ is connected
				{	
					{
						ObisForMeterID,
						"Zaehler ID"
					},
					{	
						ObisForEnergyConsumption,
						"Verbrauch"
					},
					{
						ObisForPower,
						"Leistung"
					},
					{
						"",
						""
					}
				},
				{
					EhzMeasuredDataType::String,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Null
				}
			},
			
			{
				1U,
				"Hauptwohnung",	// Name of the EHZ, whatever we like
				"/dev/ttyUSB0",			// Device Name of serial port where the EHZ is connected
				{
					{
						ObisForMeterID,
						"Zaehler ID"
					},
					{	
						ObisForEnergyConsumption,
						"Verbrauch"
					},
					{
						ObisForEnergyProduction,
						"Einspeisung"
					},
					{
						ObisForPower,
						"Gesamtleistung"
					}
				},
				{
					EhzMeasuredDataType::String,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number
				}
			},
				{
				2U,
				"Erzeugung PV1",		// Name of the EHZ, whatever we like	
				"/dev/ttyUSB7",			// Device Name of serial port where the EHZ is connected
				
				{	
					{
						ObisForMeterID,
						"Zaehler ID"
					},
					{
						ObisForEnergyProduction,
						"Erzeugung PV1"
					},
					{
						ObisForPower,
						"Leistung PV1"
					},
					{
						"",						
						""
					}
				},
				{
					EhzMeasuredDataType::String,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Null
				}
			},

			{
				3U,
				"Ueberschuss PV2",			// Name of the EHZ, whatever we like	
				"/dev/ttyUSB6",			// Device Name of serial port where the EHZ is connected
				{	
					{
						ObisForMeterID,
						"Zaehler ID"
					},
					{
						ObisForEnergyProduction,
						"Ueberschuss PV2"
					},
					{
						ObisForPower,
						"Leistung"
					},
					{
						"",						
						""
					}
				},
				{
					EhzMeasuredDataType::String,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Null
				}
			},		{
				4U,
				"Allgemein",			// Name of the EHZ, whatever we like	
				"/dev/ttyUSB4",			// Device Name of serial port where the EHZ is connected
				{	
					{
						ObisForMeterID,
						"Zaehler ID"
					},
					{	
						ObisForEnergyConsumption,
						"Verbrauch"
					},
					{
						ObisForPower,
						"Leistung"
					},
					{
						"",
						""
					}
				},
				{
					EhzMeasuredDataType::String,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Null
				}
			},		
			
			{
				5U,
				"Erzeugung PV2",	// Name of the EHZ, whatever we like
				"/dev/ttyUSB2",			// Device Name of serial port where the EHZ is connected
				{
					{
						ObisForMeterID,
						"Zaehler ID"
					},
					{	
						ObisForEnergyConsumption,
						"Verbrauch"
					},
					{
						ObisForEnergyProduction,
						"Einspeisung"
					},
					{
						ObisForPower,
						"Gesamtleistung"
					}
				},
				{
					EhzMeasuredDataType::String,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number,
					EhzMeasuredDataType::Number
				}
			}
		};

		// Number of defined Ehz in my System (sizeof array)
		const uint MyNumberOfEhz = sizeof(myEhzConfigDefinition)/sizeof(myEhzConfigDefinition[0]);


		
		
		// Dummy. Just for default initialisation. All null
		const EhzConfigDefinition ehzConfigDefinitionNULL = 
		{
			100U,		// Wrong index. Delibeartely.
			"",			// Name of the EHZ, whatever we like	
			"",			// Device Name of serial port where the EHZ is connected
			{	
				{
					"",
					""
				},
				{	
					"",
					""
				},
				{
					"",
					""
				},
				{
					"",
					""
				}
			},
			{
				EhzMeasuredDataType::Null,
				EhzMeasuredDataType::Null,
				EhzMeasuredDataType::Null,
				EhzMeasuredDataType::Null
			},
		};
	}

 

#endif
