/*!

        \file                           ICICICGlibFWInterface.h
        \brief                          ICICICGlibFWInterface init/config of the Glib and its Cbc's
        \author                         G. Auzinger, K. Uchida
        \version            1.0
        \date                           25.02.2016
        Support :                       mail to : georg.auzinger@SPAMNOT.cern.ch

 */

#include <time.h>
#include <chrono>
#include <uhal/uhal.hpp>
#include "ICGlibFWInterface.h"
#include "GlibFpgaConfig.h"
//#include "CbcInterface.h"

namespace Ph2_HwInterface
{

ICGlibFWInterface::ICGlibFWInterface( const char* puHalConfigFileName, uint32_t pBoardId ) :
    BeBoardFWInterface( puHalConfigFileName, pBoardId ),
    fpgaConfig(nullptr),
    fData( nullptr ),
    fBroadcastCbcId(8),
    fReplyBufferSize(1024)
{}


ICGlibFWInterface::ICGlibFWInterface( const char* puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler ) :
    BeBoardFWInterface( puHalConfigFileName, pBoardId ),
    fpgaConfig(nullptr),
    fData( nullptr ),
    fBroadcastCbcId(8),
    fReplyBufferSize(1024),
    fFileHandler( pFileHandler )
{
    if ( fFileHandler == nullptr ) fSaveToFile = false;
    else fSaveToFile = true;
}

ICGlibFWInterface::ICGlibFWInterface( const char* pId, const char* pUri, const char* pAddressTable ) :
    BeBoardFWInterface( pId, pUri, pAddressTable ),
    fpgaConfig( nullptr ),
    fData( nullptr ),
    fBroadcastCbcId(8),
    fReplyBufferSize(1024)
{}


ICGlibFWInterface::ICGlibFWInterface( const char* pId, const char* pUri, const char* pAddressTable, FileHandler* pFileHandler ) :
    BeBoardFWInterface( pId, pUri, pAddressTable ),
    fpgaConfig( nullptr ),
    fData( nullptr ),
    fBroadcastCbcId(8),
    fReplyBufferSize(1024),
    fFileHandler( pFileHandler )
{
    if ( fFileHandler == nullptr ) fSaveToFile = false;
    else fSaveToFile = true;
}

void GlibFWInterface::getBoardInfo()
{
    std::cout << "FMC1 present : " << ReadReg( "user_stat.current_fec_fmc2_cbc0" ) << std::endl;
    std::cout << "FMC2 present : " << ReadReg( "user_stat.current_fec_fmc2_cbc1" ) << std::endl;
    std::cout << "FW version : " << ReadReg( "user_stat.version.ver_major" ) << "." << ReadReg( "user_stat.version.ver_minor" ) << "." << ReadReg( "user_stat.version.ver_build" ) << std::endl;

    uhal::ValWord<uint32_t> cBoardType = ReadReg( "sys_regs.board_id" );

    std::cout << "BoardType : ";

    char cChar = ( ( cBoardType & cMask4 ) >> 24 );
    std::cout << cChar;

    cChar = ( ( cBoardType & cMask3 ) >> 16 );
    std::cout << cChar;

    cChar = ( ( cBoardType & cMask2 ) >> 8 );
    std::cout << cChar;

    cChar = ( cBoardType & cMask1 );
    std::cout << cChar << std::endl;
}

void ICGlibFWInterface::ConfigureBoard( const BeBoard* pBoard )
{
    //We may here switch in the future with the StackReg method of the RegManager
    //when the timeout thing will be implemented in a transparent and pretty way

    std::vector< std::pair<std::string, uint32_t> > cVecReg;

    BeBoardRegMap cGlibRegMap = pBoard->getBeBoardRegMap();
    for ( auto const& it : cGlibRegMap )
    {
        cVecReg.push_back( {it.first, it.second} );
    }

    WriteStackReg( cVecReg );
    cVecReg.clear();
}


void ICGlibFWInterface::Start()
{
    //implement something sane form Kirika's repo: daq.cc
}

void ICGlibFWInterface::Stop( uint32_t pNthAcq )
{
    //implement something sane form Kirika's repo: daq.cc
}


void ICGlibFWInterface::Pause()
{
    //implement something sane form Kirika's repo: daq.cc
    //  WriteReg( BREAK_TRIGGER, 1 );
}


void ICGlibFWInterface::Resume()
{
    //implement something sane form Kirika's repo: daq.cc
    //  WriteReg( BREAK_TRIGGER, 0 );
}

uint32_t ICGlibFWInterface::ReadData( BeBoard* pBoard, unsigned int pNthAcq, bool pBreakTrigger )
{
    //implement something sane form Kirika's repo: daq.cc
    return cNPackets;
}

std::vector<uint32_t> ICGlibFWInterface::ReadBlockRegValue( const std::string& pRegNode, const uint32_t& pBlocksize )
{
    uhal::ValVector<uint32_t> valBlock = ReadBlockReg( pRegNode, pBlocksize );
    std::vector<uint32_t> vBlock = valBlock.value();

    // To avoid the IPBUS bug
    // need to convert uHal::ValVector to vector<uint32_t> so we can replace the 256th word
    if ( pBlocksize > 255 )
    {
        std::string fSram_256 = pRegNode + "_256";
        uhal::ValWord<uint32_t> cWord = ReadReg( fSram_256 );
        vBlock[255] = cWord.value();
    }
    return vBlock;
}

bool ICGlibFWInterface::WriteBlockReg( const std::string& pRegNode, const std::vector< uint32_t >& pValues )
{
    bool cWriteCorr = RegManager::WriteBlockReg( pRegNode, pValues );

    if ( pValues.size() > 255 )
        WriteReg( pRegNode + "_256", pValues[255] );
    return cWriteCorr;
}

void ICGlibFWInterface::StartThread( BeBoard* pBoard, uint32_t uNbAcq, HwInterfaceVisitor* visitor )
{
    if ( runningAcquisition ) return;

    runningAcquisition = true;
    numAcq = 0;
    nbMaxAcq = uNbAcq;

    thrAcq = boost::thread( &Ph2_HwInterface::ICGlibFWInterface::threadAcquisitionLoop, this, pBoard, visitor );
}

void ICGlibFWInterface::threadAcquisitionLoop( BeBoard* pBoard, HwInterfaceVisitor* visitor )
{
    Start( );
//      cBlockSize = computeBlockSize( pBoard );
    cBlockSize = 0;
    while ( runningAcquisition && ( nbMaxAcq == 0 || numAcq < nbMaxAcq ) )
    {
        ReadData( nullptr, numAcq, true );
        for ( const Ph2_HwInterface::Event* cEvent = GetNextEvent( pBoard ); cEvent; cEvent = GetNextEvent( pBoard ) )
            visitor->visit( *cEvent );

        if ( runningAcquisition )
            numAcq++;

    }
    Stop( numAcq );
    runningAcquisition = false;
};

///////////////////////////////////////////////////////
//      CBC Methods                                 //
/////////////////////////////////////////////////////

// this is clearly for addressing individual CBCs, have to see how to deal with broadcast commands
void ICGlibFWInterface::EncodeReg( const CbcRegItem& pRegItem, uint8_t pCbcId, std::vector<uint32_t>& pVecReq, bool pRead, bool pWrite )
{
    //use fBroadcastCBCId for broadcast commands
    uint8_t cRW =  ( ( pRead ? 1 : 0 ) << 1 ) + ( pWrite ? 1 : 0 );
    pVecReq.push_back( ( CBCFMC_ID << 28 ) | ( pCbcId << 24 ) | ( cRW << 20 ) | ( pRegItem.fPage << 16 ) | ( pRegItem.fAddress << 8 ) | pRegItem.fValue );
}

void ICGlibFWInterface::DecodeReg( CbcRegItem& pRegItem, uint8_t& pCbcId, uint32_t pWord, bool& pRead, bool& pFailed )
{
    pCbcId   =  ( pWord & 0x07000000 ) >> 24;
    pFailed  =  ( pWord & 0x00100000 ) >> 20;
    pRead    =  ( pWord & 0x00020000 ) >> 17;
    pRegItem.fPage    =  ( pWord & 0x00010000 ) >> 16;
    pRegItem.fAddress =  ( pWord & 0x0000FF00 ) >> 8;
    pRegItem.fValue   =  ( pWord & 0x000000FF );
}

bool ICGlibFWInterface::ReadI2C( uint8_t pFeId, uint32_t pNReplies, std::vector<uint32_t>& pReplies)
{
    usleep(SINGLE_I2C_WAIT * pNReplies );

    uhal::ValVector<uint32_t> cReplies;
    bool cFailed(false);

    char tmp[256];
    sprintf( tmp, "cbc_daq_ctrl.i2c_reply_fifo_fmc%d_status.nwdata", pFeId );
    std::string cNode(tmp);
    cReplies = ReadBlockRegValue( cNode, pNReplies );
    //here i create a dummy reg item for decoding so I can find if 1 cFailed
    CbcRegItem cItem;
    uint8_t cCbcId;
    bool cRead;
    for (auto& cWord : cReplies)
    {
        DecodeReg(cItem, cCbcId, cWord, cRead, cFailed );
    }
    return cFailed;
}

bool ICGlibFWInterface::WriteI2C( unsigned pFeId, std::vector<uint32_t>& pVecSend, std::vector<uint32_t>& pReplies, bool pReadback, bool pBroadcast )
{
    //This one's recursive, beware!
    // figure out how to best determine if this is boradcast or not? (decode the 1st word of the Vector and decide based on the CBC address?)
    //cN is the size of the vector to send
    uint32_t cN( pVecSend.size() );
    //cM is the replybuffersize (1024) / 2 if i write & read, else divided by 1 / broadcastCbcId(8) or 1
    uint32_t cM( fReplyBufferSize / ( pReadback ? 2 : 1 ) / ( pBroadcast ? fBroadcastCbcId  : 1 ) );
    //division of cN/cM and remainder
    uint32_t cDivNM( cN / cM );
    uint32_t cRemNM( cN % cM );
    bool cFailed( false );

    if ( ( cDivNM == 1 && cRemNM == 0 ) || ( cDivNM == 0 && cRemNM != 0 ) )
    {
        try
        {
            WriteBlockReg( "cbc_i2c_command", pVecSend );
        }
        catch ( Exception& except )
        {
            throw except;
        }
        uint32_t cNReplies = pVecSend.size() * ( pReadback ? 2 : 1 ) * ( pBroadcast ? fBroadcastCbcId : 1 );
        if ( ReadI2C( pFeId, cNReplies, pReplies) ) cFailed = true;
    }
    else
    {
        for ( uint32_t cIndex = 0; cIndex < cDivNM; cIndex++ )
        {
            std::vector<uint32_t> cCommandBlock( pVecSend.begin() + cIndex * cM, pVecSend.begin() + ( cIndex + 1 ) * cM );
            if ( WriteI2C( pFeId, cCommandBlock, pReplies, pReadback, pBroadcast ) ) cFailed = true;
        }
        if ( cRemNM )
        {
            std::vector<uint32_t> cCommandBlock( pVecSend.begin() + cDivNM * cM, pVecSend.end() );
            if ( WriteI2C( pFeId, cCommandBlock, pReplies, pReadback, pBroadcast ) ) cFailed = true;
        }
    }
    return cFailed;
}



void WriteCbcBlockReg(uint8_t pFeId, std::vector<uint32_t>& pVecReg, bool pReadback)
{
    std::vector<uint32_t> cReplies;
    WriteI2C(pFeId, pVecReg, cReplies, pReadback, false );
    pVecReg.clear();
    pVecReg = cReplies;
    // the actual write & readback command is in the vector
    // the ref to pVecReg now contains the reply which is of different format but i should be able to decode it using the decode reg mehtod
    // I have to try to determine the broadcast variable in from the data by looking at the values in the vector
}

void ReadCbcBlockReg( uint8_t pFeId, std::vector<uint32_t>& pVecReg )
{
    std::vector<uint32_t> cReplies;
    //it sounds weird, but ReadI2C is called inside writeI2c, therefore here I have to write and disable the readback. The actual read command is in the words of the vector, no broadcast, maybe I can get rid of it
    WriteI2C(pFeID, pVecReg, cReplies, false, false);
    pVecReg.clear();
    pVecReg = cReplies;
}

void ICGlibFWInterface::FlashProm( const std::string& strConfig, const char* pstrFile )
{
    if ( fpgaConfig && fpgaConfig->getUploadingFpga() > 0 )
        throw Exception( "This board is already uploading an FPGA configuration" );

    if ( !fpgaConfig )
        fpgaConfig = new GlibFpgaConfig( this );

    fpgaConfig->runUpload( strConfig, pstrFile );
}

void ICGlibFWInterface::JumpToFpgaConfig( const std::string& strConfig )
{
    if ( fpgaConfig && fpgaConfig->getUploadingFpga() > 0 )
        throw Exception( "This board is uploading an FPGA configuration" );

    if ( !fpgaConfig )
        fpgaConfig = new GlibFpgaConfig( this );

    fpgaConfig->jumpToImage( strConfig );
}

}
// the next two can be wrapped into something nicer, also in my GlibFWInterface in order to simplify BeBoardInterface

//void ICGlibFWInterface::SetCbcI2cCommand( const CbcRegItem &pRegItem, bool r, bool w )
//{
//if ( r == false && w == true )
//{
//for ( auto &it : fI2cWriteCommandList )
//it.second.push_back( EncodeCbcI2cCommand( fBroadcastCbcId, pRegItem, r, w ) );
//}
//else if ( r == true && w == true )
//{
//for ( auto &it : fI2cWriteReadCommandList )
//it.second.push_back( EncodeCbcI2cCommand( fBroadcastCbcId, pRegItem, r, w ) );
//}
//else if ( r == true && w == false )
//{
//for ( auto &it : fI2cReadCommandList )
//it.second.push_back( EncodeCbcI2cCommand( fBroadcastCbcId, pRegItem, r, w ) );
//}
//}

//void ICGlibFWInterface::SetCbcI2cCommand( const Cbc* pCbc, const CbcRegItem &pRegItem, bool r, bool w )
//{
//if ( r == false && w == true )
//fI2cWriteCommandList[pCbc->getFeId()].push_back( EncodeCbcI2cCommand( pCbc->getCbcId(), pRegItem, r, w ) );
//else if ( r == true && w == true )
//fI2cWriteReadCommandList[pCbc->getFeId()].push_back( EncodeCbcI2cCommand( pCbc->getCbcId(), pRegItem, r, w ) );
//else if ( r == true && w == false )
//fI2cReadCommandList[pCbc->getFeId()].push_back( EncodeCbcI2cCommand( pCbc->getCbcId(), pRegItem, r, w ) );
//}


//This is really really cumbersome, she is using only the 1st method below which goes through all three containers for commands and sends whatever is in there.
//thus when she uses the wrapper to this function below, it depends on how she actually calls the SetCbcI2cCommand definde and commented above - the respective write & read flags determine in which container the 32bit words end up and it is encoded in the 32 bit word! So I could do something smarter, just create a WriteCbcBlockRead and a ReadCbcBlockReg and then decide ad hoc in the interface
//bool ICGlibFWInterface::SendCbcI2cCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies )
//{
//SendCbcI2cWriteCommandsAndReadReplies( pFeId, pReplies );
//SendCbcI2cWriteReadCommandsAndReadReplies( pFeId, pReplies );
//SendCbcI2cReadCommandsAndReadReplies( pFeId, pReplies );
//}
//bool ICGlibFWInterface::SendCbcI2cWriteCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies )
//{
//sendCbcI2cCommandsAndReadReplies( pFeId, fI2cWriteCommandList[pFeId], pReplies, false, false );
//}
//bool ICGlibFWInterface::SendCbcI2cWriteReadCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies )
//{
//sendCbcI2cCommandsAndReadReplies( pFeId, fI2cWriteReadCommandList[pFeId], pReplies, true, false );
//}
//bool ICGlibFWInterface::SendCbcI2cReadCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies )
//{
//sendCbcI2cCommandsAndReadReplies( pFeId, fI2cReadCommandList[pFeId], pReplies, false, false );
//}
