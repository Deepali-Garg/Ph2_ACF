#include "PedeNoise.h"


void PedeNoise::Initialise()
{
    //is to be called after system controller::ReadHW, ReadSettings
    // populates all the maps
    // create the canvases


    fPedestalCanvas = new TCanvas( "Pedestal & Noise", "Pedestal & Noise", 650, 650 );
    fFeSummaryCanvas = new TCanvas( "Noise for each FE", "Noise for each FE", 650, 650 );
    fNoiseCanvas = new TCanvas( "Final SCurves, Strip Noise", "Final SCurves, Noise", 650, 650 );


    // count FEs & CBCs
    uint32_t cCbcCount = 0;
    uint32_t cCbcIdMax = 0;
    uint32_t cFeCount = 0;

    for ( auto cBoard : fBoardVector )
      {
	uint32_t cBoardId = cBoard->getBeId();
	
	for ( auto cFe : cBoard->fModuleVector )
	  {
	    uint32_t cFeId = cFe->getFeId();
	    cFeCount++;
	    
	    for ( auto cCbc : cFe->fCbcVector )
	      {
		uint32_t cCbcId = cCbc->getCbcId();
		cCbcCount++;
		if ( cCbcId > cCbcIdMax ) cCbcIdMax = cCbcId;
		
		// populate the channel vector
		std::vector<Channel> cChanVec;
		
		for ( uint8_t cChan = 0; cChan < 254; cChan++ )
		  cChanVec.push_back( Channel( cBoardId, cFeId, cCbcId, cChan ) );
		
		fCbcChannelMap[cCbc] = cChanVec;
		
		// the fits are initialized when I fit!
		
		TString cHistname;
		TH1F* cHist;
		
		// for noise maps etc.
		
		cHistname = Form( "Fe%dCBC%d_Offsets", cFe->getFeId(), cCbc->getCbcId() );
		cHist = new TH1F( cHistname, cHistname, 254, -0.5, 253.5 );
		bookHistogram( cCbc, "Cbc_Offsets", cHist );
		
		cHistname = Form( "Fe%dCBC%d_Noise", cFe->getFeId(), cCbc->getCbcId() );
		cHist = new TH1F( cHistname, cHistname, 200, 0, 20 );
		bookHistogram( cCbc, "Cbc_Noise", cHist );
		
		cHistname = Form( "Fe%dCBC%d_StripNoise", cFe->getFeId(), cCbc->getCbcId() );
		cHist = new TH1F( cHistname, cHistname, 254, -0.5, 253.5 );
		bookHistogram( cCbc, "Cbc_Stripnoise", cHist );
		
		cHistname = Form( "Fe%dCBC%d_Pedestal", cFe->getFeId(), cCbc->getCbcId() );
		cHist = new TH1F( cHistname, cHistname, 510, -0.5, 254.5 );
		bookHistogram( cCbc, "Cbc_Pedestal", cHist );

		cHistname = Form( "Fe%dCBC%d_Noise_even", cFe->getFeId(), cCbc->getCbcId() );
		cHist = new TH1F( cHistname, cHistname, 128, -0.5, 127.5 );
		bookHistogram( cCbc, "Cbc_Noise_even", cHist );
		
		cHistname = Form( "Fe%dCBC%d_Noise_odd", cFe->getFeId(), cCbc->getCbcId() );
		cHist = new TH1F( cHistname, cHistname, 128, -0.5, 127.5 );
		cHist->SetLineColor( 2 );
		bookHistogram( cCbc, "Cbc_noise_odd", cHist );
		
	      }
	    
	    TString cNoisehistname =  Form( "Fe%d_Noise", cFeId );
	    TH1F* cNoise = new TH1F( cNoisehistname, cNoisehistname, 200, 0, 20 );
	    bookHistogram( cFe, "Module_noisehist", cNoise );
	    
	    cNoisehistname = Form( "Fe%d_StripNoise", cFeId );
	    TProfile* cStripnoise = new TProfile( cNoisehistname, cNoisehistname, ( NCHANNELS * cCbcCount ) + 1, -.5, cCbcCount * NCHANNELS + .5 );
	    bookHistogram( cFe, "Module_Stripnoise", cStripnoise );
	  }
	fNCbc = cCbcCount;
	fNFe = cFeCount;
      }
    uint32_t cPads = ( cCbcIdMax > cCbcCount ) ? cCbcIdMax : cCbcCount;



    fNoiseCanvas->DivideSquare( 2 * cPads );
    fPedestalCanvas->DivideSquare( 2 * cPads );
    fFeSummaryCanvas->DivideSquare( fNFe );
    
    // now read the settings from the map
    auto cSetting = fSettingsMap.find( "HoleMode" );
    fHoleMode = ( cSetting != std::end( fSettingsMap ) ) ? cSetting->second : 1;
    cSetting = fSettingsMap.find( "Nevents" );
    fEventsPerPoint = ( cSetting != std::end( fSettingsMap ) ) ? cSetting->second : 10;
    cSetting = fSettingsMap.find( "FitSCurves" );
    fFitted = ( cSetting != std::end( fSettingsMap ) ) ? cSetting->second : 0;
    cSetting = fSettingsMap.find( "TestPulseAmplitude" );
    fTestPulseAmplitude = ( cSetting != std::end( fSettingsMap ) ) ? cSetting->second : 0;
    
    // Decide if test pulse or not
    if ( ( fTestPulseAmplitude == 0x00 ) || ( fTestPulseAmplitude == 0xFF ) ) fTestPulse = 0;
    else fTestPulse = 1;
    std::cout << "Created Object Maps and parsed settings:" << std::endl;
    std::cout << "	Hole Mode = " << fHoleMode << std::endl;
    std::cout << "	Nevents = " << fEventsPerPoint << std::endl;
    std::cout << "	FitSCurves = " << int( fFitted ) << std::endl;
    std::cout << "	TestPulseAmplitude = " << int( fTestPulseAmplitude ) << std::endl;
}


void PedeNoise::measureNoise()
{
    saveInitialOffsets();
    // method to measure one final set of SCurves with the final calibration applied to extract the noise
    // now measure some SCurves
    for ( auto& cTGrpM : fTestGroupChannelMap )
    {
        // if we want to run with test pulses, we'll have to enable commissioning mode and enable the TP for each test group
        if ( fTestPulse )
        {
            BeBoardRegWriter cBeBoardWriter( fBeBoardInterface, "COMMISSIONNING_MODE_RQ", 1 );
            this->accept( cBeBoardWriter );
            cBeBoardWriter.setRegister( "COMMISSIONNING_MODE_CBC_TEST_PULSE_VALID", 1 );
            this->accept( cBeBoardWriter );
            std::cout << RED <<  "Enabling Test Pulse for Test Group " << cTGrpM.first << " with amplitude " << +fTestPulseAmplitude << RESET << std::endl;
            setSystemTestPulse( fTestPulseAmplitude, cTGrpM.first );

        }

        std::cout << GREEN << "Measuring Test Group...." << cTGrpM.first << RESET << std::endl;
        // this leaves the offset values at the tuned values for cTGrp and disables all other groups
        enableTestGroupforNoise( cTGrpM.first );

        // now initialize the Scurves
        initializeSCurves( "Final", fTestPulseAmplitude, cTGrpM.first );

        // measure the SCurves, the false is indicating that I am sweeping Vcth
        measureSCurves( cTGrpM.first );

        // now process the measured SCuvers, true indicates that I am drawing, the TGraphErrors with Vcth vs Vplus are also filled
        processSCurvesNoise( "Final", fTestPulseAmplitude, true, cTGrpM.first );
    }
    std::cout << BOLDBLUE << "Finished measuring the noise ..." << std::endl << RESET << std::endl;

    // now plot the histogram with the noise

    // instead of looping over the Histograms and finding everything according to the CBC from the map, just loop the CBCs
    for ( auto cBoard : fBoardVector )
      {
	uint32_t cBoardId = cBoard->getBeId();
	
	for ( auto cFe : cBoard->fModuleVector )
	  {
	    uint32_t cFeId = cFe->getFeId();
	    
	    // here get the per FE histograms
	    TH1F* cTmpHist = dynamic_cast<TH1F*>( getHist( cFe, "Module_noisehist" ) );
	    TProfile* cTmpProfile = dynamic_cast<TProfile*>( getHist( cFe, "Module_Stripnoise" ) );
	    
	    for ( auto cCbc : cFe->fCbcVector )
	      {
		uint32_t cCbcId = static_cast<int>( cCbc->getCbcId() );
		
		// here get the per-CBC histograms
		
		TH1F* cNoiseHist = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_Noise" ) );
		TH1F* cPedeHist  = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_Pedestal" ) );
		TH1F* cStripHist = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_Stripnoise" ) );
		TH1F* cEvenHist  = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_Noise_even" ) );
		TH1F* cOddHist   = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_noise_odd" ) );
		
		std::cout << BOLDRED << "Average noise on FE " << +cCbc->getFeId() << " CBC " << +cCbc->getCbcId() << " : " << cNoiseHist->GetMean() << " ; RMS : " << cNoiseHist->GetRMS() << " ; Pedestal : " << cPedeHist->GetMean() << " VCth units." << RESET << std::endl;
		
		fNoiseCanvas->cd( fNCbc + cCbc->getCbcId() + 1 );
		// cStripHist->DrawCopy();
		cEvenHist->DrawCopy();
		cOddHist->DrawCopy( "same" );
		
		fPedestalCanvas->cd( cCbc->getCbcId() + 1 );
		cNoiseHist->DrawCopy();
		
		fPedestalCanvas->cd( fNCbc + cCbc->getCbcId() + 1 );
		cPedeHist->DrawCopy();
		fNoiseCanvas->Update();
		fPedestalCanvas->Update();
#ifdef __HTTP__
		fHttpServer->ProcessRequests();
#endif
		// here add the CBC histos to the module histos
		cTmpHist->Add( cNoiseHist );
		for ( int cBin = 0; cBin < NCHANNELS; cBin++ )
		  {
		    // std::cout << cBin << " Strip " << +cCbcId * 254 + cBin << " Noise " << cStripHist->second->GetBinContent( cBin ) << std::endl;
		    if ( cStripHist->GetBinContent( cBin ) > 0 && cStripHist->GetBinContent( cBin ) < 255 ) cTmpProfile->Fill( cCbcId * 254 + cBin, cStripHist->GetBinContent( cBin ) );
		    // else cTmpProfile->Fill( cCbcId * 254 + cBin, 255 );
		  }
	      }
	    fFeSummaryCanvas->cd( cFeId + 1 );
	    cTmpHist->DrawCopy();
	    fFeSummaryCanvas->cd( fNFe + cFeId + 1 );
	    cTmpProfile->DrawCopy();
	    fFeSummaryCanvas->Update();
#ifdef __HTTP__
	    fHttpServer->ProcessRequests();
#endif
	  }
      }
}


//////////////////////////////////////      PRIVATE METHODS     /////////////////////////////////////////////


void PedeNoise::enableTestGroupforNoise( int  pTGrpId )
{
    uint8_t cOffset = ( fHoleMode ) ? 0x00 : 0xFF;

    for ( auto cBoard : fBoardVector )
      {
	uint32_t cBoardId = cBoard->getBeId();
	
	for ( auto cFe : cBoard->fModuleVector )
	  {
	    uint32_t cFeId = cFe->getFeId();
	    
	    for ( auto cCbc : cFe->fCbcVector )
	      {
		uint32_t cCbcId = cCbc->getCbcId();
		
		TH1F* cOffsets = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_Offsets" ) );
		
		RegisterVector cRegVec;
		// iterate the groups (first is ID, second is vec<uint8_t>)
		for ( auto& cGrp : fTestGroupChannelMap )
		  {
		    // if grpid = -1, do nothing (all channels)
		    if ( cGrp.first == -1 ) continue;
		    // if the group is not my current grout
		    if ( cGrp.first != pTGrpId )
		      {
			// iterate the channels and push back 0 or FF
			for ( auto& cChan : cGrp.second )
			  {
			    TString cRegName = Form( "Channel%03d", cChan + 1 );
			    cRegVec.push_back( { cRegName.Data(), cOffset } );
			    // std::cout << "DEBUG CBC " << cCbcId << " Channel " << +cChan << " group " << cGrp.first << " offset " << +cOffset << std::endl;
			  }
		      }
		    // if it is the current group, get the original offset values
		    else if ( cGrp.first == pTGrpId )
		      {
			// iterate over the channels in the test group and find the corresponding offset in the original offset map
			for ( auto& cChan : cGrp.second )
			  {
			    
			    uint8_t cEnableOffset = cOffsets->GetBinContent( cChan );
			    TString cRegName = Form( "Channel%03d", cChan + 1 );
			    cRegVec.push_back( { cRegName.Data(), cEnableOffset } );
			    // std::cout << GREEN << "DEBUG CBC " << cCbcId << " Channel " << +cChan << " group " << cGrp.first << " offset " << std::hex << "0x" << +cEnableOffset << std::dec << RESET << std::endl;
			  }
		      }
		  }
		
		// now I should have 0 or FF as offset for all channels except the one in my test group
		// this now needs to be written to the CBCs
		fCbcInterface->WriteCbcMultReg( cCbc, cRegVec );
	      }
	  }
      }
    std::cout << "Disabling all TGroups except " << pTGrpId << " ! " << std::endl;
}


void PedeNoise::processSCurvesNoise( TString pParameter, uint8_t pValue, bool pDraw, int  pTGrpId )
{

    // First fitHits for every Channel, then extract the midpoint and noise and fill it in fVplusVcthGraphMap
    for ( auto& cCbc : fCbcChannelMap )
    {

        TH1F* cNoiseHist = dynamic_cast<TH1F*>( getHist( cCbc.first, "Cbc_Noise" ) );
        TH1F* cPedeHist  = dynamic_cast<TH1F*>( getHist( cCbc.first, "Cbc_Pedestal" ) );
        TH1F* cStripHist = dynamic_cast<TH1F*>( getHist( cCbc.first, "Cbc_Stripnoise" ) );
        TH1F* cEvenHist  = dynamic_cast<TH1F*>( getHist( cCbc.first, "Cbc_Noise_even" ) );
        TH1F* cOddHist   = dynamic_cast<TH1F*>( getHist( cCbc.first, "Cbc_noise_odd" ) );

        // Loop the Channels
        bool cFirst = true;
        TString cOption;

        std::vector<uint8_t> cTestGrpChannelVec = fTestGroupChannelMap[pTGrpId];
        for ( auto& cChanId : cTestGrpChannelVec )
        {
            //for ( auto& cChan : cCbc.second )
            Channel cChan = cCbc.second.at( cChanId );

            // Fit or Differentiate
            if ( fFitted ) cChan.fitHist( fEventsPerPoint, fHoleMode, pValue, pParameter, fResultFile );
            else cChan.differentiateHist( fEventsPerPoint, fHoleMode, pValue, pParameter, fResultFile );


            // instead of the code below, use a histogram to histogram the noise
            if ( cChan.getNoise() == 0 || cChan.getNoise() > 255 ) std::cout << RED << "Error, SCurve Fit for Fe " << int( cCbc.first->getFeId() ) << " Cbc " << int( cCbc.first->getCbcId() ) << " Channel " << int( cChan.fChannelId ) << " did not work correctly! Noise " << cChan.getNoise() << RESET << std::endl;

            cNoiseHist->Fill( cChan.getNoise() );
            cPedeHist->Fill( cChan.getPedestal() );

            // Even and odd channel noise
            if ( ( int( cChan.fChannelId ) % 2 ) == 0 )
                cEvenHist->Fill( int( cChan.fChannelId / 2 ), cChan.getNoise() );
            else
                cOddHist->Fill( int( cChan.fChannelId / 2.0 ), cChan.getNoise() );

            // some output
            std::cout << "FE " << +cCbc.first->getFeId() << " CBC " << +cCbc.first->getCbcId() << " Chanel " << +cChan.fChannelId << " Pedestal " << cChan.getPedestal() << " Noise " << cChan.getNoise() << std::endl;

            cStripHist->Fill( cChan.fChannelId, cChan.getNoise() );

            //Draw
            if ( pDraw )
            {
                if ( cFirst )
                {
                    cOption = "P" ;
                    cFirst = false;
                }
                else cOption = "P same";

                fNoiseCanvas->cd( cCbc.first->getCbcId() + 1 );
                cChan.fScurve->DrawCopy( cOption );
                if ( fFitted )
                    cChan.fFit->DrawCopy( "same" );
                else cChan.fDerivative->DrawCopy( "same" );
            }
        }
        if ( pDraw )
        {
            fNoiseCanvas->Update();
#ifdef __HTTP__
            fHttpServer->ProcessRequests();
#endif
        }
    }

}

void PedeNoise::saveInitialOffsets()
{
    std::cout << "Initializing map with original Offsets for later ... " << std::endl;
    // save the initial offsets for Noise measurement in a map
    for ( auto cBoard : fBoardVector )
      {
	uint32_t cBoardId = cBoard->getBeId();
	
	for ( auto cFe : cBoard->fModuleVector )
	  {
	    uint32_t cFeId = cFe->getFeId();
	    
	    for ( auto cCbc : cFe->fCbcVector )
	      {
		uint32_t cCbcId = cCbc->getCbcId();
		
		// map to instert in fOffsetMap
		// <cChan, Offset>
		// std::map<uint8_t, uint8_t> cCbcOffsetMap;
		TH1F* cOffsetHist = dynamic_cast<TH1F*>( getHist( cCbc, "Cbc_Offsets" ) );
		for ( uint8_t cChan = 0; cChan < NCHANNELS; cChan++ )
		  {
		    TString cRegName = Form( "Channel%03d", cChan + 1 );
		    uint8_t cOffset = cCbc->getReg( cRegName.Data() );
		    cOffsetHist->SetBinContent( cChan, cOffset );
		    // cCbcOffsetMap[cChan] = cOffset;
		    // std::cout << "DEBUG Original Offset for CBC " << cCbcId << " channel " << +cChan << " " << +cOffset << std::endl;
		  }
		// fOffsetMap[cCbc] = cCbcOffsetMap;
	      }
	  }
      }
}

void PedeNoise::SaveResults()
{
    // just use auto iterators to write everything to disk
    // this is the old method before Tool class was cool
    fResultFile->cd();
    Tool::SaveResults();

    // Save canvasses too

    fNoiseCanvas->Write( fNoiseCanvas->GetName(), TObject::kOverwrite );
    fPedestalCanvas->Write( fPedestalCanvas->GetName(), TObject::kOverwrite );
    fFeSummaryCanvas->Write( fFeSummaryCanvas->GetName(), TObject::kOverwrite );
}
