// Copyright CERN and copyright holders of ALICE O2. This software is
// // distributed under the terms of the GNU General Public License v3 (GPL
// // Version 3), copied verbatim in the file "COPYING".
// //
// // See http://alice-o2.web.cern.ch/license for full licensing information.
// //
// // In applying this license CERN does not waive the privileges and immunities
// // granted to it by virtue of its status as an Intergovernmental Organization
// // or submit itself to any jurisdiction.
//

///
/// \file   ITSClusterTask.cxx
/// \author Artem Isakov
/// \author Antonio Palasciano
///

#include "QualityControl/QcInfoLogger.h"
#include "ITS/ITSClusterTask.h"

#include <sstream>
#include <TCanvas.h>
#include <DataFormatsParameters/GRPObject.h>
#include <ITSMFTReconstruction/DigitPixelReader.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include "ITSMFTReconstruction/ClustererParam.h"
#include "DetectorsCommonDataFormats/DetectorNameConf.h"
#include "ITStracking/IOUtils.h"
#include <DataFormatsITSMFT/ClusterTopology.h>
#include "CCDB/BasicCCDBManager.h"
#include "CCDB/CCDBTimeStampUtils.h"
#include <Framework/InputRecord.h>
#include <TH1F.h>
#include <TH2F.h>
#include "Common/Utils.h"
#include "TLine.h"
#include "TLatex.h"

#ifdef WITH_OPENMP
#include <omp.h>
#endif

using o2::itsmft::Digit;
using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSClusterTask::ITSClusterTask() : TaskInterface() {}

ITSClusterTask::~ITSClusterTask()
{
  delete hClusterVsBunchCrossing;
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    delete hClusterSizeLayerSummary[iLayer];
    delete hClusterTopologyLayerSummary[iLayer];
    delete hGroupedClusterSizeLayerSummary[iLayer];

    if (iLayer < NLayerIB) {

      delete hAverageClusterOccupancySummaryIB[iLayer];
      delete hAverageClusterSizeSummaryIB[iLayer];

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

          delete hClusterTopologySummaryIB[iLayer][iStave][iChip];
          delete hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip];
        }
      }
    } else {

      delete hAverageClusterOccupancySummaryOB[iLayer];
      delete hAverageClusterSizeSummaryOB[iLayer];

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        delete hClusterSizeSummaryOB[iLayer][iStave];
        delete hClusterTopologySummaryOB[iLayer][iStave];
        delete hGroupedClusterSizeSummaryOB[iLayer][iStave];
      }
    }
    if (mDoPublishDetailedSummary == 1) {
      delete hAverageClusterOccupancySummaryFine[iLayer];
      delete hAverageClusterSizeSummaryFine[iLayer];
      delete hAverageClusterOccupancySummaryZPhi[iLayer];
      delete hAverageClusterSizeSummaryZPhi[iLayer];
    }
  }
}

void ITSClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSClusterTask" << ENDM;

  getJsonParameters();

  if (mLocalGeometryFile == 1) {
    ILOG(Info, Support) << "Getting geometry from local file" << ENDM;
    o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  } else {
    ILOG(Info, Support) << "Getting geometry from ccdb - timestamp: " << std::stol(mGeoTimestamp) << ENDM;
    std::map<std::string, std::string> metadata;
    TaskInterface::retrieveConditionAny<TGeoManager>("GLO/Config/GeometryAligned", metadata, std::stol(mGeoTimestamp));
    if (!o2::base::GeometryManager::isGeometryLoaded()) {
      ILOG(Fatal, Support) << "Can't retrive geometry from ccdb with timestamp: " << std::stol(mGeoTimestamp) << ENDM;
      throw std::runtime_error("Can't retrive geometry from ccdb!");
    }
  }
  mGeom = o2::its::GeometryTGeo::Instance();
  mGeom->fillMatrixCache(o2::math_utils::bit2Mask(o2::math_utils::TransformType::L2G));

  // create binning for fine checks
  setRphiBinningIB();
  setZBinningIB();
  setRphiBinningOB();
  setZBinningOB();

  createAllHistos();

  mGeneralOccupancy = new TH2F("General/General_Occupancy", "General Cluster Occupancy (max n_clusters/event/chip)", 24, -12, 12, 14, 0, 14);

  addObject(mGeneralOccupancy);
  mGeneralOccupancy->SetStats(0);
  mGeneralOccupancy->SetBit(TH1::kIsAverage);
  for (int iy = 1; iy <= mGeneralOccupancy->GetNbinsY(); iy++) {
    mGeneralOccupancy->GetYaxis()->SetBinLabel(iy, mYlabels[iy - 1].c_str());
  }
  mGeneralOccupancy->GetXaxis()->SetLabelOffset(999);
  mGeneralOccupancy->GetXaxis()->SetLabelSize(0);
  mGeneralOccupancy->GetZaxis()->SetTitle("Max Avg Cluster occ (clusters/event/chip)");
  publishHistos();
}

void ITSClusterTask::startOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "startOfActivity" << ENDM;
  reset();
}

void ITSClusterTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void ITSClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  if (mTimestamp == -1) { // get dict from ccdb
    mTimestamp = std::stol(o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "dicttimestamp", "0"));
    long int ts = mTimestamp ? mTimestamp : ctx.services().get<o2::framework::TimingInfo>().creation;
    ILOG(Info, Support) << "Getting dictionary from ccdb - timestamp: " << ts << ENDM;
    auto& mgr = o2::ccdb::BasicCCDBManager::instance();
    mgr.setTimestamp(ts);
    mDict = mgr.get<o2::itsmft::TopologyDictionary>("ITS/Calib/ClusterDictionary");
    ILOG(Info, Support) << "Dictionary size: " << mDict->getSize() << ENDM;
  }

  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  ILOG(Info, Support) << "START DOING QC General" << ENDM;
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto clusRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");
  auto clusPatternArr = ctx.inputs().get<gsl::span<unsigned char>>("patterns");
  auto pattIt = clusPatternArr.begin();
  int dictSize = mDict->getSize();

  int iPattern = 0;
  int ChipIDprev = -1;

  if (mDoPublishDetailedSummary == 1) {
    for (int iLayer = 0; iLayer < NLayer; iLayer++) {
      hAverageClusterSizeSummaryZPhi[iLayer]->Multiply(hAverageClusterOccupancySummaryZPhi[iLayer]);
      hAverageClusterOccupancySummaryZPhi[iLayer]->Scale(mNRofs);
      hAverageClusterSizeSummaryFine[iLayer]->Multiply(hAverageClusterOccupancySummaryFine[iLayer]);
      hAverageClusterOccupancySummaryFine[iLayer]->Scale(mNRofs);
    }
  }

#ifdef WITH_OPENMP
  omp_set_num_threads(mNThreads);
#pragma omp parallel for schedule(dynamic)
#endif

  // Filling cluster histogram for each ROF by open_mp
  for (unsigned int iROF = 0; iROF < clusRofArr.size(); iROF++) {

    const auto& ROF = clusRofArr[iROF];
    const auto bcdata = ROF.getBCData();
    int nClustersForBunchCrossing = 0;
    for (int icl = ROF.getFirstEntry(); icl < ROF.getFirstEntry() + ROF.getNEntries(); icl++) {

      auto& cluster = clusArr[icl];
      auto ChipID = cluster.getSensorID();
      int ClusterID = cluster.getPatternID(); // used for normal (frequent) cluster shapes
      int lay, sta, ssta, mod, chip, lane;

      if (ChipID != ChipIDprev) {
        mGeom->getChipId(ChipID, lay, sta, ssta, mod, chip);
        mod = mod + (ssta * (mNHicPerStave[lay] / 2));
        int chipIdLocal = (ChipID - ChipBoundary[lay]) % (14 * mNHicPerStave[lay]);
        lane = (chipIdLocal % (14 * mNHicPerStave[lay])) / (14 / 2);
      }
      int npix = -1;
      int isGrouped = -1;

      o2::math_utils::Point3D<float> locC; // local coordinates

      if (ClusterID != o2::itsmft::CompCluster::InvalidPatternID && !mDict->isGroup(ClusterID)) { // Normal (frequent) cluster shapes
        npix = mDict->getNpixels(ClusterID);
        isGrouped = 0;
      } else {
        o2::itsmft::ClusterPattern patt(pattIt);
        npix = patt.getNPixels();
        isGrouped = 1;
      }

      locC = mDict->getClusterCoordinates(cluster);

      if (npix > 2)
        nClustersForBunchCrossing++;

      if (lay < NLayerIB) {

        mClusterOccupancyIB[lay][sta][chip]++;

        mClusterSize[lay][sta][chip] += npix;
        nClusters[lay][sta][chip]++;
        hClusterTopologySummaryIB[lay][sta][chip]->Fill(ClusterID);

        hClusterSizeLayerSummary[lay]->Fill(npix);
        hClusterTopologyLayerSummary[lay]->Fill(ClusterID);

        if (isGrouped) {
          hGroupedClusterSizeSummaryIB[lay][sta][chip]->Fill(npix);
          hGroupedClusterSizeLayerSummary[lay]->Fill(npix);
        }
      } else {

        mClusterOccupancyOB[lay][sta][lane]++;

        mClusterSize[lay][sta][lane] += npix;
        nClusters[lay][sta][lane]++;

        hClusterTopologySummaryOB[lay][sta]->Fill(ClusterID);
        hClusterSizeSummaryOB[lay][sta]->Fill(npix);
        hClusterSizeLayerSummary[lay]->Fill(npix);
        hClusterTopologyLayerSummary[lay]->Fill(ClusterID);
        if (isGrouped) {
          hGroupedClusterSizeSummaryOB[lay][sta]->Fill(npix);
          hGroupedClusterSizeLayerSummary[lay]->Fill(npix);
        }
      }

      // Transformation to the local --> global
      if (mDoPublishDetailedSummary == 1) {
        auto gloC = mGeom->getMatrixL2G(ChipID) * locC;
        float phi = (float)TMath::ATan2(gloC.Y(), gloC.X());

        phi = (float)(phi * 180 / TMath::Pi());
        hAverageClusterOccupancySummaryZPhi[lay]->Fill(gloC.Z(), phi);
        hAverageClusterSizeSummaryZPhi[lay]->Fill(gloC.Z(), phi, (float)npix);

        hAverageClusterOccupancySummaryFine[lay]->Fill(getHorizontalBin(locC.Z(), chip, lay, lane), getVerticalBin(locC.X(), sta, lay));
        hAverageClusterSizeSummaryFine[lay]->Fill(getHorizontalBin(locC.Z(), chip, lay, lane), getVerticalBin(locC.X(), sta, lay), (float)npix);
      }
    }
    hClusterVsBunchCrossing->Fill(bcdata.bc, nClustersForBunchCrossing); // we count only the number of clusters, not their sizes
  }

  mNRofs += clusRofArr.size(); // USED to calculate occupancy for the whole run

  if (mNRofs > 0) {
    for (int iLayer = 0; iLayer < NLayer; iLayer++) {

      if (!mEnableLayers[iLayer])
        continue;

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        if (iLayer < NLayerIB) {
          for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
            hAverageClusterOccupancySummaryIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, 1. * mClusterOccupancyIB[iLayer][iStave][iChip] / mNRofs);
            hAverageClusterOccupancySummaryIB[iLayer]->SetBinError(iChip + 1, iStave + 1, 1e-15);
            hAverageClusterSizeSummaryIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, nClusters[iLayer][iStave][iChip] != 0 ? (double)mClusterSize[iLayer][iStave][iChip] / nClusters[iLayer][iStave][iChip] : 0.);
            hAverageClusterSizeSummaryIB[iLayer]->SetBinError(iChip + 1, iStave + 1, 1e-15);
          }
          int ybin = iStave < (mNStaves[iLayer] / 2) ? 7 + iLayer + 1 : 7 - iLayer;
          int xbin = 12 - mNStaves[iLayer] / 4 + 1 + (iStave % (mNStaves[iLayer] / 2));
          mGeneralOccupancy->SetBinContent(xbin, ybin, (float)(*(std::max_element(mClusterOccupancyIB[iLayer][iStave], mClusterOccupancyIB[iLayer][iStave] + mNChipsPerHic[iLayer]))) / (float)(mNRofs));
          mGeneralOccupancy->SetBinError(xbin, ybin, 1e-15);
        } else {

          for (int iLane = 0; iLane < mNLanePerHic[iLayer] * mNHicPerStave[iLayer]; iLane++) {
            hAverageClusterOccupancySummaryOB[iLayer]->SetBinContent(iLane + 1, iStave + 1, 1. * mClusterOccupancyOB[iLayer][iStave][iLane] / mNRofs / (mNChipsPerHic[iLayer] / mNLanePerHic[iLayer])); // 14 To have occupation per chip -> 7 because we're considering lanes
            hAverageClusterOccupancySummaryOB[iLayer]->SetBinError(iLane + 1, iStave + 1, 1e-15);                                                                                                       // 14 To have occupation per chip
            hAverageClusterSizeSummaryOB[iLayer]->SetBinContent(iLane + 1, iStave + 1, nClusters[iLayer][iStave][iLane] != 0 ? (double)mClusterSize[iLayer][iStave][iLane] / nClusters[iLayer][iStave][iLane] : 0.);
            hAverageClusterSizeSummaryOB[iLayer]->SetBinError(iLane + 1, iStave + 1, 1e-15);
          }
          int ybin = iStave < (mNStaves[iLayer] / 2) ? 7 + iLayer + 1 : 7 - iLayer;
          int xbin = 12 - mNStaves[iLayer] / 4 + 1 + (iStave % (mNStaves[iLayer] / 2));
          mGeneralOccupancy->SetBinContent(xbin, ybin, (float)(*(std::max_element(mClusterOccupancyOB[iLayer][iStave], mClusterOccupancyOB[iLayer][iStave] + mNHicPerStave[iLayer] * mNLanePerHic[iLayer]))) / (float)(mNRofs) / (mNChipsPerHic[iLayer] / mNLanePerHic[iLayer]));
          mGeneralOccupancy->SetBinError(xbin, ybin, 1e-15);
        }
      }
      if (mDoPublishDetailedSummary == 1) {
        hAverageClusterSizeSummaryZPhi[iLayer]->Divide(hAverageClusterOccupancySummaryZPhi[iLayer]);
        hAverageClusterOccupancySummaryZPhi[iLayer]->Scale(1. / mNRofs);

        hAverageClusterSizeSummaryFine[iLayer]->Divide(hAverageClusterOccupancySummaryFine[iLayer]);
        hAverageClusterOccupancySummaryFine[iLayer]->Scale(1. / mNRofs);

        // Setting errors
        for (int i = 1; i <= hAverageClusterSizeSummaryZPhi[iLayer]->GetNbinsX(); i++) {
          for (int j = 1; j <= hAverageClusterSizeSummaryZPhi[iLayer]->GetNbinsY(); j++) {
            hAverageClusterSizeSummaryZPhi[iLayer]->SetBinError(i, j, 1.e-15);
            hAverageClusterOccupancySummaryZPhi[iLayer]->SetBinError(i, j, 1.e-15);
          }
        }
        for (int i = 1; i <= hAverageClusterSizeSummaryFine[iLayer]->GetNbinsX(); i++) {
          for (int j = 1; j <= hAverageClusterSizeSummaryFine[iLayer]->GetNbinsY(); j++) {
            hAverageClusterSizeSummaryFine[iLayer]->SetBinError(i, j, 1.e-15);
            hAverageClusterOccupancySummaryFine[iLayer]->SetBinError(i, j, 1.e-15);
          }
        }
      }
    }
  }

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info, Support) << "Time in QC Cluster Task:  " << difference << ENDM;
}

void ITSClusterTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void ITSClusterTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void ITSClusterTask::reset()
{
  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  hClusterVsBunchCrossing->Reset();
  mGeneralOccupancy->Reset();

  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    hClusterSizeLayerSummary[iLayer]->Reset();
    hGroupedClusterSizeLayerSummary[iLayer]->Reset();
    hClusterTopologyLayerSummary[iLayer]->Reset();

    if (iLayer < NLayerIB) {
      hAverageClusterOccupancySummaryIB[iLayer]->Reset();
      hAverageClusterSizeSummaryIB[iLayer]->Reset();
      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterTopologySummaryIB[iLayer][iStave][iChip]->Reset();
          hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]->Reset();
        }
      }
    } else {
      hAverageClusterOccupancySummaryOB[iLayer]->Reset();
      hAverageClusterSizeSummaryOB[iLayer]->Reset();
      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        hClusterTopologySummaryOB[iLayer][iStave]->Reset();
        hGroupedClusterSizeSummaryOB[iLayer][iStave]->Reset();
      }
    }
    if (mDoPublishDetailedSummary == 1) {
      hAverageClusterOccupancySummaryZPhi[iLayer]->Reset();
      hAverageClusterSizeSummaryZPhi[iLayer]->Reset();
      hAverageClusterOccupancySummaryFine[iLayer]->Reset();
      hAverageClusterSizeSummaryFine[iLayer]->Reset();
      addLines();
    }
  }
}

void ITSClusterTask::createAllHistos()
{

  hClusterVsBunchCrossing = new TH2F("BunchCrossingIDvsClusters", "BunchCrossingIDvsClusters", nBCbins, 0, 4095, 100, 0, 1000);
  hClusterVsBunchCrossing->SetTitle("#clusters vs BC id for clusters with npix > 2");
  addObject(hClusterVsBunchCrossing);
  formatAxes(hClusterVsBunchCrossing, "Bunch Crossing ID", "Number of clusters with npix > 2 in ROF", 1, 1.10);
  hClusterVsBunchCrossing->SetStats(0);

  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    hClusterSizeLayerSummary[iLayer] = new TH1F(Form("Layer%d/AverageClusterSizeSummary", iLayer), Form("Layer%dAverageClusterSizeSummary", iLayer), 100, 0, 100);
    hClusterSizeLayerSummary[iLayer]->SetTitle(Form("Cluster size summary for Layer %d", iLayer));
    addObject(hClusterSizeLayerSummary[iLayer]);
    formatAxes(hClusterSizeLayerSummary[iLayer], "Cluster Size (pixels)", "counts", 1, 1.10);
    hClusterSizeLayerSummary[iLayer]->SetStats(0);

    hGroupedClusterSizeLayerSummary[iLayer] = new TH1F(Form("Layer%d/AverageGroupedClusterSizeSummary", iLayer), Form("Layer%dAverageGroupedClusterSizeSummary", iLayer), 100, 0, 100);
    hGroupedClusterSizeLayerSummary[iLayer]->SetTitle(Form("Cluster size summary for Layer %d", iLayer));
    addObject(hGroupedClusterSizeLayerSummary[iLayer]);
    formatAxes(hGroupedClusterSizeLayerSummary[iLayer], "Grouped Cluster Size (pixels)", "counts", 1, 1.10);
    hGroupedClusterSizeLayerSummary[iLayer]->SetStats(0);

    hClusterTopologyLayerSummary[iLayer] = new TH1F(Form("Layer%d/ClusterTopologySummary", iLayer), Form("Layer%dClusterTopologySummary", iLayer), 300, 0, 300);
    hClusterTopologyLayerSummary[iLayer]->SetTitle(Form("Cluster topology summary for Layer %d", iLayer));
    addObject(hClusterTopologyLayerSummary[iLayer]);
    formatAxes(hClusterTopologyLayerSummary[iLayer], "Cluster Topology (ID)", "counts", 1, 1.10);
    hClusterTopologyLayerSummary[iLayer]->SetStats(0);

    if (mDoPublishDetailedSummary == 1) {
      hAverageClusterOccupancySummaryZPhi[iLayer] = new TH2F(Form("Layer%d/ClusterOccupancyZPhi", iLayer), Form("Cluster occupancy on Layer %d;z (cm);#phi (degree);", iLayer), 200, -100., 100., 360, -180., 180.);
      hAverageClusterOccupancySummaryZPhi[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryZPhi[iLayer]->SetBit(TH1::kIsAverage);
      hAverageClusterOccupancySummaryZPhi[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryZPhi[iLayer]->GetXaxis()->SetLabelSize(0.02);

      addObject(hAverageClusterOccupancySummaryZPhi[iLayer]);

      hAverageClusterSizeSummaryZPhi[iLayer] = new TH2F(Form("Layer%d/ClusterSizeZPhi", iLayer), Form("Cluster size on Layer %d;z (cm);#phi (degree);", iLayer), 200, -100., 100., 360, -180., 180.);
      hAverageClusterSizeSummaryZPhi[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryZPhi[iLayer]->SetBit(TH1::kIsAverage);
      hAverageClusterSizeSummaryZPhi[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryZPhi[iLayer]->GetXaxis()->SetLabelSize(0.02);
      addObject(hAverageClusterSizeSummaryZPhi[iLayer]);
    }

    if (iLayer < NLayerIB) {

      hAverageClusterOccupancySummaryIB[iLayer] = new TH2F(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOccupancySummaryIB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hAverageClusterOccupancySummaryIB[iLayer]);
      formatAxes(hAverageClusterOccupancySummaryIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterOccupancySummaryIB[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryIB[iLayer]->SetBit(TH1::kIsAverage);
      hAverageClusterOccupancySummaryIB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryIB[iLayer]->GetXaxis()->SetLabelSize(0.02);

      hAverageClusterSizeSummaryIB[iLayer] = new TH2F(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeSummaryIB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterSizeSummaryIB[iLayer]);
      formatAxes(hAverageClusterSizeSummaryIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterSizeSummaryIB[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryIB[iLayer]->SetBit(TH1::kIsAverage);
      hAverageClusterSizeSummaryIB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryIB[iLayer]->GetXaxis()->SetLabelSize(0.02);

      // Fine check
      if (mDoPublishDetailedSummary == 1) {
        hAverageClusterOccupancySummaryFine[iLayer] = new TH2F(Form("Layer%d/ClusterOccupancyFine", iLayer), Form("Cluster occupancy on Layer %d", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB) - 0.5, mNStaves[iLayer] * nRphiBinsIB, -0.5, mNStaves[iLayer] * nRphiBinsIB - 0.5);
        hAverageClusterOccupancySummaryFine[iLayer]->SetStats(0);
        hAverageClusterOccupancySummaryFine[iLayer]->SetBit(TH1::kIsAverage);
        hAverageClusterOccupancySummaryFine[iLayer]->GetYaxis()->SetLabelSize(0);
        hAverageClusterOccupancySummaryFine[iLayer]->GetXaxis()->SetLabelSize(0);
        hAverageClusterOccupancySummaryFine[iLayer]->GetYaxis()->SetTickSize(0);
        hAverageClusterOccupancySummaryFine[iLayer]->GetXaxis()->SetTickSize(0);
        addObject(hAverageClusterOccupancySummaryFine[iLayer]);

        hAverageClusterSizeSummaryFine[iLayer] = new TH2F(Form("Layer%d/ClusterSizeFine", iLayer), Form("Cluster size on Layer %d", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB) - 0.5, mNStaves[iLayer] * nRphiBinsIB, -0.5, mNStaves[iLayer] * nRphiBinsIB - 0.5);
        hAverageClusterSizeSummaryFine[iLayer]->SetStats(0);
        hAverageClusterSizeSummaryFine[iLayer]->SetBit(TH1::kIsAverage);
        hAverageClusterSizeSummaryFine[iLayer]->GetYaxis()->SetLabelSize(0);
        hAverageClusterSizeSummaryFine[iLayer]->GetXaxis()->SetLabelSize(0);
        hAverageClusterSizeSummaryFine[iLayer]->GetYaxis()->SetTickSize(0);
        hAverageClusterSizeSummaryFine[iLayer]->GetXaxis()->SetTickSize(0);
        addObject(hAverageClusterSizeSummaryFine[iLayer]);
      }

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        hAverageClusterSizeSummaryIB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterOccupancySummaryIB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));

        for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip] = new TH1F(Form("Layer%d/Stave%d/CHIP%d/ClusterSizeGrouped", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSizeGroped", iLayer, iStave, iChip), 100, 0, 100);
          hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size for grouped topologies on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          if (mDoPublish1DSummary == 1)
            addObject(hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]);
          formatAxes(hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hClusterTopologySummaryIB[iLayer][iStave][iChip] = new TH1F(Form("Layer%d/Stave%d/CHIP%d/ClusterTopology", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterTopology", iLayer, iStave, iChip), 300, 0, 300);
          hClusterTopologySummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Topology on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          if (mDoPublish1DSummary == 1)
            addObject(hClusterTopologySummaryIB[iLayer][iStave][iChip]);
          formatAxes(hClusterTopologySummaryIB[iLayer][iStave][iChip], "Cluster topology (ID)", "Counts", 1, 1.10);

          hAverageClusterSizeSummaryIB[iLayer]->GetXaxis()->SetBinLabel(iChip + 1, Form("Chip %i", iChip));
          hAverageClusterOccupancySummaryIB[iLayer]->GetXaxis()->SetBinLabel(iChip + 1, Form("Chip %i", iChip));
        }
      }
    } else {

      hAverageClusterOccupancySummaryOB[iLayer] = new TH2F(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNLanePerHic[iLayer] * mNHicPerStave[iLayer], 0, mNLanePerHic[iLayer] * mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOccupancySummaryOB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hAverageClusterOccupancySummaryOB[iLayer]);
      formatAxes(hAverageClusterOccupancySummaryOB[iLayer], "", "Stave Number", 1, 1.10);
      hAverageClusterOccupancySummaryOB[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryOB[iLayer]->SetBit(TH1::kIsAverage);
      hAverageClusterOccupancySummaryOB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryOB[iLayer]->GetXaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryOB[iLayer]->GetXaxis()->SetTitleOffset(1.2);

      hAverageClusterSizeSummaryOB[iLayer] = new TH2F(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNLanePerHic[iLayer] * mNHicPerStave[iLayer], 0, mNLanePerHic[iLayer] * mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeSummaryOB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterSizeSummaryOB[iLayer]);
      formatAxes(hAverageClusterSizeSummaryOB[iLayer], "", "Stave Number", 1, 1.10);
      hAverageClusterSizeSummaryOB[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryOB[iLayer]->SetOption("colz");
      hAverageClusterSizeSummaryOB[iLayer]->SetBit(TH1::kIsAverage);
      hAverageClusterSizeSummaryOB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryOB[iLayer]->GetXaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryOB[iLayer]->GetXaxis()->SetTitleOffset(1.2);

      // Fine check
      if (mDoPublishDetailedSummary == 1) {
        hAverageClusterOccupancySummaryFine[iLayer] = new TH2F(Form("Layer%d/ClusterOccupancyFine", iLayer), Form("Cluster occupancy on Layer %d", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB) - 0.5, mNStaves[iLayer] * nRphiBinsOB, -0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5);
        hAverageClusterOccupancySummaryFine[iLayer]->SetStats(0);
        hAverageClusterOccupancySummaryFine[iLayer]->SetBit(TH1::kIsAverage);
        hAverageClusterOccupancySummaryFine[iLayer]->GetYaxis()->SetLabelSize(0);
        hAverageClusterOccupancySummaryFine[iLayer]->GetXaxis()->SetLabelSize(0);
        hAverageClusterOccupancySummaryFine[iLayer]->GetYaxis()->SetTickSize(0);
        hAverageClusterOccupancySummaryFine[iLayer]->GetXaxis()->SetTickSize(0);
        addObject(hAverageClusterOccupancySummaryFine[iLayer]);

        hAverageClusterSizeSummaryFine[iLayer] = new TH2F(Form("Layer%d/ClusterSizeFine", iLayer), Form("Cluster size on Layer %d", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB) - 0.5, mNStaves[iLayer] * nRphiBinsOB, -0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5);
        hAverageClusterSizeSummaryFine[iLayer]->SetStats(0);
        hAverageClusterSizeSummaryFine[iLayer]->SetBit(TH1::kIsAverage);
        hAverageClusterSizeSummaryFine[iLayer]->GetYaxis()->SetLabelSize(0);
        hAverageClusterSizeSummaryFine[iLayer]->GetXaxis()->SetLabelSize(0);
        hAverageClusterSizeSummaryFine[iLayer]->GetYaxis()->SetTickSize(0);
        hAverageClusterSizeSummaryFine[iLayer]->GetXaxis()->SetTickSize(0);
        addObject(hAverageClusterSizeSummaryFine[iLayer]);
      }

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        hClusterSizeSummaryOB[iLayer][iStave] = new TH1F(Form("Layer%d/Stave%d/ClusterSize", iLayer, iStave), Form("Layer%dStave%dClusterSize", iLayer, iStave), 100, 0, 100);
        hClusterSizeSummaryOB[iLayer][iStave]->SetTitle(Form("Cluster Size summary for Layer %d Stave %d", iLayer, iStave));
        if (mDoPublish1DSummary == 1)
          addObject(hClusterSizeSummaryOB[iLayer][iStave]);
        formatAxes(hClusterSizeSummaryOB[iLayer][iStave], "Cluster size (Pixel)", "Counts", 1, 1.10);

        hGroupedClusterSizeSummaryOB[iLayer][iStave] = new TH1F(Form("Layer%d/Stave%d/GroupedClusterSize", iLayer, iStave), Form("Layer%dStave%dGroupedClusterSize", iLayer, iStave), 100, 0, 100);
        hGroupedClusterSizeSummaryOB[iLayer][iStave]->SetTitle(Form("Grouped Cluster Size summary for Layer %d Stave %d", iLayer, iStave));
        if (mDoPublish1DSummary == 1)
          addObject(hGroupedClusterSizeSummaryOB[iLayer][iStave]);
        formatAxes(hGroupedClusterSizeSummaryOB[iLayer][iStave], "Cluster size (Pixel)", "Counts", 1, 1.10);

        hClusterTopologySummaryOB[iLayer][iStave] = new TH1F(Form("Layer%d/Stave%d/ClusterTopology", iLayer, iStave), Form("Layer%dStave%dClusterTopology", iLayer, iStave), 300, 0, 300);
        hClusterTopologySummaryOB[iLayer][iStave]->SetTitle(Form("Cluster Toplogy summary for Layer %d Stave %d", iLayer, iStave));
        if (mDoPublish1DSummary == 1)
          addObject(hClusterTopologySummaryOB[iLayer][iStave]);
        formatAxes(hClusterTopologySummaryOB[iLayer][iStave], "Cluster toplogy (ID)", "Counts", 1, 1.10);

        hAverageClusterSizeSummaryOB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterOccupancySummaryOB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        for (int iLane = 0; iLane < mNLanePerHic[iLayer] * mNHicPerStave[iLayer]; iLane++) { // are used in TH2 construction, no need to keep on ccdb
          (iLayer < 5) ? (xLabel = Form("%s", OBLabel34[iLane])) : (xLabel = Form("%s", OBLabel56[iLane]));
          hAverageClusterSizeSummaryOB[iLayer]->GetXaxis()->SetBinLabel(iLane + 1, xLabel);
          hAverageClusterOccupancySummaryOB[iLayer]->GetXaxis()->SetBinLabel(iLane + 1, xLabel);
        }
      }
    }
  }
}

void ITSClusterTask::getJsonParameters()
{
  mLocalGeometryFile = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "isLocalGeometry", mLocalGeometryFile);
  mGeoTimestamp = o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "geomstamp", mGeoTimestamp);
  mNThreads = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nThreads", mNThreads);
  nBCbins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nBCbins", nBCbins);
  mGeomPath = o2::quality_control_modules::common::getFromConfig<string>(mCustomParameters, "geomPath", mGeomPath);
  mDoPublish1DSummary = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "publishSummary1D", mDoPublish1DSummary);
  std::string LayerConfig = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "layer", "0000000");
  mDoPublishDetailedSummary = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "publishDetailedSummary", mDoPublishDetailedSummary);

  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    if (LayerConfig[ilayer] != '0') {
      mEnableLayers[ilayer] = 1;
      ILOG(Info, Support) << "enable layer : " << ilayer << ENDM;
    } else {
      mEnableLayers[ilayer] = 0;
    }
  }
}

void ITSClusterTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Info, Support) << " ERROR: trying to add non-existent object " << ENDM;
    return;
  } else
    mPublishedObjects.push_back(aObject);
}

void ITSClusterTask::formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
{
  h->GetXaxis()->SetTitle(xTitle);
  h->GetYaxis()->SetTitle(yTitle);
  h->GetXaxis()->SetTitleOffset(xOffset);
  h->GetYaxis()->SetTitleOffset(yOffset);
}

void ITSClusterTask::publishHistos()
{
  for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++) {
    getObjectsManager()->startPublishing(mPublishedObjects.at(iObj));
  }
}

void ITSClusterTask::setRphiBinningIB(std::vector<float> bins)
{
  vRphiBinsIB = bins;
  nRphiBinsIB = (int)vRphiBinsIB.size() - 1;
}

void ITSClusterTask::setZBinningIB(std::vector<float> bins)
{
  vZBinsIB = bins;
  nZBinsIB = (int)vZBinsIB.size() - 1;
}

void ITSClusterTask::setRphiBinningOB(std::vector<float> bins)
{
  vRphiBinsOB = bins;
  nRphiBinsOB = (int)vRphiBinsOB.size() - 1;
}

void ITSClusterTask::setZBinningOB(std::vector<float> bins)
{
  vZBinsOB = bins;
  nZBinsOB = (int)vZBinsOB.size() - 1;
}

float ITSClusterTask::getHorizontalBin(float z, int chip, int layer, int lane)
{
  std::vector<float>::iterator iter_z;
  int index_z, chip_index;
  float return_index;
  if (layer < NLayerIB) {
    if (z < vZBinsIB[0] || z > vZBinsIB[nZBinsIB])
      return -999.;
    iter_z = std::upper_bound(vZBinsIB.begin(), vZBinsIB.end(), z,
                              [](const float& comp1, const float& comp2) { return comp1 < comp2; });
    index_z = std::distance(vZBinsIB.begin(), iter_z) - 1;
    chip_index = chip;
    return_index = (double)(nZBinsIB * chip_index + index_z);
  } else {
    if (z < vZBinsOB[0] || z > vZBinsOB[nZBinsOB])
      return -999.;
    iter_z = std::upper_bound(vZBinsOB.begin(), vZBinsOB.end(), z,
                              [](const float& comp1, const float& comp2) { return comp1 < comp2; });
    index_z = std::distance(vZBinsOB.begin(), iter_z) - 1;
    chip_index = chip % (mNChipsPerHic[layer] / mNLanePerHic[layer]) + (lane * (mNChipsPerHic[layer] / mNLanePerHic[layer]));
    return_index = (float)(nZBinsOB * chip_index + index_z);
  }
  return return_index;
}

float ITSClusterTask::getVerticalBin(float rphi, int stave, int layer)
{
  std::vector<float>::iterator iter_rphi;
  int index_rphi;
  float return_index;
  if (layer < 3) {
    if (rphi < vRphiBinsIB[0] || rphi > vRphiBinsIB[nRphiBinsIB])
      return -999.;
    iter_rphi = std::upper_bound(vRphiBinsIB.begin(), vRphiBinsIB.end(), rphi,
                                 [](const float& comp1, const float& comp2) { return comp1 < comp2; });
    index_rphi = std::distance(vRphiBinsIB.begin(), iter_rphi) - 1;
    return_index = (double)(stave * nRphiBinsIB + index_rphi);
  } else {
    if (rphi < vRphiBinsOB[0] || rphi > vRphiBinsOB[nRphiBinsOB])
      return -999.;
    iter_rphi = std::upper_bound(vRphiBinsOB.begin(), vRphiBinsOB.end(), rphi,
                                 [](const float& comp1, const float& comp2) { return comp1 < comp2; });
    index_rphi = std::distance(vRphiBinsOB.begin(), iter_rphi) - 1;
    return_index = (float)(stave * nRphiBinsOB + index_rphi);
  }
  return return_index;
}

void ITSClusterTask::addLines()
{
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    for (int i = 1; i <= (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer]); i++) // vertical
    {
      if (iLayer < NLayerIB) {
        if (i != (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer])) { // black lines
          TLine* l_v1 = new TLine(i * nZBinsIB - 0.5, -0.5, i * nZBinsIB - 0.5, mNStaves[iLayer] * nRphiBinsIB - 0.5);
          l_v1->SetLineWidth(2);
          hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(l_v1);
          TLine* l_v2 = new TLine(i * nZBinsIB - 0.5, -0.5, i * nZBinsIB - 0.5, mNStaves[iLayer] * nRphiBinsIB - 0.5);
          l_v2->SetLineWidth(2);
          hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(l_v2);
        }
        TLatex* latex_v1 = new TLatex(nZBinsIB / 4 + (i - 1) * nZBinsIB, -1 * nRphiBinsIB, Form("#bf{Chip %d}", i - 1));
        hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(latex_v1);
        TLatex* latex_v2 = new TLatex(nZBinsIB / 4 + (i - 1) * nZBinsIB, -1 * nRphiBinsIB, Form("#bf{Chip %d}", i - 1));
        hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(latex_v2);
      } else {
        if (i != (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer])) { // black lines
          TLine* l_v1 = new TLine(i * nZBinsOB - 0.5, -0.5, i * nZBinsOB - 0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5);
          l_v1->SetLineWidth(2);
          hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(l_v1);
          TLine* l_v2 = new TLine(i * nZBinsOB - 0.5, -0.5, i * nZBinsOB - 0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5);
          l_v2->SetLineWidth(2);
          hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(l_v2);
        }
      }
    }
    if (iLayer >= NLayerIB) {
      for (int i = 1; i <= (mNLanePerHic[iLayer] * mNHicPerStave[iLayer]); i++) // vertical
      {
        if (i != mNLanePerHic[iLayer] * mNHicPerStave[iLayer]) { // red lines
          TLine* l_red1 = new TLine(i * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer] - 0.5, -0.5, i * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer] - 0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5);
          l_red1->SetLineColor(kRed);
          l_red1->SetLineWidth(2);
          hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(l_red1);
          TLine* l_red2 = new TLine(i * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer] - 0.5, -0.5, i * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer] - 0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5);
          l_red2->SetLineColor(kRed);
          l_red2->SetLineWidth(2);
          hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(l_red2);
        }
        std::string xLabel = (iLayer < 5) ? Form("#bf{%s}", OBLabel34[i - 1]) : Form("#bf{%s}", OBLabel56[i - 1]);
        TLatex* latex_v1 = new TLatex(nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer] / 2 + (i - 1) * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer], -1 * nRphiBinsOB, xLabel.c_str());
        latex_v1->SetTextAngle(-20);
        latex_v1->SetTextSize(0.03);
        hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(latex_v1);
        TLatex* latex_v2 = new TLatex(nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer] / 2 + (i - 1) * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer], -1 * nRphiBinsOB, xLabel.c_str());
        latex_v2->SetTextAngle(-20);
        latex_v2->SetTextSize(0.03);
        hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(latex_v2);
      }
    }
    for (int j = 1; j <= mNStaves[iLayer]; j++) // horizontal
    {
      if (iLayer < NLayerIB) {
        TLine* l_h1 = new TLine(-0.5, j * nRphiBinsIB - 0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB) - 0.5, j * nRphiBinsIB - 0.5);
        l_h1->SetLineWidth(2);
        hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(l_h1);
        TLine* l_h2 = new TLine(-0.5, j * nRphiBinsIB - 0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB) - 0.5, j * nRphiBinsIB - 0.5);
        l_h2->SetLineWidth(2);
        hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(l_h2);

        TLatex* latex_h1 = new TLatex(-1 * nZBinsIB, nRphiBinsIB / 4 + (j - 1) * nRphiBinsIB, Form("#bf{Stave %d}", j - 1));
        hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(latex_h1);
        TLatex* latex_h2 = new TLatex(-1 * nZBinsIB, nRphiBinsIB / 4 + (j - 1) * nRphiBinsIB, Form("#bf{Stave %d}", j - 1));
        hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(latex_h2);
      } else {
        TLine* l_h1 = new TLine(-0.5, j * nRphiBinsOB - 0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB) - 0.5, j * nRphiBinsOB - 0.5);
        l_h1->SetLineWidth(2);
        hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(l_h1);
        TLine* l_h2 = new TLine(-0.5, j * nRphiBinsOB - 0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB) - 0.5, j * nRphiBinsOB - 0.5);
        l_h2->SetLineWidth(2);
        hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(l_h2);

        TLatex* latex_h1 = new TLatex(-1.5 * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer], nRphiBinsOB / 8 + (j - 1) * nRphiBinsOB, Form("#bf{Stave %d}", j - 1));
        latex_h1->SetTextSize(0.03);
        hAverageClusterOccupancySummaryFine[iLayer]->GetListOfFunctions()->Add(latex_h1);
        TLatex* latex_h2 = new TLatex(-1.5 * nZBinsOB * mNChipsPerHic[iLayer] / mNLanePerHic[iLayer], nRphiBinsOB / 8 + (j - 1) * nRphiBinsOB, Form("#bf{Stave %d}", j - 1));
        latex_h2->SetTextSize(0.03);
        hAverageClusterSizeSummaryFine[iLayer]->GetListOfFunctions()->Add(latex_h2);
      }
    }
  }
}

} // namespace o2::quality_control_modules::its
