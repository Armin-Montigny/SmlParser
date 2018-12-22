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
// parsetreevisitor.hpp
//
// Special Visitor for evaluation of an SmlListEntry
//
// Please refer also to parser.hpp/cpp
// After the parser has successfully read a SML File, a parse tree with all information
// regarding that file is available. That tree will be traversed. All needed information
// will be extracted. This is done using the visitor pattern. And the following class
// gets all important information for a Ehz
//



#ifndef PARSETREEVISITOR_HPP
#define PARSETREEVISITOR_HPP

#include "ehzconfig.hpp"
#include "ehzmeasureddata.hpp"
#include "parser.hpp"

 
 

namespace ParserInternal
{

// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Definition of visitor class to get all values of one EHZ
	
	class SmlListEntryEvaluation : public ParserInternal::VisitorForSmlListEntry
	{
		public:
			// Explicit constructor for this class.
			// Set references to Ehz properties and the array where the result will be stored
			explicit SmlListEntryEvaluation(const EhzConfigDefinition &ecd, EhzInternal::AllMeasuredValuesForOneEhz *const emd);
			virtual ~SmlListEntryEvaluation(void)  {}	// Empty virtual destructor
			
			// Overwrite for the visit function of the parent class
			// You need to understand the visitor pattern first. Please search the internet.
			// This visit function will be called, if the element in the parse tree is a SmlListEntry
			// It will then extract the values from the parse tree and store it in the EHZ - System
			// This is the function that acts as an interface between parsed values and the Ehz - System
			// for further data processing
			virtual void visit(ParserInternal::SmlListEntry &smlListEntry);

			void clear(void) {allMeasuredValuesForOneEhz->clear();}
		protected:
			// Reference to the properties of the Ehz. We need to know these in order to be 
			// able to interpret all values correctly for this specific Ehz
			//lint --e(1725)		class member 'Symbol' is a reference
			const EhzConfigDefinition &ehzConfigDefinition;
			
			// This functions reads measured values from the parsed SML File. The values will be stored in 
			// an array. There is an array for measured values for each Ehz in EhzSystem
			//lint --e(1725)		class member 'Symbol' is a reference
			EhzInternal::AllMeasuredValuesForOneEhz *allMeasuredValuesForOneEhz;
			
		private:
			// Standard constructor. Must not be used. Hence --> private
			//lint --e(1704)  // 1704 Constructor 'Symbol' has private access specification
			SmlListEntryEvaluation(void) : ParserInternal::VisitorForSmlListEntry(), ehzConfigDefinition(EhzInternal::ehzConfigDefinitionNULL), allMeasuredValuesForOneEhz(&emdaDummy) {}
			// Hide copy constructor. Avoid subtle problems with reference members
			//lint --e(1704) --e(1738)
			// 1704 Constructor 'Symbol' has private access specification
			//1738 non-copy constructor 'Symbol' used to initialize copy constructor
			SmlListEntryEvaluation(const SmlListEntryEvaluation &) : ParserInternal::VisitorForSmlListEntry(), ehzConfigDefinition(EhzInternal::ehzConfigDefinitionNULL), allMeasuredValuesForOneEhz(&emdaDummy) {}
			// Hide assignment operator. Avoid subtle problems with reference members
			//lint --e(1704) --e(1529)
			// 1704 Constructor 'Symbol' has private access specification
			// 1529 Symbol 'Symbol' not first checking for assignment to this
			const SmlListEntryEvaluation &operator=(const SmlListEntryEvaluation &) { return *this; }

	};


}


 

#endif
