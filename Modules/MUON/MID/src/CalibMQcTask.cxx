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

  std::array<std::string, 4> chId{ "11", "12", "21", "22" };

  mNbBadChannelTF = std::make_unique<TH1F>("NbBadChannelTF", "Anaylyzed timeframes", 1, 0, 1.);
  getObjectsManager()->startPublishing(mNbBadChannelTF.get());

  mNoise = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("MNoiseStrips", "Noise strips"));
  getObjectsManager()->startPublishing(mNoise.get());

  mBendNoiseMap = mDigitsHelper.makeStripMapHistos("MBendNoiseMap", "Bending Noise Map", 0);
  for (auto& histo : mBendNoiseMap) {
    getObjectsManager()->startPublishing(histo.get());
    getObjectsManager()->setDefaultDrawOptions(histo.get(), "COLZ");
  }

  mNBendNoiseMap = mDigitsHelper.makeStripMapHistos("MNBendNoiseMap", "Non-Bending Noise Map", 1);
  for (auto& histo : mNBendNoiseMap) {
    getObjectsManager()->startPublishing(histo.get());
    getObjectsManager()->setDefaultDrawOptions(histo.get(), "COLZ");
  }

  mDead = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("MDeadStrips", "Dead strips"));
  getObjectsManager()->startPublishing(mDead.get());

  mBendDeadMap = mDigitsHelper.makeStripMapHistos("MBendDeadMap", "Bending Dead Map", 0);
  for (auto& histo : mBendDeadMap) {
    getObjectsManager()->startPublishing(histo.get());
    getObjectsManager()->setDefaultDrawOptions(histo.get(), "COLZ");
  }

  mNBendDeadMap = mDigitsHelper.makeStripMapHistos("MNBendDeadMap", "Non-Bending Dead Map", 1);
  for (auto& histo : mNBendDeadMap) {
    getObjectsManager()->startPublishing(histo.get());
    getObjectsManager()->setDefaultDrawOptions(histo.get(), "COLZ");
  }

  mBad = std::make_unique<TH1F>(mDigitsHelper.makeStripHisto("MBadStrips", "Bad strips"));
  getObjectsManager()->startPublishing(mBad.get());

  mBendBadMap = mDigitsHelper.makeStripMapHistos("MBendBadMap", "Bending Bad Map", 0);
  for (auto& histo : mBendBadMap) {
    getObjectsManager()->startPublishing(histo.get());
    getObjectsManager()->setDefaultDrawOptions(histo.get(), "COLZ");
  }

  mNBendBadMap = mDigitsHelper.makeStripMapHistos("MNBendBadMap", "Non-Bending Bad Map", 1);
  for (auto& histo : mNBendBadMap) {
    getObjectsManager()->startPublishing(histo.get());
    getObjectsManager()->setDefaultDrawOptions(histo.get(), "COLZ");
  }
}

void CalibMQcTask::startOfActivity(const Activity& /*activity*/)
{
  reset();
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
  mDigitsHelper.fillStripMapHistos(mNoise.get(), mBendNoiseMap, mNBendNoiseMap);
  mDigitsHelper.fillStripMapHistos(mDead.get(), mBendDeadMap, mNBendDeadMap);
  mDigitsHelper.fillStripMapHistos(mBad.get(), mBendBadMap, mNBendBadMap);
}

void CalibMQcTask::endOfActivity(const Activity& /*activity*/)
{
  reset();
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
