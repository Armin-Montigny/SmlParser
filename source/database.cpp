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

 

//lint -e{830,1960,1963}


#include "database.hpp"
#include "userinterface.hpp"
#include "ehzconfig.hpp"

#include <sstream>

// ------------------------------------------------------------------------------------------------------------------------------
// 1. General definitions

	// Then names column / field / attribute are used with the same meaning
	
	namespace DatabaseInternal
	{

	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.1 SQLITE 3 column type names and column names

		// SQLLITE types
		const mchar sqliteTypeNameInteger[] = "INTEGER";
		const mchar sqliteTypeNameBoolean[] = "BOOLEAN";
		const mchar sqliteTypeNameText[] = "TEXT";
		const mchar sqliteTypeNameFloat[] = "FLOAT";

		// Basic columns names. Additional text will be appended to distinguish the columns
		const mchar timeBaseColumnName[] = "timeBase";
		const mchar *const timeBaseColumnType = sqliteTypeNameInteger;
		const mchar timeForOneEhzColumnName[] = "timeEhz";
		const mchar timePeriodColumnName[] = "timePeriod";
		const mchar unitColumnName[] = "unit";
		const mchar ehzColumnName[] = "Ehz";
		const mchar measuredValueColumnName[] = "measuredValue";
		const mchar ehzSqliteDatabaseTableName[] = "ehzMeasuredDataValues";

		

	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.2 Default constructors. Do nothing specific. Call default constructors of struct members and initialize everyting to empty
	
		// For on EHZ
		EhzColumnNameAndType::EhzColumnNameAndType(void) : 	acquisitionTime(),
															measuredValueAndUnit()
		{
		}

		//lint -e{1901,1911,1938}
		// For the whole EHZ system
		EhzSystemColumnNameAndType::EhzSystemColumnNameAndType(void) : 	timeBase(timeBaseColumnName, timeBaseColumnType),
																		ehzTimePeriodNameAndType(),
																		ehzColumnNameAndType()
																		
		{

		}																

	// ------------------------------------------------------------------------------------------------------------------------------
	// 1.3 Creation of Strings for field/type names 
		
		
		// For each EHZ we will store 
		// - Time, when data has been read
		//    For each value that the EHZ will give us we will store 
		//		- Acquisition time
		//      - The Value (Text or Double, depending on values type
		//      - the Unit
		void EhzSystemColumnNameAndType::buildEhzSystemColumnNameAndType(void)
		{

			// For all EHZ in the EHZ system 
			for (uint noEhz = null<uint>(); noEhz < EhzInternal::MyNumberOfEhz; ++noEhz)
			{
				// At the beginning of each loop we set everything to empty
				EhzColumnNameAndType ecnat;
				ecnat.clear();

				// Brackets because of name scoping
				{
					// We want to define one time for each EHZ i the EHZ system
					// And several values and units for Each EHZ in the EHZ system 
					// Build the name for the time field for one EHZ in the EHZ syte,
					std::ostringstream tempBuf;
					//lint -e{830,1960,1963,9050}
					tempBuf << timeForOneEhzColumnName << noEhz; 
					// and assign it
					ecnat.acquisitionTime.columnName = tempBuf.str();
				}
				// And the ytpe for that column
				ecnat.acquisitionTime.columnType = &sqliteTypeNameInteger[0];

				
				// Now for each value for one of the EHZ in the EHZ system
				for (uint noemd = null<uint>(); noemd< NumberOfEhzMeasuredData; ++noemd)
				{
					// at thebeginning of each loop iteration we set everything to empty
					EhzColumnNameAndType::MeasuredValueAndUnit measuredValueAndUnit;
					measuredValueAndUnit.clear();
					
					// Get the type of the value stored in the EHZ. Either Double or Text, or NULL (Nothing)
					const EhzMeasuredDataType::Type emdt = EhzInternal::myEhzConfigDefinition[noEhz].ehzMeasuredDataType[noemd];
					// If there is an associated value
					if (EhzMeasuredDataType::Null != emdt)
					{
						// Build field name with the index of the value and the index of the EHZ
						{
							std::ostringstream tempBuf;
							//lint -e{830,1960,1963,9050}
							tempBuf << measuredValueColumnName << noemd << ehzColumnName << noEhz;
							measuredValueAndUnit.measuredValue.columnName = tempBuf.str();
						}
						// Check the type of the value. Is either Number (Double) or Text
						// and then define the type of the corresponding field
						// Basically SQLITE doesnt care so much abaout types, but anyway. Lets assign the right type
						if (EhzMeasuredDataType::Number == emdt)
						{
							// Type: Number / DOUBLE / Float
							measuredValueAndUnit.measuredValue.columnType = &sqliteTypeNameFloat[0];
						}
						else
						{
							// Type: Text
							measuredValueAndUnit.measuredValue.columnType = &sqliteTypeNameText[0];
						}

						// And the individual name of the unit with index of measured value and index of EHZ
						{
							std::ostringstream tempBuf;
							//lint -e{830,1960,1963,9050}
							tempBuf << unitColumnName << noemd << ehzColumnName << noEhz;
							measuredValueAndUnit.unit.columnName = tempBuf.str();
						}
						// Unit is always of type TEXT
						measuredValueAndUnit.unit.columnType = &sqliteTypeNameText[0];
						// Add this instance to trhe array of names/types
						ecnat.measuredValueAndUnit.push_back(measuredValueAndUnit);
					}
				}
				// So, now all data for one EHZ has been defined
				// Store it in the vector
				ehzColumnNameAndType.push_back(ecnat);
			}

			// Now we will create the name and type strings for the period flag fields
			{
				
				ColumnNameAndType cnat;
				cnat.clear();
				// All time period flag fields will be of type boolean
				cnat.columnType = &sqliteTypeNameBoolean[0];
				// Fro all flag fields
				for (uint period = null<uint>(); period < EhzLogPeriodCount; ++period)
				{	
					// Create name
					std::ostringstream tempBuf;
					//lint -e{830,1960,1963,9050}
					tempBuf << timePeriodColumnName << EhzLogPeriodInS[period];
					cnat.columnName = tempBuf.str();
					// And add to the vector
					ehzTimePeriodNameAndType.push_back(cnat);
				}
			}
			
			
			{
				//std::ostringstream tempBuf;
				//tempBuf << "12345";
				//ui.msgf("Length of 12345 = %d\n",tempBuf.tellp());
			}

			
		}
		
		
// ------------------------------------------------------------------------------------------------------------------------------
// 2. Member functions for Database
		
	// --------------------------------------------------------------------------------------------------------------------------
	// 2.1 Construction / Destruction of Databae object
		
		// -----------------------------
		// 2.1.1 Standard constructor
		// Do not use
		// Sets everything to empty / Null
		EhzDataBase::EhzDataBase(void) : 								ehzSystemColumnNameAndType(),
																		ehzDatabaseName(), 
																		dbHandle(null<sqlite3 *>()), 
																		lastPeriodValue(EhzLogPeriodCount,null<EhzLogTimeUnit>()),
																		insertMdl(),
																		ddl(),
																		insertStmt(null<sqlite3_stmt *>())
		{
		}
		
		
		
		// -----------------------------
		// 2.1.2 Explicit constructor
		// Create or open Database. Initialize internal structures. Create SQL String
		// Prepare the insert statement
		// Database path and name will be given
		EhzDataBase::EhzDataBase(const std::string &ehzDatabaseNamel) : ehzSystemColumnNameAndType(),
																		ehzDatabaseName(ehzDatabaseNamel),
																		dbHandle(null<sqlite3 *>()),
																		lastPeriodValue(EhzLogPeriodCount,null<EhzLogTimeUnit>()),
																		insertMdl(),
																		ddl(),
																		insertStmt(null<sqlite3_stmt *>())
																		
		{
			sqlite3_initialize( );	//lint !e534
			
			// Create the DDL (Data Definition language) string for the database creation
			// and create the MDL for the insert statement
			createSqlStrings();

			
			// If the database is not existing
			if (!isExisting())
			{
				// then create it
				//lint -e{534}
				createDatabase();
			}
			// Open the database
			open();
			
			// Read the last period values from the database and define database class internal values
			initializeLastPeriodValues();
			// SQLITE prepare statement for inserting values
			//lint -e{971,534}
			sqlite3_prepare_v2( dbHandle, insertMdl.c_str(), -1, &insertStmt, null<const mchar **>() );
			//ui.msgf("Insert sql statement Prepare Result:   %d\n",rc);
		}

		// -----------------------------
		// 2.1.3 Destructor
		//lint -e{1579}
		EhzDataBase::~EhzDataBase(void)
		{
			try
			{
				//lint -e{534}
				// SQLITE Statement finalization
				sqlite3_finalize( insertStmt );
				// close the database
				close();
			}
			catch(...)
			{
			}
		}

	// --------------------------------------------------------------------------------------------------------------------------
	// 2.2 Database administration
		
		// -------------------
		// 2.2.1 Open Database
		void EhzDataBase::open(void)
		{
			
			dbHandle = null<sqlite3 *>();
			//lint -e{971}
			const sint rc = sqlite3_open_v2(ehzDatabaseName.c_str(), &dbHandle, SQLITE_OPEN_READWRITE, null<mchar *>() );
			if (SQLITE_OK!= rc )
			{
				dbHandle = null<sqlite3 *>();
				ui << "Could not open database"  << std::endl;
			} 
			else
			{
				ui <<"Database '" << ehzDatabaseName << "' opened"  << std::endl;
			}
		}

		// -------------------
		// 2.2.2 Close Database
		void EhzDataBase::close(void)
		{
			if (isOpen())
			{
				//lint -e{534}
				sqlite3_close(dbHandle);
			}
			dbHandle = null<sqlite3 *>();
		}

		// -------------------------------
		// 2.2.2 CHeck if database is existing
		boolean EhzDataBase::isExisting(void) const 
		{
			// LOcal instance. Shall not interfere with class database handle
			sqlite3 *db = null<sqlite3 *>();
			
			// Try to open
			//lint -e{971}
			const sint rc = sqlite3_open_v2( ehzDatabaseName.c_str(), &db, SQLITE_OPEN_READONLY, null<mchar *>() );
			boolean existing = false;
			// If we could open the database, then it is existing
			if (SQLITE_OK == rc)
			{
				// CLose again
				//lint -e{534}
				sqlite3_close(db);
				existing = true;
			}
			return existing;
		}


		// ---------------------------------------------
		// 2.2.3  Create a new database and it's schema
		void EhzDataBase::createDatabase(void) const
		{
			sqlite3 *db = null<sqlite3 *>();
			// Set flags. Open with read write and create
			//lint -e{921}
			const sint flags = static_cast<sint>(static_cast<uint>(SQLITE_OPEN_READWRITE) | static_cast<uint>(SQLITE_OPEN_CREATE));
			//lint -e{971}
			const sint rc = sqlite3_open_v2( ehzDatabaseName.c_str(), &db, flags, null<mchar *>() );
			if (SQLITE_OK == rc)
			{
				// We could open and create
				// Now create the needed table
				//lint -e{971,534}
				sqlite3_exec( db, ddl.c_str(), null<sint (*)(void*,sint,mchar**,mchar**)>(), null<void *>(), null<mchar **>() ); 	 
				// Check and inform on result
				//ui.msgf("DB Creation result: %d\n",rc);
				//lint -e{534}
				sqlite3_close(db);
			}    
		}


		// -------------------------------------------------------------------
		// 2.2.4  Create SQL strings
		// For database creation and for inserting values into the main table
		void EhzDataBase::createSqlStrings(void)
		{
			// We will use SQLITE prepared statements with parameters
			// SInce the columns of the database are deduced from the EHZ configuration
			// We do not know the count in advance. The columns are created dynamically.
			sint parameterCount = null<sint>();
			
			// Build the strings for the column name and type
			ehzSystemColumnNameAndType.buildEhzSystemColumnNameAndType();
			
			// Reset the ddl and the insert Mdl strings
			ddl.clear();
			insertMdl.clear();
			
			// Start building the ddl SQL string
			// We want to create a table. Name is a constant
			ddl += "CREATE TABLE ";
			ddl += &ehzSqliteDatabaseTableName[0];
			ddl += " (\n";
			
			// Start building the insert mdl string
			// We want to insert data in the main table
			insertMdl += "INSERT INTO ";
			insertMdl += &ehzSqliteDatabaseTableName[0];
			insertMdl += " (\n";
			
			// Create the time base column, where the time of acquisition of the complete record will be stored
			ddl += ehzSystemColumnNameAndType.timeBase.columnName;
			ddl += " ";
			ddl += ehzSystemColumnNameAndType.timeBase.columnType;
			// This is also the primary key of the main table
			ddl += " PRIMARY KEY NOT NULL";
			
			// mdl part
			insertMdl += ehzSystemColumnNameAndType.timeBase.columnName;
			// and, one new parameter
			++parameterCount;
			
			// To make evaluation of the data easier and to avoid slow queries, we do not only store a timestamp
			// per record. Additionally we will store a flag to indicate that a multiple of the timestamp was hit.
			// With that queries like, give me all values for periods of 5 minutes will be easier.
			// For all time flags
			for (std::vector<ColumnNameAndType>::iterator cnati = ehzSystemColumnNameAndType.ehzTimePeriodNameAndType.begin(); cnati != ehzSystemColumnNameAndType.ehzTimePeriodNameAndType.end(); ++cnati)
			{
				ddl += ",\n";
				ddl += (*cnati).columnName;
				ddl += " ";
				ddl += (*cnati).columnType;
				ddl += " DEFAULT FALSE";

				insertMdl += ",\n";
				insertMdl += (*cnati).columnName;
				++parameterCount;
			}
			
			// So, now the columns for each EHZ
			for (std::vector<EhzColumnNameAndType>::iterator ehzi = ehzSystemColumnNameAndType.ehzColumnNameAndType.begin(); ehzi != ehzSystemColumnNameAndType.ehzColumnNameAndType.end(); ++ehzi)
			{
				// Every EHZ has its own time stamp for the acquired data
				ddl += ",\n";
				ddl += (*ehzi).acquisitionTime.columnName;
				ddl += " ";
				ddl += (*ehzi).acquisitionTime.columnType;
				
				insertMdl += ",\n";
				insertMdl += (*ehzi).acquisitionTime.columnName;
				++parameterCount;

				// And every EHZ has up to x  measured values and units
				for (std::vector<EhzColumnNameAndType::MeasuredValueAndUnit>::iterator mvi = (*ehzi).measuredValueAndUnit.begin(); mvi != (*ehzi).measuredValueAndUnit.end(); ++mvi)
				{
					ddl += ",\n";
					ddl += (*mvi).measuredValue.columnName;
					ddl += " ";
					ddl += (*mvi).measuredValue.columnType;
					
					ddl += ",\n";
					ddl += (*mvi).unit.columnName;
					ddl += " ";
					ddl += (*mvi).unit.columnType;
					
					insertMdl += ",\n";
					insertMdl += (*mvi).measuredValue.columnName;
					insertMdl += ",\n";
					insertMdl += (*mvi).unit.columnName;
					++parameterCount;
					++parameterCount;
				}
			}
			// Finalize ddl
			ddl += ");\n";

			// Add the parameter part to the mdl
			// We counted the number of parameters
			insertMdl += ")\nVALUES (\n?1";
			
			for (sint i = 2; i <= parameterCount;++i)
			{
				insertMdl += ",\n?";
				std::ostringstream tempBuf;
				//lint -e{830,1960,1963,9050}
				tempBuf << i;
				insertMdl += tempBuf.str();
			}
			// Finalize mdl statement
			insertMdl += "\n);\n";
		}	



	// --------------------------------------------------------------------------------------------------------------------------
	// 2.3 Data manipulation
			
		// -------------------------------------------------------------------
		// 2.3.1  Set last period values
			
		// To make evaluation of the data easier and to avoid slow queries, we do not only store a timestamp
		// per record. Additionally we will store a flag to indicate that a multiple of the timestamp was hit.
		// With that queries like, give me all values for periods of 5 minutes will be easier.		
		
		// We store internally, at which time the last flag has been set
		// Then we can compare against this time stamps and set the flags accordingly
		
		// When this application is started, then no "old values" are existing
		// So we will check the database and read for each flag the "last time", when it has been set
		
		void EhzDataBase::initializeLastPeriodValues(void)
		{
			// First we will bild the sql string
			std::string sql;
			sint i = null<sint>();
			// For all flags
			for (std::vector<ColumnNameAndType>::iterator cnati = ehzSystemColumnNameAndType.ehzTimePeriodNameAndType.begin(); cnati != ehzSystemColumnNameAndType.ehzTimePeriodNameAndType.end(); ++cnati)
			{
				// New iteration. Clear string. Then build new query string
				sql.clear();
				sql += "SELECT ";
				sql += &timeBaseColumnName[0];
				sql += " FROM ";
				sql += &ehzSqliteDatabaseTableName[0];
				sql += " WHERE ";
				sql += (*cnati).columnName;
				sql += "='TRUE' ORDER BY ";
				sql += &timeBaseColumnName[0];
				sql += " DESC LIMIT 1;";
				//lastPeriodSqlStrings.push_back(sql);
				//ui.msgf("%s\n",sql.c_str());
				// We will prepare the statement and step through the results
				sqlite3_stmt *stmt = null<sqlite3_stmt *>();
				//lint -e{971,534}
				sqlite3_prepare_v2( dbHandle, sql.c_str(), -1, &stmt, null<const mchar **>() );
				
				// Execute the query
				//ui.msgf("Prepare Result:   %d\n",rc);
				if (SQLITE_ROW == sqlite3_step(stmt))
				{
					// We got a result, a time. Ok read it from the database and store it internally for later usage
					//lint -e{1960,732,917,921}
					lastPeriodValue[i] = static_cast<EhzLogTimeUnit>(sqlite3_column_int64( stmt, 0));
					//ui.msgf("%s: ++++++++++ %d\n", sql.c_str(),lastPeriodValue[i]);
				}
				else
				{
					// We could not read a value. We do not care and set the internal value to null.
					// Then, for the next storage of values, the flage will be guaranteed be set
					//lint -e{1960,732,917,921}
					lastPeriodValue[i] = null<EhzLogTimeUnit>();
					//ui.msgf("%s:   0\n",sql.c_str());
				}
				//lint -e{534}
				// Done with this flag
				sqlite3_finalize( stmt );
				// Work on next flag
				++i;
			}
		}


		// -------------------------------------------------------------------
		// 2.3.2  Store measured values
		
		// Main databases function and fucntionality
		// Store the measured values from the EHZ System
		// +time stamps and time flags
	
		void EhzDataBase::storeMeasuredValues(const AllMeasuredValuesForAllEhz &allMeasuredValuesForAllEhz)
		{
			//lint --e{534,917}
			// We use an SQLITE prepared statement with parameters
			// We will bin all values to the prepared statement
			// This variable is the index of the parameter for the related value
			sint pidx = 1;

			//Get time of now
			//lint -e{921}
			const EhzLogTimeUnit nowTime = static_cast<EhzLogTimeUnit>(time(null<time_t*>() ));
			
			// Timestamp for this record
			sqlite3_bind_int(insertStmt, pidx, nowTime);
			++pidx;

			//ui.msgf("Now: %ld ", nowTime);
			
			// To make evaluation of the data easier and to avoid slow queries, we do not only store a timestamp
			// per record. Additionally we will store a flag to indicate that a multiple of the timestamp was hit.
			// With that queries like, give me all values for periods of 5 minutes, will be easier.
			
			// Check if we will set a period flag
			for (uint period = null<uint>(); period < EhzLogPeriodCount; ++period)
			{

				// If the respective number of seconds elapsed
				if (nowTime >= (lastPeriodValue[period] + EhzLogPeriodInS[period]))
				{
					//ui.msgf("(%ld-%ld)->1 ",lastPeriodValue[period],EhzLogPeriodInS[period]);

					// Store new Value: Time of now
					lastPeriodValue[period] = nowTime;
					// And set flag
					sqlite3_bind_int(insertStmt, pidx, 1);   // true
					++pidx;
				}
				else
				{
					// No, no multiple value hit
					// Set flag to false
					sqlite3_bind_int(insertStmt, pidx, 0);   // False
					++pidx;
				}
			}
			

			// Now for all EHZ in the EHZ system
			for (uint noEhz = null<uint>(); noEhz < EhzInternal::MyNumberOfEhz; ++noEhz)
			{
				// Time, when data has been acquired by one single EHZ
				sqlite3_bind_int(insertStmt, pidx, allMeasuredValuesForAllEhz[noEhz].timeWhenDataHasBeenEvaluated );
				++pidx;
				// Now for each value for one of the EHZ in the EHZ system
				for (uint noemd = null<uint>(); noemd< NumberOfEhzMeasuredData; ++noemd)
				{
					// Get the type of the value stored in the EHZ. Either Double or Text, or NULL (Nothing)
					const EhzMeasuredDataType::Type emdt = EhzInternal::myEhzConfigDefinition[noEhz].ehzMeasuredDataType[noemd];
					// If there is an associated value
					if (EhzMeasuredDataType::Null != emdt)
					{
						// Check the type of the value. Is either Number (Double) or Text
						// and then define the type of the corresponding field
						// Basically SQLITE doesnt care so much abaout types, but anyway. Lets assign the right type
						if (EhzMeasuredDataType::Number == emdt )
						{
							// Type: Number / DOUBLE / Float
							sqlite3_bind_double(insertStmt, pidx, allMeasuredValuesForAllEhz[noEhz].measuredValueForOneEhz[noemd].doubleValue);
							++pidx;
						}
						else
						{
							// Type: Text
							sqlite3_bind_text(insertStmt, pidx,allMeasuredValuesForAllEhz[noEhz].measuredValueForOneEhz[noemd].smlByteString.c_str(),-1,null<void(*)(void*)>());
							++pidx;
						}
						// Now bind the unit
						sqlite3_bind_text(insertStmt, pidx,allMeasuredValuesForAllEhz[noEhz].measuredValueForOneEhz[noemd].unit.c_str(),-1,null<void(*)(void*)>());
						++pidx;
					}
				}
			}
			// Store all values in the database
			sqlite3_step( insertStmt );
			// And make the prepared statement ready for next usage
			sqlite3_reset( insertStmt );
		}
		
		
		
	} // End of namespace

