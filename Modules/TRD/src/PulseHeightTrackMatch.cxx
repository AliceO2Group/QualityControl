// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PulseHeightTrackMatch.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TLine.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TStopwatch.h>

#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "DataFormatsTRD/TrackTriggerRecord.h"
#include "DataFormatsTRD/TrackTRD.h"
#include "CommonDataFormat/TFIDInfo.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/PulseHeightTrackMatch.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>
#include <map>
#include <tuple>
#include "CCDB/BasicCCDBManager.h"
namespace o2::quality_control_modules::trd
{
PulseHeightTrackMatch::~PulseHeightTrackMatch()
{
}

void PulseHeightTrackMatch::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Info, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Info, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
}

void PulseHeightTrackMatch::buildHistograms()
{
   mDigitsPerEvent.reset(new TH1F("digitsperevent", "Digits per Event", 10000, 0, 10000));
   getObjectsManager()->startPublishing(mDigitsPerEvent.get());
  
   mTracksPerEvent.reset(new TH1F("tracksperevent", "Matched TRD tracks per Event", 100, 0, 10));
   getObjectsManager()->startPublishing(mTracksPerEvent.get());

   mTriggerPerTF.reset(new TH1F("triggerpertimeframe", "Triggers per TF", 100, 0, 100));
   getObjectsManager()->startPublishing(mTriggerPerTF.get());

   mTriggerWDigitPerTF.reset(new TH1F("triggerwdpertimeframe", "Triggers with Digits per TF", 10, 0, 10));
   getObjectsManager()->startPublishing(mTriggerWDigitPerTF.get());

   mTriggerWoDigitPerTF.reset(new TH1F("triggerwodpertimeframe", "Triggers without Digits per TF", 100, 0, 100));
   getObjectsManager()->startPublishing(mTriggerWoDigitPerTF.get());
   
  mPulseHeightpro.reset(new TProfile("PulseHeight/mPulseHeightpro", "Pulse height profile  plot;Timebins;Counts", 30, -0.5, 29.5));
  mPulseHeightpro.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightpro.get());
 
}

void PulseHeightTrackMatch::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize TRDDigitQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  retrieveCCDBSettings();
  buildHistograms();
  
}

void PulseHeightTrackMatch::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "startOfActivity" << ENDM;
} // set stats/stacs

void PulseHeightTrackMatch::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

bool digitIndexCompare_phtm(unsigned int A, unsigned int B, const std::vector<o2::trd::Digit>& originalDigits)
{
  // sort into ROC:padrow
  const o2::trd::Digit *a, *b;
  a = &originalDigits[A];
  b = &originalDigits[B];
  if (a->getDetector() < b->getDetector()) {
    return 1;
  }
  if (a->getDetector() > b->getDetector()) {
    return 0;
  }
  if (a->getPadRow() < b->getPadRow()) {
    return 1;
  }
  if (a->getPadRow() > b->getPadRow()) {
    return 0;
  }
  if (a->getPadCol() < b->getPadCol()) {
    return 1;
  }
  return 0;
}


void PulseHeightTrackMatch::monitorData(o2::framework::ProcessingContext& ctx)
{
   for (auto&& input : ctx.inputs()) {
      auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");    
      auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets"); 
      auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
      auto tracks = ctx.inputs().get<gsl::span<o2::trd::TrackTRD>>("tracks");
      auto triggerrecordstracks= ctx.inputs().get<gsl::span<o2::trd::TrackTriggerRecord>>("trigrectrk");

       std::vector<o2::trd::Digit> digitv(digits.begin(), digits.end());

       if (digitv.size() == 0)continue;

         std::vector<unsigned int> digitsIndex(digitv.size());
         std::iota(digitsIndex.begin(), digitsIndex.end(), 0);

	 int nTrgWDigits=0;
	 int nTrgWoDigits=0;
	 int nTotalTrigger=0;
       
	 for (const auto& trigger : triggerrecords)
	   {
	     // printf("-------------Trigger----------------------BCData for Trigger %i, %i \n ",trigger.getBCData().orbit,trigger.getBCData().bc);
	     nTotalTrigger++;
             
	     if (trigger.getNumberOfDigits() == 0)
	       {
		 nTrgWoDigits++;
		 //printf("Return: No digits in the trigger---------------------------\n");
		 continue;
	       }

	     else{nTrgWDigits++;}

	      if (trigger.getNumberOfDigits() > 10000) {
		mDigitsPerEvent->Fill(9999);
	      } else {
		mDigitsPerEvent->Fill(trigger.getNumberOfDigits());
	      }

	      // now sort digits to det,row,pad
        std::sort(std::begin(digitsIndex) + trigger.getFirstDigit(), std::begin(digitsIndex) + trigger.getFirstDigit() + trigger.getNumberOfDigits(),
                  [&digitv](unsigned int i, unsigned int j) { return digitIndexCompare_phtm(i, j, digitv); });
	
	     for (const auto& triggerTracks : triggerrecordstracks){

	       //printf("--------------TriggerTrack--------------------BCData for TriggerTrack %i, %i \n ",triggerTracks.getBCData().orbit,triggerTracks.getBCData().bc);

	         if(trigger.getBCData()!=triggerTracks.getBCData())
		 {
		   // printf("oops! tigger and triggerrecords are from different bunch crossings-------------------------------------going back...\n");
		   continue;
		 }

		 int nTracks=0;
	       for(int iTrack=triggerTracks.getFirstTrack();iTrack<triggerTracks.getFirstTrack()+triggerTracks.getNumberOfTracks();iTrack++)
		 {
		   nTracks++;
		   printf("Looping over trd trigger tracks %i \n",iTrack);
		   const auto &track =tracks[iTrack];

		   for(int iLayer=0;iLayer<6;iLayer++)
		     {
		       printf("Looping over trd layers for trigger tracks............... %i \n",iLayer);
		       int trackletIndex= track.getTrackletIndex(iLayer);

		       // printf("Index of the tracklet is %i \n",trackletIndex);
		       if(trackletIndex==-1)
			 {
			   continue;
			 }

		       const auto &trklt =tracklets[trackletIndex];

		       int det=trklt.getDetector();
		       int sec = det / 30;
		       int row=trklt.getPadRow();

		       // obtain pad number relative to MCM center
		       int padLocal = trklt.getPositionBinSigned() * o2::trd::constants::GRANULARITYTRKLPOS;
		       // MCM number in column direction (0..7)
		       int mcmCol = (trklt.getMCM() % o2::trd::constants::NMCMROBINCOL) + o2::trd::constants::NMCMROBINCOL * (trklt.getROB() % 2);
		       int colInChamber = 6.f + mcmCol * ((float) o2::trd::constants::NCOLMCM) + padLocal;

		       /* int padLocal;

		       padLocal = trklt.getPosition() ^ 0x80;
		       if (padLocal & (1 << (o2::trd::constants::NBITSTRKLPOS - 1))) {
			 padLocal = -((~(padLocal - 1)) & ((1 << o2::trd::constants::NBITSTRKLPOS) - 1));
		       } else {
			 padLocal = padLocal & ((1 << o2::trd::constants::NBITSTRKLPOS) - 1);
		       }

		       int mcmCol = (trklt.getMCM() % o2::trd::constants::NMCMROBINCOL) + o2::trd::constants::NMCMROBINCOL * (trklt.getROB() % 2);
		       float colGlb = -65.f + mcmCol * ((float) o2::trd::constants::NCOLMCM) + padLocal * o2::trd::constants::GRANULARITYTRKLPOS + 144.f * sec + 72.f;
		       int colInChamber = static_cast<int>(std::round(colGlb - 144.f * sec));*/
		       
		       for(int iDigit=trigger.getFirstDigit()+1;iDigit<trigger.getFirstDigit()+trigger.getNumberOfDigits() -1 ;++iDigit)

			 {
			   // printf("haa! We reached to digits..Looping over digits................ %i \n",iDigit);
			   const auto &digit=digits[iDigit];
			   const auto &digitBefore=digits[iDigit -1];
			   const auto &digitAfter=digits[iDigit +1];
			   
			   std::tuple<unsigned int, unsigned int, unsigned int> aa, ba, ca;
			   aa = std::make_tuple(digitBefore.getDetector(), digitBefore.getPadRow(), digitBefore.getPadCol());
			   ba = std::make_tuple(digit.getDetector(), digit.getPadRow(), digit.getPadCol());
			   ca = std::make_tuple(digitAfter.getDetector(), digitAfter.getPadRow(), digitAfter.getPadCol());
			   
			   auto [det1, row1, col1] = aa;
			   auto [det2, row2, col2] = ba;
			   auto [det3, row3, col3] = ca;
			   
			   // printf("Checking for the detectors det1(%i), det2(%i), det3(%i) \n",det1,det2,det3);
			   // printf("Checking for the rows row1(%i), row2(%i), row3(%i) \n",row1,row2,row3);
			   // printf("Checking for the columnss column1(%i), column2(%i), column3(%i) \n",col1,col2,col3);

			   // printf("--------------------------------------------------------------\n");
			   
			   //  printf("Tracklet Info--->Det %i, Row %i, Col %i \n",det,row,colInChamber);
			   //  printf("Digit Info--->Det %i, Row %i, Col %i \n",det1,row1,col1);

			   // int det1 = digit.getDetector();
			   // int row1 = digit.getPadRow();
			   //int col1 = digit.getPadCol();
			   
		       bool consecutive = false;
		       bool digitmatch =false;

		       if (det==det2 && row==row2){
			 //printf("Detectors and rows matched for tracklets and tracks \n");
			 //printf("Column for tracklet is %i and for digit is %i \n",colInChamber,col1);
			 int coldif=col2-colInChamber;
			 if(TMath::Abs(coldif) < 4)
			      digitmatch = true;			     
			   
		       }
		       if (det1 == det2 && det2 == det3 && row1 == row2 && row2 == row3 && col2 + 1 == col1 && col3 + 1 == col2) consecutive = true;

			   if(digitmatch && consecutive)
			     {
			       // printf("congrats........................detectors and rows for consecutive digits matched with the tracklets \n");
			       // printf("Tracklet Info--->Det %i, Row %i, Col %i \n",det,row,colInChamber);
			       // printf("Digit Info--->Det %i, Row %i, Col %i \n",det1,row1,col1);
			    
			       // if(colInChamber==col2 || colInChamber==col2+1 || colInChamber==col2-1 ){
				 // printf("congrats............columns for digits matched with the tracklets \n");
			       //matchingDigit=true;
			       //  const auto &digit_matched=digits[iDigit];

			        for (int iTb = 0; iTb <o2::trd::constants::TIMEBINS; ++iTb)
		      {
			mPulseHeightpro->Fill(iTb, digitBefore.getADC()[iTb] + digit.getADC()[iTb] + digitAfter.getADC()[iTb]);
			//mPulseHeightpro->Fill(iTb,  digit.getADC()[iTb]);
		      }
		   

			     }
			 }
		     }
		 }
	       mTracksPerEvent->Fill(nTracks);
	     }
	     
	   }
	 if(nTotalTrigger>100)
	   {
	     mTriggerPerTF->Fill(99);
	     mTriggerWoDigitPerTF->Fill(99);
	   }
	 else
	   {
	     mTriggerPerTF->Fill(nTotalTrigger);
	     mTriggerWoDigitPerTF->Fill(nTrgWoDigits);
	   }
	 mTriggerWDigitPerTF->Fill(nTrgWDigits);
  }
}
  
void PulseHeightTrackMatch::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void PulseHeightTrackMatch::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void PulseHeightTrackMatch::reset()
{
  // clean all the monitor objects here
  ILOG(Info) << "Resetting the histogram" << ENDM;
  mTracksPerEvent->Reset();
  mDigitsPerEvent->Reset();
  mTriggerPerTF->Reset();
  mTriggerWDigitPerTF->Reset();
  mTriggerWoDigitPerTF->Reset();
  
  mDigitsPerEvent.get()->Reset();
  mTriggerPerTF.get()->Reset();
  mTriggerWDigitPerTF.get()->Reset();
  mTriggerWoDigitPerTF.get()->Reset();
  mTracksPerEvent.get()->Reset();
  mPulseHeightpro.get()->Reset();
}
} // namespace o2::quality_control_modules::trd

