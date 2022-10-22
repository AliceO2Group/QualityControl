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
/// \file   PulseHeightPerChamber.cxx
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

#include "DataFormatsTRD/Constants.h"
#include "DataFormatsTRD/Digit.h"
#include "DataFormatsTRD/Tracklet64.h"
#include "DataFormatsTRD/TriggerRecord.h"
#include "ReconstructionDataFormats/TrackTPCITS.h"
#include "CommonDataFormat/TFIDInfo.h"
#include "CommonDataFormat/InteractionRecord.h"
#include "TRDBase/Geometry.h"
#include "DetectorsBase/Propagator.h"
#include "DetectorsBase/GeometryManager.h"
#include "QualityControl/ObjectsManager.h"
#include "QualityControl/TaskInterface.h"
#include "QualityControl/QcInfoLogger.h"
#include "TRD/PulseHeightPerChamber.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <gsl/span>
#include <map>
#include <tuple>
#include "CCDB/BasicCCDBManager.h"

namespace o2::quality_control_modules::trd
{

PulseHeightPerChamber::~PulseHeightPerChamber()
{
}
void PulseHeightPerChamber::retrieveCCDBSettings()
{
  if (auto param = mCustomParameters.find("ccdbtimestamp"); param != mCustomParameters.end()) {
    mTimestamp = std::stol(mCustomParameters["ccdbtimestamp"]);
    ILOG(Info, Support) << "configure() : using ccdbtimestamp = " << mTimestamp << ENDM;
  } else {
    mTimestamp = o2::ccdb::getCurrentTimestamp();
    ILOG(Info, Support) << "configure() : using default timestam of now = " << mTimestamp << ENDM;
  }
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setTimestamp(mTimestamp);
  mNoiseMap = mgr.get<o2::trd::NoiseStatusMCM>("/TRD/Calib/NoiseMapMCM");
  if (mNoiseMap == nullptr) {
    ILOG(Info, Support) << "mNoiseMap is null, no noisy mcm reduction" << ENDM;
  }
  mChamberStatus = mgr.get<o2::trd::HalfChamberStatusQC>("/TRD/Calib/HalfChamberStatusQC");
  if (mChamberStatus == nullptr) {
    ILOG(Info, Support) << "mChamberStatus is null, no chamber status to display" << ENDM;
  }
}

  void PulseHeightPerChamber::buildChamberIgnoreBP()
{
  mChambersToIgnoreBP.reset();
  // Vector of string to save tokens
  std::vector<std::string> tokens;
  // stringstream class check1
  std::stringstream check1(mChambersToIgnore);
  std::string intermediate;
  // Tokenizing w.r.t. space ','
  while (getline(check1, intermediate, ',')) {
    tokens.push_back(intermediate);
  }
  // Printing the token vector
  for (auto& token : tokens) {
    //token now holds something like 16_3_0
    std::vector<std::string> parts;
    std::stringstream indexcheck(token);
    std::string tokenpart;
    // Tokenizing w.r.t. space ','
    while (getline(indexcheck, tokenpart, '_')) {
      parts.push_back(tokenpart);
    }
    // now flip the bit related to the sector:stack:layer stored in parts.
    int sector = std::stoi(parts[0]);
    int stack = std::stoi(parts[1]);
    int layer = std::stoi(parts[2]);
    mChambersToIgnoreBP.set(sector * o2::trd::constants::NSTACK * o2::trd::constants::NLAYER + stack * o2::trd::constants::NLAYER + layer);
  }
}

void PulseHeightPerChamber::drawLinesOnPulseHeight(TH1F* h)
{
  TLine* lmin = new TLine(mPulseHeightPeakRegion.first, 0, mPulseHeightPeakRegion.first, 1e9);
  TLine* lmax = new TLine(mPulseHeightPeakRegion.second, 0, mPulseHeightPeakRegion.second, 1e9);
  lmin->SetLineStyle(2);
  lmax->SetLineStyle(2);
  lmin->SetLineColor(kRed);
  lmax->SetLineColor(kRed);
  h->GetListOfFunctions()->Add(lmin);
  h->GetListOfFunctions()->Add(lmax);
}

void PulseHeightPerChamber::fillLinesOnHistsPerLayer(int iLayer)
{
  std::bitset<1080> hciddone;
  hciddone.reset();
  if (mChamberStatus == nullptr) {
    //protect for if the chamber status is not pulled from the ccdb for what ever reason.
    ILOG(Info, Support) << "No half chamber status object to be able to fill the mask" << ENDM;
  } else {
    for (int iSec = 0; iSec < 18; ++iSec) {
      for (int iStack = 0; iStack < 5; ++iStack) {
        int rowMax = (iStack == 2) ? 12 : 16;
        for (int side = 0; side < 2; ++side) {
          int det = iSec * 30 + iStack * 6 + iLayer;
          int hcid = (side == 0) ? det * 2 : det * 2 + 1;
          int rowstart = iStack < 3 ? iStack * 16 : 44 + (iStack - 3) * 16;                 // pad row within whole sector
          int rowend = iStack < 3 ? rowMax + iStack * 16 : rowMax + 44 + (iStack - 3) * 16; // pad row within whole sector
          if (mChamberStatus->isMasked(hcid) && (!hciddone.test(hcid))) {
            drawHashOnLayers(iLayer, hcid, 0, rowstart, rowend);
            hciddone.set(hcid); // to avoid mutliple instances of adding lines
          }
        }
      }
    }
  }
}
  void PulseHeightPerChamber::buildHistograms()
  {
  mDigitsPerEvent.reset(new TH1F("digitsperevent", "Digits per Event", 10000, 0, 10000));
  getObjectsManager()->startPublishing(mDigitsPerEvent.get());

  mDigitsSizevsTrackletSize.reset(new TH2F("digitsvstracklets", "Tracklets Count vs Digits Count per event; Number of Tracklets;Number Of Digits", 2500, 0, 2500, 2500, 0, 2500));
  getObjectsManager()->startPublishing(mDigitsSizevsTrackletSize.get());

  mPulseHeight.reset(new TH1F("PulseHeight/mPulseHeight", Form("Pulse height plot threshold:%i;Timebins;Counts", mPulseHeightThreshold), 30, -0.5, 29.5));
  drawLinesOnPulseHeight(mPulseHeight.get());
  getObjectsManager()->startPublishing(mPulseHeight.get());
  mPulseHeight.get()->GetYaxis()->SetTickSize(0.01);

  mPulseHeightpro.reset(new TProfile("PulseHeight/mPulseHeightpro", "Pulse height profile  plot;Timebins;Counts", 30, -0.5, 29.5));
  mPulseHeightpro.get()->Sumw2();
  getObjectsManager()->startPublishing(mPulseHeightpro.get());

   for (int iLayer = 0; iLayer < 6; ++iLayer) {
    mLayers_digits.push_back(new TH2F(Form("digits_layer%i", iLayer), Form("Digit count per pad in layer %i;stack;sector", iLayer), 76, -0.5, 75.5, 2592, -0.5, 2591.5));
     mLayers_tracklets.push_back(new TH2F(Form("tracklets_layer%i", iLayer), Form("Tracklets count per pad in layer %i;stack;sector", iLayer), 76, -0.5, 75.5, 2592, -0.5, 2591.5));
      mLayers_tracks.push_back(new TH2F(Form("tracks_layer%i", iLayer), Form("Tracks count per pad in layer %i;stack;sector", iLayer), 76, -0.5, 75.5, 2592, -0.5, 2591.5));
    auto xax = mLayers_tracks.back()->GetXaxis();
    xax->SetBinLabel(8, "0");
    xax->SetBinLabel(24, "1");
    xax->SetBinLabel(38, "2");
    xax->SetBinLabel(52, "3");
    xax->SetBinLabel(68, "4");
    xax->SetTicks("-");
    xax->SetTickSize(0.01);
    xax->SetLabelSize(0.045);
    xax->SetLabelOffset(0.01);
    auto yax = mLayers_tracks.back()->GetYaxis();
    for (int iSec = 0; iSec < 18; ++iSec) {
      auto lbl = std::to_string(iSec);
      yax->SetBinLabel(iSec * 144 + 72, lbl.c_str());
    }
    yax->SetTicks("-");
    yax->SetTickSize(0.01);
    yax->SetLabelSize(0.045);
    yax->SetLabelOffset(0.01);
    mLayers_tracks.back()->SetStats(0);
    mLayers_digits.back()->SetStats(0);
    mLayers_tracklets.back()->SetStats(0);
    drawTrdLayersGrid(mLayers_digits.back());
    drawTrdLayersGrid(mLayers_tracks.back());
    drawTrdLayersGrid(mLayers_tracklets.back());
    fillLinesOnHistsPerLayer(iLayer);

    getObjectsManager()->startPublishing(mLayers_tracks.back());
    getObjectsManager()->setDefaultDrawOptions(mLayers_tracks.back()->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers_tracks.back(), "logz");

    getObjectsManager()->startPublishing(mLayers_digits.back());
    getObjectsManager()->setDefaultDrawOptions(mLayers_digits.back()->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers_digits.back(), "logz");

    getObjectsManager()->startPublishing(mLayers_tracklets.back());
    getObjectsManager()->setDefaultDrawOptions(mLayers_tracklets.back()->GetName(), "COLZ");
    getObjectsManager()->setDisplayHint(mLayers_tracklets.back(), "logz");
  }
}
  
void PulseHeightPerChamber::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info) << "initialize PulseHeightPerChamberTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("driftregionstart"); param != mCustomParameters.end()) {
    mDriftRegion.first = stof(param->second);
    ILOG(Info, Support) << "configure() : using peakregionstart = " << mDriftRegion.first << ENDM;
  } else {
    mDriftRegion.first = 7.0;
    ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("driftregionend"); param != mCustomParameters.end()) {
    mDriftRegion.second = stof(param->second);
    ILOG(Info, Support) << "configure() : using peakregionstart = " << mDriftRegion.second << ENDM;
  } else {
    mDriftRegion.second = 20.0;
    ILOG(Info, Support) << "configure() : using default dritfregionstart = " << mDriftRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeaklower"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.first = stof(param->second);
    ILOG(Info, Support) << "configure() : using pulsehheightlower	= " << mPulseHeightPeakRegion.first << ENDM;
  } else {
    mPulseHeightPeakRegion.first = 0.0;
    ILOG(Info, Support) << "configure() : using default pulseheightlower = " << mPulseHeightPeakRegion.first << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightpeakupper"); param != mCustomParameters.end()) {
    mPulseHeightPeakRegion.second = stof(param->second);
    ILOG(Info, Support) << "configure() : using pulsehheightupper	= " << mPulseHeightPeakRegion.second << ENDM;
  } else {
    mPulseHeightPeakRegion.second = 5.0;
    ILOG(Info, Support) << "configure() : using default pulseheightupper = " << mPulseHeightPeakRegion.second << ENDM;
  }
  if (auto param = mCustomParameters.find("skippedshareddigits"); param != mCustomParameters.end()) {
    mSkipSharedDigits = stod(param->second);
    ILOG(Info, Support) << "configure() : using skip shared digits 	= " << mSkipSharedDigits << ENDM;
  } else {
    mSkipSharedDigits = false;
    ILOG(Info, Support) << "configure() : using default skip shared digits = " << mSkipSharedDigits << ENDM;
  }
  if (auto param = mCustomParameters.find("pulseheightthreshold"); param != mCustomParameters.end()) {
    mPulseHeightThreshold = stod(param->second);
    ILOG(Info, Support) << "configure() : using skip shared digits 	= " << mPulseHeightThreshold << ENDM;
  } else {
    mPulseHeightThreshold = 400;
    ILOG(Info, Support) << "configure() : using default skip shared digits = " << mPulseHeightThreshold << ENDM;
  }
  if (auto param = mCustomParameters.find("chamberstoignore"); param != mCustomParameters.end()) {
    mChambersToIgnore = param->second;
    ILOG(Info, Support) << "configure() : chamberstoignore = " << mChambersToIgnore << ENDM;
  } else {
    mChambersToIgnore = "16_3_0";
    ILOG(Info, Support) << "configure() : chambers to ignore for pulseheight calculations = " << mChambersToIgnore << ENDM;
  }
  buildChamberIgnoreBP();

  retrieveCCDBSettings();
  buildHistograms();

}

void PulseHeightPerChamber::startOfActivity(Activity& activity)
{
  ILOG(Info) << "startOfActivity " << ENDM;
}

void PulseHeightPerChamber::startOfCycle()
{
  ILOG(Info) << "startOfCycle" << ENDM;
}

bool digitIndexCompare_ph(unsigned int A, unsigned int B, const std::vector<o2::trd::Digit>& originalDigits)
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

bool adjustSector(o2::track::TrackParCov& t, o2::base::Propagator* prop)
{
  o2::trd::Geometry* geo = nullptr;
  geo = o2::trd::Geometry::instance();
  float alpha = geo->getAlpha();
  float xTmp = t.getX();
  float y = t.getY();
  float yMax = t.getX() * TMath::Tan(0.5f * alpha);
  float alphaCurr = t.getAlpha();
  if (fabs(y) > 2 * yMax) {
    printf("Skipping track crossing two sector boundaries\n");
    return false;
  }
  int nTries = 0;
  while (fabs(y) > yMax) {
    if (nTries >= 2) {
      printf("Skipping track after too many tries of rotation\n");
      return false;
    }
    int sign = (y > 0) ? 1 : -1;
    float alphaNew = alphaCurr + alpha * sign;
    if (alphaNew > TMath::Pi()) {
      alphaNew -= 2 * TMath::Pi();
    } else if (alphaNew < -TMath::Pi()) {
      alphaNew += 2 * TMath::Pi();
    }
    if (!t.rotate(alphaNew)) {
      return false;
    }
    if (!prop->PropagateToXBxByBz(t, xTmp)) {
      return false;
    }
    y = t.getY();
    ++nTries;
  }
  return true;
}

int getSector(float alpha)
{
  if (alpha < 0) {
    alpha += 2.f * TMath::Pi();
  } else if (alpha >= 2.f * TMath::Pi()) {
    alpha -= 2.f * TMath::Pi();
  }
  return (int)(alpha * o2::trd::constants::NSECTOR / (2.f * TMath::Pi()));
}

bool PulseHeightPerChamber::isChamberToBeIgnored(unsigned int sm, unsigned int stack, unsigned int layer)
{
  // just to make the calling method a bit cleaner
  return mChambersToIgnoreBP.test(sm * o2::trd::constants::NLAYER * o2::trd::constants::NSTACK + stack * o2::trd::constants::NLAYER + layer);
}
void PulseHeightPerChamber::monitorData(o2::framework::ProcessingContext& ctx)
{
  for (auto&& input : ctx.inputs()) {
    if (input.header != nullptr && input.payload != nullptr) {

      //auto tfid=ctx.inputs().get<gsl::span<o2::dataformats::TFIDInfo>>("tfid");
      auto tracks= ctx.inputs().get<gsl::span<o2::dataformats::TrackTPCITS>>("tracks");
      
      auto digits = ctx.inputs().get<gsl::span<o2::trd::Digit>>("digits");
      std::vector<o2::trd::Digit> digitv(digits.begin(), digits.end());

      if (digitv.size() == 0)
        continue;
      auto tracklets = ctx.inputs().get<gsl::span<o2::trd::Tracklet64>>("tracklets"); // still deciding if we will ever need the tracklets here.
      auto triggerrecords = ctx.inputs().get<gsl::span<o2::trd::TriggerRecord>>("triggers");
      // create a vector indexing into the digits in question
      std::vector<unsigned int> digitsIndex(digitv.size());
      std::iota(digitsIndex.begin(), digitsIndex.end(), 0);
      // we now have sorted digits, can loop sequentially and be going over det/row/pad

      
      float minPtTrack = .5f;
      bool doXOR = true;


      //loop over triggers begins===========================================
      for (auto& trigger : triggerrecords)
	{
        uint64_t numtracklets = trigger.getNumberOfTracklets();
        uint64_t numdigits = trigger.getNumberOfDigits();
	
        if (trigger.getNumberOfDigits() == 0 || trigger.getNumberOfTracklets() == 0)
	  { continue;} // bail if we have no digits in this trigge

	/* geo = o2::trd::Geometry::instance();
           geo->createPadPlaneArray();
           geo->createClusterMatrixArray();

	   auto  prop = o2::base::Propagator::Instance();*/
	   
	   bool foundTrackForTrigger = false;

      // let's see if we can match tracks to digits and tracklets
	   //  std::set<std::pair<std::tuple<int, int, int>, int>> trackMap; // filled with detector, row, col for each track point and the index of the ITS-TPC track
	   //    std::multimap<std::tuple<int, int, int>, int> trackletMap; // filled with detector, row, col and with the index of each tracklet
	   //  std::multimap<std::tuple<int, int, int>, int> digitMap; // filled with detector, row, col and with the index of each tracklet
	// float trdTriggerTimeUS = trigger.getBCData().differenceInBCNS(o2::InteractionRecord{0, tfid.firstTForbit});

       int nTracks = 0;
       int nPointsTrigger = 0;
       int countInterestingTracks = 0;
       int minAdcCount = 0;

       //track loop======================================================
       /*    for (int iTrack = 0; iTrack < static_cast<int>(tracks.size()); ++iTrack){
        const auto& trkITSTPC = tracks[iTrack];

        //printf("ITS-TPC track time: %f\n", trkITSTPC.getTimeMUS().getTimeStamp());
        //if (abs(trkITSTPC.getTimeMUS().getTimeStamp() - trdTriggerTimeUS) > 2. ) {
          // track is from different interaction
          //continue;
	// }
        ++nTracks;
        ++countInterestingTracks;
        foundTrackForTrigger = true;
	// if (draw) printf("Found ITS-TPC track with time %f us\n", trkITSTPC.getTimeMUS().getTimeStamp());
        const auto& paramOut = trkITSTPC.getParamOut();
        if (fabs(paramOut.getEta()) > 0.84 || paramOut.getPt() < minPtTrack) {
          // no chance to find tracklets for these tracks
          continue;
        }
        auto param = paramOut; // copy of const object
        for (int iLy = 0; iLy < 6; ++iLy) {
	   if (!prop->PropagateToXBxByBz(param, geo->getTime0(iLy))) {
	    // if (draw) printf("Failed track propagation into layer %i\n", iLy);
            break;
	    }
	    if (!adjustSector(param, prop)) {
	      //  if (draw) printf("Failed track rotation in layer %i\n", iLy);
            break;
	    }
   
	  auto sector = getSector(param.getAlpha());
          auto stack = geo->getStack(param.getZ(), iLy);
          if (stack < 0) {
            continue;
	    }
          auto pp = geo->getPadPlane(iLy, stack);
          auto row = pp->getPadRowNumber(param.getZ());
          int rowMax = (stack == 2) ? 12 : 16;
          if (row < 0 || row >= rowMax) {
	    // if (draw) printf("WARN: row  = %i for z = %f\n", row, param.getZ());
            continue;
          }
          auto col = pp->getPadColNumber(param.getY());
          if (col < 0 || col >= 144) {
            //if (draw) printf("WARN: col  = %i for y = %f\n", col, param.getY());
            continue;
	    }
          ++nPointsTrigger;
	  trackMap.insert(std::make_pair(std::make_tuple(geo->getDetector(iLy, stack, sector), row, col), iTrack));
	   int rowGlb = stack < 3 ? row + stack * 16 : row + 44 + (stack - 3) * 16; // pad row within whole sector
          int colGlb = col + sector * 144; // pad column number from 0 to NSectors * 144
	  //mLayers_tracks[iLy]->SetBinContent(rowGlb+1, colGlb+1, 4);
        }
	}*/ // end ITS-TPC track loop ==================================================
	 
	 // now sort digits to det,row,pad
       
        if (trigger.getNumberOfDigits() > 10000) {
          mDigitsPerEvent->Fill(9999);
        } else {
          mDigitsPerEvent->Fill(trigger.getNumberOfDigits());
        }
        int trackletnumfix = trigger.getNumberOfTracklets();
        int digitnumfix = trigger.getNumberOfDigits();
        if (trigger.getNumberOfTracklets() > 2499)
          trackletnumfix = 2499;
        if (trigger.getNumberOfDigits() > 24999)
          trackletnumfix = 24999;
        mDigitsSizevsTrackletSize->Fill(trigger.getNumberOfTracklets(), trigger.getNumberOfDigits());
	/*  int tbmax = 0;
        int tbhi = 0;
        int tblo = 0;

        int det = 0;
        int row = 0;
        int pad = 0;
        int channel = 0;*/

	//digits loop begins============================================
	for (int iDigit = trigger.getFirstDigit(); iDigit < trigger.getFirstDigit() + trigger.getNumberOfDigits(); ++iDigit)	{
        const auto& digit = digits[iDigit];
        if (digit.isSharedDigit() || digit.getChannel() == 22) {
          continue;
        }
        int adcSum = 0;
        int adcMax = 0;
        // FIXME: the track performs the tracklet fit only for time bins 5..23
        // at least 8 clusters above 50 ADC counts are required for a tracklet to be found
        // this check should be added
        for (int iTb = 5; iTb < 24; ++iTb) {
          auto adc = digit.getADC()[iTb];
          if (adc > adcMax) {
            adcMax = adc;
          }
          adcSum += adc;
        }
        if (adcMax < minAdcCount) continue;
        int det = digit.getDetector();
        int layer = det % 6;
        int stack = (det % 30) / 6;
        int rowGlb = stack < 3 ? digit.getPadRow() + stack * 16 : digit.getPadRow() + 44 + (stack - 3) * 16; // pad row within whole sector
        int sec = det / 30;
        //if (sec == 2) std::cout << digit << std::endl;
        int colGlb = digit.getPadCol() + sec * 144; // pad column number from 0 to NSectors * 144
	// digitMap.insert(std::make_pair(std::make_tuple(det, digit.getPadRow(), digit.getPadCol()), iDigit));
	//mLayers_digits[layer]->SetBinContent(rowGlb+1, colGlb+1, adcMax);
	}//digits loop ends================================================

	//tracklets loop begins
	/*for (int iTrklt = trigger.getFirstTracklet(); iTrklt < trigger.getFirstTracklet() + trigger.getNumberOfTracklets(); ++iTrklt){
        const auto& trklt = tracklets[iTrklt];
        int det = trklt.getDetector();
        int layer = det % 6;
        int stack = (det % 30) / 6;
        int rowGlb = stack < 3 ? trklt.getPadRow() + stack * 16 : trklt.getPadRow() + 44 + (stack - 3) * 16; // pad row within whole sector
        int sec = det / 30;
        //trklt.print();
        int padLocal;
        if (doXOR) {
          padLocal = trklt.getPosition() ^ 0x80;
          if (padLocal & (1 << (o2::trd::constants::NBITSTRKLPOS - 1))) {
            padLocal = -((~(padLocal - 1)) & ((1 << o2::trd::constants::NBITSTRKLPOS) - 1));
          } else {
            padLocal = padLocal & ((1 << o2::trd::constants::NBITSTRKLPOS) - 1);
          }
        } else {
          padLocal = trklt.getPositionBinSigned();
        }
        int mcmCol = (trklt.getMCM() % o2::trd::constants::NMCMROBINCOL) + o2::trd::constants::NMCMROBINCOL * (trklt.getROB() % 2);
        float colGlb = -65.f + mcmCol * ((float) o2::trd::constants::NCOLMCM) + padLocal * o2::trd::constants::GRANULARITYTRKLPOS + 144.f * sec + 72.f;
        int colInChamber = static_cast<int>(std::round(colGlb - 144.f * sec));
        trackletMap.insert(std::make_pair(std::make_tuple(det, trklt.getPadRow(), colInChamber), iTrklt));
        //mLayers_tracklets[layer]->SetBinContent(rowGlb + 1, mLayers_tracklets[layer]->GetYaxis()->FindBin(colGlb), 1);
	}*/ //end of tracklet loop =======================================
	
    }       // end for r
  } // end for d
  } // for loop over digits
}

void PulseHeightPerChamber::endOfCycle()
{
  ILOG(Info) << "endOfCycle" << ENDM;
}

void PulseHeightPerChamber::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info) << "endOfActivity" << ENDM;
}

void PulseHeightPerChamber::reset()
{
  // clean all the monitor objects here
  ILOG(Info) << "Resetting the histogram" << ENDM;
 
  mDigitsPerEvent.get()->Reset();
  //mPulseHeight.get()->Reset();
  //mPulseHeightpro.get()->Reset();
  mDigitsSizevsTrackletSize.get()->Reset();
}

} // namespace o2::quality_control_modules::trd
