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

#include "MID/CalibMQcTask.h"

#include "fmt/format.h"
#include "TH1.h"
#include "TH2.h"
#include "TStyle.h"

#include "QualityControl/QcInfoLogger.h"

#include "Framework/InputRecord.h"
#include "DataFormatsMID/ColumnData.h"
#include "MIDWorkflow/ColumnDataSpecsUtils.h"

namespace o2::quality_control_modules::mid
{

CalibMQcTask::~CalibMQcTask()
{
}

void CalibMQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  gStyle->SetPalette(kRainBow);

  std::array<string, 4> chId{ "11", "12", "21", "22" };

  mNbBadChannelTF = std::make_unique<TH1F>("NbBadChannelTF", "Anaylyzed timeframes", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbBadChannelTF.get());

  mNoise = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("MNoiseStrips", "Noise strips"));
  getObjectsManager()->startPublishing(mNoise.get());

  for (int ich = 0; ich < 4; ++ich) {
    mBendNoiseMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("MBendNoiseMap{}", chId[ich]), fmt::format("Bending Noise Map MT{}", chId[ich]), 0));
    getObjectsManager()->startPublishing(mBendNoiseMap[ich].get());
  }

  for (int ich = 0; ich < 4; ++ich) {
    mNBendNoiseMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("MNBendNoiseMap{}", chId[ich]), fmt::format("Non-Bending Noise Map MT{}", chId[ich]), 1));
    getObjectsManager()->startPublishing(mNBendNoiseMap[ich].get());
  }

  mDead = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("MDeadStrips", "Dead strips"));
  getObjectsManager()->startPublishing(mDead.get());

  for (int ich = 0; ich < 4; ++ich) {
    mBendDeadMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("MBendDeadMap{}", chId[ich]), fmt::format("Bending Dead Map MT{}", chId[ich]), 0));
    getObjectsManager()->startPublishing(mBendDeadMap[ich].get());
  }

  for (int ich = 0; ich < 4; ++ich) {
    mNBendDeadMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("MNBendDeadMap{}", chId[ich]), fmt::format("Non-Bending Dead Map MT{}", chId[ich]), 1));
    getObjectsManager()->startPublishing(mNBendDeadMap[ich].get());
  }

  mBad = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("MBadStrips", "Bad strips"));
  getObjectsManager()->startPublishing(mBad.get());

  for (int ich = 0; ich < 4; ++ich) {
    mBendBadMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("MBendBadMap{}", chId[ich]), fmt::format("Bending Bad Map MT{}", chId[ich]), 0));
    getObjectsManager()->startPublishing(mBendBadMap[ich].get());
  }

  for (int ich = 0; ich < 4; ++ich) {
    mNBendBadMap[ich] = std::make_unique<TH2F>(mDigitsHelper.makeStripMapHisto(fmt::format("MNBendBadMap{}", chId[ich]), fmt::format("Non-Bending Bad Map MT{}", chId[ich]), 1));
    getObjectsManager()->startPublishing(mNBendBadMap[ich].get());
  }
}

void CalibMQcTask::startOfActivity(const Activity& /*activity*/)
{
}

void CalibMQcTask::startOfCycle()
{
}

void CalibMQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  mNbBadChannelTF->Fill(0.5);

  auto noises = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("noisych");
  auto deads = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("deadch");
  auto bads = ctx.inputs().get<gsl::span<o2::mid::ColumnData>>("badch");

  for (auto& col : noises) {
    mDigitsHelper.fillStripHisto(col, mNoise.get());
  }
  for (auto& col : deads) {
    mDigitsHelper.fillStripHisto(col, mDead.get());
  }
  for (auto& col : bads) {
    mDigitsHelper.fillStripHisto(col, mBad.get());
  }
}

void CalibMQcTask::endOfCycle()
{
  // Fill here the 2D representation of the fired strips
  // First reset the old histograms
  resetDisplayHistos();

  // Then fill from the strip histogram
  mDigitsHelper.fillMapHistos(mNoise.get(), mBendNoiseMap, mNBendNoiseMap);
  mDigitsHelper.fillMapHistos(mDead.get(), mBendDeadMap, mNBendDeadMap);
  mDigitsHelper.fillMapHistos(mBad.get(), mBendBadMap, mNBendBadMap);
}

void CalibMQcTask::endOfActivity(const Activity& /*activity*/)
{
}

void CalibMQcTask::resetDisplayHistos()
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
  for (auto& histo : mBendBadMap) {
    histo->Reset();
  }
  for (auto& histo : mNBendBadMap) {
    histo->Reset();
  }
}

void CalibMQcTask::reset()
{
  mNbBadChannelTF->Reset();
  mNoise->Reset();
  mDead->Reset();
  mBad->Reset();
  resetDisplayHistos();
}

} // namespace o2::quality_control_modules::mid
