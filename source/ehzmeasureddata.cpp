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
// ehzmeasureddata.cpp
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
//
// The software parses the datastream from an EHZ and provides resulting values as defined
// in the following data structures.
// The EHZ may transport several values of different types. In the configuration file
// the needed values are selected.
//
// Definition of functions necessary tp process EHZ measured values in data structures
//



#include "ehzmeasureddata.hpp"
#include "bytestring.hpp"
#include "userinterface.hpp"

#include <cstdlib>
#include <sstream>

//
// For data streams we us the format: STX data1 US data2 US ... dataN US ETX
//

// ------------------------------------------------------------------------------------------------------------------------------
// 1. Setters and output functions for one EHZ


	namespace EhzInternal
	{
	
		// 1.0 Assignment operator
		// Default behaviour
		OneMeasuredValueForOneEhz &OneMeasuredValueForOneEhz::operator =(const OneMeasuredValueForOneEhz &assignFromOneMeasuredValueForOneEhz)
		{
			if (this != &assignFromOneMeasuredValueForOneEhz)
			{

				
				doubleValue = assignFromOneMeasuredValueForOneEhz.doubleValue;
				smlByteString = assignFromOneMeasuredValueForOneEhz.smlByteString;
				unit = assignFromOneMeasuredValueForOneEhz.unit;
				status = assignFromOneMeasuredValueForOneEhz.status;
			}
			return *this;
		}


		// 1.1 Convert a vector of strings received via a data stream to internal structures
		
		void OneMeasuredValueForOneEhz::setValuesFromStrings(std::vector<std::string>::iterator &iter)
		{
			// Copy data and advance the iterator
			//lint -e{586}
	
			doubleValue = std::atof((*iter).c_str());++iter;
			smlByteString = *iter;++iter;
			unit = *iter;++iter;
			//lint -e{586,732,919,915,1960}
			status = atol((*iter).c_str());++iter;
		}

		// 1.2 Push data into output stream
		//lint -e{1929,957}
		std::ostream& operator<< (std::ostream &out, const OneMeasuredValueForOneEhz &oneMeasuredValueForOneEhz)
		{
			// Output internal data into a data stream
			//lint -e{1963,9050,1929}
			out << 	
					oneMeasuredValueForOneEhz.doubleValue << charUS << 
					oneMeasuredValueForOneEhz.smlByteString << charUS << 
					oneMeasuredValueForOneEhz.unit << charUS << 
					oneMeasuredValueForOneEhz.status << charUS;
			return out;
		}


// ------------------------------------------------------------------------------------------------------------------------------
// 2. Setters and output functions for an array measured values for one EHZ


		// 2.1 Convert a vector of strings received via a data stream to internal structurs
		void AllMeasuredValuesForOneEhz::setValuesFromStrings(std::vector<std::string>::iterator &iter)
		{
			for (uint i = null<uint>(); i<NumberOfEhzMeasuredData; ++i)
			{
				measuredValueForOneEhz[i].setValuesFromStrings(iter);
			}
			//lint -e{586}
			timeWhenDataHasBeenEvaluated = atol((*iter).c_str());++iter;
			timeWhenDataHasBeenEvaluatedString = *iter;++iter;
		}

		// 2.2 Helper function. Get current time as string and tm struct
		void AllMeasuredValuesForOneEhz::storeNowTime(void) 
		{	
			// Use global function to get the time of now
			timeWhenDataHasBeenEvaluated = getNowTime(timeWhenDataHasBeenEvaluatedString);
		}
		
		// 2.3 Push data into output stream
		//lint -e{1929}
		std::ostream& operator<< (std::ostream &out, const AllMeasuredValuesForOneEhz &allMeasuredValuesForOneEhz)
		{
			// Output internal data into a data stream
			// For all measured data of one EHZ
			for (uint i = null<uint>(); i<NumberOfEhzMeasuredData; ++i)
			{
				//lint -e{1963,9050}
				out << allMeasuredValuesForOneEhz.measuredValueForOneEhz[i];
			}
			//lint -e{1963,9050}
			// And append the time as tm struct and as string
			out << allMeasuredValuesForOneEhz.timeWhenDataHasBeenEvaluated << charUS << allMeasuredValuesForOneEhz.timeWhenDataHasBeenEvaluatedString << charUS;
			return out;
		}
		

		// 2.4 Clear all values
	
		void AllMeasuredValuesForOneEhz::clear(void)
		{
			for (uint i = 0U; i< NumberOfEhzMeasuredData; ++i)
			{
				measuredValueForOneEhz[i].clear();
			}
			timeWhenDataHasBeenEvaluatedString.clear();
			timeWhenDataHasBeenEvaluated = null<time_t>();
			obisValues.clear();
		}


		// 2.5 Assignment operator
		// Default behaviour
		//lint -e{1539}  Warning 1539: member 'EhzInternal::AllMeasuredValuesForOneEhz::obisValues' (line 116, file p:\project\ehz\include\ehzmeasureddata.hpp
		AllMeasuredValuesForOneEhz &AllMeasuredValuesForOneEhz::operator = (const AllMeasuredValuesForOneEhz &assignFromAllMeasuredValuesForOneEhz)
		{
		
			
			if (this != &assignFromAllMeasuredValuesForOneEhz)
			{
				for (uint i = 0U; i< NumberOfEhzMeasuredData; ++i)
				{
					measuredValueForOneEhz[i] = assignFromAllMeasuredValuesForOneEhz.measuredValueForOneEhz[i];
				}
				timeWhenDataHasBeenEvaluatedString = assignFromAllMeasuredValuesForOneEhz.timeWhenDataHasBeenEvaluatedString;
				timeWhenDataHasBeenEvaluated = assignFromAllMeasuredValuesForOneEhz.timeWhenDataHasBeenEvaluated;
				
			}
			return *this;
		}	
	
	}
	
	
// ------------------------------------------------------------------------------------------------------------------------------
// 3. Dummy for default constructors
	//lint -e{1935,1937}
	EhzInternal::AllMeasuredValuesForOneEhz emdaDummy;

