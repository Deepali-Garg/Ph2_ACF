
/*!

        \file                           ICICGlibFWInterface.h
        \brief                          ICICGlibFWInterface init/config of the Glib and its Cbc's
        \author                         G. Auzinger, K. Uchida
        \version            1.0
        \date                           25.02.2016
        Support :                       mail to : georg.auzinger@SPAMNOT.cern.ch

 */

#ifndef __ICGLIBFWINTERFACE_H__
#define __ICGLIBFWINTERFACE_H__

#include <string>
#include <map>
#include <vector>
#include <limits.h>
#include <stdint.h>
#include "BeBoardFWInterface.h"
#include "../HWDescription/Module.h"
#include "../Utils/Visitor.h"
//#include "CbcI2cReply.h"

using namespace Ph2_HwDescription;

/*!
 * \namespace Ph2_HwInterface
 * \brief Namespace regrouping all the interfaces to the hardware
 */
namespace Ph2_HwInterface
{
class FpgaConfig;
/*!
 * \class ICGlibFWInterface
 * \brief init/config of the Glib and its Cbc's
 */
class ICGlibFWInterface : public BeBoardFWInterface
{

private:
    Data* fData; /*!< Data read storage*/

    struct timeval fStartVeto;
    std::string fStrSram, fStrSramUserLogic, fStrFull, fStrReadout, fStrOtherSram, fStrOtherSramUserLogic;
    std::string fCbcStubLat, fCbcI2CCmdAck, fCbcI2CCmdRq, fCbcHardReset, fCbcFastReset;
    FpgaConfig* fpgaConfig;
    FileHandler* fFileHandler ;
    uint32_t fBroadcastCbcId;
    uint32_t fReplyBufferSize;

    const uint32_t SINGLE_I2C_WAIT = 70; //usec for 1MHz I2C
//  const uint32_t SINGLE_I2C_WAIT = 700; //usec for 100 kHz I2C
    const unsigned CBCFMC_ID = 2;

//public:
    //static uint32_t EncodeCbcI2cCommand( uint8_t pCbcId, const CbcRegItem& pRegItem, bool r = true, bool w = true ); [>!< Encode I2C commands for GLIB interface<]

private:

    /*!
     *  \brief I2C command pools
     */
    //std::map< unsigned, std::vector<uint32_t> > fI2cWriteReadCommandList;
    //std::map< unsigned, std::vector<uint32_t> > fI2cWriteCommandList;
    //std::map< unsigned, std::vector<uint32_t> > fI2cReadCommandList;

public:
    /*!
     * \brief Constructor of the ICGlibFWInterface class
     * \param puHalConfigFileName : path of the uHal Config File
     * \param pBoardId
     */
    ICGlibFWInterface( const char* puHalConfigFileName, uint32_t pBoardId );
    ICGlibFWInterface( const char* puHalConfigFileName, uint32_t pBoardId, FileHandler* pFileHandler );
    /*!
    * \brief Constructor of the ICGlibFWInterface class
    * \param pId : ID string
    * \param pUri: URI string
    * \param pAddressTable: address tabel string
    */
    ICGlibFWInterface( const char* pId, const char* pUri, const char* pAddressTable );
    ICGlibFWInterface( const char* pId, const char* pUri, const char* pAddressTable, FileHandler* pFileHandler );

    /*!
     * \brief Destructor of the ICGlibFWInterface class
     */
    ~ICGlibFWInterface()
    {
        if (fData) delete fData;
    }

///////////////////////////////////////////////////////
//      Glib Methods                                //
/////////////////////////////////////////////////////

    /*! \brief Read a block of a given size
     * \param pRegNode Param Node name
     * \param pBlocksize Number of 32-bit words to read
     * \return Vector of validated 32-bit values
     */
    std::vector<uint32_t> ReadBlockRegValue( const std::string& pRegNode, const uint32_t& pBlocksize ) override;

    bool WriteBlockReg( const std::string& pRegNode, const std::vector< uint32_t >& pValues ) override;
    /*!
     * \brief Get the FW info
     */
    void getBoardInfo();
    /*!
     * \brief Configure the board with its Config File
     * \param pBoard
     */
    void ConfigureBoard( const BeBoard* pBoard ) override;
    /*!
     * \brief Detect the right FE Id to write the right registers (not working with the latest Firmware)
     */
    void SelectFEId();
    /*!
     * \brief Start a DAQ
     */
    void Start() override;
    /*!
     * \brief Stop a DAQ
     * \param pNthAcq : actual number of acquisitions
     */
    void Stop( uint32_t pNthAcq ) override;
    /*!
     * \brief Pause a DAQ
     */
    void Pause() override;
    /*!
     * \brief Unpause a DAQ
     */
    void Resume() override;
    /*!
     * \brief Read data from DAQ
     * \param pNthAcq : actual number of acquisitions
     * \param pBreakTrigger : if true, enable the break trigger
     * \return cNPackets: the number of packets read
     */
    uint32_t ReadData( BeBoard* pBoard, uint32_t pNthAcq, bool pBreakTrigger ) override;
    /*!
     * \brief Get next event from data buffer
     * \return Next event
     */
    const Event* GetNextEvent( const BeBoard* pBoard ) const override
    {
        return fData->GetNextEvent( pBoard );
    }
    const Event* GetEvent( const BeBoard* pBoard, int i ) const override
    {
        return fData->GetEvent( pBoard, i );
    }
    const std::vector<Event*>& GetEvents( const BeBoard* pBoard ) const override
    {
        return fData->GetEvents( pBoard );
    }

    void StartThread(BeBoard* pBoard, uint32_t uNbAcq, HwInterfaceVisitor* visitor) override;
    void threadAcquisitionLoop(BeBoard* pBoard, HwInterfaceVisitor* visitor);
private:

    //I2C command sending implementation
    bool WriteI2C( unsigned pFeId, std::vector<uint32_t>& pVecSend, std::vector<uint32_t>& pReplies, bool pWriteRead, bool pBroadcast );
    bool ReadI2C( uint8_t pFeId, uint32_t pNReplies, std::vector<uint32_t>& pReplies);



public:
///////////////////////////////////////////////////////
//      CBC Methods                                 //
/////////////////////////////////////////////////////

    //Encode/Decode Cbc values
    /*!
    * \brief Encode a/several word(s) readable for a Cbc
    * \param pRegItem : RegItem containing infos (name, adress, value...) about the register to write
    * \param pCbcId : Id of the Cbc to work with
    * \param pVecReq : Vector to stack the encoded words
    */
    void EncodeReg( const CbcRegItem& pRegItem, uint8_t pCbcId, std::vector<uint32_t>& pVecReq, bool pRead, bool pWrite ); /*!< Encode a/several word(s) readable for a Cbc*/

    //I2C command sending and getting the reply for r/w the Cbc registers
    /*! \brief add I2C command to the command list
     * \param pCbc
     * \param pRegItem
     * \param r true for read
     * \param w true for write
     */
    void SetCbcI2cCommand( const CbcRegItem &pRegItem, bool r, bool w );
    void SetCbcI2cCommand( const Cbc* pCbc, const CbcRegItem &pRegItem, bool r, bool w );
    bool SendCbcI2cCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies );
    bool SendCbcI2cWriteCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies );
    bool SendCbcI2cWriteReadCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies );
    bool SendCbcI2cReadCommandsAndReadReplies( unsigned pFeId, std::vector<CbcI2cReply> &pReplies );

///////////////////////////////////////////////////////
//      FPGA CONFIG                                 //
/////////////////////////////////////////////////////

    /*! \brief Upload a firmware (FPGA configuration) from a file in MCS format into a given configuration
     * \param numConfig FPGA configuration number (1 or 2)
     * \param pstrFile path to MCS file
     */
    void FlashProm( const std::string& strConfig, const char* pstrFile );
    /*! \brief Jump to an FPGA configuration */
    void JumpToFpgaConfig( const std::string& strConfig);
    /*! \brief Is the FPGA being configured ?
     * \return FPGA configuring process or NULL if configuration occurs */
    const FpgaConfig* getConfiguringFpga()
    {
        return fpgaConfig;
    }

};
}

#endif
