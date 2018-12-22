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
// ehzmeasureddata.hpp
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
// Here we will show the data structes for one EHZ na d for the EHZ System
// These definitions will be used for further data processing for other functions
//

#ifndef EHZMEASUREDDATA_HPP
#define EHZMEASUREDDATA_HPP

#include "mytypes.hpp"


#include <vector>
#include <iostream>
#include <set>
 


// ------------------------------------------------------------------------------------------------------------------------------
// 1. Definition of the structure to hold values for one EHZ

	namespace EhzInternal
	{

		// This is a container for actual measured data from one Ehz
		struct OneMeasuredValueForOneEhz
		{
			// Standard constructor. Set everything to 0
			OneMeasuredValueForOneEhz(void) : 	doubleValue(0.0), 
												smlByteString(), 
												unit(), 
												status(null<u64>())			
			{}
			
			// Standard destructor does nothing
			virtual ~OneMeasuredValueForOneEhz(void) {}
			
			// Deep Copy. Standard behaviour
			OneMeasuredValueForOneEhz &operator =(const OneMeasuredValueForOneEhz &assignFromOneMeasuredValueForOneEhz);
			
			// Value fomr EHZ can be either a double or a string. We will not use a union here. Don't care about waste of memory
			mdouble doubleValue;			
			SmlByteString smlByteString;
			
			// Unit for a value
			std::string unit;
			
			// Possible status information. For example the electric power maybe positive or negative
			// See SML definition file for further information
			u64 status;
			
			// This fills the EHZ data structure from an vector of strings
			// It is the opposite of the << operator. But we use it in a different way
			// and must use this functionality
			void setValuesFromStrings(std::vector<std::string>::iterator &iter);
			void clear(void) { doubleValue = 0.0; smlByteString.clear(); unit.clear();status = null<u64>(); } 
		};
		

// ----------------------------------------------------------------------------------------
// 2. Definition of the structure to hold values all values for all EHZ plus some timestamp


		// As explained above, we are interested in NumberOfEhzMeasuredData from an Ehz
		// This is a container that will hold all n data and the time when the data have been aquired
		struct AllMeasuredValuesForOneEhz
		{
			// Default ctor resets everything to 0
			AllMeasuredValuesForOneEhz(void) : measuredValueForOneEhz(NumberOfEhzMeasuredData), timeWhenDataHasBeenEvaluated(null<time_t>()), timeWhenDataHasBeenEvaluatedString(), obisValues()	{ }
			virtual ~AllMeasuredValuesForOneEhz(void) {}
			
			// The measured values for all EHZ
			std::vector<OneMeasuredValueForOneEhz> measuredValueForOneEhz;
			
			// Time information for when the data have been aquired
			time_t timeWhenDataHasBeenEvaluated; 				// in time_t format
			std::string timeWhenDataHasBeenEvaluatedString;		// As string
						
			std::set<std::string> obisValues;
			
			// This function will get the time and store it in our internal variables
			void storeNowTime(void);
			
			// Convert EHZ System data to an output stream
			//lint -e{1929}
			friend std::ostream& operator<< (std::ostream &out, const AllMeasuredValuesForOneEhz &allMeasuredValuesForOneEhz);

			// This fills the EHZ System data structure from an vector of strings
			// It is the opposite of the << operator. But we use it in a different way
			// and must use this functionality
			void setValuesFromStrings(std::vector<std::string>::iterator &iter);
			void clear(void);
			
			AllMeasuredValuesForOneEhz &operator = (const AllMeasuredValuesForOneEhz &assignFromAllMeasuredValuesForOneEhz);
		};
	}
		


	// Dummy value for default initialization
	//lint -e{956}
	extern EhzInternal::AllMeasuredValuesForOneEhz emdaDummy;

	typedef std::vector<EhzInternal::AllMeasuredValuesForOneEhz> AllMeasuredValuesForAllEhz;

#endif
