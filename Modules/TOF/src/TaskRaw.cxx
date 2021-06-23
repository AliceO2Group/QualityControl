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
/// \file   TaskRaw.cxx
/// \author Nicolo' Jacazio and Francesca Ercolessi
/// \brief  Task To monitor data converted from TOF compressor, and check the diagnostic words of TOF crates received trough the TOF compressor.
///         Here are defined the counters to check the diagnostics words of the TOF crates obtained from the compressor.
///         This is why the class derives from DecoderBase: it reads data from the decoder.
///         This tasks also perform a basic noise monitoring to check the fraction of noisy channels
/// \since  20-11-2020
///

// ROOT includes
#include <TH1.h>
#include <TH1F.h>
#include <TH2F.h>

// O2 includes
#include "DataFormatsTOF/CompressedDataFormat.h"
#include <Framework/DataRefUtils.h>
#include "Headers/RAWDataHeader.h"
#include "DetectorsRaw/HBFUtils.h"
#include <Framework/InputRecord.h>

using namespace o2::framework;
using namespace o2::tof;

// Fairlogger includes
#include <fairlogger/Logger.h>

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TOF/TaskRaw.h"

namespace o2::quality_control_modules::tof
{

void RawDataDecoder::rdhHandler(const o2::header::RAWDataHeader* rdh)
{
  mCounterRDH[rdh->feeId & 0xFF].Count(0);

  //Case for the RDH word "fatal"
  if ((rdh->detectorField & 0x00010000) != 0) {
    mCounterRDH[rdh->feeId & 0xFF].Count(1);
    LOG(WARNING) << "RDH flag \"fatal\" error occurred in crate " << static_cast<int>(rdh->feeId & 0xFF);
  }

  if (rdh->stop) { // if RDH close
    //Triggers served and received (3 are expected)
    const int triggerserved = ((rdh->detectorField & 0xFF00070F) >> 24);
    const int triggerreceived = ((rdh->detectorField & 0x00FF070F) >> 16);
    if (triggerserved < triggerreceived) {
      //RDH word "trigger error": served < received
      mCounterRDH[rdh->feeId & 0xFF].Count(2);
    }
    //Numerator and denominator for the trigger efficiency
    mCounterRDHTriggers[0].Add(rdh->feeId & 0xFF, triggerserved);
    mCounterRDHTriggers[1].Add(rdh->feeId & 0xFF, triggerreceived);
  }
}

void RawDataDecoder::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit)
{

  // DRM Counter
  mCounterDRM[crateHeader->drmID].Count(0);

  // LTM Counter
  if (crateHeader->slotPartMask & (1 << 0)) {
    mCounterLTM[crateHeader->drmID].Count(0);
  }

  // Participating slot
  for (int ibit = 1; ibit < 11; ++ibit) {
    if (crateHeader->slotPartMask & (1 << ibit)) {
      // TRM Counter
      mCounterTRM[crateHeader->drmID][ibit - 1].Count(0);
    }
  }

  // Orbit ID
  mHistoOrbitID->Fill(crateOrbit->orbitID % 1048576, crateHeader->drmID);
}

void RawDataDecoder::frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                  const FrameHeader_t* frameHeader, const PackedHit_t* packedHits)
{
  // Number of hits
  mHistoHits->Fill(frameHeader->numberOfHits);
  for (int i = 0; i < frameHeader->numberOfHits; ++i) {
    const auto packedHit = packedHits + i;
    const auto drmID = crateHeader->drmID;                                                    // [0-71]
    const auto trmID = frameHeader->trmID;                                                    // [3-12]
    const auto chain = packedHit->chain;                                                      // [0-1]
    const auto tdcID = packedHit->tdcID;                                                      // [0-14]
    const auto channel = packedHit->channel;                                                  // [0-7]
    const auto indexE = channel + 8 * tdcID + 120 * chain + 240 * (trmID - 3) + 2400 * drmID; // [0-172799]
    const int time = packedHit->time + (frameHeader->frameID << 13);                          // [24.4 ps]
    const int timebc = time % 1024;

    // Equipment index (Electronics Oriented)
    mCounterIndexEO.Count(indexE);
    // Raw time
    mHistoTime->Fill(time);
    // BC time
    mCounterTimeBC.Count(timebc);
    // ToT
    mHistoTOT->Fill(packedHit->tot);
    // Equipment index for noise analysis (Electronics Oriented)
    if (time < mTimeMin || time >= mTimeMax) {
      continue;
    }
    mCounterIndexEOInTimeWin.Count(indexE);
  }
}

void RawDataDecoder::trailerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                    const CrateTrailer_t* crateTrailer, const Diagnostic_t* diagnostics,
                                    const Error_t* errors)
{
  // Diagnostic word per slot
  const int drmID = crateHeader->drmID;
  constexpr unsigned int reserved_words = 4;                       // First 4 bits are reserved
  constexpr unsigned int words_to_check = nwords - reserved_words; // Words to check
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    const int slotID = diagnostic->slotID;
    if (slotID == 1) { // Here we have a DRM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mCounterDRM[drmID].Count(j + reserved_words);
        }
      }
    } else if (slotID == 2) { // Here we have a LTM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mCounterLTM[drmID].Count(j + reserved_words);
        }
      }
    } else { // Here we have a TRM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mCounterTRM[drmID][slotID - 3].Count(j + reserved_words);
        }
      }
    }
  }

  // Number of diagnostics per crate
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    mHistoDiagnostic->Fill(crateHeader->drmID, diagnostic->slotID);
  }
  // Errors in the TDCs
  int nError = 0, nTest = 0;
  for (int i = 0; i < crateTrailer->numberOfErrors; ++i) {
    auto error = errors + i;
    if (error->undefined) {
      nTest++;
      mHistoTest->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
    } else {
      nError++;
      mHistoError->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
      for (int ibit = 0; ibit < 15; ++ibit) {
        if (error->errorFlags & (1 << ibit)) {
          mHistoErrorBits->Fill(ibit);
        }
      }
    }
  }
  mHistoNErrors->Fill(nError);
  mHistoNTests->Fill(nTest);
}

void RawDataDecoder::initHistograms() // Initialization of histograms in Decoder
{
  mHistoHits.reset(new TH1F("hHits", "Raw Hits;Hits per event", 1000, 0., 1000.));
  mHistoTime.reset(new TH1F("hTime", "Raw Time;Time (24.4 ps)", 2097152, 0., 2097152.));
  mHistoTOT.reset(new TH1F("hTOT", "Raw ToT;ToT (48.8 ps)", 2048, 0., 2048.));
  mHistoDiagnostic.reset(new TH2F("hDiagnostic", "hDiagnostic;Crate;Slot", ncrates, 0., ncrates, nslots, 1, nslots + 1));
  mHistoDiagnostic.get()->GetYaxis()->SetBinLabel(1, "DRM");
  mHistoDiagnostic.get()->GetYaxis()->SetBinLabel(2, "LTM");
  for (int k = 0; k < 10; k++) {
    mHistoDiagnostic.get()->GetYaxis()->SetBinLabel(3 + k, Form("TRM%i", k));
  }
  mHistoNErrors.reset(new TH1F("hNErrors", "Error numbers;Number of errors", 1000, 0., 1000.));
  mHistoErrorBits.reset(new TH1F("hErrorBit", "Error Bit;TDC error bit", 15, 0., 15.));
  mHistoError.reset(new TH2F("hError", "Errors;slot;TDC", 24, 1., 13., 15, 0., 15.));
  mHistoNTests.reset(new TH1F("hNTests", "Test numbers;Number of errors", 1000, 0., 1000.));
  mHistoTest.reset(new TH2F("hTest", "Tests;slot;TDC", 24, 1., 13., 15, 0., 15.));
  mHistoOrbitID.reset(new TH2F("hOrbitID", "OrbitID;OrbitID % 1048576;Crate", 1024, 0, 1048576, ncrates, 0, ncrates));
  mHistoNoiseMap.reset(new TH2F("hNoiseMap", "Noise Map (1 bin = 1 FEA = 24 channels); crate; Fea x strip", ncrates, 0., ncrates, 364, 0., nstrips));
  mHistoIndexEOHitRate.reset(new TH1F("hIndexEOHitRate", "Hit Rate (Hz); index EO", nequipments, 0., nequipments));
}

void RawDataDecoder::resetHistograms() // Reset of histograms in Decoder
{
  mHistoHits->Reset();
  mHistoTime->Reset();
  mHistoTOT->Reset();
  mHistoDiagnostic->Reset();
  mHistoNErrors->Reset();
  mHistoErrorBits->Reset();
  mHistoError->Reset();
  mHistoNTests->Reset();
  mHistoTest->Reset();
  mHistoOrbitID->Reset();
  mHistoNoiseMap->Reset();
  mHistoIndexEOHitRate->Reset();
}

void RawDataDecoder::estimateNoise(std::shared_ptr<TH1F> hIndexEOIsNoise)
{
  double IntegratedTimeFea[nstrips][ncrates][4] = { { { 0. } } };
  double IntegratedTime[nstrips][ncrates] = { { 0. } };

  for (unsigned int i = 0; i < nequipments; ++i) {
    const auto indexcounter = mCounterIndexEOInTimeWin.HowMany(i);
    const unsigned int crate = i / 2400;
    const double time_window = mTDCWidth * (mTimeMax - mTimeMin);
    const double time = mCounterDRM[crate + 1].HowMany(0) * time_window;

    // start measure time from 1 micro second
    if (time < 1.e-6) {
      continue;
    }
    // check if this channel was active
    if (indexcounter == 0) {
      continue;
    }

    const double rate = (double)indexcounter / time;

    // Fill noise rate histogram
    mHistoIndexEOHitRate->SetBinContent(i + 1, rate);

    // Noise condition
    if (rate < mNoiseThreshold) {
      continue;
    }

    const int crate_ = i % 2400;
    const int slot = crate_ / 240;
    const int slot_ = crate_ % 240;
    const int chain = slot_ / 120;
    const int chain_ = slot_ % 120;
    const int tdc = chain_ / 8;
    const int tdc_ = chain_ % 8;
    const int channel = tdc_;
    const auto eIndex = o2::tof::Geo::getECHFromIndexes(crate, slot + 3, chain, tdc, channel);
    const auto dIndex = o2::tof::Geo::getCHFromECH(eIndex);
    if (dIndex < 0) {
      continue;
    }
    const auto sector_ = dIndex % 8736;
    const auto strip = sector_ / 96;
    const auto strip_ = sector_ % 96;
    const auto strrow_ = strip_ % 48;
    const auto fea = strrow_ / 12;

    mCounterNoisyChannels.Add(i, indexcounter);
    IntegratedTime[strip][crate] += time;
    mCounterNoiseMap[crate][fea].Add(strip, indexcounter);
    IntegratedTimeFea[strip][crate][fea] += time;
  } // end loop over index

  // Fill noisy channels histogram
  mCounterNoisyChannels.FillHistogram(hIndexEOIsNoise.get());

  for (unsigned int icrate = 0; icrate < ncrates; icrate++) {
    for (unsigned int istrip = 0; istrip < nstrips; istrip++) {
      const auto itime = IntegratedTime[istrip][icrate];

      // start measure time from 1 micro second
      if (itime < 1.e-6) {
        continue;
      }

      for (int iFea = 0; iFea < 4; iFea++) {
        const auto indexcounterFea = mCounterNoiseMap[icrate][iFea].HowMany(istrip);
        const auto timeFea = IntegratedTimeFea[istrip][icrate][iFea];

        // start measure time from 1 micro second
        if (timeFea < 1.e-6) {
          continue;
        }

        // Fill noise map
        mHistoNoiseMap->SetBinContent(icrate + 1, istrip * 4 + (3 - iFea) + 1, indexcounterFea);
      } // end loop over Feas
    }   // end loop over strips
  }     // end loop over sectors
}

// Implement the Task
void TaskRaw::initialize(o2::framework::InitContext& /*ctx*/)
{
  // Set task parameters from JSON
  if (auto param = mCustomParameters.find("DecoderCONET"); param != mCustomParameters.end()) {
    if (param->second == "True") {
      LOG(INFO) << "Rig for DecoderCONET";
      mDecoderRaw.setDecoderCONET(kTRUE);
    }
  }
  if (auto param = mCustomParameters.find("TimeWindowMin"); param != mCustomParameters.end()) {
    mDecoderRaw.setTimeWindowMin(param->second);
  }
  if (auto param = mCustomParameters.find("TimeWindowMax"); param != mCustomParameters.end()) {
    mDecoderRaw.setTimeWindowMax(param->second);
  }
  if (auto param = mCustomParameters.find("NoiseThreshold"); param != mCustomParameters.end()) {
    mDecoderRaw.setNoiseThreshold(param->second);
  }

  // RDH
  mHistoRDH.reset(new TH2F("RDHCounter", "RDH Diagnostics;RDH Word;Crate;Words",
                           RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                           RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
  mDecoderRaw.mCounterRDH[0].MakeHistogram(mHistoRDH.get());
  getObjectsManager()->startPublishing(mHistoRDH.get());
  // DRM
  mHistoDRM.reset(new TH2F("DRMCounter", "DRM Diagnostics;DRM Word;Crate;Words",
                           RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                           RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
  mDecoderRaw.mCounterDRM[0].MakeHistogram(mHistoDRM.get());
  getObjectsManager()->startPublishing(mHistoDRM.get());
  // LTM
  mHistoLTM.reset(new TH2F("LTMCounter", "LTM Diagnostics;LTM Word;Crate;Words",
                           RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                           RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
  mDecoderRaw.mCounterLTM[0].MakeHistogram(mHistoLTM.get());
  getObjectsManager()->startPublishing(mHistoLTM.get());
  // TRMs
  for (unsigned int j = 0; j < RawDataDecoder::ntrms; j++) {
    mHistoTRM[j].reset(new TH2F(Form("TRMCounterSlot%02i", j + 3), Form("TRM Slot %i Diagnostics;TRM Word;Crate;Words", j + 3),
                                RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                                RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
    mDecoderRaw.mCounterTRM[0][j].MakeHistogram(mHistoTRM[j].get());
    getObjectsManager()->startPublishing(mHistoTRM[j].get());
  }
  // Whole Crates
  for (unsigned int j = 0; j < RawDataDecoder::ncrates; j++) {
    mHistoCrate[j].reset(new TH2F(Form("CrateCounter%02i", j),
                                  Form("Crate%02i Diagnostics;Word;Slot", j),
                                  RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                                  RawDataDecoder::nslots + 1, 0, RawDataDecoder::nslots + 1));
    mHistoCrate[j].get()->GetYaxis()->SetBinLabel(1, "RDH");
    mHistoCrate[j].get()->GetYaxis()->SetBinLabel(2, "DRM");
    mHistoCrate[j].get()->GetYaxis()->SetBinLabel(3, "LTM");
    for (int k = 0; k < 10; k++) {
      mHistoCrate[j].get()->GetYaxis()->SetBinLabel(4 + k, Form("TRMSlot%02i", k + 3));
    }
    getObjectsManager()->startPublishing(mHistoCrate[j].get());
  }
  // Slot participating in all crates
  mHistoSlotParticipating.reset(new TH2F("hSlotPartMask",
                                         "Slot participating;Crate;Slot",
                                         RawDataDecoder::ncrates, 0., RawDataDecoder::ncrates,
                                         RawDataDecoder::nslots + 1, 0, RawDataDecoder::nslots + 1));
  mHistoSlotParticipating.get()->GetYaxis()->SetBinLabel(1, "RDH");
  mHistoSlotParticipating.get()->GetYaxis()->SetBinLabel(2, "DRM");
  mHistoSlotParticipating.get()->GetYaxis()->SetBinLabel(3, "LTM");
  for (int k = 0; k < 10; k++) {
    mHistoSlotParticipating.get()->GetYaxis()->SetBinLabel(4 + k, Form("TRMSlot%02i", k + 3));
  }
  getObjectsManager()->startPublishing(mHistoSlotParticipating.get());

  mHistoIndexEO.reset(new TH1F("hIndexEO", "Index Electronics Oriented;index EO;Counts", RawDataDecoder::nequipments, 0., RawDataDecoder::nequipments));
  mDecoderRaw.mCounterIndexEO.MakeHistogram(mHistoIndexEO.get());
  getObjectsManager()->startPublishing(mHistoIndexEO.get());
  mHistoIndexEOInTimeWin.reset(new TH1F("hIndexEOInTimeWin", "Index Electronics Oriented for noise analysis;index EO", RawDataDecoder::nequipments, 0., RawDataDecoder::nequipments));
  mDecoderRaw.mCounterIndexEOInTimeWin.MakeHistogram(mHistoIndexEOInTimeWin.get());
  getObjectsManager()->startPublishing(mHistoIndexEOInTimeWin.get());
  mHistoTimeBC.reset(new TH1F("hTimeBC", "Raw BC Time;BC time (24.4 ps);Counts", 1024, 0., 1024.));
  mDecoderRaw.mCounterTimeBC.MakeHistogram(mHistoTimeBC.get());
  getObjectsManager()->startPublishing(mHistoTimeBC.get());
  mHistoIndexEOIsNoise.reset(new TH1F("hIndexEOIsNoise", "Noisy Channels; index EO;Counts", RawDataDecoder::nequipments, 0., RawDataDecoder::nequipments));
  mDecoderRaw.mCounterNoisyChannels.MakeHistogram(mHistoIndexEOIsNoise.get());
  getObjectsManager()->startPublishing(mHistoIndexEOIsNoise.get());
  mHistoRDHTriggers.reset(new TH1F("hRDHTriggers", "RDH Trigger Efficiency;Crate;Triggers_{served}/Triggers_{received}", RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
  mDecoderRaw.mCounterRDHTriggers[0].MakeHistogram(mHistoRDHTriggers.get());
  getObjectsManager()->startPublishing(mHistoRDHTriggers.get());

  mDecoderRaw.initHistograms();
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoHits.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoTime.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoTOT.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoDiagnostic.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoNErrors.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoErrorBits.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoError.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoNTests.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoTest.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoOrbitID.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoNoiseMap.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mHistoIndexEOHitRate.get());
}

void TaskRaw::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
  mDecoderRaw.resetHistograms();
}

void TaskRaw::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void TaskRaw::monitorData(o2::framework::ProcessingContext& ctx)
{

  for (auto iit = ctx.inputs().begin(), iend = ctx.inputs().end(); iit != iend; ++iit) {
    if (!iit.isValid()) {
      continue;
    }
    /** loop over input parts **/
    for (auto const& input : iit) {
      /** input **/
      const auto* headerIn = o2::framework::DataRefUtils::getHeader<o2::header::DataHeader*>(input);
      const auto payloadIn = input.payload;
      const auto payloadInSize = headerIn->payloadSize;
      mDecoderRaw.setDecoderBuffer(payloadIn);
      mDecoderRaw.setDecoderBufferSize(payloadInSize);
      mDecoderRaw.decode();
    }
  }
}

void TaskRaw::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
  for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) { // Filling histograms only at the end of the cycle
    mDecoderRaw.mCounterRDH[crate].FillHistogram(mHistoRDH.get(), crate + 1);
    mDecoderRaw.mCounterDRM[crate].FillHistogram(mHistoDRM.get(), crate + 1);
    mDecoderRaw.mCounterLTM[crate].FillHistogram(mHistoLTM.get(), crate + 1);
    mHistoSlotParticipating->SetBinContent(crate + 1, 2, mDecoderRaw.mCounterDRM[crate].HowMany(0));
    mHistoSlotParticipating->SetBinContent(crate + 1, 3, mDecoderRaw.mCounterLTM[crate].HowMany(0));
    for (unsigned int j = 0; j < RawDataDecoder::ntrms; j++) {
      mDecoderRaw.mCounterTRM[crate][j].FillHistogram(mHistoTRM[j].get(), crate + 1);
      mHistoSlotParticipating->SetBinContent(crate + 1, j + 4, mDecoderRaw.mCounterTRM[crate][j].HowMany(0));
    }
    mHistoSlotParticipating->SetBinContent(crate + 1, 1, mDecoderRaw.mCounterRDH[crate].HowMany(0));
    if (mDecoderRaw.mCounterRDHTriggers[1].HowMany(crate) != 0) {
      mHistoRDHTriggers->SetBinContent(crate + 1, static_cast<float>(mDecoderRaw.mCounterRDHTriggers[0].HowMany(crate)) / mDecoderRaw.mCounterRDHTriggers[1].HowMany(crate));
    }
  }
  mDecoderRaw.mCounterIndexEO.FillHistogram(mHistoIndexEO.get());
  mDecoderRaw.mCounterIndexEOInTimeWin.FillHistogram(mHistoIndexEOInTimeWin.get());
  mDecoderRaw.mCounterTimeBC.FillHistogram(mHistoTimeBC.get());
  mDecoderRaw.estimateNoise(mHistoIndexEOIsNoise);

  // Reshuffling information from the cards to the whole crate
  for (unsigned int slot = 0; slot < RawDataDecoder::nslots; slot++) { // Loop over slots
    TH2F* diagnosticHisto = nullptr;
    if (slot == 0) { // We have a DRM!
      diagnosticHisto = mHistoDRM.get();
    } else if (slot == 1) { // We have a LTM!
      diagnosticHisto = mHistoLTM.get();
    } else { // We have a TRM!
      diagnosticHisto = mHistoTRM[slot - 2].get();
    }
    if (diagnosticHisto) {
      for (int crate = 0; crate < diagnosticHisto->GetNbinsX(); crate++) { // Loop over crates
        for (int word = 0; word < diagnosticHisto->GetNbinsY(); word++) {  // Loop over words
          mHistoCrate[crate]->SetBinContent(word + 1, slot + 2,            // Shift position 1 to make room for the RDH
                                            diagnosticHisto->GetBinContent(word + 1, crate + 1));
        }
      }
    } else {
      LOG(WARNING) << "Did not find diagnostic histogram for slot " << slot << " for reshuffling";
    }
  }
  for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) {              // Loop over crates for how many RDH read
    for (unsigned int word = 0; word < mDecoderRaw.mCounterRDH[crate].Size(); word++) { // Loop over words
      mHistoCrate[crate]->SetBinContent(word + 1, 1, mDecoderRaw.mCounterRDH[crate].HowMany(word));
    }
  }
}

void TaskRaw::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void TaskRaw::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistoRDH->Reset();
  mHistoDRM->Reset();
  mHistoLTM->Reset();
  for (unsigned int j = 0; j < RawDataDecoder::ntrms; j++) {
    mHistoTRM[j]->Reset();
  }
  for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) {
    mHistoCrate[crate]->Reset();
  }
  mHistoSlotParticipating->Reset();
  mHistoIndexEO->Reset();
  mHistoIndexEOInTimeWin->Reset();
  mHistoTimeBC->Reset();
  mHistoIndexEOIsNoise->Reset();
  mHistoRDHTriggers->Reset();

  mDecoderRaw.resetHistograms();
}

const char* RawDataDecoder::RDHDiagnosticsName[3] = { "RDH_HAS_DATA", "RDH_DECODER_FATAL", "RDH_TRIGGER_ERROR" };

const char* RawDataDecoder::DRMDiagnosticName[RawDataDecoder::nwords] = {
  diagnostic::DRMDiagnosticName[0],
  diagnostic::DRMDiagnosticName[1],
  diagnostic::DRMDiagnosticName[2],
  diagnostic::DRMDiagnosticName[3],
  diagnostic::DRMDiagnosticName[4],
  diagnostic::DRMDiagnosticName[5],
  diagnostic::DRMDiagnosticName[6],
  diagnostic::DRMDiagnosticName[7],
  diagnostic::DRMDiagnosticName[8],
  diagnostic::DRMDiagnosticName[9],
  diagnostic::DRMDiagnosticName[10],
  diagnostic::DRMDiagnosticName[11],
  diagnostic::DRMDiagnosticName[12],
  diagnostic::DRMDiagnosticName[13],
  diagnostic::DRMDiagnosticName[14],
  diagnostic::DRMDiagnosticName[15],
  diagnostic::DRMDiagnosticName[16],
  diagnostic::DRMDiagnosticName[17],
  diagnostic::DRMDiagnosticName[18],
  diagnostic::DRMDiagnosticName[19],
  diagnostic::DRMDiagnosticName[20],
  diagnostic::DRMDiagnosticName[21],
  diagnostic::DRMDiagnosticName[22],
  diagnostic::DRMDiagnosticName[23],
  diagnostic::DRMDiagnosticName[24],
  diagnostic::DRMDiagnosticName[25],
  diagnostic::DRMDiagnosticName[26],
  diagnostic::DRMDiagnosticName[27],
  diagnostic::DRMDiagnosticName[28],
  diagnostic::DRMDiagnosticName[29],
  diagnostic::DRMDiagnosticName[30],
  diagnostic::DRMDiagnosticName[31]
};

const char* RawDataDecoder::LTMDiagnosticName[RawDataDecoder::nwords] = {
  diagnostic::LTMDiagnosticName[0],
  diagnostic::LTMDiagnosticName[1],
  diagnostic::LTMDiagnosticName[2],
  diagnostic::LTMDiagnosticName[3],
  diagnostic::LTMDiagnosticName[4],
  diagnostic::LTMDiagnosticName[5],
  diagnostic::LTMDiagnosticName[6],
  diagnostic::LTMDiagnosticName[7],
  diagnostic::LTMDiagnosticName[8],
  diagnostic::LTMDiagnosticName[9],
  diagnostic::LTMDiagnosticName[10],
  diagnostic::LTMDiagnosticName[11],
  diagnostic::LTMDiagnosticName[12],
  diagnostic::LTMDiagnosticName[13],
  diagnostic::LTMDiagnosticName[14],
  diagnostic::LTMDiagnosticName[15],
  diagnostic::LTMDiagnosticName[16],
  diagnostic::LTMDiagnosticName[17],
  diagnostic::LTMDiagnosticName[18],
  diagnostic::LTMDiagnosticName[19],
  diagnostic::LTMDiagnosticName[20],
  diagnostic::LTMDiagnosticName[21],
  diagnostic::LTMDiagnosticName[22],
  diagnostic::LTMDiagnosticName[23],
  diagnostic::LTMDiagnosticName[24],
  diagnostic::LTMDiagnosticName[25],
  diagnostic::LTMDiagnosticName[26],
  diagnostic::LTMDiagnosticName[27],
  diagnostic::LTMDiagnosticName[28],
  diagnostic::LTMDiagnosticName[29],
  diagnostic::LTMDiagnosticName[30],
  diagnostic::LTMDiagnosticName[31]
};

const char* RawDataDecoder::TRMDiagnosticName[RawDataDecoder::nwords] = {
  diagnostic::TRMDiagnosticName[0],
  diagnostic::TRMDiagnosticName[1],
  diagnostic::TRMDiagnosticName[2],
  diagnostic::TRMDiagnosticName[3],
  diagnostic::TRMDiagnosticName[4],
  diagnostic::TRMDiagnosticName[5],
  diagnostic::TRMDiagnosticName[6],
  diagnostic::TRMDiagnosticName[7],
  diagnostic::TRMDiagnosticName[8],
  diagnostic::TRMDiagnosticName[9],
  diagnostic::TRMDiagnosticName[10],
  diagnostic::TRMDiagnosticName[11],
  diagnostic::TRMDiagnosticName[12],
  diagnostic::TRMDiagnosticName[13],
  diagnostic::TRMDiagnosticName[14],
  diagnostic::TRMDiagnosticName[15],
  diagnostic::TRMDiagnosticName[16],
  diagnostic::TRMDiagnosticName[17],
  diagnostic::TRMDiagnosticName[18],
  diagnostic::TRMDiagnosticName[19],
  diagnostic::TRMDiagnosticName[20],
  diagnostic::TRMDiagnosticName[21],
  diagnostic::TRMDiagnosticName[22],
  diagnostic::TRMDiagnosticName[23],
  diagnostic::TRMDiagnosticName[24],
  diagnostic::TRMDiagnosticName[25],
  diagnostic::TRMDiagnosticName[26],
  diagnostic::TRMDiagnosticName[27],
  diagnostic::TRMDiagnosticName[28],
  diagnostic::TRMDiagnosticName[29],
  diagnostic::TRMDiagnosticName[30],
  diagnostic::TRMDiagnosticName[31]
};

} // namespace o2::quality_control_modules::tof