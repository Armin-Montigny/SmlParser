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
// General Description
//
// This is a part of a systems that reads data from electronic meters.
// 
// This data will be stored in a sqlite database.
// So we inplement a data logger here.
//
// Some requirements
//
// We want to store measured values from the EHZ into a database
// The data can be retrieved from the database to show historical values
// The time of the measured values is important
// Measured values shall be stored at fixed intervals. This we call Base interval
// The base interval is x s
// It shall be possible to retrieve values for multiples of the base interval
// The folowing periodes shall be used: 30s, 1min, 5min, 10min, 30min, 1h, 2h, 6h, 12h, 1day, 1week. 1 month, 1 year
// The database shall be created based on the EHZ configuration
//
// So as per definition of the EHZ System we will create the necessary fields in the database.
// At program start it will be checked, if the database exists. If not, it will be created.
// The names of the fields, the number of the fields and all associated data will be derived from the
// EHZ configuration.
// 
// To make evaluation of the data easier and to avoid slow queries, we do not only store a timestamp
// per record. Additionally we will store a flag to indicate that a multiple of the timestamp was hit.
// With that queries like, give me all values for periods of 5 minutes easier.
//
#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "ehzmeasureddata.hpp"

#include "sqlite3.h"


// ------------------------------------------------------------------------------------------------------------------------------
// 1. General definitions

	namespace DatabaseInternal
	{
		// The name of the database. At the moment hardcoded.
		const char EhzDatabaseName[] = ROOT_DIRECTORY "/ehz.db";
		
	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.1 Helper structs for the defintion of SQLITE 3 names for columns and types
			
		// -----------------------------------------------------------------------------------------------------------
		// 1.1.1 General struct that can hold the name of the any field/column/attribute and its type in SQLITE terms.
		struct ColumnNameAndType
		{
			public:
				// Main constructor. Simply copy name and type into this structure
				explicit ColumnNameAndType(const std::string &cn, const std::string &ct) : columnName(cn), columnType(ct) {}
				// Default constructor will create empty Strings
				ColumnNameAndType(void) : columnName(), columnType() {}
				// No specific destructor needed. The destructor of the struct members will be called
				virtual ~ColumnNameAndType(void){}
				
				// Data member for field/column/attribute
				std::string columnName;
				// Data member for type
				std::string columnType;
				
				// For reuse and initialization of this struct, we want to clear (set to empty) the names
				void clear(void) { columnName.clear(); columnType.clear(); }
		};

		
		// -----------------------------------------------------------------------------------------------------------
		// 1.1.2 EHZ specific struct that can hold the name of the field/column/attribute and its type in SQLITE terms.
		// We will store aquisition time and all measured values and units for a EHZ
		struct EhzColumnNameAndType
		{		
			// Default consrtuctor. Set all values to default (empty, null)
			EhzColumnNameAndType(void);
			// No specific destructor needed. The destructor of the struct members will be called
			virtual ~EhzColumnNameAndType(void) {}
			
			// Sub struct for EHZ measured values 
			struct MeasuredValueAndUnit
			{
				// Default construct. Initalize everything to standard.
				MeasuredValueAndUnit(void) : measuredValue(), unit() {}
				// No specific destructor needed. The destructor of the struct members will be called
				virtual ~MeasuredValueAndUnit(void) {}
				// Name and Type strings for the measured numerical (text) value
				ColumnNameAndType measuredValue;
				// Name and Type strings for the unit of the measured numerical (text) value
				ColumnNameAndType unit;
				// For reuse and initialization of this struct, we want to clear (set to empty) the names
				void clear(void) { measuredValue.clear(); unit.clear();}
			};
			// Name and Type strings for the aquisition time of measured values
			ColumnNameAndType acquisitionTime;
			// One EHZ may have many measured values. All will be stroed in this vector
			std::vector<MeasuredValueAndUnit> measuredValueAndUnit;
			// For reuse and initialization of this struct, we want to clear (set to empty) the names
			void clear(void) { acquisitionTime.clear(); measuredValueAndUnit.clear(); }
		};

		// -----------------------------------------------------------------------------------------
		// 1.1.3 Definition of acquisition time multipliers
		// As described above we will store additional flage for special acquisition time periods
		// to make queries simpler
		// Here we define all periods that will be stored via a flag
		typedef time_t EhzLogTimeUnit;
		const EhzLogTimeUnit EhzLogPeriodInS[] = 
		{
			30L,
			60L,
			300L,
			600L,
			1800L,
			3600L,
			7200L,
			21600L,
			43200L,
			604800L,
			2628000L
		};
		// Number of periods
		const uint EhzLogPeriodCount = sizeof(EhzLogPeriodInS) / sizeof(EhzLogPeriodInS[0]);

		// -------------------------------------------------------------------------------------------------------------------
		// 1.1.4 EHZ SYSTEM specific struct that can hold the name of the field/column/attribute and its type in SQLITE terms.
		struct EhzSystemColumnNameAndType
		{
			// Default consrtuctor. Set all values to default (empty, null)
			EhzSystemColumnNameAndType(void);
			// No specific destructor needed. The destructor of the struct members will be called
			~EhzSystemColumnNameAndType(void) {}
			
			// Base time. The time when all data from all EHZ will be stored in one shot
			const ColumnNameAndType timeBase;
			// All multiples of periods (see above)
			std::vector<ColumnNameAndType> ehzTimePeriodNameAndType;
			// An all names and types for all EHZ that are in the EHZ system
			std::vector<EhzColumnNameAndType> ehzColumnNameAndType;
			
			void buildEhzSystemColumnNameAndType(void);
		};


		
// ------------------------------------------------------------------------------------------------------------------------------
// 2. The database
	
	// Functionality to store data acquired from all EHZ in a database
	class EhzDataBase
	{
		public:
			// Constructor creates (if necessary) and opens the database
			explicit EhzDataBase(const std::string &ehzDatabaseNamel);
			// Close database
			~EhzDataBase(void);
			
			// Store the measured values and times in the database
			void storeMeasuredValues(const AllMeasuredValuesForAllEhz &allMeasuredValuesForAllEhz);
			
			
		protected:
			// Check if this database is existing in the specified path
			boolean isExisting(void) const;
			// And check, if it is already open
			boolean isOpen(void)  const  { return (null<sqlite3 *>() != dbHandle);}

			// Create the database from scratch
			void createDatabase(void) const;
			
			// Database has some fixed SQL strings for
			//- creating the database
			//- insert data into the database
			// This SQL strings will be created here
			void createSqlStrings(void);

			// Open the SQLITE database
			void open(void);
			// Close the SQLITE database
			void close(void);
			
			// all field/column/attribute names and types
			EhzSystemColumnNameAndType ehzSystemColumnNameAndType;
			
			// Database name. Given in constructor
			//lint -e{1725}
			const std::string ehzDatabaseName;
			// The handle to the open SQLITE database
			sqlite3 *dbHandle;
			
			// For all of the given time periods that we want to use,
			// we store the last "time" when the period occurred
			// This is the initial setting of the values
			void initializeLastPeriodValues(void);
			// And this are the last periods values themselves
			std::vector<DatabaseInternal::EhzLogTimeUnit> lastPeriodValue;
			
			// String with the SQL statement for inserting a new record
			std::string insertMdl;
			
			// String with the ddl. The data definition language.
			std::string ddl;
			
			// Handle to SQLITE prepared insert statement
			sqlite3_stmt *insertStmt;
			
		private: 
		
			// Default constructor must not be used
			//lint -e{1704}
			EhzDataBase(void);
			
	};
}

 


#endif
