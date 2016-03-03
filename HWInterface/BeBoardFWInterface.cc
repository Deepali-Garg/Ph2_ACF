/*

        FileName :                    BeBoardFWInterface.cc
        Content :                     BeBoardFWInterface base class of all type of boards
        Programmer :                  Lorenzo BIDEGAIN, Nicolas PIERRE
        Version :                     1.0
        Date of creation :            31/07/14
        Support :                     mail to : lorenzo.bidegain@gmail.com, nico.pierre@icloud.com

 */



#include "BeBoardFWInterface.h"
#include "FpgaConfig.h"

namespace Ph2_HwInterface {

    //Constructor, makes the board map
    BeBoardFWInterface::BeBoardFWInterface ( const char* puHalConfigFileName, uint32_t pBoardId ) :
        RegManager ( puHalConfigFileName, pBoardId ),
        fNTotalAcq ( 0 ),
        runningAcquisition ( false ),
        numAcq ( 0 ),
        fSaveToFile ( false ),
        fFileHandler ( nullptr )
    {
    }

    //Constructor, makes the board map
    BeBoardFWInterface::BeBoardFWInterface ( const char* pId, const char* pUri, const char* pAddressTable ) :
        RegManager ( pId, pUri, pAddressTable ),
        fNTotalAcq ( 0 ),
        runningAcquisition ( false ),
        numAcq ( 0 ),
        fSaveToFile ( false ),
        fFileHandler ( nullptr )
    {
    }

    std::string BeBoardFWInterface::getBoardType()
    {
        std::string cBoardTypeString;

        uhal::ValWord<uint32_t> cBoardType = ReadReg ( "board_id" );

        char cChar = ( ( cBoardType & cMask4 ) >> 24 );
        cBoardTypeString.push_back ( cChar );

        cChar = ( ( cBoardType & cMask3 ) >> 16 );
        cBoardTypeString.push_back ( cChar );

        cChar = ( ( cBoardType & cMask2 ) >> 8 );
        cBoardTypeString.push_back ( cChar );

        cChar = ( cBoardType & cMask1 );
        cBoardTypeString.push_back ( cChar );

        return cBoardTypeString;

    }

    //void BeBoardFWInterface::getBoardInfo()
    //{
    //std::cout << "FMC1 present : " << ReadReg( "status.fmc1_present" ) << std::endl;
    //std::cout << "FMC2 present : " << ReadReg( "status.fmc2_present" ) << std::endl;
    //std::cout << "FW version : " << ReadReg( "firm_id.firmware_major" ) << "." << ReadReg( "firm_id.firmware_minor" ) << "." << ReadReg( "firm_id.firmware_build" ) << std::endl;

    //uhal::ValWord<uint32_t> cBoardType = ReadReg( "board_id" );

    //std::cout << "BoardType : ";

    //char cChar = ( ( cBoardType & cMask4 ) >> 24 );
    //std::cout << cChar;

    //cChar = ( ( cBoardType & cMask3 ) >> 16 );
    //std::cout << cChar;

    //cChar = ( ( cBoardType & cMask2 ) >> 8 );
    //std::cout << cChar;

    //cChar = ( cBoardType & cMask1 );
    //std::cout << cChar << std::endl;

    //std::cout << "FMC User Board ID : " << ReadReg( "user_wb_ttc_fmc_regs.user_board_id" ) << std::endl;
    //std::cout << "FMC User System ID : " << ReadReg( "user_wb_ttc_fmc_regs.user_sys_id" ) << std::endl;
    //std::cout << "FMC User Version : " << ReadReg( "user_wb_ttc_fmc_regs.user_version" ) << std::endl;

    //}


    void BeBoardFWInterface::StopThread()
    {
        if ( runningAcquisition )
        {
            runningAcquisition = false;

            try
            {
                thrAcq.join();
            }
            catch ( std::exception& e )
            {
                std::cerr << "Death to Stop in BeBoardFWInterface::StopThread()" << e.what() << std::endl;
            }
            catch ( ... )
            {
                std::cerr << "Death to Stop in BeBoardFWInterface::StopThread(). failed to perform thrAcq.join()" << std::endl;
            }
        }

    }

    int BeBoardFWInterface::getNumAcqThread()
    {
        return ( int ) numAcq;
    }
}
