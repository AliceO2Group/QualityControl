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
/// \file   ClusterVisualizer.cxx
/// \author Thomas Klemenz
///

// O2 includes
#if __has_include("TPCBase/Painter.h")
#include "TPCBase/Painter.h"
#include "TPCBase/CDBInterface.h"
#else
#include "TPCBaseRecSim/Painter.h"
#include "TPCBaseRecSim/CDBInterface.h"
#endif
#include "TPCQC/Helpers.h"
#include "DetectorsBase/GRPGeomHelper.h"

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "TPC/ClusterVisualizer.h"
#include "TPC/Utility.h"
#include "TPC/ClustersData.h"

// root includes
#include "TCanvas.h"
#include "TPaveText.h"

#include <fmt/format.h>
#include <boost/property_tree/ptree.hpp>
#include <algorithm>

using namespace o2::quality_control::postprocessing;

namespace o2::quality_control_modules::tpc
{

void ClusterVisualizer::configure(const boost::property_tree::ptree& config)
{
  auto& id = getID();

  for (const auto& timestamp : config.get_child("qc.postprocessing." + id + ".timestamps")) {
    mTimestamps.emplace_back(std::stol(timestamp.second.data()));
  }

  std::vector<std::string> keyVec{};
  std::vector<std::string> valueVec{};
  for (const auto& data : config.get_child("qc.postprocessing." + id + ".lookupMetaData")) {
    mLookupMaps.emplace_back(std::map<std::string, std::string>());
    if (const auto& keys = data.second.get_child_optional("keys"); keys.has_value()) {
      for (const auto& key : keys.value()) {
        keyVec.emplace_back(key.second.data());
      }
    }
    if (const auto& values = data.second.get_child_optional("values"); values.has_value()) {
      for (const auto& value : values.value()) {
        valueVec.emplace_back(value.second.data());
      }
    }
    auto vecIter = 0;
    if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
      for (const auto& key : keyVec) {
        mLookupMaps.back().insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
        vecIter++;
      }
    }
    if (keyVec.size() != valueVec.size()) {
      ILOG(Error, Support) << "Number of keys and values for lookupMetaData are not matching" << ENDM;
    }
    keyVec.clear();
    valueVec.clear();
  }

  for (const auto& data : config.get_child("qc.postprocessing." + id + ".storeMetaData")) {
    mStoreMaps.emplace_back(std::map<std::string, std::string>());
    if (const auto& keys = data.second.get_child_optional("keys"); keys.has_value()) {
      for (const auto& key : keys.value()) {
        keyVec.emplace_back(key.second.data());
      }
    }
    if (const auto& values = data.second.get_child_optional("values"); values.has_value()) {
      for (const auto& value : values.value()) {
        valueVec.emplace_back(value.second.data());
      }
    }
    auto vecIter = 0;
    if ((keyVec.size() > 0) && (keyVec.size() == valueVec.size())) {
      for (const auto& key : keyVec) {
        mStoreMaps.back().insert(std::pair<std::string, std::string>(key, valueVec.at(vecIter)));
        vecIter++;
      }
    }
    if (keyVec.size() != valueVec.size()) {
      ILOG(Error, Support) << "Number of keys and values for storeMetaData are not matching" << ENDM;
    }
    keyVec.clear();
    valueVec.clear();
  }

  for (const auto& entry : config.get_child("qc.postprocessing." + id + ".histogramRanges")) {
    for (const auto& type : entry.second) {
      for (const auto& value : type.second) {
        mRanges[type.first].emplace_back(std::stof(value.second.data()));
      }
    }
  }

  mPath = config.get<std::string>("qc.postprocessing." + id + ".path");
  mHost = config.get<std::string>("qc.postprocessing." + id + ".dataSourceURL");

  const auto type = config.get<std::string>("qc.postprocessing." + id + ".dataType");
  if (type == "clusters") {
    mIsClusters = true;
    mObservables = {
      "N_Clusters",
      "Q_Max",
      "Q_Tot",
      "Sigma_Pad",
      "Sigma_Time",
      "Time_Bin",
      "Occupancy"
    };
  } else if (type == "raw") {
    mIsClusters = false;
    mObservables = {
      "N_RawDigits",
      "Q_Max",
      "Time_Bin",
      "Occupancy"
    };
  } else {
    ILOG(Error, Support) << "No valid data type given. 'dataType' has to be either 'clusters' or 'raw'." << ENDM;
  }
}

void ClusterVisualizer::initialize(Trigger, framework::ServiceRegistryRef)
{
  mCdbApi.init(mHost);

  mNHBFPerTF = o2::base::GRPGeomHelper::instance().getNHBFPerTF();

  if (mCalDetCanvasVec.size() > 0) {
    mCalDetCanvasVec.clear();
  }

  auto calDetIter = 0;
  for (const auto& type : mObservables) {
    mCalDetCanvasVec.emplace_back(std::vector<std::unique_ptr<TCanvas>>());
    addAndPublish(getObjectsManager(),
                  mCalDetCanvasVec.back(),
                  { fmt::format("c_Sides_{}", type).data(),
                    fmt::format("c_ROCs_{}_1D", type).data(),
                    fmt::format("c_ROCs_{}_2D", type).data() },
                  mStoreMaps.size() > 1 ? mStoreMaps.at(calDetIter) : mStoreMaps.at(0));
    calDetIter++;
  }
  mCalDetCanvasVec.emplace_back(std::vector<std::unique_ptr<TCanvas>>());
  addAndPublish(getObjectsManager(),
                mCalDetCanvasVec.back(),
                { "c_radial_profile_Occupancy" },
                mStoreMaps.size() > 1 ? mStoreMaps.at(calDetIter) : mStoreMaps.at(0));
}

void ClusterVisualizer::update(Trigger t, framework::ServiceRegistryRef)
{
  ILOG(Info, Support) << "Trigger type is: " << t.triggerType << ", the timestamp is " << t.timestamp << ENDM;

  for (auto& vec : mCalDetCanvasVec) {
    for (auto& canvas : vec) {
      canvas.get()->Clear();
    }
  }

  auto calDetIter = 0;

  std::unique_ptr<ClustersData> clusterData(mCdbApi.retrieveFromTFileAny<ClustersData>(mPath,
                                                                                       mLookupMaps.size() > 1 ? mLookupMaps.at(calDetIter) : mLookupMaps.at(0),
                                                                                       mTimestamps.size() > 0 ? mTimestamps.at(calDetIter) : t.timestamp));

  auto& clusters = clusterData->getClusters();

  auto& calDet = clusters.getNClusters();
  auto vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));

  o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
  calDetIter++;

  calDet = clusters.getQMax();
  vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
  calDetIter++;

  if (mIsClusters) {
    calDet = clusters.getQTot();
    vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;

    calDet = clusters.getSigmaPad();
    vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;

    calDet = clusters.getSigmaTime();
    vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
    o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
    calDetIter++;
  }

  calDet = clusters.getTimeBin();
  vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  o2::tpc::painter::makeSummaryCanvases(calDet, int(mRanges[calDet.getName()].at(0)), mRanges[calDet.getName()].at(1), mRanges[calDet.getName()].at(2), false, &vecPtr);
  calDetIter++;

  auto occupancy = clusters.getOccupancy(mNHBFPerTF);
  vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  o2::tpc::painter::makeSummaryCanvases(occupancy, int(mRanges[occupancy.getName()].at(0)), mRanges[occupancy.getName()].at(1), mRanges[occupancy.getName()].at(2), false, &vecPtr);
  calDetIter++;
  vecPtr = toVector(mCalDetCanvasVec.at(calDetIter));
  makeRadialProfile(occupancy, vecPtr.at(0), int(mRanges[occupancy.getName()].at(0)), mRanges[occupancy.getName()].at(1), mRanges[occupancy.getName()].at(2));
  calDetIter++;
}

void ClusterVisualizer::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (const auto& calDetCanvasVec : mCalDetCanvasVec) {
    for (const auto& canvas : calDetCanvasVec) {
      getObjectsManager()->stopPublishing(canvas.get());
    }
  }

  if (mCalDetCanvasVec.size() > 0) {
    mCalDetCanvasVec.clear();
  }
}

template <class T>
void ClusterVisualizer::makeRadialProfile(o2::tpc::CalDet<T>& calDet, TCanvas* canv, int nbinsY, float yMin, float yMax)
{
  const std::string_view calName = calDet.getName();
  const auto radialBinning = o2::tpc::painter::getRowBinningCM();

  auto hAside2D = new TH2D(fmt::format("h_{}_radialProfile_Aside", calName).data(), fmt::format("{}: Radial profile (A-Side)", calName).data(), radialBinning.size() - 1, radialBinning.data(), nbinsY, yMin, yMax);
  hAside2D->GetXaxis()->SetTitle("x (cm)");
  hAside2D->GetYaxis()->SetTitle(fmt::format("{}", calName).data());
  hAside2D->SetTitleOffset(1.05, "XY");
  hAside2D->SetTitleSize(0.05, "XY");
  hAside2D->SetStats(0);

  auto hCside2D = new TH2D(fmt::format("h_{}_radialProfile_Cside", calName).data(), fmt::format("{}: Radial profile (C-Side)", calName).data(), radialBinning.size() - 1, radialBinning.data(), nbinsY, yMin, yMax);
  hCside2D->GetXaxis()->SetTitle("x (cm)");
  hCside2D->GetYaxis()->SetTitle(fmt::format("{}", calName).data());
  hCside2D->SetTitleOffset(1.05, "XY");
  hCside2D->SetTitleSize(0.05, "XY");
  hCside2D->SetStats(0);

  fillRadialHisto(*hAside2D, calDet, o2::tpc::Side::A);
  fillRadialHisto(*hCside2D, calDet, o2::tpc::Side::C);

  canv->Divide(1, 2);
  canv->cd(1);
  hAside2D->Draw("colz");
  hAside2D->SetStats(0);
  hAside2D->ProfileX("profile_ASide", 1, -1, "d,same");

  canv->cd(2);
  hCside2D->Draw("colz");
  hCside2D->ProfileX("profile_CSide", 1, -1, "d,same");
  hAside2D->SetStats(0);

  hAside2D->SetBit(TObject::kCanDelete);
  hCside2D->SetBit(TObject::kCanDelete);
}

template <class T>
void ClusterVisualizer::fillRadialHisto(TH2D& h2D, const o2::tpc::CalDet<T>& calDet, const o2::tpc::Side side)
{
  const o2::tpc::Mapper& mapper = o2::tpc::Mapper::instance();

  for (o2::tpc::ROC roc; !roc.looped(); ++roc) {
    if (roc.side() != side) {
      continue;
    }
    const int nrows = mapper.getNumberOfRowsROC(roc);
    for (int irow = 0; irow < nrows; ++irow) {
      const int npads = mapper.getNumberOfPadsInRowROC(roc, irow);
      const int globalRow = irow + (roc >= o2::tpc::Mapper::getNumberOfIROCs()) * o2::tpc::Mapper::getNumberOfRowsInIROC();
      for (int ipad = 0; ipad < npads; ++ipad) {
        const auto val = calDet.getValue(roc, irow, ipad);
        const o2::tpc::LocalPosition2D pos = mapper.getPadCentre(o2::tpc::PadPos(globalRow, ipad));
        h2D.Fill(pos.X(), val);
      }
    }
  }
}

} // namespace o2::quality_control_modules::tpc
