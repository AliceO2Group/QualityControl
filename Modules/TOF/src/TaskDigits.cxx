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

Int_t TaskDigits::fgNbinsMultiplicity = 2000;       /// Number of bins in multiplicity plot
Int_t TaskDigits::fgRangeMinMultiplicity = 0;       /// Min range in multiplicity plot
Int_t TaskDigits::fgRangeMaxMultiplicity = 1000;    /// Max range in multiplicity plot
Int_t TaskDigits::fgNbinsTime = 250;                /// Number of bins in time plot
const Float_t TaskDigits::fgkNbinsWidthTime = 2.44; /// Width of bins in time plot
Float_t TaskDigits::fgRangeMinTime = 0.0;           /// Range min in time plot
Float_t TaskDigits::fgRangeMaxTime = 620.0;         /// Range max in time plot
Int_t TaskDigits::fgCutNmaxFiredMacropad = 50;      /// Cut on max number of fired macropad
const Int_t TaskDigits::fgkFiredMacropadLimit = 50; /// Limit on cut on number of fired macropad

TaskDigits::TaskDigits() : TaskInterface()
{
}

TaskDigits::~TaskDigits()
{
  mTOFRawsMulti.reset();
  mTOFRawsMultiIA.reset();
  mTOFRawsMultiOA.reset();
  mTOFRawsMultiIC.reset();
  mTOFRawsMultiOC.reset();
  mTOFRawsTime.reset();
  mTOFRawsTimeIA.reset();
  mTOFRawsTimeOA.reset();
  mTOFRawsTimeIC.reset();
  mTOFRawsTimeOC.reset();
  mTOFRawsToT.reset();
  mTOFRawsToTIA.reset();
  mTOFRawsToTOA.reset();
  mTOFRawsToTIC.reset();
  mTOFRawsToTOC.reset();
  mTOFRawsLTMHits.reset();
  mTOFrefMap.reset();
  mTOFRawHitMap.reset();
  mTOFDecodingErrors.reset();
  mTOFOrphansTime.reset();
  mTOFRawTimeVsTRM035.reset();
  mTOFRawTimeVsTRM3671.reset();
  mTOFTimeVsStrip.reset();
  mTOFtimeVsBCID.reset();
  mTOFchannelEfficiencyMap.reset();
  mTOFhitsCTTM.reset();
  mTOFmacropadCTTM.reset();
  mTOFmacropadDeltaPhiTime.reset();
  mBXVsCttmBit.reset();
  mTimeVsCttmBit.reset();
  mTOFRawHitMap24.reset();
  mHitMultiVsDDL.reset();
  mNfiredMacropad.reset();
  mOrbitID.reset();
  mTimeBC.reset();
  mEventCounter.reset();
}

void TaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize TaskDigits" << ENDM;

  mOrbitID.reset(new TH2F("OrbitID", "OrbitID;OrbitID % 1048576;Crate", 1024, 0, 1048576, 72, 0, 72));
  getObjectsManager()->startPublishing(mOrbitID.get());

  mTimeBC.reset(new TH2F("TimeBC", "Raw BC Time;BC time (24.4 ps);Crate", 1024, 0., 1024., 72, 0, 72));
  getObjectsManager()->startPublishing(mTimeBC.get());

  mEventCounter.reset(new TH2F("EventCounter", "Event Counter;Event counter % 1000;Crate", 1000, 0., 1000., 72, 0, 72));
  getObjectsManager()->startPublishing(mEventCounter.get());

  mTOFRawsMulti.reset(new TH1I("TOFRawsMulti", "TOF raw hit multiplicity; TOF raw hits number; Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity));
  getObjectsManager()->startPublishing(mTOFRawsMulti.get());

  mTOFRawsMultiIA.reset(new TH1I("TOFRawsMultiIA", "TOF raw hit multiplicity - I/A side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity));
  getObjectsManager()->startPublishing(mTOFRawsMultiIA.get());

  mTOFRawsMultiOA.reset(new TH1I("TOFRawsMultiOA", "TOF raw hit multiplicity - O/A side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity));
  getObjectsManager()->startPublishing(mTOFRawsMultiOA.get());

  mTOFRawsMultiIC.reset(new TH1I("TOFRawsMultiIC", "TOF raw hit multiplicity - I/C side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity));
  getObjectsManager()->startPublishing(mTOFRawsMultiIC.get());

  mTOFRawsMultiOC.reset(new TH1I("TOFRawsMultiOC", "TOF raw hit multiplicity - O/C side; TOF raw hits number;Events ", fgNbinsMultiplicity, fgRangeMinMultiplicity, fgRangeMaxMultiplicity));
  getObjectsManager()->startPublishing(mTOFRawsMultiOC.get());

  mTOFRawsTime.reset(new TH1F("TOFRawsTime", "TOF Raws - Hit time (ns);Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawsTime.get());

  mTOFRawsTimeIA.reset(new TH1F("TOFRawsTimeIA", "TOF Raws - Hit time (ns) - I/A side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawsTimeIA.get());

  mTOFRawsTimeOA.reset(new TH1F("TOFRawsTimeOA", "TOF Raws - Hit time (ns) - O/A side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawsTimeOA.get());

  mTOFRawsTimeIC.reset(new TH1F("TOFRawsTimeIC", "TOF Raws - Hit time (ns) - I/C side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawsTimeIC.get());

  mTOFRawsTimeOC.reset(new TH1F("TOFRawsTimeOC", "TOF Raws - Hit time (ns) - O/C side;Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawsTimeOC.get());

  mTOFRawsToT.reset(new TH1F("TOFRawsToT", "TOF Raws - Hit ToT (ns);Measured Hit ToT (ns);Hits", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mTOFRawsToT.get());

  mTOFRawsToTIA.reset(new TH1F("TOFRawsToTIA", "TOF Raws - Hit ToT (ns) - I/A side;Measured Hit ToT (ns);Hits", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mTOFRawsToTIA.get());

  mTOFRawsToTOA.reset(new TH1F("TOFRawsToTOA", "TOF Raws - Hit ToT (ns) - O/A side;Measured Hit ToT (ns);Hits", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mTOFRawsToTOA.get());

  mTOFRawsToTIC.reset(new TH1F("TOFRawsToTIC", "TOF Raws - Hit ToT (ns) - I/C side;Measured Hit ToT (ns);Hits", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mTOFRawsToTIC.get());

  mTOFRawsToTOC.reset(new TH1F("TOFRawsToTOC", "TOF Raws - Hit ToT (ns) - O/C side;Measured Hit ToT (ns);Hits", 100, 0., 48.8));
  getObjectsManager()->startPublishing(mTOFRawsToTOC.get());

  mTOFRawsLTMHits.reset(new TH1F("TOFRawsLTMHits", "LTMs OR signals; Crate; Counts", 72, 0., 72.));
  getObjectsManager()->startPublishing(mTOFRawsLTMHits.get());

  mTOFrefMap.reset(new TH2F("TOFrefMap", "TOF enabled channel reference map;sector;strip", 72, 0., 18., 91, 0., 91.));
  getObjectsManager()->startPublishing(mTOFrefMap.get());

  mTOFRawHitMap.reset(new TH2F("TOFRawHitMap", "TOF raw hit map (1 bin = 1 FEA = 24 channels);sector;strip", 72, 0., 18., 91, 0., 91.));
  getObjectsManager()->startPublishing(mTOFRawHitMap.get());

  mTOFDecodingErrors.reset(new TH2I("TOFDecodingErrors", "Decoding error monitoring; DDL; Error ", 72, 0, 72, 13, 1, 14));
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

  mTOFOrphansTime.reset(new TH1F("TOFOrphansTime", "TOF Raws - Orphans time (ns);Measured Hit time [ns];Hits", fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFOrphansTime.get());

  mTOFRawTimeVsTRM035.reset(new TH2F("TOFRawTimeVsTRM035", "TOF raws - Hit time vs TRM - crates 0 to 35; TRM index = DDL*10+TRM(0-9);TOF raw time [ns]", 361, 0., 361., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawTimeVsTRM035.get());

  mTOFRawTimeVsTRM3671.reset(new TH2F("TOFRawTimeVsTRM3671", "TOF raws - Hit time vs TRM - crates 36 to 72; TRM index = DDL**10+TRM(0-9);TOF raw time [ns]", 361, 360., 721., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFRawTimeVsTRM3671.get());

  mTOFTimeVsStrip.reset(new TH2F("TOFTimeVsStrip", "TOF raw hit time vs. MRPC (along z axis); MRPC index along z axis; Raws TOF time (ns) ", 91, 0., 91, fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFTimeVsStrip.get());

  mTOFtimeVsBCID.reset(new TH2F("TOFtimeVsBCID", "TOF time vs BCID; BCID; time (ns) ", 3564, 0., 3564., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTOFtimeVsBCID.get());

  mTOFchannelEfficiencyMap.reset(new TH2F("TOFchannelEfficiencyMap", "TOF channels (HWok && efficient && !noisy && !problematic);sector;strip", 72, 0., 18., 91, 0., 91.));
  getObjectsManager()->startPublishing(mTOFchannelEfficiencyMap.get());

  mTOFhitsCTTM.reset(new TH2F("TOFhitsCTTM", "Map of hit pads according to CTTM numbering;LTM index;bit index", 72, 0., 72., 23, 0., 23.));
  getObjectsManager()->startPublishing(mTOFhitsCTTM.get());

  mTOFmacropadCTTM.reset(new TH2F("TOFmacropadCTTM", "Map of hit macropads according to CTTM numbering;LTM index; bit index", 72, 0., 72., 23, 0., 23.));
  getObjectsManager()->startPublishing(mTOFmacropadCTTM.get());

  mTOFmacropadDeltaPhiTime.reset(new TH2F("TOFmacropadDeltaPhiTime", "#Deltat vs #Delta#Phi of hit macropads;#Delta#Phi (degrees);#DeltaBX", 18, 0., 180., 20, 0., 20.0));
  getObjectsManager()->startPublishing(mTOFmacropadDeltaPhiTime.get());

  mBXVsCttmBit.reset(new TH2I("BXVsCttmBit", "BX ID in TOF matching window vs trg channel; trg channel; BX", 1728, 0, 1728, 24, 0, 24));
  getObjectsManager()->startPublishing(mBXVsCttmBit.get());

  mTimeVsCttmBit.reset(new TH2F("TimeVsCttmBit", "TOF raw time vs trg channel; trg channel; raw time (ns)", 1728, 0., 1728., fgNbinsTime, fgRangeMinTime, fgRangeMaxTime));
  getObjectsManager()->startPublishing(mTimeVsCttmBit.get());

  mTOFRawHitMap24.reset(new TH2F("TOFRawHitMap24", "TOF average raw hits/channel map (1 bin = 1 FEA = 24 channels);sector;strip", 72, 0., 18., 91, 0., 91.));
  getObjectsManager()->startPublishing(mTOFRawHitMap24.get());

  mHitMultiVsDDL.reset(new TH2I("itMultiVsDDL", "TOF raw hit multiplicity per event vs DDL ; DDL; TOF raw hits number; Events ", 72, 0., 72., 500, 0, 500));
  getObjectsManager()->startPublishing(mHitMultiVsDDL.get());

  mNfiredMacropad.reset(new TH1I("NfiredMacropad", "Number of fired TOF macropads per event; number of fired macropads; Events ", 50, 0, 50));
  getObjectsManager()->startPublishing(mNfiredMacropad.get());
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
  const auto digits = ctx.inputs().get<gsl::span<o2::tof::Digit>>("tofdigits");
  // Get TOF Readout window
  const auto rows = ctx.inputs().get<std::vector<o2::tof::ReadoutWindowData>>("readoutwin");

  Int_t eta, phi;       // Eta and phi indices
  Int_t det[5] = { 0 }; // Coordinates
  Int_t strip = 0;      // Strip
  Float_t tdc_time = 0;
  Float_t tot_time = 0;
  // SM in side I: 14-17, 0-4 -> 4 + 5
  // SM in side O: 5-13 -> 9
  // phi is counted every pad starting from SM 0.
  // There are 48 pads per SM. Side I is from phi 0:48*4 and 48*14:48*18
  const Int_t phi_I1 = 48 * 4;
  const Int_t phi_I2 = 48 * 14;
  // eta is counted every half strip starting from strip 0.
  // Halves strips in side A 0-90, in side C 91-181
  const Int_t half_eta = 91;
  Bool_t isSectorI = kFALSE;
  Int_t ndigits[4] = { 0 }; // Number of digits per side I/A,O/A,I/C,O/C

  // Loop on readout windows
  for (const auto& row : rows) {
    for (int i = 0; i < 72; i++) { // Loop on all crates
      if (row.isEmptyCrate(i)) {   // Only for active crates
        continue;
      }
      mOrbitID->Fill(row.mFirstIR.orbit % 1048576, i);
      mTimeBC->Fill(row.mFirstIR.bc % 1024, i);
      mEventCounter->Fill(row.mEventCounter % 1000, i);
    }
    mTOFRawsMulti->Fill(row.size()); // Number of digits inside a readout window

    const auto digits_in_row = row.getBunchChannelData(digits); // Digits inside a readout window
    // Loop on digits
    for (auto const& digit : digits_in_row) {
      o2::tof::Geo::getVolumeIndices(digit.getChannel(), det);
      strip = o2::tof::Geo::getStripNumberPerSM(det[1], det[2]); // Strip index in the SM
      const Int_t ech = o2::tof::Geo::getECHFromCH(digit.getChannel());
      mHitCounterPerStrip[strip].Count(o2::tof::Geo::getCrateFromECH(ech));
      mHitCounterPerChannel.Count(digit.getChannel());
      // TDC time and ToT time
      tdc_time = digit.getTDC() * o2::tof::Geo::TDCBIN * 0.001;
      tot_time = digit.getTOT() * o2::tof::Geo::TOTBIN_NS;
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
}

void TaskDigits::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  for (int i = 0; i < 91; i++) {
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

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
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
  mOrbitID->Reset();
  mTimeBC->Reset();
  mEventCounter->Reset();
}

} // namespace o2::quality_control_modules::tof
