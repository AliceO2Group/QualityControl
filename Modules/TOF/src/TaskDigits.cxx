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
/// \author Nicolò Jacazio <nicolo.jacazio@cern.ch>
/// \brief  Task to monitor quantities in TOF digits in both data and MC
///

// ROOT includes
#include <TCanvas.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH2S.h>
#include <TH1I.h>
#include <TH2I.h>
#include <TProfile2D.h>
#include <TProfile.h>

// O2 includes
#include "TOFBase/Digit.h"
#include "TOFBase/Geo.h"
#include <Framework/InputRecord.h>
#include "CommonConstants/LHCConstants.h"
#include "DataFormatsTOF/Diagnostic.h"
#include "CCDB/BasicCCDBManager.h"
#include "Framework/TimingInfo.h"
#include "DetectorsBase/GeometryManager.h"

// Fairlogger includes
#include <fairlogger/Logger.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskDigits.h"
#include "TOF/Utils.h"

namespace o2::quality_control_modules::tof
{

TaskDigits::TaskDigits() : TaskInterface()
{
}

void TaskDigits::initialize(o2::framework::InitContext& /*ctx*/)
{
  // Define parameters
  utils::parseIntParameter(mCustomParameters, "NbinsMultiplicity", mBinsMultiplicity);
  utils::parseIntParameter(mCustomParameters, "RangeMaxMultiplicity", mRangeMaxMultiplicity);

  utils::parseIntParameter(mCustomParameters, "NbinsTime", mBinsTime);
  utils::parseFloatParameter(mCustomParameters, "kNbinsWidthTime", fgkNbinsWidthTime);
  utils::parseFloatParameter(mCustomParameters, "RangeMinTime", mRangeMinTime);
  utils::parseFloatParameter(mCustomParameters, "RangeMaxTime", mRangeMaxTime);

  utils::parseIntParameter(mCustomParameters, "NbinsToT", mBinsToT);
  utils::parseFloatParameter(mCustomParameters, "RangeMinTime", mRangeMinToT);
  utils::parseFloatParameter(mCustomParameters, "RangeMaxTime", mRangeMaxToT);

  if (utils::parseIntParameter(mCustomParameters, "NoiseClassSelection", mNoiseClassSelection)) {
    if (mNoiseClassSelection < -1 || mNoiseClassSelection >= nNoiseClasses) {
      ILOG(Error, Support) << "Asked to discard noise class " << mNoiseClassSelection << " but it is invalid, use -1, 0, 1, 2. Setting it to -1 (no selection)" << ENDM;
      mNoiseClassSelection = -1;
    }
  }
  if (auto param = mCustomParameters.find("applyCalib"); param != mCustomParameters.end()) {
    ILOG(Debug, Devel) << "Custom parameter - applyCalib: " << param->second << ENDM;
    if (param->second == "true" || param->second == "True" || param->second == "TRUE") {
      mApplyCalib = true;
    }
  }
  utils::parseBooleanParameter(mCustomParameters, "Diagnostic", mFlagEnableDiagnostic);
  utils::parseBooleanParameter(mCustomParameters, "PerChannel", mFlagEnableOrphanPerChannel);

  // Define histograms
  ILOG(Debug, Devel) << "initialize TaskDigits" << ENDM;

  // Event info
  mHistoOrbitID = std::make_shared<TH2F>("OrbitID", Form("TOF OrbitID;OrbitID %% %i;Crate", mRangeMaxOrbitId), mBinsOrbitId, 0, mRangeMaxOrbitId, RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mHistoOrbitID.get());

  mHistoBCID = std::make_shared<TH2F>("TimeBC", "TOF readout window BC ID;BC ID in orbit (~25 ns);Crate", mBinsBC, 0., mRangeMaxBC, RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mHistoBCID.get());

  mHistoEventCounter = std::make_shared<TH2F>("EventCounter", Form("TOF event Counter;Event counter %% %i;Crate", mRangeMaxEventCounter), mRangeMaxEventCounter, 0., mRangeMaxEventCounter, RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mHistoEventCounter.get());

  mHistoHitMap = std::make_shared<TH2F>("HitMap", "TOF hit map;Sector;Strip", RawDataDecoder::ncrates, 0., RawDataDecoder::nsectors, RawDataDecoder::nstrips, 0., RawDataDecoder::nstrips);
  mHistoHitMap->SetBit(TH1::kNoStats);
  getObjectsManager()->startPublishing(mHistoHitMap.get());
  getObjectsManager()->setDefaultDrawOptions(mHistoHitMap.get(), "colz logz");
  getObjectsManager()->setDisplayHint(mHistoHitMap.get(), "colz logz");

  mHistoHitMapNoiseFiltered = std::make_shared<TH2F>("HitMapNoiseFiltered", "TOF hit map (noise filtered);Sector;Strip", RawDataDecoder::ncrates, 0., RawDataDecoder::nsectors, RawDataDecoder::nstrips, 0., RawDataDecoder::nstrips);
  mHistoHitMapNoiseFiltered->SetBit(TH1::kNoStats);
  getObjectsManager()->startPublishing(mHistoHitMapNoiseFiltered.get());
  getObjectsManager()->setDefaultDrawOptions(mHistoHitMapNoiseFiltered.get(), "colz logz");
  getObjectsManager()->setDisplayHint(mHistoHitMapNoiseFiltered.get(), "colz logz");

  mHistoTimeVsBCID = std::make_shared<TH2F>("TimeVsBCID", "TOF time vs BC ID;BC ID in orbit (~25 ns);time (ns)", mBinsBC, 0., mRangeMaxBC, mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTimeVsBCID.get());

  mHistoOrbitVsCrate = std::make_shared<TProfile2D>("OrbitVsCrate", "TOF Orbits in TF vs Crate;Crate;Orbits in TF;Fraction", RawDataDecoder::ncrates, 0., RawDataDecoder::ncrates, mBinsOrbitPerTimeFrame, 0, mRangeMaxOrbitPerTimeFrame);
  getObjectsManager()->startPublishing(mHistoOrbitVsCrate.get());

  mHistoROWSize = std::make_shared<TH1I>("ReadoutWindowSize", "N Orbits in TF;Orbits in TF", 300, 0., 300.);
  getObjectsManager()->startPublishing(mHistoROWSize.get());

  if (mFlagEnableDiagnostic) {
    mHistoDecodingErrors = std::make_shared<TH2I>("DecodingErrors", "TOF decoding error;Crate;Error", RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates, 13, 1, 14);
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(1, "DRM");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(2, "LTM");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(3, "TRM 3");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(4, "TRM 4");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(5, "TRM 5");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(6, "TRM 6");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(7, "TRM 7");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(8, "TRM 8");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(9, "TRM 9");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(10, "TRM 10");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(11, "TRM 11");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(12, "TRM 12");
    mHistoDecodingErrors->GetYaxis()->SetBinLabel(13, "recovered");
    getObjectsManager()->startPublishing(mHistoDecodingErrors.get());
  }

  if (mFlagEnableOrphanPerChannel) {
    mHistoOrphanPerChannel = std::make_shared<TH1S>("OrphanPerChannel", "TOF orphans vs channel;Channel;Counts", nchannels, 0., nchannels);
    getObjectsManager()->startPublishing(mHistoOrphanPerChannel.get());
  }

  mHistoNoisyChannels = std::make_shared<TH2S>("NoisyChannels", "TOF orphans vs channel;Channel;Counts", nchannels, 0., nchannels, nNoiseClasses, 0, nNoiseClasses);
  for (int i = 0; i < nNoiseClasses; i++) {
    mHistoNoisyChannels->GetYaxis()->SetBinLabel(1 + i, Form("Class %i", i));
  }
  getObjectsManager()->startPublishing(mHistoNoisyChannels.get());

  // if mBinsMultiplicity > 1000 limit it in TH2F
  int nBinsMultForTH2 = mBinsMultiplicity;
  if (nBinsMultForTH2 > 1000) {
    nBinsMultForTH2 = 1000;
    ILOG(Info, Support) << "Requested Nbins in multiplicity is " << mBinsMultiplicity << " but limited to 1000 ONLY for TH2 " << ENDM;
  }

  // Multiplicity
  mHistoMultiplicity = std::make_shared<TH1I>("Multiplicity/Integrated", "TOF hit multiplicity;TOF hits;Events ", mBinsMultiplicity, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHistoMultiplicity.get());

  mHistoMultiplicityIA = std::make_shared<TH1I>("Multiplicity/SectorIA", "TOF hit multiplicity - I/A side;TOF hits;Events ", mBinsMultiplicity, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHistoMultiplicityIA.get());

  mHistoMultiplicityOA = std::make_shared<TH1I>("Multiplicity/SectorOA", "TOF hit multiplicity - O/A side;TOF hits;Events ", mBinsMultiplicity, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHistoMultiplicityOA.get());

  mHistoMultiplicityIC = std::make_shared<TH1I>("Multiplicity/SectorIC", "TOF hit multiplicity - I/C side;TOF hits;Events ", mBinsMultiplicity, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHistoMultiplicityIC.get());

  mHistoMultiplicityOC = std::make_shared<TH1I>("Multiplicity/SectorOC", "TOF hit multiplicity - O/C side;TOF hits;Events ", mBinsMultiplicity, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHistoMultiplicityOC.get());

  mHitMultiplicityVsCrate = std::make_shared<TH2F>("Multiplicity/VsCrate", "TOF hit multiplicity vs Crate;Crate;TOF hits", RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates, nBinsMultForTH2, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHitMultiplicityVsCrate.get());

  mHitMultiplicityVsCratepro = std::make_shared<TProfile>("Multiplicity/VsCratepro", "TOF hit multiplicity vs Crate;Crate;#LT TOF hits #GT", RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates);
  getObjectsManager()->startPublishing(mHitMultiplicityVsCratepro.get());

  mHitMultiplicityVsBC = std::make_shared<TH2F>("Multiplicity/VsBC", "TOF hit multiplicity vs BC;BC;#TOF hits;Events", mBinsBCForMultiplicity, 0, mRangeMaxBC, nBinsMultForTH2, mRangeMinMultiplicity, mRangeMaxMultiplicity);
  getObjectsManager()->startPublishing(mHitMultiplicityVsBC.get());

  mHitMultiplicityVsBCpro = std::make_shared<TProfile>("Multiplicity/VsBCpro", "TOF hit multiplicity vs BC;BC;#TOF hits;Events", mBinsBCForMultiplicity, 0, mRangeMaxBC);
  getObjectsManager()->startPublishing(mHitMultiplicityVsBCpro.get());

  // Time
  mHistoTime = std::make_shared<TH1F>("Time/Integrated", "TOF hit time;Hit time (ns);Hits", mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTime.get());

  mHistoTimeIA = std::make_shared<TH1F>("Time/SectorIA", "TOF hit time - I/A side;Hit time (ns);Hits", mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTimeIA.get());

  mHistoTimeOA = std::make_shared<TH1F>("Time/SectorOA", "TOF hit time - O/A side;Hit time (ns);Hits", mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTimeOA.get());

  mHistoTimeIC = std::make_shared<TH1F>("Time/SectorIC", "TOF hit time - I/C side;Hit time (ns);Hits", mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTimeIC.get());

  mHistoTimeOC = std::make_shared<TH1F>("Time/SectorOC", "TOF hit time - O/C side;Hit time (ns);Hits", mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTimeOC.get());

  mHistoTimeOrphans = std::make_shared<TH1F>("Time/Orphans", "TOF hit time - orphans;Hit time (ns);Hits", mBinsTime, mRangeMinTime, mRangeMaxTime);
  getObjectsManager()->startPublishing(mHistoTimeOrphans.get());

  // ToT
  mHistoToT = std::make_shared<TH1F>("ToT/Integrated", "TOF hit ToT;Hit ToT (ns);Hits", mBinsToT, mRangeMinToT, mRangeMaxToT);
  getObjectsManager()->startPublishing(mHistoToT.get());

  mHistoToTIA = std::make_shared<TH1F>("ToT/SectorIA", "TOF hit ToT - I/A side;Hit ToT (ns);Hits", mBinsToT, mRangeMinToT, mRangeMaxToT);
  getObjectsManager()->startPublishing(mHistoToTIA.get());

  mHistoToTOA = std::make_shared<TH1F>("ToT/SectorOA", "TOF hit ToT - O/A side;Hit ToT (ns);Hits", mBinsToT, mRangeMinToT, mRangeMaxToT);
  getObjectsManager()->startPublishing(mHistoToTOA.get());

  mHistoToTIC = std::make_shared<TH1F>("ToT/SectorIC", "TOF hit ToT - I/C side;Hit ToT (ns);Hits", mBinsToT, mRangeMinToT, mRangeMaxToT);
  getObjectsManager()->startPublishing(mHistoToTIC.get());

  mHistoToTOC = std::make_shared<TH1F>("ToT/SectorOC", "TOF hit ToT - O/C side;Hit ToT (ns);Hits", mBinsToT, mRangeMinToT, mRangeMaxToT);
  getObjectsManager()->startPublishing(mHistoToTOC.get());

  // mBXVsCttmBit = std::make_shared<TH2I>("BXVsCttmBit", "BX ID in TOF matching window vs trg channel; trg channel; BX", 1728, 0, 1728, 24, 0, 24);
  // getObjectsManager()->startPublishing(mBXVsCttmBit.get());

  // mTimeVsCttmBit = std::make_shared<TH2F>("TimeVsCttmBit", "TOF raw time vs trg channel; trg channel; raw time (ns)", 1728, 0., 1728., mBinsTime, mRangeMinTime, mRangeMaxTime);
  // getObjectsManager()->startPublishing(mTimeVsCttmBit.get());
}

void TaskDigits::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void TaskDigits::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void TaskDigits::monitorData(o2::framework::ProcessingContext& ctx)
{
  if (mApplyCalib && !mCalChannel) {
    auto creationTime = ctx.services().get<o2::framework::TimingInfo>().creation;
    mCalChannel = o2::ccdb::BasicCCDBManager::instance().getForTimeStamp<o2::dataformats::CalibTimeSlewingParamTOF>("TOF/Calib/ChannelCalib", creationTime);
    mLHCphase = o2::ccdb::BasicCCDBManager::instance().getForTimeStamp<o2::dataformats::CalibLHCphaseTOF>("TOF/Calib/LHCphase", creationTime);
  }

  // Get TOF digits
  const auto& digits = ctx.inputs().get<gsl::span<o2::tof::Digit>>("tofdigits");
  // Get TOF Readout window
  const auto& rows = ctx.inputs().get<std::vector<o2::tof::ReadoutWindowData>>("readoutwin");
  // Get Diagnostic frequency to check noisy channels in the current TF
  const auto& diafreq = ctx.inputs().get<o2::tof::Diagnostic*>("diafreq");

  int nent = mHitMultiplicityVsBC->GetEntries();

  int eta, phi;       // Eta and phi indices
  int det[5] = { 0 }; // Coordinates
  int strip = 0;      // Strip
  float tdc_time = 0.f;
  float tot_time = 0.f;
  // SM in side I: 14-17, 0-4 -> 4 + 5
  // SM in side O: 5-13 -> 9
  // phi is counted every pad starting from SM 0.
  // There are 48 pads per SM. Side I is from phi 0:48*4 and 48*14:48*18
  constexpr int phi_I1 = 48 * 4;
  constexpr int phi_I2 = 48 * 14;
  // eta is counted every half strip starting from strip 0.
  // Halves strips in side A 0-90, in side C RawDataDecoder::nstrips-181
  constexpr int half_eta = RawDataDecoder::nstrips;
  bool isSectorI = false;
  std::array<int, 4> ndigitsPerQuater = { 0 };                      // Number of digits per side I/A,O/A,I/C,O/C
  std::array<int, RawDataDecoder::ncrates> ndigitsPerCrate = { 0 }; // Number of hits in one event per crate
  constexpr int nOrbits = 128;                                      // Number of orbits
  int ndigitsPerBC[nOrbits][mBinsBCForMultiplicity] = {};           // number of digit per oribit, BC/18

  mHistoROWSize->Fill(rows.size() / 3.0);

  int currentReadoutWindow = 0;
  int currentDiagnostics = 0;
  // Loop on readout windows
  for (const auto& row : rows) {
    for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) { // Loop on all crates
      mHistoOrbitVsCrate->Fill(crate, currentReadoutWindow / 3.0, !row.isEmptyCrate(crate));
      //
      if (row.isEmptyCrate(crate)) { // Only for active crates
        continue;
      }
      mHistoOrbitID->Fill(row.mFirstIR.orbit % mRangeMaxOrbitId, crate);
      mHistoBCID->Fill(row.mFirstIR.bc, crate);
      mHistoEventCounter->Fill(row.mEventCounter % mRangeMaxEventCounter, crate);

      // check patterns
      if (mFlagEnableDiagnostic) {
        // Get TOF Diagnostic words
        const auto& diagnostics = ctx.inputs().get<std::vector<uint8_t>>("patterns");

        int nDia = row.getDiagnosticInCrate(crate);

        mHistoDecodingErrors->Fill(crate, 1);

        int slot = -1;
        int lastslot = -1;
        for (int idia = currentDiagnostics; idia < currentDiagnostics + nDia; idia++) {
          const uint8_t& el = diagnostics[idia];

          if (el > 28) { // new slot
            slot = el - 28;
          } else if (slot > -1 && lastslot != slot) { // fill only one time per TRM and row
            // fill error
            mHistoDecodingErrors->Fill(crate, slot);
            lastslot = slot;
          }
        }
        currentDiagnostics += nDia;
      }
    }
    currentReadoutWindow++;

    const auto& digits_in_row = row.getBunchChannelData(digits); // Digits inside a readout window
    // Loop on digits
    for (auto const& digit : digits_in_row) {
      if (digit.getChannel() < 0) {
        LOG(error) << "No valid channel";
        continue;
      }

      for (int i = 0; i < nNoiseClasses; i++) {
        if (!diafreq->isNoisyChannel(digit.getChannel(), i)) {
          continue;
        }
        mCounterNoisyChannels[i].Count(digit.getChannel());
      }

      // Fill hit map counter no matter the selection
      o2::tof::Geo::getVolumeIndices(digit.getChannel(), det);
      strip = o2::tof::Geo::getStripNumberPerSM(det[1], det[2]); // Strip index in the SM
      mCounterHitsPerStrip[strip].Count(det[0] * 4 + det[4] / 12);

      if (mNoiseClassSelection >= 0 &&
          diafreq->isNoisyChannel(digit.getChannel(), mNoiseClassSelection)) {
        //        LOG(info) << "noisy channel " << digit.getChannel();
        continue;
      }
      //      LOG(info) << "good channel " << digit.getChannel();

      // Correct BC index
      int bcCorr = digit.getIR().bc - o2::tof::Geo::LATENCYWINDOW_IN_BC;
      if (bcCorr < 0) {
        bcCorr += o2::constants::lhc::LHCMaxBunches;
      }

      int ech = o2::tof::Geo::getECHFromCH(digit.getChannel());
      int crateECH = o2::tof::Geo::getCrateFromECH(ech);
      int slotECH = o2::tof::Geo::getTRMFromECH(ech);
      int chainECH = o2::tof::Geo::getChainFromECH(ech);
      int tdcECH = o2::tof::Geo::getTDCFromECH(ech);
      int bcCorrCable = bcCorr;
      float pos[3];
      o2::tof::Geo::getPos(det, pos);
      float length = sqrt(pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]);

      if (mCalChannel) {                                                              // calibration
        float timeTDCcorr = digit.getTDC() * o2::tof::Geo::TDCBIN;                    // in ps
        timeTDCcorr -= mCalChannel->evalTimeSlewing(digit.getChannel(), 0.0);
        timeTDCcorr -= mLHCphase->getLHCphase(0);
        timeTDCcorr -= length * 33.356410 - 1000; // subract path (1ns margin)
        bcCorrCable += int(o2::constants::lhc::LHCMaxBunches + timeTDCcorr * o2::tof::Geo::BC_TIME_INPS_INV) - o2::constants::lhc::LHCMaxBunches; // to truncate in the proper way
      } else {
        bcCorrCable -= (o2::tof::Geo::getCableTimeShiftBin(crateECH, slotECH, chainECH, tdcECH) - digit.getTDC()) / 1024; // just cable length
      }

      if (bcCorrCable < 0) {
        bcCorrCable += o2::constants::lhc::LHCMaxBunches;
      }

      if (bcCorrCable >= o2::constants::lhc::LHCMaxBunches) {
        bcCorrCable -= o2::constants::lhc::LHCMaxBunches;
      }

      ndigitsPerBC[row.mFirstIR.orbit % nOrbits][bcCorrCable]++;

      ndigitsPerCrate[crateECH]++;
      mCounterHitsPerStripNoiseFiltered[strip].Count(det[0] * 4 + det[4] / 12);
      mCounterHitsPerChannel.Count(digit.getChannel());
      // TDC time and ToT time
      constexpr float TDCBIN_NS = o2::tof::Geo::TDCBIN * 0.001;
      tdc_time = (digit.getTDC() + bcCorr * 1024) * TDCBIN_NS;
      tot_time = digit.getTOT() * o2::tof::Geo::TOTBIN_NS;
      mHistoTimeVsBCID->Fill(row.mFirstIR.bc, tdc_time);
      mHistoTime->Fill(tdc_time);
      if (tot_time <= 0.f) {
        mHistoTimeOrphans->Fill(tdc_time);
        if (mFlagEnableOrphanPerChannel) {
          mCounterOrphansPerChannel.Count(digit.getChannel());
        }
      }
      mHistoToT->Fill(tot_time);
      digit.getPhiAndEtaIndex(phi, eta);
      isSectorI = phi < phi_I1 || phi > phi_I2;
      if (eta < half_eta) { // Sector A
        if (isSectorI) {    // Sector I/A
          mHistoTimeIA->Fill(tdc_time);
          mHistoToTIA->Fill(tot_time);
          ndigitsPerQuater[0]++;
        } else { // Sector O/A
          mHistoTimeOA->Fill(tdc_time);
          mHistoToTOA->Fill(tot_time);
          ndigitsPerQuater[1]++;
        }
      } else {           // Sector C
        if (isSectorI) { // Sector I/C
          mHistoTimeIC->Fill(tdc_time);
          mHistoToTIC->Fill(tot_time);
          ndigitsPerQuater[2]++;
        } else { // Sector O/C
          mHistoTimeOC->Fill(tdc_time);
          mHistoToTOC->Fill(tot_time);
          ndigitsPerQuater[3]++;
        }
      }
    }
    // Filling histograms of hit multiplicity
    mHistoMultiplicity->Fill(ndigitsPerQuater[0] + ndigitsPerQuater[1] + ndigitsPerQuater[2] + ndigitsPerQuater[3]); // Number of digits inside a readout window
    // Filling quarter histograms
    mHistoMultiplicityIA->Fill(ndigitsPerQuater[0]);
    mHistoMultiplicityOA->Fill(ndigitsPerQuater[1]);
    mHistoMultiplicityIC->Fill(ndigitsPerQuater[2]);
    mHistoMultiplicityOC->Fill(ndigitsPerQuater[3]);

    for (int crate = 0; crate < RawDataDecoder::ncrates; crate++) {
      mHitMultiplicityVsCratepro->Fill(crate, ndigitsPerCrate[crate]);
      mHitMultiplicityVsCrate->Fill(crate, ndigitsPerCrate[crate]);
      ndigitsPerCrate[crate] = 0;
    }
    //
    ndigitsPerQuater[0] = 0;
    ndigitsPerQuater[1] = 0;
    ndigitsPerQuater[2] = 0;
    ndigitsPerQuater[3] = 0;
  }

  for (int iorb = 0; iorb < nOrbits; iorb++) {
    for (int ibc = 0; ibc < mBinsBCForMultiplicity; ibc++) {
      mHitMultiplicityVsBC->Fill(ibc, ndigitsPerBC[iorb][ibc]);
      mHitMultiplicityVsBCpro->Fill(ibc, ndigitsPerBC[iorb][ibc]);
    }
  }

  // To complete the second TF in case it receives orbits
  for (; currentReadoutWindow < 768; currentReadoutWindow++) {
    for (unsigned int i = 0; i < RawDataDecoder::ncrates; i++) { // Loop on all crates
      mHistoOrbitVsCrate->Fill(i, currentReadoutWindow / 3.0, 0);
    }
  }
}

void TaskDigits::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  for (unsigned int i = 0; i < RawDataDecoder::nstrips; i++) {
    mCounterHitsPerStrip[i].FillHistogram(mHistoHitMap.get(), i + 1);
    mCounterHitsPerStripNoiseFiltered[i].FillHistogram(mHistoHitMapNoiseFiltered.get(), i + 1);
  }
  if (mFlagEnableOrphanPerChannel) {
    mCounterOrphansPerChannel.FillHistogram(mHistoOrphanPerChannel.get());
  }
  for (unsigned int i = 0; i < nNoiseClasses; i++) {
    mCounterNoisyChannels[i].FillHistogram(mHistoNoisyChannels.get(), i + 1);
  }
}

void TaskDigits::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void TaskDigits::reset()
{
  // clean all the monitor objects here
  ILOG(Debug, Devel) << "Resetting the counters" << ENDM;
  for (unsigned int i = 0; i < RawDataDecoder::nstrips; i++) {
    mCounterHitsPerStrip[i].Reset();
    mCounterHitsPerStripNoiseFiltered[i].Reset();
  }
  mCounterHitsPerChannel.Reset();
  mCounterOrphansPerChannel.Reset();
  for (unsigned int i = 0; i < nNoiseClasses; i++) {
    mCounterNoisyChannels[i].Reset();
  }

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  // Event info
  mHistoOrbitID->Reset();
  mHistoBCID->Reset();
  mHistoEventCounter->Reset();
  mHistoHitMap->Reset();
  mHistoTimeVsBCID->Reset();
  mHistoOrbitVsCrate->Reset();
  mHistoROWSize->Reset();
  if (mFlagEnableDiagnostic) {
    mHistoDecodingErrors->Reset();
  }
  if (mFlagEnableOrphanPerChannel) {
    mHistoOrphanPerChannel->Reset();
  }
  // Multiplicity
  mHistoMultiplicity->Reset();
  mHistoMultiplicityIA->Reset();
  mHistoMultiplicityOA->Reset();
  mHistoMultiplicityIC->Reset();
  mHistoMultiplicityOC->Reset();
  mHitMultiplicityVsCrate->Reset();
  mHitMultiplicityVsCratepro->Reset();
  mHitMultiplicityVsBC->Reset();
  mHitMultiplicityVsBCpro->Reset();
  // Time
  mHistoTime->Reset();
  mHistoTimeIA->Reset();
  mHistoTimeOA->Reset();
  mHistoTimeIC->Reset();
  mHistoTimeOC->Reset();
  mHistoTimeOrphans->Reset();
  // ToT
  mHistoToT->Reset();
  mHistoToTIA->Reset();
  mHistoToTOA->Reset();
  mHistoToTIC->Reset();
  mHistoToTOC->Reset();
}

} // namespace o2::quality_control_modules::tof
