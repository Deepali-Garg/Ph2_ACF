/*

        FileName :                     Event.cc
        Content :                      Event handling from DAQ
        Programmer :                   Nicolas PIERRE
        Version :                      1.0
        Date of creation :             10/07/14
        Support :                      mail to : nicolas.pierre@icloud.com

 */

#include "../Utils/Event.h"

using namespace Ph2_HwDescription;


uint32_t swap_bytes( const std::vector<uint8_t>& list, uint32_t indx )
{
	uint32_t ui32 = list[indx] | ( list[indx + 1] << 8 ) | ( list[indx + 2] << 16 ) | ( list[indx + 3] << 24 );
	return ( ( ui32 >> 24 ) & 0xFF ) |
		   ( ( ui32 >>  8 ) & 0x0000FF00 ) |
		   ( ( ui32 <<  8 ) & 0x00FF0000 ) |
		   ( ( ui32 << 24 ) & 0xFF000000 );
}

namespace Ph2_HwInterface
{

	// Event implementation
	Event::Event( uint32_t pNbCbc, const std::vector<uint8_t>& list )
	{
		SetSize( pNbCbc );
		SetEvent( list );
	}


	Event::Event( const BeBoard* pBoard, uint32_t pNbCbc, const std::vector<uint8_t>& list )
	{
		SetSize( pNbCbc );
		AddBoard( pBoard );
		SetEvent( list );
	}

	Event::Event( const Event& pEvent ) :
		fBunch( pEvent.fBunch ),
		fOrbit( pEvent.fOrbit ),
		fLumi( pEvent.fLumi ),
		fEventCount( pEvent.fEventCount ),
		fEventCountCBC( pEvent.fEventCountCBC ),
		fTDC( pEvent.fTDC ),
		fEventMap( pEvent.fEventMap ),
		ClusterMap( pEvent.ClusterMap)
	{

	}

	void Event::SetSize( uint32_t pNbCbc )
	{
		//  need to introduce a factor of 2 because the 2CBC FW is written for 4 CBCs actually
		int cFactor = ( pNbCbc == 2 ) ? 2 : 1;

		fEventSize = pNbCbc * cFactor * CBC_EVENT_SIZE_CHAR  + EVENT_HEADER_TDC_SIZE_CHAR;
		fOffsetTDC = EVENT_HEADER_SIZE_32 * cFactor + CBC_EVENT_SIZE_32 * pNbCbc * 2; //in 32 bit words

#ifdef __CBCDAQ_DEV__

		std::cout << "DEBUG EVENT SET SIZE: Event size(char) " << fEventSize << " nCBC = "
				  << cFactor* pNbCbc <<  " this should be 168 with 4cbc" << " and Offset TDC " << fOffsetTDC
				  << std::endl;
#endif
	}

	void Event::AddBoard( const BeBoard* pBoard )
	{
		uint32_t cNFe = static_cast<uint32_t>( pBoard->getNFe() );

		for ( uint32_t i = 0; i < cNFe; i++ )
		{
			uint32_t cNCbc;
			// if the NCbcDataSize in the FW Version node of the BeBoard is set, the DataSize is assumed fixed:
			// if 1 module is defined, the number of CBCs is the datasize given in the xml
			// if more than one module is defined, the number of CBCs for each FE is the datasize divided by the nFe
			if ( pBoard->getNCbcDataSize() )
			{
				if ( cNFe == 1 ) cNCbc = static_cast<uint32_t>( pBoard->getNCbcDataSize() );
				else cNCbc = static_cast<uint32_t>( pBoard->getNCbcDataSize() / cNFe );
			}
			// if there is no FWVersion node in the xml, the CBCs will be counted for each module according to the xml file
			else cNCbc = static_cast<uint32_t>( pBoard->getModule( i )->getNCbc() );
			FeEventMap cFeEventMap;

			for ( uint32_t j = 0; j < cNCbc; j++ )
				cFeEventMap[j] = {0, 0};

			fEventMap[i] = cFeEventMap;
		}
	}


	int Event::SetEvent( const std::vector<uint8_t>& list )
	{
		int vsize = sizeof( uint32_t );

		// Debug
		//    for(int i = 0; i< list.size(); i++) {
		//    	std::cout << std::bitset<8>(list.at(i)) << " ";
		// 	if((i+1)%4 == 0 && i != 0) std::cout << std::endl;
		// }

		fBunch = 0x00FFFFFF & swap_bytes( list, 0 );
		fOrbit = 0x00FFFFFF & swap_bytes( list, vsize );
		fLumi = 0x00FFFFFF & swap_bytes( list, 2 * vsize );
		fEventCount = 0x00FFFFFF & swap_bytes( list, 3 * vsize );
		fEventCountCBC = 0x00FFFFFF & swap_bytes( list, 4 * vsize );
		fTDC = 0x000000FF & swap_bytes( list, list.size() - vsize );

		uint32_t event_size = EVENT_HEADER_SIZE_CHAR;

		// uint32_t begin = EVENT_HEADER_SIZE_CHAR;
		uint32_t begin = 0;

		uint32_t end = 0;
		for ( auto& it : fEventMap )
		{
			uint8_t cFeId = static_cast<uint8_t>( it.first );
			uint8_t cNCbc = static_cast<uint8_t>( it.second.size() );
			event_size += cNCbc * CBC_EVENT_SIZE_CHAR;
			for ( auto& jt : it.second )
			{
				uint8_t cCbcId = static_cast<uint8_t>( jt.first );
				// begin += cFeId * CBC_EVENT_SIZE_CHAR * cNCbc + cCbcId * CBC_EVENT_SIZE_CHAR;
				begin = EVENT_HEADER_SIZE_CHAR + cFeId * CBC_EVENT_SIZE_CHAR * cNCbc + cCbcId * CBC_EVENT_SIZE_CHAR;
				end = begin + CBC_EVENT_SIZE_CHAR - 1;
				jt.second = {begin, end};

#ifdef __CBCDAQ_DEV__
				std::cout << "DEBUG FE " << +cFeId << "  with " << +cNCbc << " cbcs on CBC " << +cCbcId
						  << " and the start and end indices of CBC data: {"
						  << begin << ", " << end << "}"
						  << std::endl;
#endif
			}
		}
		fEventData.reserve( event_size + vsize );
		fEventData.assign( list.begin(), list.begin() + event_size );
		for ( uint8_t i = vsize; i > 0; i-- )
			fEventData.push_back( list[list.size() - i] );

		return 0;
	}

	void Event::GetCbcEvent( const uint8_t& pFeId, const uint8_t& pCbcId, std::vector< uint8_t >& cbcData )  const
	{
		cbcData.clear();

		EventMap::const_iterator cIt = fEventMap.find( pFeId );
		if ( cIt != fEventMap.end() )
		{
			FeEventMap::const_iterator cJt = cIt->second.find( pCbcId );
			if ( cJt != cIt->second.end() )
			{
				cbcData.reserve( cJt->second.second - cJt->second.first + 1 );
				cbcData.assign( fEventData.begin() + cJt->second.first, fEventData.begin() + cJt->second.second + 1 );
			}
			else
				std::cout << "Event: CBC " << +pCbcId << " is not found." << std::endl;
		}
		else
			std::cout << "Event: FE " << +pFeId << " is not found." << std::endl;
	}

	std::string Event::HexString() const
	{
		std::stringbuf tmp;
		std::ostream os( &tmp );

		os << std::hex;

		for ( uint32_t i = 0; i < fEventSize; i++ )
			os << std::uppercase << std::setw( 2 ) << std::setfill( '0' ) << ( fEventData[i] & 0xFF );

		os << std::endl;

		return tmp.str();
	}


	bool Event::Bit( uint8_t pFeId, uint8_t pCbcId, uint32_t pPosition ) const
	{
		uint32_t cByteP = pPosition / 8;
		uint32_t cBitP = pPosition % 8;
		std::vector< uint8_t > vTemp;
		GetCbcEvent( pFeId, pCbcId, vTemp );
		if ( cByteP >= vTemp.size() ) return 0;
		//return ( vTemp[cByteP] & ( 1 << ( 7 - cBitP ) ) );
		return ( ( vTemp[cByteP] >> ( 7 - cBitP ) ) & 0x1 );
	}


	bool Event::Error( uint8_t pFeId, uint8_t pCbcId, uint32_t i ) const
	{
		return Bit( pFeId, pCbcId, i + OFFSET_ERROR );
	}

	uint32_t Event::Error( uint8_t pFeId, uint8_t pCbcId ) const
	{

		uint32_t val( 0 );
		uint32_t cWidth = WIDTH_ERROR;

		for ( uint32_t i = 0; i < cWidth; i++ )
		{
			val <<= 1;
			val |= Error( pFeId, pCbcId, i );
		}

		return val;
	}


	uint32_t Event::PipelineAddress( uint8_t pFeId, uint8_t pCbcId ) const
	{
		uint32_t cOffset = OFFSET_PIPELINE_ADDRESS;
		uint32_t cWidth = WIDTH_PIPELINE_ADDRESS;
		uint32_t val( 0 );

		for ( uint32_t i = 0; i < cWidth; i++ )
		{
			val <<= 1;
			val |= Bit( pFeId, pCbcId, cOffset + i );
		}

		return val;
	}

	bool Event::DataBit( uint8_t pFeId, uint8_t pCbcId, uint32_t i ) const
	{
		if ( i > NCHANNELS )
			return 0;

		return Bit( pFeId, pCbcId, i + OFFSET_CBCDATA );
	}


	std::string Event::BitString( uint8_t pFeId, uint8_t pCbcId, uint32_t OFFSET_DATA, uint32_t pWidth ) const
	{
		std::vector<uint8_t> cbcData;
		GetCbcEvent( pFeId, pCbcId, cbcData );

		std::ostringstream os;
		for ( uint32_t i = 0; i < pWidth; ++i )
		{
			uint32_t pos = i + OFFSET_DATA;
			uint32_t cByteP = pos / 8;
			uint32_t cBitP = pos % 8;
			if ( cByteP >= cbcData.size() ) break;
			//os << ((cbcData[cByteP] & ( 1 << ( 7 - cBitP ) ))?"1":"0");
			os << ( ( cbcData[cByteP] >> ( 7 - cBitP ) ) & 0x1 );
		}
		return os.str();
	}
	std::vector<bool> Event::BitVector( uint8_t pFeId, uint8_t pCbcId, uint32_t OFFSET_DATA, uint32_t pWidth ) const
	{
		std::vector< uint8_t > cbcData;
		GetCbcEvent( pFeId, pCbcId, cbcData );

		std::vector<bool> blist;
		for ( uint32_t i = 0; i < pWidth; ++i )
		{
			uint32_t pos = i + OFFSET_DATA;
			uint32_t cByteP = pos / 8;
			uint32_t cBitP = pos % 8;
			if ( cByteP >= cbcData.size() ) break;
			//blist.push_back(cbcData[cByteP] & ( 1 << ( 7 - cBitP ) ));
			blist.push_back( ( cbcData[cByteP] >> ( 7 - cBitP ) ) & 0x1 );
		}
		return blist;
	}
	std::string Event::DataBitString( uint8_t pFeId, uint8_t pCbcId ) const
	{
		return BitString( pFeId, pCbcId, OFFSET_CBCDATA, WIDTH_CBCDATA );
	}


	std::vector<bool> Event::DataBitVector( uint8_t pFeId, uint8_t pCbcId ) const
	{
		return BitVector( pFeId, pCbcId, OFFSET_CBCDATA, WIDTH_CBCDATA );
	}

	std::vector<bool> Event::DataBitVector( uint8_t pFeId, uint8_t pCbcId, const std::vector<uint8_t>& channelList ) const
	{
		uint32_t OFFSET_DATA = OFFSET_CBCDATA;

		std::vector< uint8_t > cbcData;
		GetCbcEvent( pFeId, pCbcId, cbcData );

		std::vector<bool> blist;
		for ( auto i :  channelList )
		{
			uint32_t pos = i + OFFSET_DATA;
			uint32_t cByteP = pos / 8;
			uint32_t cBitP = pos % 8;
			if ( cByteP >= cbcData.size() ) break;
			blist.push_back( ( cbcData[cByteP] >> ( 7 - cBitP ) ) & 0x1 );
			//blist.push_back(cbcData[cByteP] & ( 1 << ( 7 - cBitP ) ));
		}
		return blist;
	}



#if 0
	std::string Event::DataHexString( uint8_t pFeId, uint8_t pCbcId ) const
	{
		std::stringbuf tmp;
		std::ostream os( &tmp );

		os << std::hex;

		uint32_t cFirstByteP = OFFSET_CBCDATA / 8;
		uint32_t cFirstBitP = OFFSET_CBCDATA % 8;
		uint32_t cLastByteP = ( cFirstByteP + WIDTH_CBCDATA - 1 ) / 8;
		uint32_t cLastBitP = ( cFirstByteP + WIDTH_CBCDATA - 1 ) % 8;

		uint32_t cMask( 0 );
		uint32_t cMaskLastBit( 0 );
		uint32_t cMaskWidth( 0 );

		//First byte
		cMaskLastBit = cFirstByteP < cLastByteP ? 7 : cLastBitP;
		cMaskWidth = cMaskLastBit - cFirstBitP + 1;
		cMask = ( 1 << ( 7 - cMaskLastBit ) );

		for ( uint32_t i = 0; i < cMaskWidth; i++ )
		{
			cMask <<= 1;
			cMask |= 1;
		}

		os << std::uppercase << std::setw( 2 ) << std::setfill( '0' ) << ( GetCbcEvent( pFeId, pCbcId )[cFirstByteP]&cMask );

		if ( cFirstByteP == cLastByteP )
			return tmp.str();

		//Second to the second last byte
		if ( cFirstByteP != cLastByteP - 1 )
		{
			for ( uint32_t j = cFirstByteP + 1; j < cLastByteP; j++ )
				os << std::uppercase << std::setw( 2 ) << std::setfill( '0' ) << ( GetCbcEvent( pFeId, pCbcId )[j] & 0xFF );
		}

		//Last byte
		cMaskLastBit = cLastBitP;
		cMaskWidth = cMaskLastBit + 1;
		cMask = ( 1 << ( 7 - cMaskLastBit ) );

		for ( uint32_t i = 0; i < cMaskWidth; i++ )
		{
			cMask <<= 1;
			cMask |= 1;
		}

		os << std::uppercase << std::setw( 2 ) << std::setfill( '0' ) << ( GetCbcEvent( pFeId, pCbcId )[cFirstByteP]&cMask );

		return tmp.str();
	}
#endif


	std::string Event::GlibFlagString( uint8_t pFeId, uint8_t pCbcId ) const
	{
		return BitString( pFeId, pCbcId, OFFSET_GLIBFLAG, WIDTH_GLIBFLAG );
	}


	std::string Event::StubBitString( uint8_t pFeId, uint8_t pCbcId ) const
	{
		return BitString( pFeId, pCbcId, OFFSET_CBCSTUBDATA, WIDTH_CBCSTUBDATA );
	}

	bool Event::StubBit( uint8_t pFeId, uint8_t pCbcId ) const
	{
		return Bit( pFeId, pCbcId, OFFSET_CBCSTUBDATA );
	}

	std::ostream& operator<<( std::ostream& os, const Event& ev )
	{
		os << BOLDBLUE <<  "  L1A Counter: " << ev.GetEventCount() << std::endl;
		os << "  CBC Counter: " << ev.GetEventCountCBC() << RESET << std::endl;
		os << "Bunch Counter: " << ev.GetBunch() << std::endl;
		os << "Orbit Counter: " << ev.GetOrbit() << std::endl;
		os << " Lumi Section: " << ev.GetLumi() << std::endl;
		os << "  TDC Counter: " << ev.GetTDC() << std::endl;

		os << "CBC Data:" << std::endl;
		const EventMap& evmap = ev.GetEventMap();
		const int FIRST_LINE_WIDTH = 22;
		const int LINE_WIDTH = 32;
		const int LAST_LINE_WIDTH = 8;
		for ( auto const& it : evmap )
		{
			uint32_t feId = it.first;
			for ( auto const& jt : it.second )
			{
				uint32_t cbcId = jt.first;
				std::string data( ev.DataBitString( feId, cbcId ) );
				os << GREEN << "FEId = " << feId << " CBCId = " << cbcId << RESET << " len(data) = " << data.size() << std::endl;
				os << RED << "Error: " << static_cast<std::bitset<10>>( ev.Error( feId, cbcId ) ) << RESET << std::endl;
				os << "Ch. Data: " << data.substr( 0, FIRST_LINE_WIDTH ) << std::endl;
				for ( int i = 0; i < 7; ++i )
					os << data.substr( FIRST_LINE_WIDTH + LINE_WIDTH * i, LINE_WIDTH ) << std::endl;
				os << data.substr( FIRST_LINE_WIDTH + LINE_WIDTH * 7, LAST_LINE_WIDTH ) << std::endl;
				os << "Stubs: " << ev.StubBitString( feId, cbcId ).c_str() << std::endl;
			}

			os << std::endl;
		}
		return os;
	}

	void Event::BuildClusters( uint8_t pFeId)  
	{ 
                bool bit;
                bool InCluster;
               	
		uint32_t firststrip,cbcstrip,CbcId;
		uint32_t pos;
		uint32_t cByteP;
		uint32_t cBitP;
		int32_t Cbc_Nb;
		uint32_t position, width ;
	
          
          EventMap::const_iterator cIt = fEventMap.find( pFeId );

// strips -> firstStrip

		for(firststrip=0; firststrip<2; firststrip++) //loop for two strips
		 {  //Flag for clusters across CBCs
		   Cbc_Nb = -1; 
		   width = 0;
                   InCluster=0;
		   for(CbcId=0; CbcId< (cIt->second.size()) ; CbcId++) // loop over all CBCs in the Hybrid
		    {  
			FeEventMap::const_iterator cJt = cIt->second.find( CbcId );
	                    std::vector<Cluster> & Clusters = ClusterMap[CbcId] ; 
			   Clusters.clear();
// sensor -> cbcStrip
		       for(cbcstrip = firststrip; cbcstrip < WIDTH_CBCDATA ; cbcstrip +=2 )
			{ pos = cbcstrip+OFFSET_CBCDATA;
			  cByteP = pos / 8;
			  cBitP = pos % 8;
			  if ( cByteP >= cJt->second.second - cJt->second.first +1 ) 
			  { break; 
			   std::cout<<"WARNING : CBC DATA for the CBC ID =" << CbcId << "; FE ID =" << pFeId << "doesn't exist" ;
			  }
		         bit =  ( fEventData[cJt->second.first + cByteP] >> ( 7 - cBitP ) ) & 0x1;
			 
			 if(bit)
			 { if( InCluster == 0)
			    { InCluster = 1;
			      position = cbcstrip;
				Cbc_Nb = CbcId;
			    }
			   width++; // increase the width either for a new cluster or for an existing one
			   
			  }

			  else
			  { if(InCluster == 1) 
			     ClusterMap[Cbc_Nb].push_back(Cluster(position,width));
			      
			      InCluster = 0;
			      width = 0;
			     }
			  }
			}
		      }
		 

		return;
	}

std::vector<Cluster>& Event::GetCluster( uint8_t pFeId, uint8_t pCbcId) 
	{
		if ((ClusterMap.find(pCbcId))== ClusterMap.end()){	//if clusters associated with pCbcId don't exist
			BuildClusters( pFeId);
		}
		
		// Display ClusterMap and DataBit to compare
		std::cout<< "ClusterMap for the FE Id : " << pFeId << "and CBC Id : "<< pCbcId <<"is : \n" ;		
		for(uint32_t i=0; i< ClusterMap[pCbcId].size(); i++)
		{
			std::cout<< ClusterMap[pCbcId][i].GetPosition() ;
			std::cout<< ClusterMap[pCbcId][i].GetWidth() ;
		}
		std::cout<<"\n" ;
		
		std::vector<bool> Data  = DataBitVector(pFeId,pCbcId) ;
		std::cout<<"CBC DATA for the FE Id : " << pFeId << "and CBC Id : " << pCbcId << "is : \n" ;
		for(uint32_t i=0; i< Data.size(); i++)
		{
			std::cout<< Data[i] ;
		}
		std::cout<<"\n" ;
		//

		return ClusterMap[pCbcId];
	}


	
	
}
