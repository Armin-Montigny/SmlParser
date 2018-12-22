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
// parsetreevisitor.cpp
//
// Special Visitor for evaluation of an SmlListEntry
//
// Please refer also to parser.hpp/cpp
// After the parser has successfully read a SML File, a parse tree with all information
// regarding that file is available. That tree will be traversed. All needed information
// will be extracted. This is done using the visitor pattern. And the following class
// gets all important information for a Ehz
//


#include "parsetreevisitor.hpp"
#include "bytestring.hpp"
#include "obisunit.hpp"
#include "userinterface.hpp"
#include "crc16.hpp"
#include <cstring>

namespace ParserInternal
{
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Visitor class member functions


	// -----------------------------------------------------------------------
	// 1.1 Simple constructor

	// Constructor for our visitor class
	SmlListEntryEvaluation::SmlListEntryEvaluation(const EhzConfigDefinition &ecd, EhzInternal::AllMeasuredValuesForOneEhz *const emd) : ParserInternal::VisitorForSmlListEntry(), ehzConfigDefinition(ecd), allMeasuredValuesForOneEhz(emd)
	{
	}

	// -----------------------------------------------------------------------
	// 1.2 Visitor function (called by accept) for gettinmg all values of an SML List Entry

	
	// This is the implementation for the specific visitor. It is an override for the visit function
	// of the abstract Visitor Base class
	void SmlListEntryEvaluation::visit(ParserInternal::SmlListEntry &smlListEntry)				
	{ 
	
		// First store all OBIS IDs for debug purposes
		// Ignore Return Value
		allMeasuredValuesForOneEhz->obisValues.insert(convertSmlByteStringToHex(smlListEntry.objName.value));	//lint !e534

	
		// NumberOfEhzMeasuredData is a constant. It was defined for the special context of EHZ that can produce 4 (x) relevant values
		// Now go through all possible x values and search, if this desired value is in the SML List Entry
		for (uint indexEhzMeasuredData=0U; indexEhzMeasuredData<NumberOfEhzMeasuredData; ++indexEhzMeasuredData)
		{
			// A SML list measuredValueForOneEhz contains a so called OBIS identifier that indicates what type of value is tored
			// We are looking for specific values. For example the Electric power. This has an associated OBIS ID
			
			
			
			
			
			// Check, if this measuredValueForOneEhz is of the desired type
			if (0 == std::memcmp(
									ehzConfigDefinition.ehzDataValueDefinition[indexEhzMeasuredData].ObisForDataValue,
									smlListEntry.objName.value.c_str(),
									EhzInternal::ObisDataLength
								)
				)
			{
				// We found a match
				
				// Status
				// First we want to check the "status" info. This may or may not be available. 
				// The status info in a SmlListEntry has different flags with information for the result.
				// For example a direction flag that shows if current is flowing in or out of the meter.
				// This value maybe an optional value. If omitted, there is no real data available
				// So, if it is optional
				if (smlListEntry.status.isOptional)
				{
					// then set status to 0
					allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].status = 0ULL;
				}
				else
				{
					// Status info is avalable. Store it
					allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].status = smlListEntry.status.value;
				}
				// Data
				// The actual data could be a text or a string (or nothing, not existing). Check what
				switch( ehzConfigDefinition.ehzMeasuredDataType[indexEhzMeasuredData])
				{
					// It is a number
					case EhzMeasuredDataType::Number:
						//lint --e(922)   // 922 Cast from Type to Type
						// Calculate the value. It consists of a integer type base value and an exponent.
						// Calculate result and store it
						allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].doubleValue =  (smlListEntry.value.value * pow(10.0,static_cast<mdouble>(smlListEntry.scaler.value)));
						break;

					// It is a text
					case EhzMeasuredDataType::String:
						// Simply copy the text
						allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].smlByteString = smlListEntry.value.sbs;

						break;
					
					// Data Type not existing or error
					case EhzMeasuredDataType::Null:  
						//FALLTHRU
					default:
						// Reset result to 0
						allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].doubleValue = 0.0;
						allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].smlByteString = "";
						break;
				}
				
				// Unit
				// Data may have a associated unit like "kWh" or "A" or "V"
				// or may not. The unit is a optional information
				// Check if unit was omitted
				if (smlListEntry.unit.isOptional)
				{
					// No unit available.
					allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].unit = "";
				}
				else
				{
					// Store unit String
					//lint --e(921) // Cast OK
					allMeasuredValuesForOneEhz->measuredValueForOneEhz[indexEhzMeasuredData].unit = EhzInternal::ObisUnitLookup[static_cast<uint>(smlListEntry.unit.value)].unit;
				}

				break;
			}
		}
		// And last but not least store the timestamp of now
		allMeasuredValuesForOneEhz->storeNowTime();
	}

}

