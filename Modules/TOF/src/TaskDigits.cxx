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
/// \file   TaskDigits.cxx
/// \author Nicolo' Jacazio
/// \brief  Task to monitor quantities in TOF digits in both data and MC
///

// ROOT includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1I.h>
#include <TH2I.h>
#include <TProfile2D.h>

// O2 includes
#include "TOFBase/Digit.h"
#include "TOFBase/Geo.h"
#include <Framework/InputRecord.h>

// Fairlogger includes
#include <fairlogger/Logger.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskDigits.h"

namespace o2::quality_control_modules::tof
{

TaskDigits::TaskDigits() : TaskInterface()
{
}

void TaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  // Define parameters
  if (auto param = mCustomParameters.find("NbinsMultiplicity"); param != mCustomParameters.end()) {
    fgNbinsMultiplicity = ::atoi(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("RangeMaxMultiplicity"); param != mCustomParameters.end()) {
    fgRangeMaxMultiplicity = ::atoi(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("NbinsTime"); param != mCustomParameters.end()) {
    fgNbinsTime = ::atoi(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("kNbinsWidthTime"); param != mCustomParameters.end()) {
    fgkNbinsWidthTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("RangeMinTime"); param != mCustomParameters.end()) {
    fgRangeMinTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("RangeMaxTime"); param != mCustomParameters.end()) {
    fgRangeMaxTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("NbinsToT"); param != mCustomParameters.end()) {
    fgNbinsToT = ::atoi(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("RangeMinTime"); param != mCustomParameters.end()) {
    fgRangeMinTime = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("RangeMinToT"); param != mCustomParameters.end()) {
    fgRangeMinToT = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("RangeMaxToT"); param != mCustomParameters.end()) {
    fgRangeMaxToT = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("Diagnostic"); param != mCustomParameters.end()) {
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      fgDiagnostic = true;
    }
  }

  // Define histograms
  ILOG(Info, Support) << "initialize TaskDigits" << ENDM;
  mOrbitID = std::make_shared<TH2F>("OrbitID", "OrbitID;OrbitID % 1048576;Crate", 1024, 0, 1048576, RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mOrbitID.get());

  mTimeBC = std::make_shared<TH2F>("TimeBC", "Raw BC ID;BC ID (~25 ns);Crate", 641, 0., 3564., RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mTimeBC.get());

  mEventCounter = std::make_shared<TH2F>("EventCounter", "Event Counter;Event counter % 1000;Crate", 1000, 0., 1000., RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mEventCounter.get());

  mTOFRawsMulti = std::make_shared<TH1I>("TOFRawsMulti", "TOF raw hit multiplicity; TOF raw hits number; Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMulti.get());

  mTOFRawsMultiIA = std::make_shared<TH1I>("TOFRawsMultiIA", "TOF raw hit multiplicity - I/A side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiIA.get());

  mTOFRawsMultiOA = std::make_shared<TH1I>("TOFRawsMultiOA", "TOF raw hit multiplicity - O/A side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiOA.get());

  mTOFRawsMultiIC = std::make_shared<TH1I>("TOFRawsMultiIC", "TOF raw hit multiplicity - I/C side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiIC.get());

  mTOFRawsMultiOC = std::make_shared<TH1I>("TOFRawsMultiOC", "TOF raw hit multiplicity - O/C side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mTOFRawsMultiOC.get());

  mTOFRawsTime = std::make_shared<TH1F>("TOFRawsTime", "TOF Raws - Hit time (ns);Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTime.get());

  mTOFRawsTimeIA = std::make_shared<TH1F>("TOFRawsTimeIA", "TOF Raws - Hit time (ns) - I/A side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeIA.get());

  mTOFRawsTimeOA = std::make_shared<TH1F>("TOFRawsTimeOA", "TOF Raws - Hit time (ns) - O/A side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeOA.get());

  mTOFRawsTimeIC = std::make_shared<TH1F>("TOFRawsTimeIC", "TOF Raws - Hit time (ns) - I/C side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeIC.get());

  mTOFRawsTimeOC = std::make_shared<TH1F>("TOFRawsTimeOC", "TOF Raws - Hit time (ns) - O/C side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFRawsTimeOC.get());

  mTOFRawsToT = std::make_shared<TH1F>("TOFRawsToT", "TOF Raws - Hit ToT (ns);Measured Hit ToT (ns);Hits", fgNbinsToT, fgRangeMinToT, fgRangeMaxToT);
  getObjectsManager()->startPublishing(mTOFRawsToT.get());

  mTOFRawsToTIA = std::make_shared<TH1F>("TOFRawsToTIA", "TOF Raws - Hit ToT (ns) - I/A side;Measured Hit ToT (ns);Hits", fgNbinsToT, fgRangeMinToT, fgRangeMaxToT);
  getObjectsManager()->startPublishing(mTOFRawsToTIA.get());

  mTOFRawsToTOA = std::make_shared<TH1F>("TOFRawsToTOA", "TOF Raws - Hit ToT (ns) - O/A side;Measured Hit ToT (ns);Hits", fgNbinsToT, fgRangeMinToT, fgRangeMaxToT);
  getObjectsManager()->startPublishing(mTOFRawsToTOA.get());

  mTOFRawsToTIC = std::make_shared<TH1F>("TOFRawsToTIC", "TOF Raws - Hit ToT (ns) - I/C side;Measured Hit ToT (ns);Hits", fgNbinsToT, fgRangeMinToT, fgRangeMaxToT);
  getObjectsManager()->startPublishing(mTOFRawsToTIC.get());

  mTOFRawsToTOC = std::make_shared<TH1F>("TOFRawsToTOC", "TOF Raws - Hit ToT (ns) - O/C side;Measured Hit ToT (ns);Hits", fgNbinsToT, fgRangeMinToT, fgRangeMaxToT);
  getObjectsManager()->startPublishing(mTOFRawsToTOC.get());

  mTOFRawHitMap = std::make_shared<TH2F>("TOFRawHitMap", "TOF raw hit map;sector + FEA/4; strip", RawDataDecoder::ncrates, 0., 18., RawDataDecoder::nstrips, 0., RawDataDecoder::nstrips);
  mTOFRawHitMap->SetBit(TH1::kNoStats);
  getObjectsManager()->startPublishing(mTOFRawHitMap.get());
  getObjectsManager()->setDefaultDrawOptions(mTOFRawHitMap.get(), "colz logz");
  getObjectsManager()->setDisplayHint(mTOFRawHitMap.get(), "colz logz");

  if (fgDiagnostic) {
    mTOFDecodingErrors = std::make_shared<TH2I>("TOFDecodingErrors", "Decoding error monitoring; DDL; Error ", RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates, 13, 1, 14);
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
    getObjectsManager()->startPublishing(mTOFDecodingErrors.get());
  }
  // mTOFOrphansTime = std::make_shared<TH1F>("TOFOrphansTime", "TOF Raws - Orphans time (ns);Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  // getObjectsManager()->startPublishing(mTOFOrphansTime.get());

  // mTOFRawTimeVsTRM035 = std::make_shared<TH2F>("TOFRawTimeVsTRM035", "TOF raws - Hit time vs TRM - crates 0 to 35; TRM index = DDL*10+TRM(0-9);TOF raw time [ns]", 361, 0., 361., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  // getObjectsManager()->startPublishing(mTOFRawTimeVsTRM035.get());

  // mTOFRawTimeVsTRM3671 = std::make_shared<TH2F>("TOFRawTimeVsTRM3671", "TOF raws - Hit time vs TRM - crates 36 to RawDataDecoder::ncrates; TRM index = DDL**10+TRM(0-9);TOF raw time [ns]", 361, 360., 721., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  // getObjectsManager()->startPublishing(mTOFRawTimeVsTRM3671.get());

  // mTOFTimeVsStrip = std::make_shared<TH2F>("TOFTimeVsStrip", "TOF raw hit time vs. MRPC (along z axis); MRPC index along z axis; Raws TOF time (ns) ", RawDataDecoder::nstrips, 0., RawDataDecoder::nstrips, fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  // getObjectsManager()->startPublishing(mTOFTimeVsStrip.get());

  mTOFtimeVsBCID = std::make_shared<TH2F>("TOFtimeVsBCID", "TOF time vs BCID;BC time (24.4 ps);time (ns) ", 1024, 0., 1024., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  getObjectsManager()->startPublishing(mTOFtimeVsBCID.get());

  // mTOFchannelEfficiencyMap = std::make_shared<TH2F>("TOFchannelEfficiencyMap", "TOF channels (HWok && efficient && !noisy && !problematic);sector;strip", RawDataDecoder::ncrates, 0., 18., RawDataDecoder::nstrips, 0., RawDataDecoder::nstrips);
  // getObjectsManager()->startPublishing(mTOFchannelEfficiencyMap.get());

  // mTOFhitsCTTM = std::make_shared<TH2F>("TOFhitsCTTM", "Map of hit pads according to CTTM numbering;LTM index;bit index", RawDataDecoder::ncrates, 0., RawDataDecoder::ncrates, 23, 0., 23.);
  // getObjectsManager()->startPublishing(mTOFhitsCTTM.get());

  // mTOFmacropadCTTM = std::make_shared<TH2F>("TOFmacropadCTTM", "Map of hit macropads according to CTTM numbering;LTM index; bit index", RawDataDecoder::ncrates, 0., RawDataDecoder::ncrates, 23, 0., 23.);
  // getObjectsManager()->startPublishing(mTOFmacropadCTTM.get());

  // mTOFmacropadDeltaPhiTime = std::make_shared<TH2F>("TOFmacropadDeltaPhiTime", "#Deltat vs #Delta#Phi of hit macropads;#Delta#Phi (degrees);#DeltaBX", 18, 0., 180., 20, 0., 20.0);
  // getObjectsManager()->startPublishing(mTOFmacropadDeltaPhiTime.get());

  // mBXVsCttmBit = std::make_shared<TH2I>("BXVsCttmBit", "BX ID in TOF matching window vs trg channel; trg channel; BX", 1728, 0, 1728, 24, 0, 24);
  // getObjectsManager()->startPublishing(mBXVsCttmBit.get());

  // mTimeVsCttmBit = std::make_shared<TH2F>("TimeVsCttmBit", "TOF raw time vs trg channel; trg channel; raw time (ns)", 1728, 0., 1728., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime);
  // getObjectsManager()->startPublishing(mTimeVsCttmBit.get());

  // mTOFRawHitMap24 = std::make_shared<TH2F>("TOFRawHitMap24", "TOF average raw hits/channel map (1 bin = 1 FEA = 24 channels);sector;strip", RawDataDecoder::ncrates, 0., 18., RawDataDecoder::nstrips, 0., RawDataDecoder::nstrips);
  // getObjectsManager()->startPublishing(mTOFRawHitMap24.get());

  // mHitMultiVsDDL = std::make_shared<TH2I>("itMultiVsDDL", "TOF raw hit multiplicity per event vs DDL ; DDL; TOF raw hits number; Events ", RawDataDecoder::ncrates, 0., RawDataDecoder::ncrates, 500, 0, 500);
  // getObjectsManager()->startPublishing(mHitMultiVsDDL.get());

  // mNfiredMacropad = std::make_shared<TH1I>("NfiredMacropad", "Number of fired TOF macropads per event; number of fired macropads; Events ", 50, 0, 50);
  // getObjectsManager()->startPublishing(mNfiredMacropad.get());

  mOrbitDDL = std::make_shared<TProfile2D>("OrbitDDL", "Orbits in TF vs DDL ; DDL; Orbits in TF; Fraction", RawDataDecoder::ncrates, 0., RawDataDecoder::ncrates, 256 * 3, 0, 256);
  getObjectsManager()->startPublishing(mOrbitDDL.get());

  mROWSize = std::make_shared<TH1I>("mROWSize", "N Orbits in TF; Orbits in TF", 300, 0., 300.);
  getObjectsManager()->startPublishing(mROWSize.get());
}

void TaskDigits::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
}

void TaskDigits::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  // Get TOF digits
  const auto& digits = ctx.inputs().get<gsl::span<o2::tof::Digit>>("tofdigits");
  // Get TOF Readout window
  const auto& rows = ctx.inputs().get<std::vector<o2::tof::ReadoutWindowData>>("readoutwin");
  // Get TOF Diagnostic words
  //  const auto& diagnostics = ctx.inputs().get<std::vector<uint8_t>>("patterns");

  int eta, phi;       // Eta and phi indices
  int det[5] = { 0 }; // Coordinates
  int strip = 0;      // Strip
  float tdc_time = 0;
  float tot_time = 0;
  // SM in side I: 14-17, 0-4 -> 4 + 5
  // SM in side O: 5-13 -> 9
  // phi is counted every pad starting from SM 0.
  // There are 48 pads per SM. Side I is from phi 0:48*4 and 48*14:48*18
  constexpr int phi_I1 = 48 * 4;
  constexpr int phi_I2 = 48 * 14;
  // eta is counted every half strip starting from strip 0.
  // Halves strips in side A 0-90, in side C RawDataDecoder::nstrips-181
  constexpr int half_eta = RawDataDecoder::nstrips;
  Bool_t isSectorI = kFALSE;
  int ndigits[4] = { 0 }; // Number of digits per side I/A,O/A,I/C,O/C

  mROWSize->Fill(rows.size() / 3.0);

  int currentrow = 0;
  int currentDia = 0;
  // Loop on readout windows
  for (const auto& row : rows) {
    for (unsigned int i = 0; i < RawDataDecoder::ncrates; i++) { // Loop on all crates
      mOrbitDDL->Fill(i, currentrow / 3.0, !row.isEmptyCrate(i));
      //
      if (row.isEmptyCrate(i)) { // Only for active crates
        continue;
      }
      mOrbitID->Fill(row.mFirstIR.orbit % 1048576, i);
      mTimeBC->Fill(row.mFirstIR.bc % o2::tof::Geo::BC_IN_ORBIT, i);
      mEventCounter->Fill(row.mEventCounter % 1000, i);

      // check patterns
      if (fgDiagnostic) {
        // Get TOF Diagnostic words
        const auto& diagnostics = ctx.inputs().get<std::vector<uint8_t>>("patterns");

        int nDia = row.getDiagnosticInCrate(i);

        mTOFDecodingErrors->Fill(i, 1);

        int slot = -1;
        int lastslot = -1;
        for (int idia = currentDia; idia < currentDia + nDia; idia++) {
          const uint8_t& el = diagnostics[idia];

          if (el > 28) { // new slot
            slot = el - 28;
          } else if (slot > -1 && lastslot != slot) { // fill only one time per TRM and row
            // fill error
            mTOFDecodingErrors->Fill(i, slot);
            lastslot = slot;
          }
        }
        currentDia += nDia;
      }
    }
    currentrow++;
    //
    mTOFRawsMulti->Fill(row.size()); // Number of digits inside a readout window

    const auto& digits_in_row = row.getBunchChannelData(digits); // Digits inside a readout window
    // Loop on digits
    for (auto const& digit : digits_in_row) {
      if (digit.getChannel() < 0) {
        LOG(error) << "No valid channel";
        continue;
      }
      o2::tof::Geo::getVolumeIndices(digit.getChannel(), det);
      strip = o2::tof::Geo::getStripNumberPerSM(det[1], det[2]); // Strip index in the SM
      mHitCounterPerStrip[strip].Count(det[0] * 4 + det[4] / 12);
      mHitCounterPerChannel.Count(digit.getChannel());
      // TDC time and ToT time
      tdc_time = (digit.getTDC() + digit.getIR().bc * 1024) * o2::tof::Geo::TDCBIN * 0.001;
      tot_time = digit.getTOT() * o2::tof::Geo::TOTBIN_NS;
      mTOFtimeVsBCID->Fill(row.mFirstIR.bc % 1024, tdc_time);
      mTOFRawsTime->Fill(tdc_time);
      mTOFRawsToT->Fill(tot_time);
      digit.getPhiAndEtaIndex(phi, eta);
      isSectorI = phi < phi_I1 || phi > phi_I2;
      if (eta < half_eta) { // Sector A
        if (isSectorI) {    // Sector I/A
          mTOFRawsTimeIA->Fill(tdc_time);
          mTOFRawsToTIA->Fill(tot_time);
          ndigits[0]++;
        } else { // Sector O/A
          mTOFRawsTimeOA->Fill(tdc_time);
          mTOFRawsToTOA->Fill(tot_time);
          ndigits[1]++;
        }
      } else {           // Sector C
        if (isSectorI) { // Sector I/C
          mTOFRawsTimeIC->Fill(tdc_time);
          mTOFRawsToTIC->Fill(tot_time);
          ndigits[2]++;
        } else { // Sector O/C
          mTOFRawsTimeOC->Fill(tdc_time);
          mTOFRawsToTOC->Fill(tot_time);
          ndigits[3]++;
        }
      }
    }
    // Filling histograms of hit multiplicity
    mTOFRawsMultiIA->Fill(ndigits[0]);
    mTOFRawsMultiOA->Fill(ndigits[1]);
    mTOFRawsMultiIC->Fill(ndigits[2]);
    mTOFRawsMultiOC->Fill(ndigits[3]);
    //
    ndigits[0] = 0;
    ndigits[1] = 0;
    ndigits[2] = 0;
    ndigits[3] = 0;
  }

  //To complete the second TF in case it receives orbits
  for (; currentrow < 768; currentrow++) {
    for (unsigned int i = 0; i < RawDataDecoder::ncrates; i++) { // Loop on all crates
      mOrbitDDL->Fill(i, currentrow / 3.0, 0);
    }
  }
}

void TaskDigits::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  for (unsigned int i = 0; i < RawDataDecoder::nstrips; i++) {
    mHitCounterPerStrip[i].FillHistogram(mTOFRawHitMap.get(), i + 1);
  }
}

void TaskDigits::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskDigits::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Support) << "Resetting the counters" << ENDM;
  for (unsigned int i = 0; i < RawDataDecoder::nstrips; i++) {
    mHitCounterPerStrip[i].Reset();
  }
  mHitCounterPerChannel.Reset();

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  // Event info
  mOrbitID->Reset();
  mTimeBC->Reset();
  mEventCounter->Reset();
  mTOFRawHitMap->Reset();
  mTOFtimeVsBCID->Reset();
  mOrbitDDL->Reset();
  mROWSize->Reset();
  // Multiplicity
  mTOFRawsMulti->Reset();
  mTOFRawsMultiIA->Reset();
  mTOFRawsMultiOA->Reset();
  mTOFRawsMultiIC->Reset();
  mTOFRawsMultiOC->Reset();
  // Time
  mTOFRawsTime->Reset();
  mTOFRawsTimeIA->Reset();
  mTOFRawsTimeOA->Reset();
  mTOFRawsTimeIC->Reset();
  mTOFRawsTimeOC->Reset();
  // ToT
  mTOFRawsToT->Reset();
  mTOFRawsToTIA->Reset();
  mTOFRawsToTOA->Reset();
  mTOFRawsToTIC->Reset();
  mTOFRawsToTOC->Reset();
  // mTOFRawsLTMHits->Reset();
  // mTOFrefMap->Reset();
  if (fgDiagnostic) {
    mTOFDecodingErrors->Reset();
  }
  // mTOFOrphansTime->Reset();
  // mTOFRawTimeVsTRM035->Reset();
  // mTOFRawTimeVsTRM3671->Reset();
  // mTOFTimeVsStrip->Reset();
  // mTOFchannelEfficiencyMap->Reset();
  // mTOFhitsCTTM->Reset();
  // mTOFmacropadCTTM->Reset();
  // mTOFmacropadDeltaPhiTime->Reset();
  // mBXVsCttmBit->Reset();
  // mTimeVsCttmBit->Reset();
  // mTOFRawHitMap24->Reset();
  // mHitMultiVsDDL->Reset();
  // mNfiredMacropad->Reset();
}

} // namespace o2::quality_control_modules::tof
