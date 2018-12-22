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
// obisunit.hpp
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
// The unit of a value is encoded 
//
// Please look for "COSEM interface classes and OBIS identification system"
//
// We will use a lookup table for getting the Unit String



#ifndef OBISUNIT_HPP
#define OBISUNIT_HPP

#ifndef MYTYPES_HPP
#include "mytypes.hpp"
#endif

 
 
//lint -save
//lint -e1905		implicit default constructor generated for class
//lint -e1907		implicit destructor generated for class
//lint -e1901		Creating a temporary of type 'std::basic_string<char>'
//lint -e1911		Implicit call of constructor 'std::basic_string<char>::basic_string(const char *, const std::allocator<char> &)' (see text)
//lint -e915		Implicit conversion (initialization) array to struct


// Please look for "COSEM interface classes and OBIS identification system"


namespace EhzInternal
{
// ------------------------------------------------------------------------------------------------------------------------------------------------
// 1. Structure of Lookup table for Units

	struct ObisUnitEntry
	{
		//lint -e{935}
		const uint index;
		const std::string unit;
		const std::string quantity;
		const std::string unitName;
		const std::string siDefinition;
	};

	namespace EhzInternalObisUnit
	{

// ------------------------------------------------------------------------------------------------------------------------------------------------
// 2. Lookup table with units


	// c++98 cannot init vectors by initializer list. Do it manually via a array of struct

		
		// Lookup Table
		// This is a fixed 256 Entry array.
		
		const ObisUnitEntry ObisUnit[] =
		{
			{ 0U, "", "", "", "" },
			{ 1U, "a", "time", "year", "" },
			{ 2U, "mo", "time", "month", "" },
			{ 3U, "wk", "time", "week", "7*24*60*60 s" },
			{ 4U, "d", "time", "day", "24*60*60 s" },
			{ 5U, "h", "time", "hour", "60*60 s" },
			{ 6U, "min.", "time", "min", "60 s" },
			{ 7U, "s", "time (t)", "second", "s" },
			{ 8U, "°", "(phase) angle", "degree", "rad*180/p" },
			{ 9U, "°C", "temperature (T)", "degree-celsius", "K-273.15" },
			{ 10U, "currency", "(local) currency", "", "" },
			{ 11U, "m", "length (l)", "meter", "m" },
			{ 12U, "m/s", "speed (v)", "meter per second", "m/s" },
			{ 13U, "m3", "volume (V), rV , meter constant or pulse value (volume)", "cubic meter", "m3" },
			{ 14U, "m3", "corrected volume", "cubic meter", "m3" },
			{ 15U, "m3/h", "volume flux", "cubic meter per hour", "m3/(60*60s)" },
			{ 16U, "m3/h", "corrected volume flux", "cubic meter per hour", "m3/(60*60s)" },
			{ 17U, "m3/d", "volume flux", "", "m3/(24*60*60s)" },
			{ 18U, "m3/d", "corrected volume flux", "", "m3/(24*60*60s)" },
			{ 19U, "l", "volume", "litre", "10- 3  m3" },
			{ 20U, "kg", "mass (m)", "kilogram", "" },
			{ 21U, "N", "force (F)", "newton", "" },
			{ 22U, "Nm", "energy", "newton meter", "J = Nm = Ws" },
			{ 23U, "Pa", "pressure (p)", "pascal", "N/m2" },
			{ 24U, "bar", "pressure (p)", "bar", "105 N/m2" },
			{ 25U, "J", "energy", "joule", "J = Nm = Ws" },
			{ 26U, "J/h", "thermal power", "joule per hour", "J/(60*60s)" },
			{ 27U, "W", "active power (P)", "watt", "W = J/s" },
			{ 28U, "VA", "apparent power (S)", "volt-ampere", "" },
			{ 29U, "var", "reactive power (Q)", "var", "" },
			{ 30U, "Wh", "active energy, rW , active energy meter constant or pulse value", "watt-hour", "W*(60*60s)" },
			{ 31U, "VAh", "apparent energy, rS , apparent energy meter constant or pulse value", "volt-ampere-hour", "VA*(60*60s)" },
			{ 32U, "varh", "reactive energy, rB , reactive energy meter constant or pulse value", "var-hour", "var*(60*60s)" },
			{ 33U, "A", "current (I)", "ampere", "A" },
			{ 34U, "C", "electrical charge (Q)", "coulomb", "C = As" },
			{ 35U, "V", "voltage (U)", "volt", "V" },
			{ 36U, "V/m", "electric field strength (E)", "volt per meter", "V/m" },
			{ 37U, "F", "capacitance (C)", "farad", "C/V = As/V" },
			{ 38U, "W", "resistance (R)", "ohm", "W = V/A" },
			{ 39U, "Wm2/m", "resistivity (r)", "", "Wm" },
			{ 40U, "Wb", "magnetic flux (F)", "weber", "Wb = Vs" },
			{ 41U, "T", "magnetic flux density (B)", "tesla", "Wb/m2" },
			{ 42U, "A/m", "magnetic field strength (H)", "ampere per meter", "A/m" },
			{ 43U, "H", "inductance (L)", "henry", "H = Wb/A" },
			{ 44U, "Hz", "frequency (f, ω)", "hertz", "1/s" },
			{ 45U, "1/(Wh)", "RW , active energy meter constant or pulse value", "", "" },
			{ 46U, "1/(varh)", "RB , reactive energy meter constant or pulse value", "", "" },
			{ 47U, "1/(VAh)", "RS , apparent energy meter constant or pulse value", "", "" },
			{ 48U, "V2h", "volt-squared hour, rU 2h  , volt-squared hour meter constant or pulse value", "volt-squared-hours", "V2(60*60s)" },
			{ 49U, "A2h", "ampere-squared hour, rI2h  ,ampere-squared hour meter constant or pulse value", "ampere-squared- hours", "A2(60*60s)" },
			{ 50U, "kg/s", "mass flux", "kilogram per second", "kg/s" },
			{ 51U, "S, mho", "conductance", "siemens", "1/W" },
			{ 52U, "K", "temperature (T)", "kelvin", "" },
			{ 53U, "1/(V2 h)", "RU2h  , volt-squared hour meter constant or pulse value", "", "" },
			{ 54U, "1/(A2 h)", "RI2h  , ampere-squared hour meter constant or pulse value", "", "" },
			{ 55U, "1/m3", "RV  , meter constant or pulse value (volume)", "", "" },
			{ 56U, "", "percentage", "%", "" },
			{ 57U, "Ah", "ampere-hours", "Ampere-hour", "" },
			{ 58U, "", "", "", "" },
			{ 59U, "", "", "", "" },
			{ 60U, "Wh/m3", "energy per volume", "3,6*103  J/m3", "" },
			{ 61U, "J/m3", "calorific value, wobbe", "", "" },
			{ 62U, "Mol %", "molar fraction of gas composition", "mole percent", "(Basic gas composition unit)" },
			{ 63U, "g/m3", "mass density, quantity of material", "", "(Gas analysis, accompanying elements)" },
			{ 64U, "Pa s", "dynamic viscosity", "pascal second", "(Characteristic of gas stream)" },
			{ 65U, "J/kg", "Specific energy, NOTE  The amount of energy per unit of mass of a substance", "Joule / kilogram", "m2  .  kg .  s -2 / kg= m2  .  s –2" },
			{ 66U, "", "", "", "" },
			{ 67U, "", "", "", "" },
			{ 68U, "", "", "", "" },
			{ 69U, "", "", "", "" },
			{ 70U, "dBm", "Signal strength, dB milliwatt (e.g. of GSM radio systems)", "", "" },
			{ 71U, "dbµV", "Signal strength, dB microvolt", "", "" },
			{ 72U, "dB", "Logarithmic unit that expresses the ratio between two values of a physical quantity", "", "" },
			{ 73U, "", "", "", "" },
			{ 74U, "", "", "", "" },
			{ 75U, "", "", "", "" },
			{ 76U, "", "", "", "" },
			{ 77U, "", "", "", "" },
			{ 78U, "", "", "", "" },
			{ 79U, "", "", "", "" },
			{ 80U, "", "", "", "" },
			{ 81U, "", "", "", "" },
			{ 82U, "", "", "", "" },
			{ 83U, "", "", "", "" },
			{ 84U, "", "", "", "" },
			{ 85U, "", "", "", "" },
			{ 86U, "", "", "", "" },
			{ 87U, "", "", "", "" },
			{ 88U, "", "", "", "" },
			{ 89U, "", "", "", "" },
			{ 90U, "", "", "", "" },
			{ 91U, "", "", "", "" },
			{ 92U, "", "", "", "" },
			{ 93U, "", "", "", "" },
			{ 94U, "", "", "", "" },
			{ 95U, "", "", "", "" },
			{ 96U, "", "", "", "" },
			{ 97U, "", "", "", "" },
			{ 98U, "", "", "", "" },
			{ 99U, "", "", "", "" },
			{ 100U, "", "", "", "" },
			{ 101U, "", "", "", "" },
			{ 102U, "", "", "", "" },
			{ 103U, "", "", "", "" },
			{ 104U, "", "", "", "" },
			{ 105U, "", "", "", "" },
			{ 106U, "", "", "", "" },
			{ 107U, "", "", "", "" },
			{ 108U, "", "", "", "" },
			{ 109U, "", "", "", "" },
			{ 110U, "", "", "", "" },
			{ 111U, "", "", "", "" },
			{ 112U, "", "", "", "" },
			{ 113U, "", "", "", "" },
			{ 114U, "", "", "", "" },
			{ 115U, "", "", "", "" },
			{ 116U, "", "", "", "" },
			{ 117U, "", "", "", "" },
			{ 118U, "", "", "", "" },
			{ 119U, "", "", "", "" },
			{ 120U, "", "", "", "" },
			{ 121U, "", "", "", "" },
			{ 122U, "", "", "", "" },
			{ 123U, "", "", "", "" },
			{ 124U, "", "", "", "" },
			{ 125U, "", "", "", "" },
			{ 126U, "", "", "", "" },
			{ 127U, "", "", "", "" },
			{ 128U, "", "", "", "" },
			{ 129U, "", "", "", "" },
			{ 130U, "", "", "", "" },
			{ 131U, "", "", "", "" },
			{ 132U, "", "", "", "" },
			{ 133U, "", "", "", "" },
			{ 134U, "", "", "", "" },
			{ 135U, "", "", "", "" },
			{ 136U, "", "", "", "" },
			{ 137U, "", "", "", "" },
			{ 138U, "", "", "", "" },
			{ 139U, "", "", "", "" },
			{ 140U, "", "", "", "" },
			{ 141U, "", "", "", "" },
			{ 142U, "", "", "", "" },
			{ 143U, "", "", "", "" },
			{ 144U, "", "", "", "" },
			{ 145U, "", "", "", "" },
			{ 146U, "", "", "", "" },
			{ 147U, "", "", "", "" },
			{ 148U, "", "", "", "" },
			{ 149U, "", "", "", "" },
			{ 150U, "", "", "", "" },
			{ 151U, "", "", "", "" },
			{ 152U, "", "", "", "" },
			{ 153U, "", "", "", "" },
			{ 154U, "", "", "", "" },
			{ 155U, "", "", "", "" },
			{ 156U, "", "", "", "" },
			{ 157U, "", "", "", "" },
			{ 158U, "", "", "", "" },
			{ 159U, "", "", "", "" },
			{ 160U, "", "", "", "" },
			{ 161U, "", "", "", "" },
			{ 162U, "", "", "", "" },
			{ 163U, "", "", "", "" },
			{ 164U, "", "", "", "" },
			{ 165U, "", "", "", "" },
			{ 166U, "", "", "", "" },
			{ 167U, "", "", "", "" },
			{ 168U, "", "", "", "" },
			{ 169U, "", "", "", "" },
			{ 170U, "", "", "", "" },
			{ 171U, "", "", "", "" },
			{ 172U, "", "", "", "" },
			{ 173U, "", "", "", "" },
			{ 174U, "", "", "", "" },
			{ 175U, "", "", "", "" },
			{ 176U, "", "", "", "" },
			{ 177U, "", "", "", "" },
			{ 178U, "", "", "", "" },
			{ 179U, "", "", "", "" },
			{ 180U, "", "", "", "" },
			{ 181U, "", "", "", "" },
			{ 182U, "", "", "", "" },
			{ 183U, "", "", "", "" },
			{ 184U, "", "", "", "" },
			{ 185U, "", "", "", "" },
			{ 186U, "", "", "", "" },
			{ 187U, "", "", "", "" },
			{ 188U, "", "", "", "" },
			{ 189U, "", "", "", "" },
			{ 190U, "", "", "", "" },
			{ 191U, "", "", "", "" },
			{ 192U, "", "", "", "" },
			{ 193U, "", "", "", "" },
			{ 194U, "", "", "", "" },
			{ 195U, "", "", "", "" },
			{ 196U, "", "", "", "" },
			{ 197U, "", "", "", "" },
			{ 198U, "", "", "", "" },
			{ 199U, "", "", "", "" },
			{ 200U, "", "", "", "" },
			{ 201U, "", "", "", "" },
			{ 202U, "", "", "", "" },
			{ 203U, "", "", "", "" },
			{ 204U, "", "", "", "" },
			{ 205U, "", "", "", "" },
			{ 206U, "", "", "", "" },
			{ 207U, "", "", "", "" },
			{ 208U, "", "", "", "" },
			{ 209U, "", "", "", "" },
			{ 210U, "", "", "", "" },
			{ 211U, "", "", "", "" },
			{ 212U, "", "", "", "" },
			{ 213U, "", "", "", "" },
			{ 214U, "", "", "", "" },
			{ 215U, "", "", "", "" },
			{ 216U, "", "", "", "" },
			{ 217U, "", "", "", "" },
			{ 218U, "", "", "", "" },
			{ 219U, "", "", "", "" },
			{ 220U, "", "", "", "" },
			{ 221U, "", "", "", "" },
			{ 222U, "", "", "", "" },
			{ 223U, "", "", "", "" },
			{ 225U, "", "", "", "" },
			{ 226U, "", "", "", "" },
			{ 227U, "", "", "", "" },
			{ 228U, "", "", "", "" },
			{ 229U, "", "", "", "" },
			{ 230U, "", "", "", "" },
			{ 231U, "", "", "", "" },
			{ 232U, "", "", "", "" },
			{ 233U, "", "", "", "" },
			{ 234U, "", "", "", "" },
			{ 235U, "", "", "", "" },
			{ 236U, "", "", "", "" },
			{ 237U, "", "", "", "" },
			{ 238U, "", "", "", "" },
			{ 239U, "", "", "", "" },
			{ 240U, "", "", "", "" },
			{ 241U, "", "", "", "" },
			{ 242U, "", "", "", "" },
			{ 243U, "", "", "", "" },
			{ 244U, "", "", "", "" },
			{ 245U, "", "", "", "" },
			{ 246U, "", "", "", "" },
			{ 247U, "", "", "", "" },
			{ 248U, "", "", "", "" },
			{ 249U, "", "", "", "" },
			{ 250U, "", "", "", "" },
			{ 251U, "", "", "", "" },
			{ 252U, "", "", "", "" },
			{ 253U, "", "reserved", "", "" },
			{ 254U, "other", "other unit", "", "" },
			{ 255U, "count", "no unit, unitless, count", "", "" },
		};
	} // End of namespace

// ------------------------------------------------------------------------------------------------------------------------------------------------
// 3. Initializsation of Vector with unit information

	
	// c++98 cannot init vectors by initializer list. Do it manually via a array of struct. Iterator constructor
	const std::vector<ObisUnitEntry>  ObisUnitLookup (&EhzInternalObisUnit::ObisUnit[0],
	                                                  &EhzInternalObisUnit::ObisUnit[sizeof(EhzInternalObisUnit::ObisUnit)/sizeof(EhzInternalObisUnit::ObisUnit[0])]);
}

 
//lint -restore

#endif
