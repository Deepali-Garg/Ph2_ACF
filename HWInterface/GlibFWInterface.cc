/*

        FileName :                    GlibFWInterface.h
        Content :                     GlibFWInterface init/config of the Glib and its Cbc's
        Programmer :                  Lorenzo BIDEGAIN, Nicolas PIERRE
        Version :                     1.0
        Date of creation :            28/07/14
        Support :                     mail to : lorenzo.bidegain@gmail.com, nico.pierre@icloud.com

 */

#include <uhal/uhal.hpp>
#include <time.h>
#include "GlibFWInterface.h"
#include <chrono>
#include <thread>


namespace Ph2_HwInterface
{

	GlibFWInterface::GlibFWInterface( const char* puHalConfigFileName, uint32_t pBoardId ) :
		BeBoardFWInterface( puHalConfigFileName, pBoardId )
	{

	}


	void GlibFWInterface::ConfigureBoard( const BeBoard* pBoard )
	{

		//We may here switch in the future with the StackReg method of the RegManager
		//when the timeout thing will be implemented in a transparent and pretty way

		std::vector< std::pair<std::string, uint32_t> > cVecReg;
		std::pair<std::string, uint32_t> cPairReg;

		std::chrono::milliseconds cPause( 200 );

		//Primary Configuration
		cPairReg.first = PC_CONFIG_OK;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM1_END_READOUT;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM2_END_READOUT;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM1_USR_LOGIC;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM2_USR_LOGIC;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );

		WriteStackReg( cVecReg );

		cVecReg.clear();

		std::this_thread::sleep_for( cPause );

		/*
		        GlibRegMap : map<std::string,uint8_t> created from Glib class

		   Mandatory to go through a created cGlibRegMap.
		   If you want to put directly pGlib.getGlibRegMap(), you'll end up with
		   a seg fault error, as it is not putting all the map in mem but only
		   begin() and end().
		 */

		BeBoardRegMap cGlibRegMap = pBoard->getBeBoardRegMap();
		for ( auto const& it : cGlibRegMap )
		{
			cPairReg.first = it.first;
			cPairReg.second = it.second;
			cVecReg.push_back( cPairReg );
		}

		WriteStackReg( cVecReg );

		cVecReg.clear();

		cPairReg.first = SPURIOUS_FRAME;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = FORCE_BG0_START;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = CBC_TRIGGER_1SHOT;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		// cPairReg.first = BREAK_TRIGGER;
		// cPairReg.second = 1;
		cVecReg.push_back( cPairReg );

		WriteStackReg( cVecReg );

		cVecReg.clear();


		cPairReg.first = PC_CONFIG_OK;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM1_END_READOUT;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM2_END_READOUT;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM1_USR_LOGIC;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );
		cPairReg.first = SRAM2_USR_LOGIC;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );

		WriteStackReg( cVecReg );

		cVecReg.clear();

		std::this_thread::sleep_for( cPause );

		cPairReg.first = SPURIOUS_FRAME;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = FORCE_BG0_START;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = CBC_TRIGGER_1SHOT;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		// cPairReg.first = BREAK_TRIGGER;
		// cPairReg.second = 1;
		cVecReg.push_back( cPairReg );

		WriteStackReg( cVecReg );

		cVecReg.clear();

		std::this_thread::sleep_for( cPause * 3 );

	}


	void GlibFWInterface::SelectFEId()
	{
		if ( static_cast<uint32_t>( ReadReg( HYBRID_TYPE ) ) == 8 )
		{
			fCbcStubLat  = ( static_cast<uint32_t>( ReadReg( FMC1_PRESENT ) ) ? CBC_STUB_LATENCY_FE1 : CBC_STUB_LATENCY_FE2 );
			fCbcI2CCmdAck = ( static_cast<uint32_t>( ReadReg( FMC1_PRESENT ) ) ? CBC_I2C_CMD_ACK_FE1 : CBC_I2C_CMD_ACK_FE2 );
			fCbcI2CCmdRq = ( static_cast<uint32_t>( ReadReg( FMC1_PRESENT ) ) ? CBC_I2C_CMD_RQ_FE1 : CBC_I2C_CMD_RQ_FE2 );
			fCbcHardReset = ( static_cast<uint32_t>( ReadReg( FMC1_PRESENT ) ) ? CBC_HARD_RESET_FE1 : CBC_HARD_RESET_FE2 );
			fCbcFastReset = ( static_cast<uint32_t>( ReadReg( FMC1_PRESENT ) ) ? CBC_FAST_RESET_FE1 : CBC_FAST_RESET_FE2 );
		}
		else
		{
			fCbcStubLat  = CBC_STUB_LATENCY;
			fCbcI2CCmdAck =  CBC_I2C_CMD_ACK;
			fCbcI2CCmdRq = CBC_I2C_CMD_RQ;
			fCbcHardReset = CBC_HARD_RESET;
			fCbcFastReset = CBC_FAST_RESET;
		}
	}

	void GlibFWInterface::Start()
	{
		std::vector< std::pair<std::string, uint32_t> > cVecReg;
		std::pair<std::string, uint32_t> cPairReg;

		//Starting the DAQ

		cPairReg.first = BREAK_TRIGGER;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = PC_CONFIG_OK;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );
		cPairReg.first = FORCE_BG0_START;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );

		WriteStackReg( cVecReg );

		cVecReg.clear();

	}

	void GlibFWInterface::Stop( uint32_t pNthAcq )
	{

		std::vector< std::pair<std::string, uint32_t> > cVecReg;
		std::pair<std::string, uint32_t> cPairReg;

		uhal::ValWord<uint32_t> cVal;

		//Select SRAM
		SelectDaqSRAM( pNthAcq );

		//Stop the DAQ
		cPairReg.first = BREAK_TRIGGER;
		cPairReg.second = 1;
		cVecReg.push_back( cPairReg );
		cPairReg.first = PC_CONFIG_OK;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );
		cPairReg.first = FORCE_BG0_START;
		cPairReg.second = 0;
		cVecReg.push_back( cPairReg );

		WriteStackReg( cVecReg );
		cVecReg.clear();

		std::chrono::milliseconds cWait( 100 );

		//Wait for the selected SRAM to be full then empty it
		do
		{
			cVal = ReadReg( fStrFull );

			if ( cVal == 1 )
				std::this_thread::sleep_for( cWait );

		}
		while ( cVal == 1 );

		WriteReg( fStrReadout, 0 );
		fNTotalAcq++;
	}


	void GlibFWInterface::Pause()
	{

		WriteReg( BREAK_TRIGGER, 1 );

	}


	void GlibFWInterface::Resume()
	{

		WriteReg( BREAK_TRIGGER, 0 );
	}

	void GlibFWInterface::ReadData( BeBoard* pBoard, unsigned int pNthAcq, bool pBreakTrigger )
	{
		//Readout settings
		std::chrono::milliseconds cWait( 1 );

		uhal::ValWord<uint32_t> cVal;

		// Since the Number of  Packets is a FW register, it should be read from the Settings Table which is one less than is actually read
		uint32_t cNPackets = pBoard->getReg( "user_wb_ttc_fmc_regs.pc_commands.CBC_DATA_PACKET_NUMBER" ) + 1 ;


		//use a counting visitor to find out the number of CBCs
		struct CbcCounter : public HwDescriptionVisitor
		{
			uint32_t fNCbc = 0;

			void visit( Cbc& pCbc ) {
				fNCbc++;
			}
			uint32_t getNCbc() {
				if ( fNCbc == 2 )
					// since the 2 CBC FW outputs data for 4 CBCs (beamtest heritage, might have to change in the future)
					return 2 * fNCbc;
				else return fNCbc;
			}
		};

		CbcCounter cCounter;
		pBoard->accept( cCounter );
		// compute the block size according to the number of CBC's on this board
		// this will have to change with a more generic FW
		uint32_t cBlockSize = cNPackets * ( cCounter.getNCbc() * CBC_EVENT_SIZE_32 + EVENT_HEADER_TDC_SIZE_32 ); // in 32 bit words

		//Wait for start acknowledge
		do
		{
			cVal = ReadReg( CMD_START_VALID );

			if ( cVal == 0 )
				std::this_thread::sleep_for( cWait );

		}
		while ( cVal == 0 );

		//FIFO goes to write_data state
		//Select SRAM
		SelectDaqSRAM( pNthAcq );

		//Wait for the SRAM full condition.
		cVal = ReadReg( fStrFull );

		do
		{
			std::this_thread::sleep_for( cWait );

			cVal = ReadReg( fStrFull );

		}
		while ( cVal == 0 );

		//break trigger
		if ( pBreakTrigger )
			WriteReg( BREAK_TRIGGER, 1 );

		//Set read mode to SRAM
		WriteReg( fStrSramUserLogic, 0 );

		//Read SRAM
		uhal::ValVector<uint32_t> cData = ReadBlockReg( fStrSram, cBlockSize );

		// To avoid the IPBUS bug
		// need to convert uHal::ValVector to vector<uint32_t> so we can replace the 256th word
		std::vector<uint32_t> cDataAlt = cData.value();
		if ( cBlockSize > 255 )
		{
			std::string fSram_256 = fStrSram + "_256";
			uhal::ValWord<uint32_t> cWord = ReadReg( fSram_256 );
			cDataAlt[255] = cWord.value();
		}


		WriteReg( fStrSramUserLogic, 1 );
		WriteReg( fStrReadout, 1 );

		//Wait for the non SRAM full condition starts,
		do
		{
			cVal = ReadReg( fStrFull );

			if ( cVal == 1 )
				std::this_thread::sleep_for( cWait );

		}
		while ( cVal == 1 );

		//Wait for the non SRAM full condition ends.

		WriteReg( fStrReadout, 0 );

		// just creates a new Data object, setting the pointers and getting the correct sizes happens in Set()
		if ( fData ) delete fData;
		fData = new Data();

		// set the vector<uint32_t> as event buffer and let him know how many packets it contains
		fData->Set( &cDataAlt , cNPackets );
	}

	void GlibFWInterface::SelectDaqSRAM( uint32_t pNthAcq )
	{
		fStrSram  = ( ( pNthAcq % 2 + 1 ) == 1 ? SRAM1 : SRAM2 );
		fStrSramUserLogic = ( ( pNthAcq % 2 + 1 ) == 1 ? SRAM1_USR_LOGIC : SRAM2_USR_LOGIC );
		fStrFull = ( ( pNthAcq % 2 + 1 ) == 1 ? SRAM1_FULL : SRAM2_FULL );
		fStrReadout = ( ( pNthAcq % 2 + 1 ) == 1 ? SRAM1_END_READOUT : SRAM2_END_READOUT );
	}


	const Event* GlibFWInterface::GetNextEvent( const BeBoard* pBoard )
	{
		return fData->GetNextEvent( pBoard );
	}


	const char* GlibFWInterface::GetBuffer( uint32_t& pBufSize ) const
	{
		return fData->GetBuffer( pBufSize );
	}


	//Methods for Cbc's:

	void GlibFWInterface::SelectFeSRAM( uint32_t pFe )
	{
		pFe = 0;
		fStrSram = ( pFe ? SRAM2 : SRAM1 );
		fStrOtherSram = ( pFe ? SRAM1 : SRAM2 );
		fStrSramUserLogic = ( pFe ? SRAM2_USR_LOGIC : SRAM1_USR_LOGIC );
		fStrOtherSramUserLogic = ( pFe ? SRAM2_USR_LOGIC : SRAM1_USR_LOGIC );
	}

	bool GlibFWInterface::I2cCmdAckWait( uint32_t pAckVal, uint8_t pNcount )
	{
		unsigned int cWait( 100 );

		if ( pAckVal )
			cWait = pNcount * 500;


		usleep( cWait );

		uhal::ValWord<uint32_t> cVal;
		uint32_t cLoop = 0;

		do
		{
			cVal = ReadReg( CBC_I2C_CMD_ACK );

			if ( cVal != pAckVal )
			{
				// std::cout << "Waiting for the I2c command acknowledge to be " << pAckVal << " for " << pNcount << " registers." << std::endl;
				usleep( cWait );
			}

		}
		while ( cVal != pAckVal && ++cLoop < MAX_NB_LOOP );

		if ( cLoop >= MAX_NB_LOOP )
		{
			std::cout << "Warning: time out in I2C acknowledge loop (" << pAckVal << ")" << std::endl;
			return false;
		}

		return true;
	}

	void GlibFWInterface::SendBlockCbcI2cRequest( std::vector<uint32_t>& pVecReq, bool pWrite )
	{
		WriteReg( fStrSramUserLogic, 1 );

		pVecReq.push_back( 0xFFFFFFFF );

		WriteReg( fStrSramUserLogic, 0 );

		WriteBlockReg( fStrSram, pVecReq );
		WriteReg( fStrOtherSram, 0xFFFFFFFF );

		WriteReg( fStrSramUserLogic, 1 );

		WriteReg( CBC_HARD_RESET, 0 );

		//r/w request
		WriteReg( CBC_I2C_CMD_RQ, pWrite ? 3 : 1 );
		// WriteReg( CBC_I2C_CMD_RQ, 1 );

		pVecReq.pop_back();

		if ( I2cCmdAckWait( ( uint32_t )1, pVecReq.size() ) == 0 )
			throw Exception( Form( "%s: I2cCmdAckWait %d failed.", "CbcInterface", 1 ) );

		WriteReg( CBC_I2C_CMD_RQ, 0 );

		if ( I2cCmdAckWait( ( uint32_t )0, pVecReq.size() ) == 0 )
			throw Exception( Form( "%s: I2cCmdAckWait %d failed.", "CbcInterface", 0 ) );

	}

	void GlibFWInterface::ReadI2cBlockValuesInSRAM( std::vector<uint32_t>& pVecReq )
	{

		WriteReg( fStrSramUserLogic, 0 );

		uhal::ValVector<uint32_t> cData = ReadBlockReg( fStrSram, pVecReq.size() );
		uhal::ValWord<uint32_t> cWord;
		// To avoid the IPBUS bug
		//  replace the 256th word
		if ( pVecReq.size() > 255 )
		{
			std::string fSram_256 = fStrSram + "_256";
			cWord = ReadReg( fSram_256 );
			std::cout << "WARNING: Reading more than 255 32-bit words from SRAM, thus need to avoid the uHAL-GLIB bug!" << std::endl;
		}
		WriteReg( fStrSramUserLogic, 1 );
		WriteReg( CBC_I2C_CMD_RQ, 0 );

		std::vector<uint32_t>::iterator it = pVecReq.begin();
		uhal::ValVector< uint32_t >::const_iterator itValue = cData.begin();

		while ( it != pVecReq.end() )
		{
			*it = *itValue;
			it++;
			itValue++;
		}
		// To avoid the IPBUS bug
		//  replace the 256th word
		if ( pVecReq.size() > 255 )
		{
			pVecReq.at( 255 ) = cWord.value();
			// std::cout << "256th ReadbackValue " <<  std::bitset<32>( pVecReq.at( 255 ) ) << " - 2nd read value " <<  std::bitset<32> ( cWord.value() )  << std::endl;
		}

	}


	void GlibFWInterface::EnableI2c( bool pEnable )
	{
		uint32_t cValue = I2C_CTRL_ENABLE;

		if ( !pEnable )
			cValue = I2C_CTRL_DISABLE;

		WriteReg( I2C_SETTINGS, cValue );

		if ( pEnable )
			usleep( 100000 );
	}

	void GlibFWInterface::WriteCbcBlockReg( uint8_t pFeId, std::vector<uint32_t>& pVecReq )
	{
		SelectFeSRAM( pFeId );
		EnableI2c( 1 );

		try
		{
			SendBlockCbcI2cRequest( pVecReq, true );
		}

		catch ( Exception& except )
		{
			throw except;
		}

		EnableI2c( 0 );
	}

	void GlibFWInterface::ReadCbcBlockReg( uint8_t pFeId, std::vector<uint32_t>& pVecReq )
	{
		SelectFeSRAM( pFeId );
		EnableI2c( 1 );

		try
		{
			SendBlockCbcI2cRequest( pVecReq, false );
		}

		catch ( Exception& e )
		{
			throw e;
		}

		ReadI2cBlockValuesInSRAM( pVecReq );

		EnableI2c( 0 );
	}
}
