// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   TOFTask.cxx
/// \author Nicolo' Jacazio
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1I.h>
#include <TH2I.h>

#include "QualityControl/QcInfoLogger.h"
#include "TOF/TOFTask.h"

namespace o2::quality_control_modules::tof
{

Int_t TOFTask::fgNbinsMultiplicity = 2000;       /// Number of bins in multiplicity plot
Int_t TOFTask::fgRangeMinMultiplicity = 0;       /// Min range in multiplicity plot
Int_t TOFTask::fgRangeMaxMultiplicity = 1000;    /// Max range in multiplicity plot
Int_t TOFTask::fgNbinsTime = 250;                /// Number of bins in time plot
const Float_t TOFTask::fgkNbinsWidthTime = 2.44; /// Width of bins in time plot
Float_t TOFTask::fgRangeMinTime = 0.0;           /// Range min in time plot
Float_t TOFTask::fgRangeMaxTime = 620.0;         /// Range max in time plot
Int_t TOFTask::fgCutNmaxFiredMacropad = 50;      /// Cut on max number of fired macropad
const Int_t TOFTask::fgkFiredMacropadLimit = 50; /// Limit on cut on number of fired macropad

TOFTask::TOFTask() : TaskInterface(),
                     mTOFRawsMulti(nullptr),
                     mTOFRawsMultiIA(nullptr),
                     mTOFRawsMultiOA(nullptr),
                     mTOFRawsMultiIC(nullptr),
                     mTOFRawsMultiOC(nullptr),
                     mTOFRawsTime(nullptr),
                     mTOFRawsTimeIA(nullptr),
                     mTOFRawsTimeOA(nullptr),
                     mTOFRawsTimeIC(nullptr),
                     mTOFRawsTimeOC(nullptr),
                     mTOFRawsToT(nullptr),
                     mTOFRawsToTIA(nullptr),
                     mTOFRawsToTOA(nullptr),
                     mTOFRawsToTIC(nullptr),
                     mTOFRawsToTOC(nullptr),
                     mTOFRawsLTMHits(nullptr),
                     mTOFrefMap(nullptr),
                     mTOFRawHitMap(nullptr),
                     mTOFDecodingErrors(nullptr),
                     mTOFOrphansTime(nullptr),
                     mTOFRawTimeVsTRM035(nullptr),
                     mTOFRawTimeVsTRM3671(nullptr),
                     mTOFTimeVsStrip(nullptr),
                     mTOFtimeVsBCID(nullptr),
                     mTOFchannelEfficiencyMap(nullptr),
                     mTOFhitsCTTM(nullptr),
                     mTOFmacropadCTTM(nullptr),
                     mTOFmacropadDeltaPhiTime(nullptr),
                     mBXVsCttmBit(nullptr),
                     mTimeVsCttmBit(nullptr),
                     mTOFRawHitMap24(nullptr),
                     mHitMultiVsDDL(nullptr),
                     mNfiredMacropad(nullptr)
{
}

TOFTask::~TOFTask()
{
  if (mTOFRawsMulti)
    delete mTOFRawsMulti;
  if (mTOFRawsMultiIA)
    delete mTOFRawsMultiIA;
  if (mTOFRawsMultiOA)
    delete mTOFRawsMultiOA;
  if (mTOFRawsMultiIC)
    delete mTOFRawsMultiIC;
  if (mTOFRawsMultiOC)
    delete mTOFRawsMultiOC;
  if (mTOFRawsTime)
    delete mTOFRawsTime;
  if (mTOFRawsTimeIA)
    delete mTOFRawsTimeIA;
  if (mTOFRawsTimeOA)
    delete mTOFRawsTimeOA;
  if (mTOFRawsTimeIC)
    delete mTOFRawsTimeIC;
  if (mTOFRawsTimeOC)
    delete mTOFRawsTimeOC;
  if (mTOFRawsToT)
    delete mTOFRawsToT;
  if (mTOFRawsToTIA)
    delete mTOFRawsToTIA;
  if (mTOFRawsToTOA)
    delete mTOFRawsToTOA;
  if (mTOFRawsToTIC)
    delete mTOFRawsToTIC;
  if (mTOFRawsToTOC)
    delete mTOFRawsToTOC;
  if (mTOFRawsLTMHits)
    delete mTOFRawsLTMHits;
  if (mTOFrefMap)
    delete mTOFrefMap;
  if (mTOFRawHitMap)
    delete mTOFRawHitMap;
  if (mTOFDecodingErrors)
    delete mTOFDecodingErrors;
  if (mTOFOrphansTime)
    delete mTOFOrphansTime;
  if (mTOFRawTimeVsTRM035)
    delete mTOFRawTimeVsTRM035;
  if (mTOFRawTimeVsTRM3671)
    delete mTOFRawTimeVsTRM3671;
  if (mTOFTimeVsStrip)
    delete mTOFTimeVsStrip;
  if (mTOFtimeVsBCID)
    delete mTOFtimeVsBCID;
  if (mTOFchannelEfficiencyMap)
    delete mTOFchannelEfficiencyMap;
  if (mTOFhitsCTTM)
    delete mTOFhitsCTTM;
  if (mTOFmacropadCTTM)
    delete mTOFmacropadCTTM;
  if (mTOFmacropadDeltaPhiTime)
    delete mTOFmacropadDeltaPhiTime;
  if (mBXVsCttmBit)
    delete mBXVsCttmBit;
  if (mTimeVsCttmBit)
    delete mTimeVsCttmBit;
  if (mTOFRawHitMap24)
    delete mTOFRawHitMap24;
  if (mHitMultiVsDDL)
    delete mHitMultiVsDDL;
  if (mNfiredMacropad)
    delete mNfiredMacropad;
}

void TOFTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  QcInfoLogger::GetInstance() << "initialize TOFTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mTOFRawsMulti = new TH1I("TOFRawsMulti", "TOF raw hit multiplicity; TOF raw hits number; Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMulti);
  getObjectsManager()->addCheck(mTOFRawsMulti, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsMultiIA = new TH1I("TOFRawsMultiIA", "TOF raw hit multiplicity - I/A side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiIA);
  getObjectsManager()->addCheck(mTOFRawsMultiIA, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsMultiOA = new TH1I("TOFRawsMultiOA", "TOF raw hit multiplicity - O/A side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiOA);
  getObjectsManager()->addCheck(mTOFRawsMultiOA, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsMultiIC = new TH1I("TOFRawsMultiIC", "TOF raw hit multiplicity - I/C side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiIC);
  getObjectsManager()->addCheck(mTOFRawsMultiIC, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsMultiOC = new TH1I("TOFRawsMultiOC", "TOF raw hit multiplicity - O/C side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiOC);
  getObjectsManager()->addCheck(mTOFRawsMultiOC, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsTime = new TH1F("TOFRawsTime", "TOF Raws - Hit time (ns);Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTime);
  getObjectsManager()->addCheck(mTOFRawsTime, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsTimeIA = new TH1F("TOFRawsTimeIA", "TOF Raws - Hit time (ns) - I/A side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeIA);
  getObjectsManager()->addCheck(mTOFRawsTimeIA, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsTimeOA = new TH1F("TOFRawsTimeOA", "TOF Raws - Hit time (ns) - O/A side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeOA);
  getObjectsManager()->addCheck(mTOFRawsTimeOA, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsTimeIC = new TH1F("TOFRawsTimeIC", "TOF Raws - Hit time (ns) - I/C side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeIC);
  getObjectsManager()->addCheck(mTOFRawsTimeIC, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsTimeOC = new TH1F("TOFRawsTimeOC", "TOF Raws - Hit time (ns) - O/C side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeOC);
  getObjectsManager()->addCheck(mTOFRawsTimeOC, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsToT = new TH1F("TOFRawsToT", "TOF Raws - Hit ToT (ns);Measured Hit ToT (ns);Hits", 100, 0., 48.8);
  getObjectsManager()->startPublishing(mTOFRawsToT);
  getObjectsManager()->addCheck(mTOFRawsToT, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsToTIA = new TH1F("TOFRawsToTIA", "TOF Raws - Hit ToT (ns) - I/A side;Measured Hit ToT (ns);Hits", 100, 0., 48.8);
  getObjectsManager()->startPublishing(mTOFRawsToTIA);
  getObjectsManager()->addCheck(mTOFRawsToTIA, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsToTOA = new TH1F("TOFRawsToTOA", "TOF Raws - Hit ToT (ns) - O/A side;Measured Hit ToT (ns);Hits", 100, 0., 48.8);
  getObjectsManager()->startPublishing(mTOFRawsToTOA);
  getObjectsManager()->addCheck(mTOFRawsToTOA, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsToTIC = new TH1F("TOFRawsToTIC", "TOF Raws - Hit ToT (ns) - I/C side;Measured Hit ToT (ns);Hits", 100, 0., 48.8);
  getObjectsManager()->startPublishing(mTOFRawsToTIC);
  getObjectsManager()->addCheck(mTOFRawsToTIC, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsToTOC = new TH1F("TOFRawsToTOC", "TOF Raws - Hit ToT (ns) - O/C side;Measured Hit ToT (ns);Hits", 100, 0., 48.8);
  getObjectsManager()->startPublishing(mTOFRawsToTOC);
  getObjectsManager()->addCheck(mTOFRawsToTOC, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawsLTMHits = new TH1F("TOFRawsLTMHits", "LTMs OR signals; Crate; Counts", 72, 0., 72.);
  getObjectsManager()->startPublishing(mTOFRawsLTMHits);
  getObjectsManager()->addCheck(mTOFRawsLTMHits, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFrefMap = new TH2F("TOFrefMap", "TOF enabled channel reference map;sector;strip", 72, 0., 18., 91, 0., 91.);
  getObjectsManager()->startPublishing(mTOFrefMap);
  getObjectsManager()->addCheck(mTOFrefMap, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawHitMap = new TH2F("TOFRawHitMap", "TOF raw hit map (1 bin = 1 FEA = 24 channels);sector;strip", 72, 0., 18., 91, 0., 91.);
  getObjectsManager()->startPublishing(mTOFRawHitMap);
  getObjectsManager()->addCheck(mTOFRawHitMap, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFDecodingErrors = new TH2I("TOFDecodingErrors", "Decoding error monitoring; DDL; Error ", 72, 0, 72, 13, 1, 14);
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(1, "DRM ");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(2, "LTM ");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(3, "TRM 3 ");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(4, "TRM 4");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(5, "TRM 5");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(6, "TRM 6");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(7, "TRM 7");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(8, "TRM 8");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(9, "TRM 9");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(10, "TRM 10");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(11, "TRM 11");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(12, "TRM 12");
  mTOFDecodingErrors->GetYaxis()->SetBinLabel(13, "recovered");
  getObjectsManager()->startPublishing(mTOFDecodingErrors);
  getObjectsManager()->addCheck(mTOFDecodingErrors, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFOrphansTime = new TH1F("TOFOrphansTime", "TOF Raws - Orphans time (ns);Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFOrphansTime);
  getObjectsManager()->addCheck(mTOFOrphansTime, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawTimeVsTRM035 = new TH2F("TOFRawTimeVsTRM035", "TOF raws - Hit time vs TRM - crates 0 to 35; TRM index = DDL*10+TRM(0-9);TOF raw time [ns]", 361, 0., 361., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawTimeVsTRM035);
  getObjectsManager()->addCheck(mTOFRawTimeVsTRM035, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawTimeVsTRM3671 = new TH2F("TOFRawTimeVsTRM3671", "TOF raws - Hit time vs TRM - crates 36 to 72; TRM index = DDL**10+TRM(0-9);TOF raw time [ns]", 361, 360., 721., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawTimeVsTRM3671);
  getObjectsManager()->addCheck(mTOFRawTimeVsTRM3671, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFTimeVsStrip = new TH2F("TOFTimeVsStrip", "TOF raw hit time vs. MRPC (along z axis); MRPC index along z axis; Raws TOF time (ns) ", 91, 0., 91, fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFTimeVsStrip);
  getObjectsManager()->addCheck(mTOFTimeVsStrip, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFtimeVsBCID = new TH2F("TOFtimeVsBCID", "TOF time vs BCID; BCID; time (ns) ", 3564, 0., 3564., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFtimeVsBCID);
  getObjectsManager()->addCheck(mTOFtimeVsBCID, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFchannelEfficiencyMap = new TH2F("TOFchannelEfficiencyMap", "TOF channels (HWok && efficient && !noisy && !problematic);sector;strip", 72, 0., 18., 91, 0., 91.);
  getObjectsManager()->startPublishing(mTOFchannelEfficiencyMap);
  getObjectsManager()->addCheck(mTOFchannelEfficiencyMap, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFhitsCTTM = new TH2F("TOFhitsCTTM", "Map of hit pads according to CTTM numbering;LTM index;bit index", 72, 0., 72., 23, 0., 23.);
  getObjectsManager()->startPublishing(mTOFhitsCTTM);
  getObjectsManager()->addCheck(mTOFhitsCTTM, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFmacropadCTTM = new TH2F("TOFmacropadCTTM", "Map of hit macropads according to CTTM numbering;LTM index; bit index", 72, 0., 72., 23, 0., 23.);
  getObjectsManager()->startPublishing(mTOFmacropadCTTM);
  getObjectsManager()->addCheck(mTOFmacropadCTTM, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFmacropadDeltaPhiTime = new TH2F("TOFmacropadDeltaPhiTime", "#Deltat vs #Delta#Phi of hit macropads;#Delta#Phi (degrees);#DeltaBX", 18, 0., 180., 20, 0., 20.0);
  getObjectsManager()->startPublishing(mTOFmacropadDeltaPhiTime);
  getObjectsManager()->addCheck(mTOFmacropadDeltaPhiTime, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mBXVsCttmBit = new TH2I("BXVsCttmBit", "BX ID in TOF matching window vs trg channel; trg channel; BX", 1728, 0, 1728, 24, 0, 24);
  getObjectsManager()->startPublishing(mBXVsCttmBit);
  getObjectsManager()->addCheck(mBXVsCttmBit, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTimeVsCttmBit = new TH2F("TimeVsCttmBit", "TOF raw time vs trg channel; trg channel; raw time (ns)", 1728, 0., 1728., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTimeVsCttmBit);
  getObjectsManager()->addCheck(mTimeVsCttmBit, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mTOFRawHitMap24 = new TH2F("TOFRawHitMap24", "TOF average raw hits/channel map (1 bin = 1 FEA = 24 channels);sector;strip", 72, 0., 18., 91, 0., 91.);
  getObjectsManager()->startPublishing(mTOFRawHitMap24);
  getObjectsManager()->addCheck(mTOFRawHitMap24, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mHitMultiVsDDL = new TH2I("itMultiVsDDL", "TOF raw hit multiplicity per event vs DDL ; DDL; TOF raw hits number; Events ", 72, 0., 72., 500, 0, 500);
  getObjectsManager()->startPublishing(mHitMultiVsDDL);
  getObjectsManager()->addCheck(mHitMultiVsDDL, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");

  mNfiredMacropad = new TH1I("NfiredMacropad", "Number of fired TOF macropads per event; number of fired macropads; Events ", 50, 0, 50);
  getObjectsManager()->startPublishing(mNfiredMacropad);
  getObjectsManager()->addCheck(mNfiredMacropad, "checkFromTOF", "o2::quality_control_modules::tof::TOFCheck", "QcTOF");
}

void TOFTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  mTOFRawsMulti->Reset();
  mTOFRawsMultiIA->Reset();
  mTOFRawsMultiOA->Reset();
  mTOFRawsMultiIC->Reset();
  mTOFRawsMultiOC->Reset();
  mTOFRawsTime->Reset();
  mTOFRawsTimeIA->Reset();
  mTOFRawsTimeOA->Reset();
  mTOFRawsTimeIC->Reset();
  mTOFRawsTimeOC->Reset();
  mTOFRawsToT->Reset();
  mTOFRawsToTIA->Reset();
  mTOFRawsToTOA->Reset();
  mTOFRawsToTIC->Reset();
  mTOFRawsToTOC->Reset();
  mTOFRawsLTMHits->Reset();
  mTOFrefMap->Reset();
  mTOFRawHitMap->Reset();
  mTOFDecodingErrors->Reset();
  mTOFOrphansTime->Reset();
  mTOFRawTimeVsTRM035->Reset();
  mTOFRawTimeVsTRM3671->Reset();
  mTOFTimeVsStrip->Reset();
  mTOFtimeVsBCID->Reset();
  mTOFchannelEfficiencyMap->Reset();
  mTOFhitsCTTM->Reset();
  mTOFmacropadCTTM->Reset();
  mTOFmacropadDeltaPhiTime->Reset();
  mBXVsCttmBit->Reset();
  mTimeVsCttmBit->Reset();
  mTOFRawHitMap24->Reset();
  mHitMultiVsDDL->Reset();
  mNfiredMacropad->Reset();
}

void TOFTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TOFTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //  {
  //    "binding": "random",
  //    "dataOrigin": "ITS",
  //    "dataDescription": "RAWDATA"
  //  }

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // 1. In a loop
  for (auto&& input : ctx.inputs()) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = header::get<header::DataHeader*>(input.header);
      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;

      // for the sake of an example, let's fill the histogram with payload sizes
      mTOFRawsMulti->Fill(header->payloadSize);
    }
  }

  // 2. Using get("<binding>")

  // get the payload of a specific input, which is a char array. "random" is the binding specified in the config file.
  //   auto payload = ctx.inputs().get("random").payload;

  // get payload of a specific input, which is a structure array:
  //  const auto* header = header::get<header::DataHeader*>(ctx.inputs().get("random").header);
  //  struct s {int a; double b;};
  //  auto array = ctx.inputs().get<s*>("random");
  //  for (int j = 0; j < header->payloadSize / sizeof(s); ++j) {
  //    int i = array.get()[j].a;
  //  }

  // get payload of a specific input, which is a root object
  //   auto h = ctx.inputs().get<TH1F*>("histos");
  //   Double_t stats[4];
  //   h->GetStats(stats);
  //   auto s = ctx.inputs().get<TObjString*>("string");
  //   LOG(INFO) << "String is " << s->GetString().Data();
}

void TOFTask::endOfCycle()
{
  QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TOFTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void TOFTask::reset()
{
  // clean all the monitor objects here

  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;
  mTOFRawsMulti->Reset();
  mTOFRawsMultiIA->Reset();
  mTOFRawsMultiOA->Reset();
  mTOFRawsMultiIC->Reset();
  mTOFRawsMultiOC->Reset();
  mTOFRawsTime->Reset();
  mTOFRawsTimeIA->Reset();
  mTOFRawsTimeOA->Reset();
  mTOFRawsTimeIC->Reset();
  mTOFRawsTimeOC->Reset();
  mTOFRawsToT->Reset();
  mTOFRawsToTIA->Reset();
  mTOFRawsToTOA->Reset();
  mTOFRawsToTIC->Reset();
  mTOFRawsToTOC->Reset();
  mTOFRawsLTMHits->Reset();
  mTOFrefMap->Reset();
  mTOFRawHitMap->Reset();
  mTOFDecodingErrors->Reset();
  mTOFOrphansTime->Reset();
  mTOFRawTimeVsTRM035->Reset();
  mTOFRawTimeVsTRM3671->Reset();
  mTOFTimeVsStrip->Reset();
  mTOFtimeVsBCID->Reset();
  mTOFchannelEfficiencyMap->Reset();
  mTOFhitsCTTM->Reset();
  mTOFmacropadCTTM->Reset();
  mTOFmacropadDeltaPhiTime->Reset();
  mBXVsCttmBit->Reset();
  mTimeVsCttmBit->Reset();
  mTOFRawHitMap24->Reset();
  mHitMultiVsDDL->Reset();
  mNfiredMacropad->Reset();
}

} // namespace o2::quality_control_modules::tof
