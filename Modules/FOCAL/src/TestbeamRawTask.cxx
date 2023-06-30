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
/// \file   TestbeamRawTask.cxx
/// \author My Name
///

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <unordered_set>

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TProfile2D.h>
#include <TMath.h>

#include "QualityControl/QcInfoLogger.h"
#include "FOCAL/TestbeamRawTask.h"
#include <CommonConstants/Triggers.h>
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include <DPLUtils/DPLRawParser.h>
#include <DetectorsRaw/RDHUtils.h>
#include <FOCALCalib/PadBadChannelMap.h>
#include <FOCALCalib/PadPedestal.h>
#include <Headers/DataHeader.h>
#include <Headers/RDHAny.h>

namespace o2::quality_control_modules::focal
{

TestbeamRawTask::~TestbeamRawTask()
{
  if (mTFerrorCounter) {
    delete mTFerrorCounter;
  }
  if (mFEENumberHBF) {
    delete mFEENumberHBF;
  }
  if (mFEENumberTF) {
    delete mFEENumberTF;
  }
  if (mNumLinksTF) {
    delete mNumLinksTF;
  }
  if (mNumHBFPerCRU) {
    delete mNumHBFPerCRU;
  }
  if (mCRUcounter) {
    delete mCRUcounter;
  }
  for (auto& hist : mPadASICChannelADC) {
    delete hist;
  }
  for (auto& hist : mPadASICChannelTOA) {
    delete hist;
  }
  for (auto& hist : mPadASICChannelTOT) {
    delete hist;
  };
  if (mPayloadSizePadsGBT) {
    delete mPayloadSizePadsGBT;
  }
  if (mPayloadSizePixelsGBT) {
    delete mPayloadSizePixelsGBT;
  }
  if (mPayloadSizeTF) {
    delete mPayloadSizeTF;
  }
  if (mLinksWithPayloadPixel) {
    delete mLinksWithPayloadPixel;
  }
  if (mTriggersFeePixel) {
    delete mTriggersFeePixel;
  }
  if (mAverageHitsChipPixel) {
    delete mAverageHitsChipPixel;
  }
  if (mHitsChipPixel) {
    delete mHitsChipPixel;
  }
  if (mPixelHitsTriggerAll) {
    delete mPixelHitsTriggerAll;
  }
  if (mPixelChipsIDsFound) {
    delete mPixelChipsIDsFound;
  }
  if (mPixelChipsIDsHits) {
    delete mPixelChipsIDsHits;
  }
  for (auto& hist : mPixelLaneIDChipIDFEE) {
    delete hist;
  }
  for (auto& hist : mPixelChipHitProfileLayer) {
    delete hist;
  }
  for (auto& hist : mPixelChipHitmapLayer) {
    delete hist;
  }
  for (auto& hist : mPixelSegmentHitProfileLayer) {
    delete hist;
  }
  for (auto& hist : mPixelSegmentHitmapLayer) {
    delete hist;
  }
  for (auto& hist : mPixelHitDistribitionLayer) {
    delete hist;
  }
  for (auto& hist : mPixelHitsTriggerLayer) {
    delete hist;
  }
}

void TestbeamRawTask::default_init()
{
  std::fill(mPadASICChannelADC.begin(), mPadASICChannelADC.end(), nullptr);
  std::fill(mPadASICChannelTOA.begin(), mPadASICChannelTOA.end(), nullptr);
  std::fill(mPadASICChannelTOT.begin(), mPadASICChannelTOT.end(), nullptr);
  std::fill(mHitMapPadASIC.begin(), mHitMapPadASIC.end(), nullptr);
  std::fill(mPadTOTSumASIC.begin(), mPadTOTSumASIC.end(), nullptr);
  std::fill(mPadADCSumASIC.begin(), mPadADCSumASIC.end(), nullptr);
  std::fill(mPadTOTCorrASIC.begin(), mPadTOTCorrASIC.end(), nullptr);
  std::fill(mPadADCCorrASIC.begin(), mPadADCCorrASIC.end(), nullptr);

  std::fill(mPixelLaneIDChipIDFEE.begin(), mPixelLaneIDChipIDFEE.end(), nullptr);
  std::fill(mPixelChipHitProfileLayer.begin(), mPixelChipHitProfileLayer.end(), nullptr);
  std::fill(mPixelChipHitmapLayer.begin(), mPixelChipHitmapLayer.end(), nullptr);
  std::fill(mPixelSegmentHitProfileLayer.begin(), mPixelSegmentHitProfileLayer.end(), nullptr);
  std::fill(mPixelSegmentHitmapLayer.begin(), mPixelSegmentHitmapLayer.end(), nullptr);
  std::fill(mPixelHitDistribitionLayer.begin(), mPixelHitDistribitionLayer.end(), nullptr);
  std::fill(mPixelHitsTriggerLayer.begin(), mPixelHitsTriggerLayer.end(), nullptr);
  std::fill(mPadTRIGvsWindowASIC.begin(), mPadTRIGvsWindowASIC.end(), nullptr);
}

void TestbeamRawTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  auto get_bool = [](const std::string_view input) -> bool {
    return input == "true";
  };
  int winDur = 20;
  auto hasWinDur = mCustomParameters.find("WinDur");
  if (hasWinDur != mCustomParameters.end()) {
    winDur = std::stoi(hasWinDur->second);
  }
  mPadDecoder.setTriggerWinDur(winDur);

  auto hasDebugParam = mCustomParameters.find("Debug");
  if (hasDebugParam != mCustomParameters.end()) {
    mDebugMode = get_bool(hasDebugParam->second);
  }

  auto hasDisablePads = mCustomParameters.find("DisablePads");
  if (hasDisablePads != mCustomParameters.end()) {
    mDisablePads = get_bool(hasDisablePads->second);
  }
  auto hasDisablePixels = mCustomParameters.find("DisablePixels");
  if (hasDisablePixels != mCustomParameters.end()) {
    mDisablePixels = get_bool(hasDisablePixels->second);
  }
  auto hasPadPedestalSubtraction = mCustomParameters.find("SubtractPadPedestals");
  if (hasPadPedestalSubtraction != mCustomParameters.end()) {
    mEnablePedestalSubtraction = get_bool(hasPadPedestalSubtraction->second);
  }
  auto hasPadBadChannelMap = mCustomParameters.find("PadBadChannelMasking");
  if (hasPadBadChannelMap != mCustomParameters.end()) {
    mEnableBadChannelMask = get_bool(hasPadBadChannelMap->second);
  }

  mChannelsPadProjections = { 52, 16, 19, 46, 59, 14, 42 };
  if (!mDisablePads) {
    auto hasChannelsPadProjections = mCustomParameters.find("ChannelsPadProjections");
    if (hasChannelsPadProjections != mCustomParameters.end()) {
      std::stringstream parser(hasChannelsPadProjections->second);
      std::string buffer;
      while (std::getline(parser, buffer, ',')) {
        mChannelsPadProjections.emplace_back(std::stoi(buffer));
      }
    }
    std::sort(mChannelsPadProjections.begin(), mChannelsPadProjections.end(), std::less<int>());

    auto hasCutTOT = mCustomParameters.find("PadCutTOT");
    if (hasCutTOT != mCustomParameters.end()) {
      mPadTOTCutADC = std::stoi(hasCutTOT->second);
    }
  }

  default_init();

  ILOG(Info, Support) << "Pad QC:     " << (mDisablePads ? "disabled" : "enabled") << ENDM;
  ILOG(Info, Support) << "Pixel QC:   " << (mDisablePixels ? "disabled" : "enabled") << ENDM;
  if (!mDisablePads) {
    ILOG(Info, Support) << "Window dur:  " << winDur << ENDM;
    std::stringstream channelstring;
    bool firstchannel = true;
    for (auto chan : mChannelsPadProjections) {
      if (firstchannel) {
        firstchannel = false;
      } else {
        channelstring << ", ";
      }
      channelstring << chan;
    }
    ILOG(Info, Support) << "Selected channels for pad ADC projections: " << channelstring.str() << ENDM;
  }
  ILOG(Info, Support) << "Debug mode: " << (mDebugMode ? "yes" : "no") << ENDM;

  if (!mDisablePixels) {
    o2::focal::PixelMapper::MappingType_t mappingtype = o2::focal::PixelMapper::MappingType_t::MAPPING_IB;
    auto pixellayout = mCustomParameters.find("Pixellayout");
    if (pixellayout != mCustomParameters.end()) {
      if (pixellayout->second == "IB") {
        mappingtype = o2::focal::PixelMapper::MappingType_t::MAPPING_IB;
      } else if (pixellayout->second == "OB") {
        mappingtype = o2::focal::PixelMapper::MappingType_t::MAPPING_OB;
      } else {
        ILOG(Fatal, Support) << "Unknown pixel setup: " << pixellayout->second << ENDM;
      }
    }
    switch (mappingtype) {
      case o2::focal::PixelMapper::MappingType_t::MAPPING_IB:
        ILOG(Info, Support) << "Using pixel layout: IB" << ENDM;
        break;

      case o2::focal::PixelMapper::MappingType_t::MAPPING_OB:
        ILOG(Info, Support) << "Using pixel layout: OB" << ENDM;
        break;
      default:
        break;
    }
    std::string mappingfile;
    auto mappingfilename = mCustomParameters.find("Pixelmapping");
    if (mappingfilename != mCustomParameters.end()) {
      mappingfile = mappingfilename->second;
      ILOG(Info, Support) << "Using pixel chip mapping from file: " << mappingfile << ENDM;
    }
    mPixelMapper = std::make_unique<o2::focal::PixelMapper>(mappingfile.length() ? o2::focal::PixelMapper::MappingType_t::MAPPING_UNKNOWN : mappingtype);
    if (mappingfile.length()) {
      try {
        mPixelMapper->setMappingFile(mappingfile, mappingtype);
      } catch (o2::focal::PixelMapper::MappingNotSetException& e) {
        ILOG(Fatal, Support) << "Unable to initialize pixel chip mapping: " << e << ENDM;
      }
    }
  }

  /////////////////////////////////////////////////////////////////
  /// general histograms
  /////////////////////////////////////////////////////////////////
  mTFerrorCounter = new TH1F("NumberOfTFerror", "Number of TFbuilder errors", 2, 0.5, 2.5);
  mTFerrorCounter->GetYaxis()->SetTitle("Time Frame Builder Error");
  mTFerrorCounter->GetXaxis()->SetBinLabel(1, "empty");
  mTFerrorCounter->GetXaxis()->SetBinLabel(2, "filled");
  getObjectsManager()->startPublishing(mTFerrorCounter);

  mFEENumberHBF = new TH1F("NumberOfHBFPerFEE", "Number of HBFs per FEE", 100001, -0.5, 100000.5);
  mFEENumberHBF->GetXaxis()->SetTitle("FEE ID");
  mFEENumberHBF->GetYaxis()->SetTitle("Number of HBFs");
  getObjectsManager()->startPublishing(mFEENumberHBF);

  mFEENumberTF = new TH1F("NumberOfTFPerFEE", "Number of TFs per FEE", 100001, -0.5, 100000.5);
  mFEENumberTF->GetXaxis()->SetTitle("FEE ID");
  mFEENumberTF->GetYaxis()->SetTitle("Number of TFs");
  getObjectsManager()->startPublishing(mFEENumberTF);

  mNumLinksTF = new TH1F("NumberOfLinksPerTF", "Number of Links / timeframe", 101, -0.5, 100.5);
  mNumLinksTF->GetXaxis()->SetTitle("Number of links");
  mNumLinksTF->GetYaxis()->SetTitle("Number of TFs");
  getObjectsManager()->startPublishing(mNumLinksTF);

  mNumHBFPerCRU = new TH1F("NumberOfHBFPerCRU", "Number of HBFs / CRU", 10001, -0.5, 10000.5);
  mNumHBFPerCRU->GetXaxis()->SetTitle("Endpoint ID");
  mNumHBFPerCRU->GetYaxis()->SetTitle("Link ID");
  getObjectsManager()->startPublishing(mNumHBFPerCRU);

  mCRUcounter = new TH2D("CRUcounter", "Number of HBFs per CRU link", 2, -0.5, 1.5, 21, -0.5, 20.5);
  mCRUcounter->GetXaxis()->SetTitle("Endpoint ID");
  mCRUcounter->GetYaxis()->SetTitle("Link ID");
  getObjectsManager()->startPublishing(mCRUcounter);

  mPayloadSizeTF = new TH1D("PayloadSizeTF", "Payload size TF", 10000, 0., 10000.);
  getObjectsManager()->startPublishing(mPayloadSizeTF);

  /////////////////////////////////////////////////////////////////
  /// PAD histograms
  /////////////////////////////////////////////////////////////////

  if (!mDisablePads) {
    constexpr int PAD_CHANNELS = 76;
    constexpr int RANGE_ADC = 1024;
    constexpr int RANGE_TOA = 1024;
    constexpr int RANGE_TOT = 4096;

    mPadTOTSumGlobal = new TH1D("PadTOTSumGlobal", "Sum of all TOTs in a time frame; TOT sum; counts", 250, 0., 50000);
    mPadTOTSumGlobal->SetStats(false);
    mPadADCSumGlobal = new TH1D("PadADCSumGlobal", "Sum of all ADCs in a time frame; ADC sum; counts", 250, 0., 200E3);
    mPadADCSumGlobal->SetStats(false);

    getObjectsManager()->startPublishing(mPadTOTSumGlobal);
    getObjectsManager()->startPublishing(mPadADCSumGlobal);

    for (int iasic = 0; iasic < PAD_ASICS; iasic++) {
      mPadASICChannelADC[iasic] = new TH2D(Form("PadADC_ASIC_%d", iasic), Form("ADC vs. channel ID for ASIC %d; channel ID; ADC", iasic), PAD_CHANNELS, -0.5, PAD_CHANNELS - 0.5, RANGE_ADC, 0., RANGE_ADC);
      mPadASICChannelADC[iasic]->SetStats(false);
      mPadASICChannelTOA[iasic] = new TH2D(Form("PadTOA_ASIC_%d", iasic), Form("TOA vs. channel ID for ASIC %d; channel ID; TOA", iasic), PAD_CHANNELS, -0.5, PAD_CHANNELS - 0.5, RANGE_TOA, 0., RANGE_TOA);
      mPadASICChannelTOA[iasic]->SetStats(false);
      mPadASICChannelTOT[iasic] = new TH2D(Form("PadTOT_ASIC_%d", iasic), Form("TOT vs. channel ID for ASIC %d; channel ID; TOT", iasic), PAD_CHANNELS, -0.5, PAD_CHANNELS - 0.5, RANGE_TOT / 4, 0., RANGE_TOT);
      mPadASICChannelTOT[iasic]->SetStats(false);
      mHitMapPadASIC[iasic] = new TProfile2D(Form("HitmapPadASIC_%d", iasic), Form("Hitmap for ASIC %d; col; row", iasic), o2::focal::PadMapper::NROW, -0.5, o2::focal::PadMapper::NROW - 0.5, o2::focal::PadMapper::NCOLUMN, -0.5, o2::focal::PadMapper::NCOLUMN - 0.5);
      mHitMapPadASIC[iasic]->SetStats(false);

      mPadTOTSumASIC[iasic] = new TH1D(Form("PadTOTSumASIC%d", iasic), Form("TOT sum for ASIC %d; TOT ASIC %d", iasic, iasic), 300, 0., 6000);
      mPadTOTSumASIC[iasic]->SetStats(false);

      mPadADCSumASIC[iasic] = new TH1D(Form("PadADCSumASIC_%d", iasic), Form("ADC sum for ASIC %d; ADC ASIC %d", iasic, iasic), 400, 0., 16000);
      mPadADCSumASIC[iasic]->SetStats(false);

      mPadTOTCorrASIC[iasic] = new TH2D(Form("PadTOTCorrASIC_%d_%d", iasic + 1, iasic), Form("TOT ASIC %d vs. ASIC %d; TOT ASIC %d; TOT ASIC %d", iasic, iasic + 1, iasic, iasic + 1), 300, 0., 6000, 300, 0., 6000);
      mPadTOTCorrASIC[iasic]->SetStats(false);

      mPadADCCorrASIC[iasic] = new TH2D(Form("PadADCCorrASIC_%d_%d", iasic + 1, iasic), Form("ADC ASIC %d vs. ASIC %d; ADC ASIC %d; ADC ASIC %d", iasic, iasic + 1, iasic, iasic + 1), 400, 0., 16000, 400, 0., 16000);
      mPadADCCorrASIC[iasic]->SetStats(false);

      mPadTRIGvsWindowASIC[iasic] = new TH2D(Form("PadTRIGvsWindowASIC_%d", iasic), Form("TRIG vs. Window ASIC %d; Window; Trig", iasic), 20, 0., 20., 128, 0., 128);
      mPadTRIGvsWindowASIC[iasic]->SetStats(false);

      getObjectsManager()->startPublishing(mPadASICChannelADC[iasic]);
      getObjectsManager()->startPublishing(mPadASICChannelTOA[iasic]);
      getObjectsManager()->startPublishing(mPadASICChannelTOT[iasic]);
      getObjectsManager()->startPublishing(mHitMapPadASIC[iasic]);
      getObjectsManager()->startPublishing(mPadTOTSumASIC[iasic]);
      getObjectsManager()->startPublishing(mPadADCSumASIC[iasic]);
      getObjectsManager()->startPublishing(mPadTOTCorrASIC[iasic]);
      getObjectsManager()->startPublishing(mPadADCCorrASIC[iasic]);
      getObjectsManager()->startPublishing(mPadTRIGvsWindowASIC[iasic]);

      mPadChannelProjections[iasic] = std::make_unique<PadChannelProjections>();
      mPadChannelProjections[iasic]->init(mChannelsPadProjections, iasic);
      mPadChannelProjections[iasic]->startPublishing(*getObjectsManager());
    }

    mPayloadSizePadsGBT = new TH1D("PayloadSizePadGBT", "Payload size GBT words", 10000, 0., 10000.);
    getObjectsManager()->startPublishing(mPayloadSizePadsGBT);

    // Pad average TOA per ASIC for all channels with TOA>0
    mPadTOAvsASIC = new TH2D("PadTOAvsASIC", "average Pad TOA vs. ASIC (for TOA>0); TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC->SetStats(false);
    getObjectsManager()->startPublishing(mPadTOAvsASIC);

    mPadTOAvsASIC_Ch14 = new TH2D("PadTOAvsASIC_Ch14", "TOA vs. ASIC for channel 14; TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC_Ch16 = new TH2D("PadTOAvsASIC_Ch16", "TOA vs. ASIC for channel 16; TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC_Ch19 = new TH2D("PadTOAvsASIC_Ch19", "TOA vs. ASIC for channel 19; TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC_Ch48 = new TH2D("PadTOAvsASIC_Ch48", "TOA vs. ASIC for channel 48; TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC_Ch52 = new TH2D("PadTOAvsASIC_Ch52", "TOA vs. ASIC for channel 52; TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC_Ch61 = new TH2D("PadTOAvsASIC_Ch61", "TOA vs. ASIC for channel 61; TOA; ASIC NO.", 512, 0., 1024., PAD_ASICS, 0, PAD_ASICS);
    mPadTOAvsASIC_Ch14->SetStats(false);
    mPadTOAvsASIC_Ch16->SetStats(false);
    mPadTOAvsASIC_Ch19->SetStats(false);
    mPadTOAvsASIC_Ch48->SetStats(false);
    mPadTOAvsASIC_Ch52->SetStats(false);
    mPadTOAvsASIC_Ch61->SetStats(false);
    getObjectsManager()->startPublishing(mPadTOAvsASIC_Ch14);
    getObjectsManager()->startPublishing(mPadTOAvsASIC_Ch16);
    getObjectsManager()->startPublishing(mPadTOAvsASIC_Ch19);
    getObjectsManager()->startPublishing(mPadTOAvsASIC_Ch48);
    getObjectsManager()->startPublishing(mPadTOAvsASIC_Ch52);
    getObjectsManager()->startPublishing(mPadTOAvsASIC_Ch61);

    mPadGlobalMIPADC_Ch14_Asic0 = new TH1D("PadGlobalMIPADC_Ch14_Asic0", "Pad global ADC after CMN sub for channel 14, ASIC 0; ADC", 130, -30., 100.);
    mPadGlobalMIPADC_Ch16_Asic0 = new TH1D("PadGlobalMIPADC_Ch16_Asic0", "Pad global ADC after CMN sub for channel 16, ASIC 0; ADC", 130, -30., 100.);
    mPadGlobalMIPADC_Ch19_Asic0 = new TH1D("PadGlobalMIPADC_Ch19_Asic0", "Pad global ADC after CMN sub for channel 19, ASIC 0; ADC", 130, -30., 100.);
    mPadGlobalMIPADC_Ch48_Asic0 = new TH1D("PadGlobalMIPADC_Ch48_Asic0", "Pad global ADC after CMN sub for channel 48, ASIC 0; ADC", 130, -30., 100.);
    mPadGlobalMIPADC_Ch52_Asic0 = new TH1D("PadGlobalMIPADC_Ch52_Asic0", "Pad global ADC after CMN sub for channel 52, ASIC 0; ADC", 130, -30., 100.);
    mPadGlobalMIPADC_Ch61_Asic0 = new TH1D("PadGlobalMIPADC_Ch61_Asic0", "Pad global ADC after CMN sub for channel 61, ASIC 0; ADC", 130, -30., 100.);
    mPadGlobalMIPADC_Ch14_Asic0->SetStats(false);
    mPadGlobalMIPADC_Ch16_Asic0->SetStats(false);
    mPadGlobalMIPADC_Ch19_Asic0->SetStats(false);
    mPadGlobalMIPADC_Ch48_Asic0->SetStats(false);
    mPadGlobalMIPADC_Ch52_Asic0->SetStats(false);
    mPadGlobalMIPADC_Ch61_Asic0->SetStats(false);
    getObjectsManager()->startPublishing(mPadGlobalMIPADC_Ch14_Asic0);
    getObjectsManager()->startPublishing(mPadGlobalMIPADC_Ch16_Asic0);
    getObjectsManager()->startPublishing(mPadGlobalMIPADC_Ch19_Asic0);
    getObjectsManager()->startPublishing(mPadGlobalMIPADC_Ch48_Asic0);
    getObjectsManager()->startPublishing(mPadGlobalMIPADC_Ch52_Asic0);
    getObjectsManager()->startPublishing(mPadGlobalMIPADC_Ch61_Asic0);

    mPadGlobalTOAvsADC = new TH2D("PadGlobalTOAvsADC", "Pad average TOA vs. sum ADC; sum ADC; average TOA", 250, 0., 200E3, 512, 0., 1024.);
    mPadGlobalTOAvsADC->SetStats(false);
    getObjectsManager()->startPublishing(mPadGlobalTOAvsADC);
    mPadGlobalTOTvsADC = new TH2D("PadGlobalTOTvsADC", "Pad sum TOT vs. sum ADC; sum ADC; sum TOT", 250, 0., 200E3, 250, 0., 50000);
    mPadGlobalTOTvsADC->SetStats(false);
    getObjectsManager()->startPublishing(mPadGlobalTOTvsADC);
  }

  /////////////////////////////////////////////////////////////////
  /// Pixel histograms
  /////////////////////////////////////////////////////////////////
  if (!mDisablePixels) {
    constexpr int FEES = 30;
    constexpr int MAX_CHIPS = 14;    // For the moment leave completely open
    constexpr int MAX_TRIGGERS = 10; // Number of triggers / HBF usually sparse
    mLinksWithPayloadPixel = new TH1D("Pixel_PagesFee", "HBF vs. FEE ID; FEE ID; HBFs", FEES, -0.5, FEES - 0.5);
    getObjectsManager()->startPublishing(mLinksWithPayloadPixel);
    mTriggersFeePixel = new TH2D("Pixel_NumTriggerHBF", "Number of triggers per HBF and FEE; FEE ID; Number of triggers / HBF", FEES, -0.5, FEES - 0.5, MAX_TRIGGERS, -0.5, MAX_TRIGGERS - 0.5);
    mTriggersFeePixel->SetStats(false);
    getObjectsManager()->startPublishing(mTriggersFeePixel);
    mAverageHitsChipPixel = new TProfile2D("Pixel_AverageNumberOfHitsChip", "Average number of hits / chip", FEES, -0.5, FEES - 0.5, MAX_CHIPS, -0.5, MAX_CHIPS - 0.5);
    mAverageHitsChipPixel->SetStats(false);
    getObjectsManager()->startPublishing(mAverageHitsChipPixel);
    mHitsChipPixel = new TH1D("Pixel_NumberHits", "Number of hits / chip", 50, 0., 50);
    mHitsChipPixel->SetStats(false);
    getObjectsManager()->startPublishing(mHitsChipPixel);
    mPixelHitsTriggerAll = new TH1D("Pixel_TotalNumberHitsTrigger", "Number of hits per trigger; Number of hits; Number of events", 1500, -0.5, 15000);
    mPixelHitsTriggerAll->SetStats(false);
    getObjectsManager()->startPublishing(mPixelHitsTriggerAll);
    mPixelChipsIDsFound = new TH2D("Pixel_ChipIDsFEE", "Chip ID vs. FEE ID", FEES, -0.5, FEES - 0.5, MAX_CHIPS, -0.5, MAX_CHIPS - 0.5);
    mPixelChipsIDsFound->SetDirectory(nullptr);
    getObjectsManager()->startPublishing(mPixelChipsIDsFound);
    mPixelChipsIDsHits = new TH2D("Pixel_ChipIDsFilledFEE", "Chip ID with hits vs. FEE ID", FEES, -0.5, FEES - 0.5, MAX_CHIPS, -0.5, MAX_CHIPS - 0.5);
    mPixelChipsIDsHits->SetDirectory(nullptr);
    getObjectsManager()->startPublishing(mPixelChipsIDsHits);

    mPayloadSizePixelsGBT = new TH1D("PayloadSizePixelsGBT", "Payload size GBT words", 10000, 0., 10000.);
    getObjectsManager()->startPublishing(mPayloadSizePixelsGBT);

    for (int ifee = 0; ifee < 4; ifee++) {
      mPixelLaneIDChipIDFEE[ifee] = new TH2D(Form("Pixel_LaneIDChipID_FEE%d", ifee), Form("Lane ID vs. Chip ID for FEE %d; Lane ID; ChipID", ifee), 57, -0.5, 56.5, 31, -0.5, 30.5);
      mPixelLaneIDChipIDFEE[ifee]->SetDirectory(nullptr);
      getObjectsManager()->startPublishing(mPixelLaneIDChipIDFEE[ifee]);
    }

    constexpr int PIXEL_LAYERS = 2;
    auto pixel_segments = getNumberOfPixelSegments(mPixelMapper->getMappingType());
    auto pixel_columns = mPixelMapper->getNumberOfColumns(),
         pixel_rows = mPixelMapper->getNumberOfRows(),
         pixel_chips = pixel_columns * pixel_rows,
         segments_colums = pixel_columns * pixel_segments.first,
         segments_rows = pixel_rows * pixel_segments.second;
    ILOG(Info, Support) << "Pixel hitmap, segments per chip (" << pixel_segments.first << " columns, " << pixel_segments.second << " rows), segment columns " << segments_colums << ", rows " << segments_rows << ENDM;
    std::array<int, 2> pixelLayerIndex = { { 10, 5 } };
    ILOG(Info, Support) << "Setup acceptance histograms " << pixel_columns << " colums and " << pixel_rows << " rows (" << pixel_chips << " chips)" << ENDM;
    for (int ilayer = 0; ilayer < PIXEL_LAYERS; ilayer++) {
      mPixelChipHitProfileLayer[ilayer] = new TProfile2D(Form("Pixel_Hitprofile_%d", ilayer), Form("Pixel hit profile in layer %d; X; Y", pixelLayerIndex[ilayer]), pixel_columns, -0.5, pixel_columns - 0.5, pixel_rows, -0.5, pixel_rows - 0.5);
      mPixelChipHitProfileLayer[ilayer]->SetStats(false);
      getObjectsManager()->startPublishing(mPixelChipHitProfileLayer[ilayer]);
      mPixelChipHitmapLayer[ilayer] = new TH2D(Form("Pixel_Hitmap_%d", ilayer), Form("Pixel hitmap in layer %d; X; Y", pixelLayerIndex[ilayer]), pixel_columns, -0.5, pixel_columns - 0.5, pixel_rows, -0.5, pixel_rows - 0.5);
      mPixelChipHitmapLayer[ilayer]->SetStats(false);
      getObjectsManager()->startPublishing(mPixelChipHitmapLayer[ilayer]);
      mPixelSegmentHitProfileLayer[ilayer] = new TProfile2D(Form("Pixel_Segment_Hitprofile_%d", ilayer), Form("Pixel hit profile in layer %d; X; Y", pixelLayerIndex[ilayer]), segments_colums, -0.5, segments_colums - 0.5, segments_rows, -0.5, segments_rows - 0.5);
      mPixelSegmentHitProfileLayer[ilayer]->SetStats(false);
      getObjectsManager()->startPublishing(mPixelSegmentHitProfileLayer[ilayer]);
      mPixelSegmentHitmapLayer[ilayer] = new TH2D(Form("Pixel_Segment_Hitmap_%d", ilayer), Form("Pixel hitmap in layer %d; X; Y", pixelLayerIndex[ilayer]), segments_colums, -0.5, segments_colums - 0.5, segments_rows, -0.5, segments_rows - 0.5);
      mPixelSegmentHitmapLayer[ilayer]->SetStats(false);
      getObjectsManager()->startPublishing(mPixelSegmentHitmapLayer[ilayer]);
      mPixelHitDistribitionLayer[ilayer] = new TH2D(Form("Pixel_Hitdist_%d", ilayer), Form("Pixel hit distribution in layer %d; Number of hits / chip; Number of events", pixelLayerIndex[ilayer]), pixel_chips, -0.5, pixel_chips - 0.5, 101, -0.5, 100.5);
      mPixelHitDistribitionLayer[ilayer]->SetStats(false);
      getObjectsManager()->startPublishing(mPixelHitDistribitionLayer[ilayer]);
      mPixelHitsTriggerLayer[ilayer] = new TH1D(Form("Pixel_NumberHitsTrigger_%d", ilayer), Form("Number of hits per trigger in layer %d; Number of hits / layer; Number of events", pixelLayerIndex[ilayer]), 1500, 0, 15000.);
      mPixelHitsTriggerLayer[ilayer]->SetStats(false);
      getObjectsManager()->startPublishing(mPixelHitsTriggerLayer[ilayer]);
    }
    mHitSegmentCounter.resize(segments_colums * segments_rows);
  }
}

void TestbeamRawTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
  // Pedestal come from pedestal runs, usually not updated during the run
  std::map<std::string, std::string> metadata;
  if (mEnablePedestalSubtraction) {
    mPadPedestalHandler = retrieveConditionAny<o2::focal::PadPedestal>("FOC/Calib/PadPedestals", metadata);
    if (mPadPedestalHandler) {
      ILOG(Info, Support) << "Pedestal data found - pedestals will be subtracted for Pads" << ENDM;
    } else {
      ILOG(Error, Support) << "No pedestal data found - pedestal subtraction not possible" << ENDM;
    }
  }
  if (mEnableBadChannelMask) {
    mPadBadChannelMap = retrieveConditionAny<o2::focal::PadBadChannelMap>("FOC/Calib/PadBadChannelMap", metadata);
    if (mPadPedestalHandler) {
      ILOG(Info, Support) << "Bad channel map for pads found - bad pad channels will be masked" << ENDM;
    } else {
      ILOG(Error, Support) << "No bad channel map found for pads - bad channel mask cannot be applied" << ENDM;
    }
  }
}

void TestbeamRawTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TestbeamRawTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  ILOG(Info, Support) << "Called" << ENDM;
  ILOG(Info, Support) << "Received " << ctx.inputs().size() << " inputs" << ENDM;
  mPixelNHitsAll.clear();
  for (auto& hitcounterLayer : mPixelNHitsLayer) {
    hitcounterLayer.clear();
  }

  if (isLostTimeframe(ctx)) {
    mTFerrorCounter->Fill(1);
    return;
  }
  mTFerrorCounter->Fill(2);

  std::unordered_set<uint64_t> feeIDs;

  int inputs = 0;
  std::vector<char> rawbuffer;
  uint16_t currentfee = 0;
  for (const auto& rawData : framework::InputRecordWalker(ctx.inputs())) {
    if (rawData.header != nullptr && rawData.payload != nullptr) {
      const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(rawData);
      mPayloadSizeTF->Fill(payloadSize);
      auto header = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(rawData);
      ILOG(Debug, Support) << "Channel " << header->dataOrigin.str << "/" << header->dataDescription.str << "/" << header->subSpecification << ENDM;

      gsl::span<const char> databuffer(rawData.payload, payloadSize);
      ILOG(Debug, Support) << "Payload buffer has size " << databuffer.size() << ENDM;
      int currentpos = 0;
      while (currentpos < databuffer.size()) {
        auto rdh = reinterpret_cast<const o2::header::RDHAny*>(databuffer.data() + currentpos);
        if (mDebugMode) {
          o2::raw::RDHUtils::printRDH(rdh);
        }

        if (o2::raw::RDHUtils::getMemorySize(rdh) > o2::raw::RDHUtils::getHeaderSize(rdh)) {
          // non-0 payload size:
          auto payloadsize = o2::raw::RDHUtils::getMemorySize(rdh) - o2::raw::RDHUtils::getHeaderSize(rdh);
          auto fee = static_cast<int>(o2::raw::RDHUtils::getFEEID(rdh));
          ILOG(Debug, Support) << "Next RDH: " << ENDM;
          stringstream ss;
          ss << "Found fee                   0x" << std::hex << fee << std::dec << " (System " << (fee == 0xcafe ? "Pads" : "Pixels") << ")";
          ILOG(Debug, Support) << ss.str() << ENDM;
          ILOG(Debug, Support) << "Found trigger BC:           " << o2::raw::RDHUtils::getTriggerBC(rdh) << ENDM;
          ILOG(Debug, Support) << "Found trigger Oribt:        " << o2::raw::RDHUtils::getTriggerOrbit(rdh) << ENDM;
          ILOG(Debug, Support) << "Found payload size:         " << payloadsize << ENDM;
          ILOG(Debug, Support) << "Found offset to next:       " << o2::raw::RDHUtils::getOffsetToNext(rdh) << ENDM;
          ILOG(Debug, Support) << "Stop bit:                   " << (o2::raw::RDHUtils::getStop(rdh) ? "yes" : "no") << ENDM;
          ILOG(Debug, Support) << "Number of GBT words:        " << (payloadsize * sizeof(char) / (fee == 0xcafe ? sizeof(o2::focal::PadGBTWord) : sizeof(o2::itsmft::GBTWord))) << ENDM;
          auto page_payload = databuffer.subspan(currentpos + o2::raw::RDHUtils::getHeaderSize(rdh), payloadsize);
          std::copy(page_payload.begin(), page_payload.end(), std::back_inserter(rawbuffer));
        }

        auto trigger = o2::raw::RDHUtils::getTriggerType(rdh);
        if (trigger & o2::trigger::SOT || trigger & o2::trigger::HB) {
          if (o2::raw::RDHUtils::getStop(rdh)) {
            ILOG(Debug, Support) << "Stop bit received - processing payload" << ENDM;
            mFEENumberHBF->Fill(currentfee);
            mNumHBFPerCRU->Fill(o2::raw::RDHUtils::getCRUID(rdh));
            mCRUcounter->Fill(o2::raw::RDHUtils::getEndPointID(rdh), o2::raw::RDHUtils::getLinkID(rdh));
            auto fee_found = feeIDs.find(currentfee);
            if (fee_found == feeIDs.end()) {
              feeIDs.insert(currentfee);
            }
            // Data ready
            if (currentfee == 0xcafe) { // Use FEE ID 0xcafe for PAD data
              // Pad data
              if (!mDisablePads) {
                ILOG(Debug, Support) << "Processing PAD data" << ENDM;
                auto payloadsizeGBT = rawbuffer.size() * sizeof(char) / sizeof(o2::focal::PadGBTWord);
                mPayloadSizePadsGBT->Fill(payloadsizeGBT);
                ILOG(Debug) << "Found pixel payload of size " << rawbuffer.size() << " (" << payloadsizeGBT << " GBT words)" << ENDM;
                processPadPayload(gsl::span<const o2::focal::PadGBTWord>(reinterpret_cast<const o2::focal::PadGBTWord*>(rawbuffer.data()), payloadsizeGBT));
              }
            } else { // All other FEEs are pixel FEEs
              // Pixel data
              if (!mDisablePixels) {
                auto feeID = o2::raw::RDHUtils::getFEEID(rdh);
                ILOG(Debug, Support) << "Processing Pixel data from FEE " << feeID << ENDM;
                auto payloadsizeGBT = rawbuffer.size() * sizeof(char) / sizeof(o2::itsmft::GBTWord);
                mPayloadSizePixelsGBT->Fill(payloadsizeGBT);
                ILOG(Debug, Support) << "Found pixel payload of size " << rawbuffer.size() << " (" << payloadsizeGBT << " GBT words)" << ENDM;
                processPixelPayload(gsl::span<const o2::itsmft::GBTWord>(reinterpret_cast<const o2::itsmft::GBTWord*>(rawbuffer.data()), payloadsizeGBT), feeID);
              }
            }
            rawbuffer.clear();
          } else {
            ILOG(Debug, Support) << "New HBF or Timeframe" << ENDM;
            currentfee = o2::raw::RDHUtils::getFEEID(rdh);
            stringstream ss;
            ss << "Using FEE ID: 0x" << std::hex << currentfee << std::dec;
            ILOG(Debug, Support) << ss.str() << ENDM;
          }
        }
        currentpos += o2::raw::RDHUtils::getOffsetToNext(rdh);
      }
    } else {
      ILOG(Error, Support) << "Input " << inputs << ": Either header or payload is nullptr" << ENDM;
    }
    inputs++;
  }

  // Fill number of hit/trigger histogram of the pixels
  for (auto& [trg, nhits] : mPixelNHitsAll) {
    mPixelHitsTriggerAll->Fill(nhits);
  }
  for (int ilayer = 0; ilayer < 2; ilayer++) {
    for (auto& [trg, nhits] : mPixelNHitsLayer[ilayer]) {
      mPixelHitsTriggerLayer[ilayer]->Fill(nhits);
    }
  }
  mNumLinksTF->Fill(feeIDs.size());
  for (auto& fee : feeIDs) {
    ILOG(Debug, Support) << "Found fee: " << fee << ENDM;
    mFEENumberTF->Fill(fee);
  }
}

void TestbeamRawTask::processPadPayload(gsl::span<const o2::focal::PadGBTWord> padpayload)
{
  // processPadEvent(padpayload);

  constexpr std::size_t EVENTSIZEPADGBT = 1180;
  int nevents = padpayload.size() / EVENTSIZEPADGBT;
  for (int iev = 0; iev < nevents; iev++) {
    processPadEvent(padpayload.subspan(iev * EVENTSIZEPADGBT, EVENTSIZEPADGBT));
  }
}

void TestbeamRawTask::processPadEvent(gsl::span<const o2::focal::PadGBTWord> padpayload)
{
  // adc vs. TOT
  // adc vs. TOA
  // adc for MIP -30 to 100 in 130bin
  mPadDecoder.reset();
  mPadDecoder.decodeEvent(padpayload);
  const auto& eventdata = mPadDecoder.getData();
  double totglobalsum = 0;
  double adcglobalsum = 0;
  double toaglobalaverage = 0; // toa average of all layers

  std::array<double, PAD_ASICS> kTOTsum = { 0 };
  std::array<double, PAD_ASICS> kADCsum = { 0 };
  double cmn = 0;
std:
  array<double, 66> kADCForCMN = { 0 };
  for (int iasic = 0; iasic < PAD_ASICS; iasic++) {
    const auto& asic = eventdata[iasic].getASIC();
    ILOG(Debug, Support) << "ASIC " << iasic << ", Header 0: " << asic.getFirstHeader() << ENDM;
    ILOG(Debug, Support) << "ASIC " << iasic << ", Header 1: " << asic.getSecondHeader() << ENDM;
    int currentchannel = 0;
    double totsum = 0;
    double adcsum = 0;

    double adc_ch14 = 0;
    double adc_ch16 = 0;
    double adc_ch19 = 0;
    double adc_ch48 = 0;
    double adc_ch52 = 0;
    double adc_ch61 = 0;

    double asicAverageTOA = 0; // average TOA of all channels with TOA>0
    int nChWithTOA = 0;        // number of ASICs with TOA>0
    int icmn = 0;
    for (const auto& chan : asic.getChannels()) {
      bool skipChannel = false;
      if (mPadBadChannelMap) {
        try {
          skipChannel = (mPadBadChannelMap->getChannelStatus(iasic, currentchannel) != o2::focal::PadBadChannelMap::MaskType_t::GOOD_CHANNEL);
        } catch (o2::focal::PadBadChannelMap::ChannelIndexException& e) {
          ILOG(Error, Support) << "Error accessing channel status: " << e.what() << ENDM;
        }
      }
      if (skipChannel) {
        continue;
      }
      if (chan.getTOT() < mPadTOTCutADC) {
        double adc = chan.getADC(); // must be converted to floating point number for pedestal subtraction
        if (mPadPedestalHandler && iasic < 18) {
          try {
            double pedestal = mPadPedestalHandler->getPedestal(iasic, currentchannel);
            adc -= pedestal;
          } catch (o2::focal::PadPedestal::InvalidChannelException& e) {
            ILOG(Error, Support) << e.what() << ENDM;
          }
        }
        mPadASICChannelADC[iasic]->Fill(currentchannel, adc);
        if ((chan.getTOT() == 0) && (chan.getTOT() != 4095)) {
          adcsum += adc;
          kADCsum[iasic] += adc;
        }
        auto [column, row] = mPadMapper.getRowColFromChannelID(currentchannel);
        mHitMapPadASIC[iasic]->Fill(row, column, adc);

        // get average TOA of all channels in the ASIC
        if (chan.getTOA() > 0) {
          asicAverageTOA += chan.getTOA();
          nChWithTOA++;
        }

        if (iasic == 0) {
          if ((currentchannel != 14) && (currentchannel != 16) && (currentchannel != 19) && (currentchannel != 48) && (currentchannel != 52) && (currentchannel != 61)) {
            kADCForCMN[icmn] += adc;
            icmn++;
          }

          if (currentchannel == 14)
            adc_ch14 = adc;
          if (currentchannel == 16)
            adc_ch16 = adc;
          if (currentchannel == 19)
            adc_ch19 = adc;
          if (currentchannel == 48)
            adc_ch48 = adc;
          if (currentchannel == 52)
            adc_ch52 = adc;
          if (currentchannel == 61)
            adc_ch61 = adc;
        }
      }
      mPadASICChannelTOA[iasic]->Fill(currentchannel, chan.getTOA());
      if (chan.getTOT()) {
        mPadASICChannelTOT[iasic]->Fill(currentchannel, chan.getTOT());
        if (chan.getTOT() != 4095) {
          totsum += chan.getTOT();
          kTOTsum[iasic] += chan.getTOT();
        }
      }
      if (std::find(mChannelsPadProjections.begin(), mChannelsPadProjections.end(), currentchannel) != mChannelsPadProjections.end()) {
        auto hist = mPadChannelProjections[iasic]->mHistos.find(currentchannel);
        if (hist != mPadChannelProjections[iasic]->mHistos.end()) {
          hist->second->Fill(chan.getADC());
        }
      }
      if (currentchannel == 14)
        mPadTOAvsASIC_Ch14->Fill(chan.getTOA(), iasic);
      if (currentchannel == 16)
        mPadTOAvsASIC_Ch16->Fill(chan.getTOA(), iasic);
      if (currentchannel == 19)
        mPadTOAvsASIC_Ch19->Fill(chan.getTOA(), iasic);
      if (currentchannel == 48)
        mPadTOAvsASIC_Ch48->Fill(chan.getTOA(), iasic);
      if (currentchannel == 52)
        mPadTOAvsASIC_Ch52->Fill(chan.getTOA(), iasic);
      if (currentchannel == 61)
        mPadTOAvsASIC_Ch61->Fill(chan.getTOA(), iasic);

      currentchannel++;
      // calculate common noise
    }

    asicAverageTOA /= nChWithTOA;

    toaglobalaverage += asicAverageTOA;

    // fill average TOA
    mPadTOAvsASIC->Fill(asicAverageTOA, iasic);

    mPadTOTSumASIC[iasic]->Fill(totsum);
    mPadADCSumASIC[iasic]->Fill(adcsum);
    totglobalsum += totsum;
    adcglobalsum += adcsum;

    if (iasic == 0) {
      cmn = TMath::Median(icmn, kADCForCMN.data());
      mPadGlobalMIPADC_Ch14_Asic0->Fill(adc_ch14 - cmn);
      mPadGlobalMIPADC_Ch16_Asic0->Fill(adc_ch16 - cmn);
      mPadGlobalMIPADC_Ch19_Asic0->Fill(adc_ch19 - cmn);
      mPadGlobalMIPADC_Ch48_Asic0->Fill(adc_ch48 - cmn);
      mPadGlobalMIPADC_Ch52_Asic0->Fill(adc_ch52 - cmn);
      mPadGlobalMIPADC_Ch61_Asic0->Fill(adc_ch61 - cmn);
    }

    // find all trigger windows of asic (20 windows)
    auto trigwindows = eventdata[iasic].getTriggerWords();

    // loop over span of all trigger windows
    int w = 0;
    for (auto& trigwindow : trigwindows) {
      // find largest value from all trigger regions
      uint64_t maxTrig = 0;
      if (trigwindow.mTrigger0 >= maxTrig)
        maxTrig = trigwindow.mTrigger0;
      if (trigwindow.mTrigger1 >= maxTrig)
        maxTrig = trigwindow.mTrigger1;
      if (trigwindow.mTrigger2 >= maxTrig)
        maxTrig = trigwindow.mTrigger2;
      if (trigwindow.mTrigger3 >= maxTrig)
        maxTrig = trigwindow.mTrigger3;
      if (trigwindow.mTrigger4 >= maxTrig)
        maxTrig = trigwindow.mTrigger4;
      if (trigwindow.mTrigger5 >= maxTrig)
        maxTrig = trigwindow.mTrigger5;
      if (trigwindow.mTrigger6 >= maxTrig)
        maxTrig = trigwindow.mTrigger6;
      if (trigwindow.mTrigger7 >= maxTrig)
        maxTrig = trigwindow.mTrigger7;
      mPadTRIGvsWindowASIC[iasic]->Fill(w, maxTrig);
      w++;
    }
    // Fill CMN channels
    if (asic.getFirstCMN().getTOT() < mPadTOTCutADC) {
      mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getFirstCMN().getADC());
    }
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getFirstCMN().getTOA());
    if (asic.getFirstCMN().getTOT()) {
      mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getFirstCMN().getTOT());
    }
    currentchannel++;
    if (asic.getSecondCMN().getTOT() < mPadTOTCutADC) {
      mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getSecondCMN().getADC());
    }
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getSecondCMN().getTOA());
    if (asic.getSecondCMN().getTOT()) {
      mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getSecondCMN().getTOT());
    }
    currentchannel++;
    // Fill Calib channels
    if (asic.getFirstCalib().getTOT() < mPadTOTCutADC) {
      mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getFirstCalib().getADC());
    }
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getFirstCalib().getTOA());
    if (asic.getFirstCalib().getTOT()) {
      mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getFirstCalib().getTOT());
    }
    currentchannel++;
    if (asic.getSecondCalib().getTOT() < mPadTOTCutADC) {
      mPadASICChannelADC[iasic]->Fill(currentchannel, asic.getSecondCalib().getADC());
    }
    mPadASICChannelTOA[iasic]->Fill(currentchannel, asic.getSecondCalib().getTOA());
    if (asic.getSecondCalib().getTOT()) {
      mPadASICChannelTOT[iasic]->Fill(currentchannel, asic.getSecondCalib().getTOT());
    }
    currentchannel++;
  }

  // fill global average TOA
  toaglobalaverage /= PAD_ASICS;

  // loop over all TOT sums to fill correlations
  for (int i = 0; i < PAD_ASICS - 1; i++) {
    mPadTOTCorrASIC[i]->Fill(kTOTsum[i], kTOTsum[i + 1]);
    mPadADCCorrASIC[i]->Fill(kADCsum[i], kADCsum[i + 1]);
  }

  mPadTOTSumGlobal->Fill(totglobalsum);
  mPadADCSumGlobal->Fill(adcglobalsum);

  mPadGlobalTOTvsADC->Fill(adcglobalsum, totglobalsum);
  mPadGlobalTOAvsADC->Fill(adcglobalsum, toaglobalaverage);
}

void TestbeamRawTask::processPixelPayload(gsl::span<const o2::itsmft::GBTWord> pixelpayload, uint16_t feeID)
{
  auto fee = feeID & 0x00FF,
       branch = (feeID & 0x0F00) >> 8;
  ILOG(Debug, Support) << "Decoded FEE ID " << feeID << " -> FEE " << fee << ", branch " << branch << ENDM;
  auto useFEE = branch * 10 + fee;
  mLinksWithPayloadPixel->Fill(useFEE);
  int layer = -1;

  // Testbeam November 2022
  auto mappingtype = mPixelMapper->getMappingType();
  auto numbersegments = getNumberOfPixelSegments(mappingtype);
  int totalsegmentsCol = numbersegments.first * mPixelMapper->getNumberOfColumns(),
      totalsegmentsRow = numbersegments.second * mPixelMapper->getNumberOfRows(),
      totalsegments = totalsegmentsCol * totalsegmentsRow;

  mPixelDecoder.reset();
  mPixelDecoder.decodeEvent(pixelpayload);
  if (pixelpayload.size()) {
    ILOG(Debug, Support) << "Found pixel payload of size: " << pixelpayload.size() << " -> " << mPixelDecoder.getChipData().size() << " chips" << ENDM;
  }

  mTriggersFeePixel->Fill(useFEE, mPixelDecoder.getChipData().size());

  for (const auto& [trigger, chips] : mPixelDecoder.getChipData()) {
    int nhitsAll = 0;
    std::unordered_set<int> chipIDsFound, chipIDsHits;
    memset(mHitSegmentCounter.data(), 0, sizeof(int) * mHitSegmentCounter.size());
    for (const auto& chip : chips) {
      ILOG(Debug, Support) << "[In task] Chip " << static_cast<int>(chip.mChipID) << " from lane " << static_cast<int>(chip.mLaneID) << ", " << chip.mHits.size() << " hit(s) ..." << ENDM;
      nhitsAll += chip.mHits.size();
      mHitsChipPixel->Fill(chip.mHits.size());
      mAverageHitsChipPixel->Fill(useFEE, chip.mChipID, chip.mHits.size());
      mPixelLaneIDChipIDFEE[fee]->Fill(chip.mLaneID, chip.mChipID);
      chipIDsFound.insert(chip.mChipID);
      if (chip.mHits.size()) {
        chipIDsHits.insert(chip.mChipID);
      }
      try {
        auto position = mPixelMapper->getPosition(feeID, chip);
        if (layer < 0) {
          layer = position.mLayer;
        }
        mPixelChipHitProfileLayer[position.mLayer]->Fill(position.mColumn, position.mRow, chip.mHits.size());
        mPixelChipHitmapLayer[position.mLayer]->Fill(position.mColumn, position.mRow, chip.mHits.size());
        int chipIndex = position.mRow * 7 + position.mColumn;
        mPixelHitDistribitionLayer[position.mLayer]->Fill(chipIndex, chip.mHits.size());
        for (auto& hit : chip.mHits) {
          auto segment = getPixelSegment(hit, mappingtype, position);
          auto segment_col = position.mColumn * numbersegments.first + segment.first;
          auto segment_row = position.mRow * numbersegments.second + segment.second;
          mPixelSegmentHitmapLayer[position.mLayer]->Fill(segment_col, segment_row);
          int segment_id = segment_row * totalsegmentsCol + segment_col;
          mHitSegmentCounter[segment_id]++;
        }
      } catch (o2::focal::PixelMapper::InvalidChipException& e) {
        ILOG(Error, Support) << "Error in chip index: " << e << ENDM;
      } catch (o2::focal::PixelMapper::UninitException& e) {
        ILOG(Error, Support) << e << ENDM;
      }
    }
    for (auto chipID : chipIDsFound) {
      mPixelChipsIDsFound->Fill(useFEE, chipID);
    }
    for (auto chipID : chipIDsHits) {
      mPixelChipsIDsHits->Fill(useFEE, chipID);
    }
    auto triggerfoundAll = mPixelNHitsAll.find(trigger);
    if (triggerfoundAll == mPixelNHitsAll.end()) {
      mPixelNHitsAll[trigger] = nhitsAll;
    } else {
      triggerfoundAll->second += nhitsAll;
    }
    auto triggerfoundLayer = mPixelNHitsLayer[layer].find(trigger);
    if (triggerfoundLayer == mPixelNHitsLayer[layer].end()) {
      mPixelNHitsLayer[layer][trigger] = nhitsAll;
    } else {
      triggerfoundLayer->second += nhitsAll;
    }
    for (int segmentID = 0; segmentID < mHitSegmentCounter.size(); segmentID++) {
      if (mHitSegmentCounter[segmentID]) {
        int segment_row = segmentID / totalsegmentsCol,
            segment_col = segmentID % totalsegmentsCol;
        mPixelSegmentHitProfileLayer[layer]->Fill(segment_col, segment_row, mHitSegmentCounter[segmentID]);
      }
    }
  }
}

std::pair<int, int> TestbeamRawTask::getPixelSegment(const o2::focal::PixelHit& hit, o2::focal::PixelMapper::MappingType_t mappingtype, const o2::focal::PixelMapper::ChipPosition& chipMapping) const
{
  int row = -1, col = -1, absColumn = -1, absRow = -1;
  switch (mappingtype) {
    case o2::focal::PixelMapper::MappingType_t::MAPPING_IB:
      absColumn = chipMapping.mInvertColumn ? PIXEL_COLS_IB - hit.mColumn : hit.mColumn;
      absRow = chipMapping.mInvertRow ? PIXEL_ROWS_IB - hit.mRow : hit.mRow;
      col = absColumn / PIXEL_COL_SEGMENSIZE_IB;
      row = absRow / PIXEL_ROW_SEGMENTSIZE_IB;
      break;
    case o2::focal::PixelMapper::MappingType_t::MAPPING_OB:
      absColumn = chipMapping.mInvertColumn ? PIXEL_COLS_OB - hit.mColumn : hit.mColumn;
      absRow = chipMapping.mInvertRow ? PIXEL_ROWS_OB - hit.mRow : hit.mRow;
      col = absColumn / PIXEL_COL_SEGMENSIZE_OB;
      row = absRow / PIXEL_ROW_SEGMENTSIZE_OB;
      break;
    default:
      break;
  };
  return { col, row };
}

std::pair<int, int> TestbeamRawTask::getNumberOfPixelSegments(o2::focal::PixelMapper::MappingType_t mappingtype) const
{
  int rows = -1, cols = -1;
  switch (mappingtype) {
    case o2::focal::PixelMapper::MappingType_t::MAPPING_IB:
      rows = PIXEL_ROWS_IB / PIXEL_ROW_SEGMENTSIZE_IB;
      cols = PIXEL_COLS_IB / PIXEL_COL_SEGMENSIZE_IB;
      break;
    case o2::focal::PixelMapper::MappingType_t::MAPPING_OB:
      rows = PIXEL_ROWS_OB / PIXEL_ROW_SEGMENTSIZE_OB;
      cols = PIXEL_COLS_OB / PIXEL_COL_SEGMENSIZE_OB;
      break;

    default:
      break;
  };
  return { cols, rows };
}

void TestbeamRawTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void TestbeamRawTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TestbeamRawTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mTFerrorCounter->Reset();
  mFEENumberHBF->Reset();
  mFEENumberTF->Reset();
  mNumLinksTF->Reset();
  mNumHBFPerCRU->Reset();
  mCRUcounter->Reset();
  mPayloadSizeTF->Reset();
  if (!mDisablePads) {
    mPayloadSizePadsGBT->Reset();
    for (auto padadc : mPadASICChannelADC) {
      if (padadc) {
        padadc->Reset();
      }
    }
    for (auto padtoa : mPadASICChannelTOA) {
      if (padtoa) {
        padtoa->Reset();
      }
    }
    for (auto padtot : mPadASICChannelTOT) {
      if (padtot) {
        padtot->Reset();
      }
    }
    for (auto hitmap : mHitMapPadASIC) {
      if (hitmap) {
        hitmap->Reset();
      }
    }
    for (auto mtotsum : mPadTOTSumASIC) {
      if (mtotsum) {
        mtotsum->Reset();
      }
    }
    for (auto madcsum : mPadADCSumASIC) {
      if (madcsum) {
        madcsum->Reset();
      }
    }
    for (auto& projections : mPadChannelProjections) {
      if (projections) {
        projections->reset();
      }
    }

    for (auto hist : mPadTOTCorrASIC) {
      if (hist) {
        hist->Reset();
      }
    }
    for (auto hist : mPadADCCorrASIC) {
      if (hist) {
        hist->Reset();
      }
    }

    for (auto hist : mPadTRIGvsWindowASIC) {
      if (hist) {
        hist->Reset();
      }
    }
    mPadTOTSumGlobal->Reset();
    mPadADCSumGlobal->Reset();
    mPadGlobalTOTvsADC->Reset();
    mPadGlobalTOAvsADC->Reset();
    mPadTOAvsASIC->Reset();

    mPadTOAvsASIC_Ch14->Reset();
    mPadTOAvsASIC_Ch16->Reset();
    mPadTOAvsASIC_Ch19->Reset();
    mPadTOAvsASIC_Ch48->Reset();
    mPadTOAvsASIC_Ch52->Reset();
    mPadTOAvsASIC_Ch61->Reset();

    mPadGlobalMIPADC_Ch14_Asic0->Reset();
    mPadGlobalMIPADC_Ch16_Asic0->Reset();
    mPadGlobalMIPADC_Ch19_Asic0->Reset();
    mPadGlobalMIPADC_Ch48_Asic0->Reset();
    mPadGlobalMIPADC_Ch52_Asic0->Reset();
    mPadGlobalMIPADC_Ch61_Asic0->Reset();
  }

  if (!mDisablePixels) {
    mPayloadSizePixelsGBT->Reset();
    if (mLinksWithPayloadPixel) {
      mLinksWithPayloadPixel->Reset();
    }
    if (mTriggersFeePixel) {
      mTriggersFeePixel->Reset();
    }
    if (mAverageHitsChipPixel) {
      mAverageHitsChipPixel->Reset();
    }
    if (mHitsChipPixel) {
      mHitsChipPixel->Reset();
    }
    if (mPixelHitsTriggerAll) {
      mPixelHitsTriggerAll->Reset();
    }
    if (mPixelChipsIDsFound) {
      mPixelChipsIDsFound->Reset();
    }

    for (auto& hist : mPixelLaneIDChipIDFEE) {
      if (hist) {
        hist->Reset();
      }
    }

    for (auto& hist : mPixelChipHitProfileLayer) {
      if (hist) {
        hist->Reset();
      }
    }
    for (auto& hist : mPixelChipHitmapLayer) {
      if (hist) {
        hist->Reset();
      }
    }
    for (auto& hist : mPixelSegmentHitProfileLayer) {
      if (hist) {
        hist->Reset();
      }
    }
    for (auto& hist : mPixelSegmentHitmapLayer) {
      if (hist) {
        hist->Reset();
      }
    }
    for (auto& hist : mPixelHitDistribitionLayer) {
      if (hist) {
        hist->Reset();
      }
    }
    for (auto& hist : mPixelHitsTriggerLayer) {
      if (hist) {
        hist->Reset();
      }
    }
  }
}

TestbeamRawTask::PadChannelProjections::~PadChannelProjections()
{
  for (auto& [chan, hist] : mHistos) {
    delete hist;
  }
}

void TestbeamRawTask::PadChannelProjections::init(const std::vector<int> channels, int ASICid)
{
  for (auto chan : channels) {
    auto hist = new TH1D(Form("Pad_ProjADC_ASIC%d_Chan%d", ASICid, chan), Form("Pad ADC projection ASIC %d channel %d", ASICid, chan), 101, -0.5, 100.5);
    hist->SetStats(false);
    mHistos.insert({ chan, hist });
  }
}

void TestbeamRawTask::PadChannelProjections::startPublishing(o2::quality_control::core::ObjectsManager& mgr)
{
  for (auto& [chan, hist] : mHistos) {
    mgr.startPublishing(hist);
  }
}

void TestbeamRawTask::PadChannelProjections::reset()
{
  for (auto& [chan, hist] : mHistos) {
    hist->Reset();
  }
}

bool TestbeamRawTask::isLostTimeframe(framework::ProcessingContext& ctx) const
{
  // direct data
  constexpr auto originFOC = header::gDataOriginFOC;
  o2::framework::InputSpec dummy{ "dummy",
                                  framework::ConcreteDataMatcher{ originFOC,
                                                                  header::gDataDescriptionRawData,
                                                                  0xDEADBEEF } };
  for (const auto& ref : o2::framework::InputRecordWalker(ctx.inputs(), { dummy })) {
    // auto posReadout = ctx.inputs().getPos("readout");
    // auto nslots = ctx.inputs().getNofParts(posReadout);
    // for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    //   const auto& ref = ctx.inputs().getByPos(posReadout, islot);
    const auto dh = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
    const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(ref);
    // if (dh->subSpecification == 0xDEADBEEF) {
    if (payloadSize == 0) {
      return true;
      //  }
    }
  }
  // sampled data
  o2::framework::InputSpec dummyDS{ "dummyDS",
                                    framework::ConcreteDataMatcher{ "DS",
                                                                    "focrawdata0",
                                                                    0xDEADBEEF } };
  for (const auto& ref : o2::framework::InputRecordWalker(ctx.inputs(), { dummyDS })) {
    // auto posReadout = ctx.inputs().getPos("readout");
    // auto nslots = ctx.inputs().getNofParts(posReadout);
    // for (decltype(nslots) islot = 0; islot < nslots; islot++) {
    //   const auto& ref = ctx.inputs().getByPos(posReadout, islot);
    const auto dh = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(ref);
    const auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(ref);
    // if (dh->subSpecification == 0xDEADBEEF) {
    if (payloadSize == 0) {
      return true;
      //  }
    }
  }

  return false;
}

} // namespace o2::quality_control_modules::focal
