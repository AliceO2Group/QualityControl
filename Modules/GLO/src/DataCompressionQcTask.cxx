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
/// \file   DataCompressionQcTask.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include <Framework/InputRecord.h>
#include "DetectorsCommonDataFormats/DetID.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "GLO/DataCompressionQcTask.h"
#include "Common/Utils.h"

const std::vector<std::string> histVecNames{ "entropy_compression", "compression" };

namespace o2::quality_control_modules::glo
{

void DataCompressionQcTask::initialize(o2::framework::InitContext&)
{
  // ILOG(Debug, Devel) << "initialize DataCompression QC task" << ENDM;

  TH1::AddDirectory(false);

  mIsMergeable = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "mergeableOutput");
  const auto nBins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nBins");
  const auto xMin = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "xMin");
  const auto xMax = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "xMax");

  // initialize histograms for active detectors
  bool useAll = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, "useAll");
  for (int iDet = 0; iDet < o2::detectors::DetID::nDetectors; ++iDet) {
    const auto detName = o2::detectors::DetID::getName(iDet);
    const bool useDet = o2::quality_control_modules::common::getFromConfig<bool>(mCustomParameters, detName);
    if (useDet || useAll) {
      for (const auto& type : histVecNames) {
        mCompressionHists[detName].emplace_back(std::make_unique<TH1F>(fmt::format("h_{}_{}", detName, type).data(),
                                                                       fmt::format("{} of {} data", type, detName).data(),
                                                                       nBins, xMin, xMax));
      }
    }
  }

  // register single histograms for publishing
  for (const auto& pair : mCompressionHists) {
    for (const auto& hist : pair.second) {
      getObjectsManager()->startPublishing(hist.get());
    }
  }

  if (!mIsMergeable) {
    //  initialize canvases and register them for publishing
    //  putting the histos to canvases makes the trending very easy because of the way the trending is implemented at the moment
    mEntropyCompressionCanvas = std::make_unique<TCanvas>("c_entropy_compression", "Entropy Compression Factor", 1000, 1000);
    mCompressionCanvas = std::make_unique<TCanvas>("c_compression", "Compression Factor", 1000, 1000);

    mEntropyCompressionCanvas->DivideSquare(mCompressionHists.size());
    mCompressionCanvas->DivideSquare(mCompressionHists.size());

    getObjectsManager()->startPublishing(mEntropyCompressionCanvas.get());
    getObjectsManager()->startPublishing(mCompressionCanvas.get());
  }
}

void DataCompressionQcTask::startOfActivity(const Activity&)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;

  for (const auto& pair : mCompressionHists) {
    for (const auto& hist : pair.second) {
      hist->Reset();
    }
  }
}

void DataCompressionQcTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void DataCompressionQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // loop over active detectors and process the data
  for (const auto& det : mCompressionHists) {
    auto ctfEncRep = ctx.inputs().get<o2::ctf::CTFIOSize>(fmt::format("ctfEncRep{}", det.first).data());
    processMessage(ctfEncRep, det.first);
  }

  if (!mIsMergeable) {
    // draw histograms to the canvases
    size_t padIter = 1;
    for (const auto& det : mCompressionHists) {
      mEntropyCompressionCanvas->cd(padIter);
      det.second[0]->Draw();
      mCompressionCanvas->cd(padIter);
      det.second[1]->Draw();
      padIter++;
    }
  }
}

void DataCompressionQcTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void DataCompressionQcTask::endOfActivity(const Activity&)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void DataCompressionQcTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histogramss" << ENDM;

  for (const auto& pair : mCompressionHists) {
    for (const auto& hist : pair.second) {
      hist->Reset();
    }
  }
}

void DataCompressionQcTask::processMessage(const o2::ctf::CTFIOSize& ctfEncRep, const std::string detector)
{
  float entropyCompression = 0;
  float compression = 0;

  if (ctfEncRep.ctfIn != 0) {
    entropyCompression = float(ctfEncRep.ctfIn - ctfEncRep.ctfOut) / float(ctfEncRep.ctfIn);
  }
  if (ctfEncRep.rawIn != 0) {
    compression = float(ctfEncRep.rawIn - ctfEncRep.ctfOut) / float(ctfEncRep.rawIn);
  }

  mCompressionHists[detector][0]->Fill(entropyCompression);
  mCompressionHists[detector][1]->Fill(compression);
}

} // namespace o2::quality_control_modules::glo
