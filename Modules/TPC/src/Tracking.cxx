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
/// \file   Tracking.cxx
/// \author David Rohr
///

// root includes
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH1D.h>
#include <TGraphAsymmErrors.h>

// O2 includes
#include "Framework/ProcessingContext.h"
#include "Framework/DataRefUtils.h"
#include <Framework/InputRecord.h>
#include "Framework/InputRecordWalker.h"
#include "DataFormatsTPC/TrackTPC.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "SimulationDataFormat/ConstMCTruthContainer.h"
#include "DataFormatsTPC/ClusterNative.h"
#include "DataFormatsTPC/WorkflowHelper.h"
#include "TPCQC/Helpers.h"
#include "GPUO2InterfaceQA.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/Tracking.h"

using namespace o2::tpc;
using namespace o2::dataformats;
using namespace o2::framework;
using namespace o2::header;

namespace o2::quality_control_modules::tpc
{

Tracking::Tracking() : TaskInterface() {}

Tracking::~Tracking()
{
}

void Tracking::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize TPC Tracking QC task" << ENDM;
  mOutputMode = o2::tpc::qc::Tracking::outputMergeable;
  mQCTracking.initialize(mOutputMode);

  if (mOutputMode == o2::tpc::qc::Tracking::outputMergeable) {
    const std::vector<TH1F>* h1;
    const std::vector<TH2F>* h2;
    const std::vector<TH1D>* h3;
    const std::vector<TGraphAsymmErrors>* h4;
    mQCTracking.getHists(h1, h2, h3, h4);
    for (auto& hist : *h1) {
      getObjectsManager()->startPublishing((TObject*)&hist);
    }
    for (auto& hist : *h2) {
      getObjectsManager()->startPublishing((TObject*)&hist);
    }
    for (auto& hist : *h3) {
      getObjectsManager()->startPublishing((TObject*)&hist);
    }
    for (auto& hist : *h4) {
      getObjectsManager()->startPublishing((TObject*)&hist);
    }
  }
}

void Tracking::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  mQCTracking.resetHistograms();
}

void Tracking::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void Tracking::monitorData(o2::framework::ProcessingContext& ctx)
{
  auto tracks = ctx.inputs().get<std::vector<o2::tpc::TrackTPC>>("inputTracks");
  auto trackLabels = ctx.inputs().get<std::vector<o2::MCCompLabel>>("inputTrackLabels");
  auto clusRefs = ctx.inputs().get<std::vector<o2::tpc::TPCClRefElem>>("inputClusRefs");
  const auto& inputsTPCclusters = o2::tpc::getWorkflowTPCInput(ctx, 0, true);

  LOG(info) << "RECEIVED tracks " << tracks.size() << " (MC " << trackLabels.size() << ", ClusRefs " << clusRefs.size() << ") clusters " << inputsTPCclusters->clusterIndex.nClustersTotal << " (MC " << inputsTPCclusters->clusterIndex.clustersMCTruth->getNElements() << ")";
  mQCTracking.processTracks(&tracks, &trackLabels, &inputsTPCclusters->clusterIndex);
}

void Tracking::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void Tracking::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void Tracking::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mQCTracking.resetHistograms();
}

} // namespace o2::quality_control_modules::tpc
