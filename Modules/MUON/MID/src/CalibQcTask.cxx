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
/// \author Valerie Ramillien

#include "MID/CalibQcTask.h"

#include "fmt/format.h"
#include "TH1.h"
#include "TH2.h"

#include "QualityControl/QcInfoLogger.h"

#include "Framework/InputRecord.h"
#include "DataFormatsMID/ColumnData.h"
#include "DataFormatsMID/ROFRecord.h"
#include "MIDBase/DetectorParameters.h"
#include "MIDWorkflow/ColumnDataSpecsUtils.h"

namespace o2::quality_control_modules::mid
{

CalibQcTask::~CalibQcTask()
{
}

void CalibQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Devel) << "initialize CalibQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.
  mNbTimeFrame = std::make_unique<TH1F>("NbTimeFrame", "NbTimeFrame", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbTimeFrame.get());

  mNbNoiseROF = std::make_unique<TH1F>("NbNoiseROF", "NbNoiseROF", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbNoiseROF.get());

  mNbDeadROF = std::make_unique<TH1F>("NbDeadROF", "NbDeadROF", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbDeadROF.get());

  std::array<string, 4> chId{ "11", "12", "21", "22" };

  // Noise strips Histograms ::
  for (int ich = 0; ich < 4; ++ich) {
    mMultNoiseB[ich] = std::make_unique<TH1F>(fmt::format("MultNoiseMT{}B", chId[ich]).c_str(), fmt::format("Multiplicity Noise strips - MT{} bending plane", chId[ich]).c_str(), 300, 0, 300);
    getObjectsManager()->startPublishing(mMultNoiseB[ich].get());
    mMultNoiseNB[ich] = std::make_unique<TH1F>(fmt::format("MultNoiseMT{}NB", chId[ich]).c_str(), fmt::format("Multiplicity Noise strips - MT{} non-bending plane", chId[ich]).c_str(), 300, 0, 300);
    getObjectsManager()->startPublishing(mMultNoiseNB[ich].get());
  }

  mNoise = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("NoiseStrips", "Noise strips"));
  getObjectsManager()->startPublishing(mNoise.get());

  for (int ich = 0; ich < 4; ++ich) {
    mBendNoiseMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("BendNoiseMap{}", chId[ich]), fmt::format("Bending Noise Map MT{}", chId[ich]), 0));
    getObjectsManager()->startPublishing(mBendNoiseMap[ich].get());
  }

  for (int ich = 0; ich < 4; ++ich) {
    mNBendNoiseMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("NBendNoiseMap{}", chId[ich]), fmt::format("Non-Bending Noise Map MT{}", chId[ich]), 1));
    getObjectsManager()->startPublishing(mNBendNoiseMap[ich].get());
  }

  // Dead strips Histograms ::
  for (int ich = 0; ich < 4; ++ich) {
    mMultDeadB[ich] = std::make_unique<TH1F>(fmt::format("MultDeadMT{}B", chId[ich]).c_str(), fmt::format("Multiplicity Dead strips - MT{} bending plane", chId[ich]).c_str(), 300, 0, 300);
    getObjectsManager()->startPublishing(mMultDeadB[ich].get());
    mMultDeadNB[ich] = std::make_unique<TH1F>(fmt::format("MultDeadMT{}NB", chId[ich]).c_str(), fmt::format("Multiplicity Dead strips - MT{} non-bending plane", chId[ich]).c_str(), 300, 0, 300);
    getObjectsManager()->startPublishing(mMultDeadNB[ich].get());
  }

  mDead = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("DeadStrips", "Dead strips"));
  getObjectsManager()->startPublishing(mDead.get());

  for (int ich = 0; ich < 4; ++ich) {
    mBendDeadMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("BendDeadMap{}", chId[ich]), fmt::format("Bending Dead Map MT{}", chId[ich]), 0));
    getObjectsManager()->startPublishing(mBendDeadMap[ich].get());
  }

  for (int ich = 0; ich < 4; ++ich) {
    mNBendDeadMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("NBendDeadMap{}", chId[ich]), fmt::format("Non-Bending Dead Map MT{}", chId[ich]), 1));
    getObjectsManager()->startPublishing(mNBendDeadMap[ich].get());
  }
}

void CalibQcTask::startOfActivity(const Activity& /*activity*/)
{
}

void CalibQcTask::startOfCycle()
{
}

void CalibQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mNbTimeFrame->Fill(0.5, 1.);

  auto noises = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("noise");
  auto noiserofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("noiserofs");

  auto deads = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("dead");
  auto deadrofs = ctx.inputs().get<gsl::span<o2::mid::ROFRecord>>("deadrofs");

  // Noise
  std::array<unsigned long int, 4> evtSizeB{};
  std::array<unsigned long int, 4> evtSizeNB{};

  for (const auto& rof : noiserofs) {
    evtSizeB.fill(0);
    evtSizeNB.fill(0);
    mNbNoiseROF->Fill(0.5, 1.);
    for (auto& col : noises.subspan(rof.firstEntry, rof.nEntries)) {
      auto ich = o2::mid::detparams::getChamber(col.deId);
      evtSizeB[ich] = mDigitsHelper.countDigits(col, 0);
      evtSizeNB[ich] = mDigitsHelper.countDigits(col, 1);
      mDigitsHelper.fillStripHisto(col, mNoise.get());
    }
    for (int ich = 0; ich < 4; ++ich) {
      mMultNoiseB[ich]->Fill(evtSizeB[ich]);
      mMultNoiseNB[ich]->Fill(evtSizeNB[ich]);
    }
  }

  for (const auto& rof : deadrofs) {
    mNbDeadROF->Fill(0.5, 1.);
    evtSizeB.fill(0);
    evtSizeNB.fill(0);
    for (auto& col : deads.subspan(rof.firstEntry, rof.nEntries)) {
      auto ich = o2::mid::detparams::getChamber(col.deId);
      evtSizeB[ich] = mDigitsHelper.countDigits(col, 0);
      evtSizeNB[ich] = mDigitsHelper.countDigits(col, 1);
      mDigitsHelper.fillStripHisto(col, mDead.get());
    }
    for (int ich = 0; ich < 4; ++ich) {
      mMultDeadB[ich]->Fill(evtSizeB[ich]);
      mMultDeadNB[ich]->Fill(evtSizeNB[ich]);
    }
  }
}

void CalibQcTask::endOfCycle()
{
  // Fill here the 2D representation of the fired strips
  // First reset the old histograms
  resetDisplayHistos();

  // Then fill from the strip histogram
  mDigitsHelper.fillMapHistos(mNoise.get(), mBendNoiseMap, mNBendNoiseMap);
  mDigitsHelper.fillMapHistos(mDead.get(), mBendDeadMap, mNBendDeadMap);
}

void CalibQcTask::endOfActivity(const Activity& /*activity*/)
{
}

void CalibQcTask::resetDisplayHistos()
{
  for (auto& histo : mBendNoiseMap) {
    histo->Reset();
  }
  for (auto& histo : mNBendNoiseMap) {
    histo->Reset();
  }
  for (auto& histo : mBendDeadMap) {
    histo->Reset();
  }
  for (auto& histo : mNBendDeadMap) {
    histo->Reset();
  }
}

void CalibQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Devel) << "Resetting the histogram" << ENDM;

  mNbTimeFrame->Reset();
  mNbNoiseROF->Reset();
  mNbDeadROF->Reset();

  for (auto& histo : mMultNoiseB) {
    histo->Reset();
  }
  for (auto& histo : mMultNoiseNB) {
    histo->Reset();
  }
  mNoise->Reset();
  mDead->Reset();
  resetDisplayHistos();
}

} // namespace o2::quality_control_modules::mid
