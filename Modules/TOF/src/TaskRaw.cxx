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
/// \brief  Task To monitor data converted from TOF compressor and diagnostic words from TOF crates
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

void RawDataDecoder::decode()
{
  DecoderBase::run();
}

void RawDataDecoder::rdhHandler(const o2::header::RAWDataHeader* rdh)
{
  mRDHCounter[rdh->feeId & 0xFF].Count(0);
}

void RawDataDecoder::headerHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* crateOrbit)
{

  // DRM Counter
  mDRMCounter[crateHeader->drmID].Count(0);

  // LTM Counter
  if (crateHeader->slotPartMask & (1 << 0)) {
    mLTMCounter[crateHeader->drmID].Count(0);
  }

  // Participating slot
  for (int ibit = 1; ibit < 11; ++ibit) {
    if (crateHeader->slotPartMask & (1 << ibit)) {
      // TRM Counter
      mTRMCounter[crateHeader->drmID][ibit - 1].Count(0);
    }
  }

  // Orbit ID
  mOrbitID->Fill(crateOrbit->orbitID % 1048576, crateHeader->drmID);
}

void RawDataDecoder::frameHandler(const CrateHeader_t* crateHeader, const CrateOrbit_t* /*crateOrbit*/,
                                  const FrameHeader_t* frameHeader, const PackedHit_t* packedHits)
{
  // Number of hits
  mHits->Fill(frameHeader->numberOfHits);
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

    // Equipment index
    mCounterIndexEquipment.Count(indexE);
    // Raw time
    mTime->Fill(time);
    // BC time
    mCounterTimeBC.Count(timebc);
    // ToT
    mTOT->Fill(packedHit->tot);
    // Equipment index for noise analysis
    if (time < mTimeMin || time >= mTimeMax) {
      continue;
    }
    mCounterIndexEquipmentInTimeWin.Count(indexE);
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
          mDRMCounter[drmID].Count(j + reserved_words);
        }
      }
    } else if (slotID == 2) { // Here we have a LTM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mLTMCounter[drmID].Count(j + reserved_words);
        }
      }
    } else { // Here we have a TRM
      for (unsigned int j = 0; j < words_to_check; j++) {
        if (diagnostic->faultBits & 1 << j) {
          mTRMCounter[drmID][slotID - 3].Count(j + reserved_words);
        }
      }
    }
  }

  // Number of diagnostics per crate
  for (int i = 0; i < crateTrailer->numberOfDiagnostics; ++i) {
    auto diagnostic = diagnostics + i;
    mDiagnostic->Fill(crateHeader->drmID, diagnostic->slotID);
  }
  // Errors in the TDCs
  int nError = 0, nTest = 0;
  for (int i = 0; i < crateTrailer->numberOfErrors; ++i) {
    auto error = errors + i;
    if (error->undefined) {
      nTest++;
      mTest->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
    } else {
      nError++;
      mError->Fill(error->slotID + 0.5 * error->chain, error->tdcID);
      for (int ibit = 0; ibit < 15; ++ibit) {
        if (error->errorFlags & (1 << ibit)) {
          mErrorBits->Fill(ibit);
        }
      }
    }
  }
  mNErrors->Fill(nError);
  mNTests->Fill(nTest);
}

void RawDataDecoder::initHistograms() // Initialization of histograms in Decoder
{
  mHits.reset(new TH1F("hHits", "Raw Hits;Hits per event", 1000, 0., 1000.));
  //
  mTime.reset(new TH1F("hTime", "Raw Time;Time (24.4 ps)", 2097152, 0., 2097152.));
  //
  mTOT.reset(new TH1F("hTOT", "Raw ToT;ToT (48.8 ps)", 2048, 0., 2048.));
  //
  mSlotPartMask.reset(new TH2F("hSlotPartMask", "Slot participating;Crate;Slot", ncrates, 0., ncrates, nslots, 1, nslots + 1));
  //
  mDiagnostic.reset(new TH2F("hDiagnostic", "hDiagnostic;Crate;Slot", ncrates, 0., ncrates, nslots, 1, nslots + 1));
  //
  mNErrors.reset(new TH1F("hNErrors", "Error numbers;Number of errors", 1000, 0., 1000.));
  //
  mErrorBits.reset(new TH1F("hErrorBit", "Error Bit;TDC error bit", 15, 0., 15.));
  //
  mError.reset(new TH2F("hError", "Errors;slot;TDC", 24, 1., 13., 15, 0., 15.));
  //
  mNTests.reset(new TH1F("hNTests", "Test numbers;Number of errors", 1000, 0., 1000.));
  //
  mTest.reset(new TH2F("hTest", "Tests;slot;TDC", 24, 1., 13., 15, 0., 15.));
  //
  mOrbitID.reset(new TH2F("hOrbitID", "OrbitID;OrbitID % 1048576;Crate", 1024, 0, 1048576, ncrates, 0, ncrates));
  //
  mNoiseMap.reset(new TH2F("hNoiseMap", "Noise Map; crate; Fea x strip", ncrates, 0., ncrates., 364, 0., nstrips));
  //
  mIndexEquipmentHitRate.reset(new TH1F("hIndexEquipmentHitRate", "Hit Rate (Hz); index EO", 172800, 0., 172800.));
  //
}

void RawDataDecoder::resetHistograms() // Reset of histograms in Decoder
{
  mHits->Reset();
  mTime->Reset();
  mTOT->Reset();
  mSlotPartMask->Reset();
  mDiagnostic->Reset();
  mNErrors->Reset();
  mErrorBits->Reset();
  mError->Reset();
  mNTests->Reset();
  mTest->Reset();
  mOrbitID->Reset();
  mNoiseMap->Reset();
  mIndexEquipmentHitRate->Reset();
}

void RawDataDecoder::estimateNoise(std::shared_ptr<TH2F> hNoiseMap, std::shared_ptr<TH1F> hIndexEquipmentHitRate, std::shared_ptr<TH1F> hIndexEquipmentIsNoise)
{
  double IntegratedTimeFea[nstrips][ncrates][4] = { { { 0. } } };
  double IntegratedTime[nstrips][ncrates] = { { 0. } };

  for (unsigned int i = 0; i < 172800; ++i) {
    const auto indexcounter = mCounterIndexEquipmentInTimeWin.HowMany(i);
    const unsigned int crate = i / 2400;
    const auto time_window = (mTimeMax - mTimeMin) * tdc_width;
    const auto time = mDRMCounter[crate + 1].HowMany(0) * time_window;

    // start measure time from 1 micro second
    if (time < 1.e-6) {
      continue;
    }
    // check if this channel was active
    if (indexcounter == 0) {
      continue;
    }

    const auto rate = (float)indexcounter / time;

    // Fill noise rate histogram
    hIndexEquipmentHitRate->SetBinContent(i + 1, rate);

    // Noise condition
    if (rate < RawDataDecoder::max_noise) {
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
  mCounterNoisyChannels.FillHistogram(hIndexEquipmentIsNoise.get());

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
        hNoiseMap->SetBinContent(icrate + 1, istrip * 4 + (3 - iFea) + 1, indexcounterFea);
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
    mDecoderRaw.setTimeMin(param->second);
  }
  if (auto param = mCustomParameters.find("TimeWindowMax"); param != mCustomParameters.end()) {
    mDecoderRaw.setTimeMax(param->second);
  }
  if (auto param = mCustomParameters.find("NoiseThreshold"); param != mCustomParameters.end()) {
    mDecoderRaw.setMaxNoise(param->second);
  }

  // DRM
  mDRMHisto.reset(new TH2F("DRMCounter", "DRM Diagnostics;DRM Word;Crate;Words",
                           RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                           RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
  mDecoderRaw.mDRMCounter[0].MakeHistogram(mDRMHisto.get());
  getObjectsManager()->startPublishing(mDRMHisto.get());
  // LTM
  mLTMHisto.reset(new TH2F("LTMCounter", "LTM Diagnostics;LTM Word;Crate;Words",
                           RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                           RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
  mDecoderRaw.mLTMCounter[0].MakeHistogram(mLTMHisto.get());
  getObjectsManager()->startPublishing(mLTMHisto.get());
  // TRMs
  for (unsigned int j = 0; j < RawDataDecoder::ntrms; j++) {
    mTRMHisto[j].reset(new TH2F(Form("TRMCounterSlot%i", j), Form("TRM %i Diagnostics;TRM Word;Crate;Words", j),
                                RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                                RawDataDecoder::ncrates, 0, RawDataDecoder::ncrates));
    mDecoderRaw.mTRMCounter[0][j].MakeHistogram(mTRMHisto[j].get());
    getObjectsManager()->startPublishing(mTRMHisto[j].get());
  }
  // Whole Crates
  for (unsigned int j = 0; j < RawDataDecoder::ncrates; j++) {
    mCrateHisto[j].reset(new TH2F(Form("CrateCounter%02i", j),
                                  Form("Crate%02i Diagnostics;Word;Slot", j),
                                  RawDataDecoder::nwords, 0, RawDataDecoder::nwords,
                                  RawDataDecoder::nslots + 1, 0, RawDataDecoder::nslots + 1));
    getObjectsManager()->startPublishing(mCrateHisto[j].get());
  }

  mIndexEquipment.reset(new TH1F("hIndexEquipment", "Equipment index;index EO", 172800, 0., 172800.));
  mDecoderRaw.mCounterIndexEquipment.MakeHistogram(mIndexEquipment.get());
  getObjectsManager()->startPublishing(mIndexEquipment.get());
  mIndexEquipmentInTimeWin.reset(new TH1F("hIndexEquipmentInTimeWin", "Equipment index for noise analysis;index EO", 172800, 0., 172800.));
  mDecoderRaw.mCounterIndexEquipmentInTimeWin.MakeHistogram(mIndexEquipmentInTimeWin.get());
  getObjectsManager()->startPublishing(mIndexEquipmentInTimeWin.get());
  mTimeBC.reset(new TH1F("hTimeBC", "Raw BC Time;BC time (24.4 ps)", 1024, 0., 1024.));
  mDecoderRaw.mCounterTimeBC.MakeHistogram(mTimeBC.get());
  getObjectsManager()->startPublishing(mTimeBC.get());
  mIndexEquipmentIsNoise.reset(new TH1F("hIndexEquipmentIsNoise", "Noisy Channels; index EO", 172800, 0., 172800.));
  mDecoderRaw.mCounterNoisyChannels.MakeHistogram(mIndexEquipmentIsNoise.get());
  getObjectsManager()->startPublishing(mIndexEquipmentIsNoise.get());

  mDecoderRaw.initHistograms();
  getObjectsManager()->startPublishing(mDecoderRaw.mHits.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTime.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTOT.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mSlotPartMask.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mDiagnostic.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mNErrors.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mErrorBits.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mError.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mNTests.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mTest.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mOrbitID.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mNoiseMap.get());
  getObjectsManager()->startPublishing(mDecoderRaw.mIndexEquipmentHitRate.get());
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
  for (unsigned int i = 0; i < RawDataDecoder::ncrates; i++) { // Filling histograms only at the end of the cycle
    mDecoderRaw.mDRMCounter[i].FillHistogram(mDRMHisto.get(), i + 1);
    mDecoderRaw.mLTMCounter[i].FillHistogram(mLTMHisto.get(), i + 1);
    mDecoderRaw.mSlotPartMask->SetBinContent(i + 1, 1, mDecoderRaw.mDRMCounter[i].HowMany(0));
    mDecoderRaw.mSlotPartMask->SetBinContent(i + 1, 2, mDecoderRaw.mLTMCounter[i].HowMany(0));
    for (unsigned int j = 0; j < RawDataDecoder::ntrms; j++) {
      mDecoderRaw.mTRMCounter[i][j].FillHistogram(mTRMHisto[j].get(), i + 1);
      mDecoderRaw.mSlotPartMask->SetBinContent(i + 1, j + 3, mDecoderRaw.mTRMCounter[i][j].HowMany(0));
    }
  }

  mDecoderRaw.mCounterIndexEquipment.FillHistogram(mIndexEquipment.get());
  mDecoderRaw.mCounterIndexEquipmentInTimeWin.FillHistogram(mIndexEquipmentInTimeWin.get());
  mDecoderRaw.mCounterTimeBC.FillHistogram(mTimeBC.get());
  mDecoderRaw.estimateNoise(mDecoderRaw.mNoiseMap, mDecoderRaw.mIndexEquipmentHitRate, mIndexEquipmentIsNoise);

  // Reshuffling information from the cards to the whole crate
  for (unsigned int slot = 0; slot < RawDataDecoder::nslots; slot++) { // Loop over slots
    TH2F* diagnosticHisto = nullptr;
    if (slot == 0) { // We have a DRM!
      diagnosticHisto = mDRMHisto.get();
    } else if (slot == 1) { // We have a LTM!
      diagnosticHisto = mLTMHisto.get();
    } else { // We have a TRM!
      diagnosticHisto = mTRMHisto[slot - 2].get();
    }
    if (diagnosticHisto) {
      for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) { // Loop over crates
        for (unsigned int word = 0; word < RawDataDecoder::nwords; word++) {   // Loop over words
          mCrateHisto[crate]->SetBinContent(word + 1, slot + 2,
                                            diagnosticHisto->GetBinContent(word + 1, crate + 1));
        }
      }
    }
  }
  for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) { // Loop over crates for how many RDH read
    mCrateHisto[crate]->SetBinContent(1, 1, mDecoderRaw.mRDHCounter[crate].HowMany(0));
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
  mDRMHisto->Reset();
  mLTMHisto->Reset();
  for (unsigned int j = 0; j < RawDataDecoder::ntrms; j++) {
    mTRMHisto[j]->Reset();
  }
  for (unsigned int crate = 0; crate < RawDataDecoder::ncrates; crate++) {
    mCrateHisto[crate]->Reset();
  }

  mIndexEquipment->Reset();
  mIndexEquipmentInTimeWin->Reset();
  mTimeBC->Reset();
  mIndexEquipmentIsNoise->Reset();

  mDecoderRaw.resetHistograms();
}

const char* RawDataDecoder::RDHDiagnosticsName[2] = { "RDH_HAS_DATA", "" };

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