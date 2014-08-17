/*!

	Filename : 				BeBoard.cc
	Content : 				BeBoard Description class, configs of the BeBoard
	Programmer : 			Lorenzo BIDEGAIN
	Version :				1.0
	Date of Creation : 		14/07/14
	Support : 				mail to : lorenzo.bidegain@cern.ch

*/

#include "BeBoard.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace Ph2_HwDescription{

	// Constructors

	BeBoard::BeBoard():
		fBeId(0),
		fShelveId(0)
	{
		loadConfigFile( DEFAULT_GLIB_FILE );
	}

	BeBoard::BeBoard( uint8_t pShelveId, uint8_t pBeId, std::string filename ):
		fBeId( pBeId ),
		fShelveId( pShelveId )
	{

		// fModuleVector.reserve(pNFe);
		loadConfigFile( filename );

	}

	BeBoard::BeBoard( uint8_t pShelveId, uint8_t pBeId, /*uint8_t pNFe, uint8_t pFMCConfiguration,*/ bool pExtTrg, bool pFakeData, std::string filename):
		fBeId( pBeId ),
		fShelveId( pShelveId )
	{

		//FMCConfiguration are not yet in the registers of the firmware
		//fRegMap["FMCConfiguration"]=pFMCConfiguration;
		// fModuleVector.reserve(pNFe);
		loadConfigFile( filename );
		fRegMap[EXT_TRG]=pExtTrg;
		fRegMap[FAKE_DATA]=pFakeData;

	}

	// Public Members:


	uint16_t BeBoard::getReg( std::string pReg )
	{
		BeBoardRegMap::iterator i;
		i = fRegMap.find( pReg );
		if( i == fRegMap.end() )
		{
			std::cout << "Register " << pReg << " not found in BeBoard Register Map!" << std::endl;
			return 0;
		}
		else return i->second;
	}

	void BeBoard::setReg( std::string pReg, uint16_t psetValue )
	{
		BeBoardRegMap::iterator i;
		i = fRegMap.find( pReg );
		if( i == fRegMap.end() ) std::cout << "Register " << pReg << " not found in BeBoard Register Map!" << std::endl;
		else i->second = psetValue;
	}

	void BeBoard::addModule( Module& pModule )
	{
		fModuleVector.push_back(pModule);
	}

	bool BeBoard::removeModule( uint8_t pModuleId )
	{
		std::vector < Module > :: iterator i;
		bool j=false;
		for (i=fModuleVector.begin();i!=fModuleVector.end();++i)
		{
			if (i->fModuleId==pModuleId)
			{
				fModuleVector.erase(i);
				j=true;
				i--;   //erase reduces the container size by the number of elements removed, which are destroyed. To avoid that the iterator point an unallocated part of the memory, we need to decrease the iterator
			}
		}
		if (j==true)
		return true;
		else
		{
			std::cout<<"Error:The BeBoard doesn't have the module "<<pModuleId<<std::endl;
			return false;
		}
	}

	Module* BeBoard::getModule( uint8_t pModuleId )
	{
		std::vector < Module > :: iterator i;
		for (i=fModuleVector.begin();i!=fModuleVector.end();++i)
		{
			if (i->fModuleId==pModuleId)
				{
					return &*i;
				}
		}
		return NULL;
	}

	// Private Members:

	void BeBoard::loadConfigFile( std::string filename )

	{

		std::ifstream cFile( filename.c_str(),std::ios::in );
		if( ! cFile ) std::cerr << "The BeBoard Settings File " << filename << " could not be opened!" << std::endl;
		else
		{

			fRegMap.clear();
			std::string cLine, cName, cValue, cFound;

			while ( ! ( getline( cFile, cLine ).eof() ) )
			{

				if( cLine.find_first_not_of( " \t" ) == std::string::npos ) continue;
				if( cLine.at(0) == '#' || cLine.at(0) =='*' ) continue;
				if( cLine.find( ":" ) == std::string::npos ) continue;

				std::istringstream input(cLine);
				input>> cName >> cFound>> cValue;


				// Here the Reg name sits in cName and the Reg value sits in cValue
				if( cValue.find("0x") != std::string::npos )
				{
					fRegMap[cName] = strtol( cValue.c_str(), 0, 16 );
				}
				else
				{
					fRegMap[cName] = strtol( cValue.c_str(), 0, 10 );
				}
			}

			cFile.close();

		}

	}

}
