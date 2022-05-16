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
#include <THnSparse.h>

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
  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    // delete sClustersSize[iLayer];
    delete hClusterSizeLayerSummary[iLayer];
    delete hClusterTopologyLayerSummary[iLayer];
    delete hGroupedClusterSizeLayerSummary[iLayer];

    if (iLayer < 3) {

      delete hAverageClusterOccupancySummaryIB[iLayer];
      delete hAverageClusterOccupancyMonitorIB[iLayer];
      delete hAverageClusterSizeSummaryIB[iLayer];
      delete hAverageClusterSizeMonitorIB[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

          delete hClusterSizeSummaryIB[iLayer][iStave][iChip];
          delete hClusterSizeMonitorIB[iLayer][iStave][iChip];
          delete hClusterTopologySummaryIB[iLayer][iStave][iChip];
          delete hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip];
        }
      }
    } else {

      delete hAverageClusterOccupancySummaryOB[iLayer];
      delete hAverageClusterOccupancyMonitorOB[iLayer];
      delete hAverageClusterSizeSummaryOB[iLayer];
      delete hAverageClusterSizeMonitorOB[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        delete hClusterSizeSummaryOB[iLayer][iStave];
        delete hClusterTopologySummaryOB[iLayer][iStave];
        delete hGroupedClusterSizeSummaryOB[iLayer][iStave];

        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          delete hClusterSizeOB[iLayer][iStave][iHic];
          delete hClusterSizeMonitorOB[iLayer][iStave][iHic];
        }
      }
    }
  }
}

void ITSClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Info, Support) << "initialize ITSClusterTask" << ENDM;

  getJsonParameters();

  o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  mGeom = o2::its::GeometryTGeo::Instance();

  createAllHistos();

  mGeneralOccupancy = new TH2Poly();
  mGeneralOccupancy->SetTitle("General Occupancy (max clusters /event/chip);mm;mm");
  mGeneralOccupancy->SetName("General/General_Occupancy");

  for (int iLayer = 0; iLayer < 7; iLayer++) {
    for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
      double* px = new double[4];
      double* py = new double[4];
      getStavePoint(iLayer, iStave, px, py);
      mGeneralOccupancy->AddBin(4, px, py);
    }
  }

  addObject(mGeneralOccupancy);
  mGeneralOccupancy->SetStats(0);
  mGeneralOccupancy->SetBit(TH1::kIsAverage);

  publishHistos();

  // get dict from ccdb
  mTimestamp = std::stol(mCustomParameters["dicttimestamp"]);
  long int ts = mTimestamp ? mTimestamp : o2::ccdb::getCurrentTimestamp();
  ILOG(Info, Support) << "Getting dictionary from ccdb - timestamp: " << ts << ENDM;
  auto& mgr = o2::ccdb::BasicCCDBManager::instance();
  mgr.setURL("http://alice-ccdb.cern.ch");
  mgr.setTimestamp(ts);
  mDict = mgr.get<o2::itsmft::TopologyDictionary>("ITS/Calib/ClusterDictionary");
  ILOG(Info, Support) << "Dictionary size: " << mDict->getSize() << ENDM;
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
      int lay, sta, ssta, mod, chip;

      if (ChipID != ChipIDprev) {
        mGeom->getChipId(ChipID, lay, sta, ssta, mod, chip);
        mod = mod + (ssta * (mNHicPerStave[lay] / 2));
      }
      int npix = -1;
      int isGrouped = -1;
      if (ClusterID != o2::itsmft::CompCluster::InvalidPatternID && !mDict->isGroup(ClusterID)) { // Normal (frequent) cluster shapes
        npix = mDict->getNpixels(ClusterID);
        isGrouped = 0;
      } else {
        o2::itsmft::ClusterPattern patt(pattIt);
        npix = patt.getNPixels();
        isGrouped = 1;
      }

      if (npix > 2)
        nClustersForBunchCrossing++;

      if (lay < 3) {

        mClusterOccupancyIB[lay][sta][chip]++;
        mClusterOccupancyIBmonitor[lay][sta][chip]++;
        if (ClusterID < dictSize) {

          // Double_t ClusterSizeFill[3] = {1.*sta, 1.*chip,1.* mDict.getNpixels(ClusterID)};
          // sClustersSize[lay]->Fill(ClusterSizeFill, 1.);

          hClusterTopologySummaryIB[lay][sta][chip]->Fill(ClusterID);
          hClusterSizeSummaryIB[lay][sta][chip]->Fill(npix);

          hClusterSizeLayerSummary[lay]->Fill(npix);
          hClusterTopologyLayerSummary[lay]->Fill(ClusterID);

          hClusterSizeMonitorIB[lay][sta][chip]->Fill(npix);
          if (isGrouped) {
            hGroupedClusterSizeSummaryIB[lay][sta][chip]->Fill(npix);
            hGroupedClusterSizeLayerSummary[lay]->Fill(npix);
          }
        }
      } else {

        mClusterOccupancyOB[lay][sta][mod]++;
        mClusterOccupancyOBmonitor[lay][sta][mod]++;
        if (ClusterID < dictSize) {
          // Double_t ClusterSizeFill[3] = {1.*sta, 1.*mod, 1.*mDict.getNpixels(ClusterID)};
          // sClustersSize[lay]->Fill(ClusterSizeFill, 1.);

          hClusterSizeOB[lay][sta][mod]->Fill(npix);
          hClusterSizeMonitorOB[lay][sta][mod]->Fill(npix);

          hClusterTopologySummaryOB[lay][sta]->Fill(ClusterID);
          hClusterSizeSummaryOB[lay][sta]->Fill(npix);
          hClusterSizeLayerSummary[lay]->Fill(npix);
          hClusterTopologyLayerSummary[lay]->Fill(ClusterID);
          if (isGrouped) {
            hGroupedClusterSizeSummaryOB[lay][sta]->Fill(npix);
            hGroupedClusterSizeLayerSummary[lay]->Fill(npix);
          }
        }
      }
    }
    hClusterVsBunchCrossing->Fill(bcdata.bc, nClustersForBunchCrossing); // we count only the number of clusters, not their sizes
  }

  mNRofs += clusRofArr.size();        // USED to calculate occupancy for the whole run
  mNRofsMonitor += clusRofArr.size(); // Occupancy in the last N ROFs

  if (mNRofs > 0) {
    for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

      if (!mEnableLayers[iLayer])
        continue;

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        if (iLayer < 3) {
          for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
            hAverageClusterOccupancySummaryIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, 1. * mClusterOccupancyIB[iLayer][iStave][iChip] / mNRofs);
            hAverageClusterOccupancySummaryIB[iLayer]->SetBinError(iChip + 1, iStave + 1, mClusterOccupancyIB[iLayer][iStave][iChip] < 1e-15 ? 1e200 : 0);
            hAverageClusterSizeSummaryIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, hClusterSizeSummaryIB[iLayer][iStave][iChip]->GetMean());
            hAverageClusterSizeSummaryIB[iLayer]->SetBinError(iChip + 1, iStave + 1, hClusterSizeSummaryIB[iLayer][iStave][iChip]->GetMean() < 1e-15 ? 1e200 : 0);
          }
          mGeneralOccupancy->SetBinContent(iStave + 1 + StaveBoundary[iLayer], (float)(*(std::max_element(mClusterOccupancyIB[iLayer][iStave], mClusterOccupancyIB[iLayer][iStave] + mNChipsPerHic[iLayer]))) / (float)(mNRofs));
          mGeneralOccupancy->SetBinError(iStave + 1 + StaveBoundary[iLayer], mGeneralOccupancy->GetBinContent(iStave + 1 + StaveBoundary[iLayer]) < 1e-15 ? 1e200 : 0);
        } else {

          for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
            hAverageClusterOccupancySummaryOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, 1. * mClusterOccupancyOB[iLayer][iStave][iHic] / mNRofs / 14); // 14 To have occupation per chip
            hAverageClusterOccupancySummaryOB[iLayer]->SetBinError(iHic + 1, iStave + 1, mClusterOccupancyOB[iLayer][iStave][iHic] < 1e-15 ? 1e200 : 0);  // 14 To have occupation per chip
            hAverageClusterSizeSummaryOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, hClusterSizeOB[iLayer][iStave][iHic]->GetMean());
            hAverageClusterSizeSummaryOB[iLayer]->SetBinError(iHic + 1, iStave + 1, hClusterSizeOB[iLayer][iStave][iHic]->GetMean() < 1e-15 ? 1e200 : 0);
          }

          mGeneralOccupancy->SetBinContent(iStave + 1 + StaveBoundary[iLayer], (float)(*(std::max_element(mClusterOccupancyOB[iLayer][iStave], mClusterOccupancyOB[iLayer][iStave] + mNHicPerStave[iLayer]))) / (float)(mNRofs) / 14.);
          mGeneralOccupancy->SetBinError(iStave + 1 + StaveBoundary[iLayer], mGeneralOccupancy->GetBinContent(iStave + 1 + StaveBoundary[iLayer]) < 1e-15 ? 1e200 : 0);
        }
      }
    }
  }

  if (mNRofsMonitor >= mOccUpdateFrequency) {
    updateOccMonitorPlots();
    mNRofsMonitor = 0;
    memset(mClusterOccupancyIBmonitor, 0, sizeof(mClusterOccupancyIBmonitor));
    memset(mClusterOccupancyOBmonitor, 0, sizeof(mClusterOccupancyOBmonitor));

    for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
      if (!mEnableLayers[iLayer])
        continue;
      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        if (iLayer < 3)
          for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++)
            hClusterSizeMonitorIB[iLayer][iStave][iChip]->Reset();

        else
          for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++)
            hClusterSizeMonitorOB[iLayer][iStave][iHic]->Reset();
      }
    }
  }

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info, Support) << "Time in QC Cluster Task:  " << difference << ENDM;
}

void ITSClusterTask::updateOccMonitorPlots()
{

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++)
      if (iLayer < 3) {

        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hAverageClusterOccupancyMonitorIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, 1. * mClusterOccupancyIBmonitor[iLayer][iStave][iChip] / mNRofsMonitor);
          hAverageClusterOccupancyMonitorIB[iLayer]->SetBinError(iChip + 1, iStave + 1, mClusterOccupancyIBmonitor[iLayer][iStave][iChip] < 1e-15 ? 1e200 : 0);
          hAverageClusterSizeMonitorIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, hClusterSizeMonitorIB[iLayer][iStave][iChip]->GetMean());
          hAverageClusterSizeMonitorIB[iLayer]->SetBinError(iChip + 1, iStave + 1, hClusterSizeMonitorIB[iLayer][iStave][iChip]->GetMean() < 1e-15 ? 1e200 : 0);
        }
      } else {

        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hAverageClusterOccupancyMonitorOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, 1. * mClusterOccupancyOBmonitor[iLayer][iStave][iHic] / mNRofsMonitor / 14); // 14 To have occupation per chip
          hAverageClusterOccupancyMonitorOB[iLayer]->SetBinError(iHic + 1, iStave + 1, mClusterOccupancyOBmonitor[iLayer][iStave][iHic] < 1e-15 ? 1e200 : 0);
          hAverageClusterSizeMonitorOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, hClusterSizeMonitorOB[iLayer][iStave][iHic]->GetMean());
          hAverageClusterSizeMonitorOB[iLayer]->SetBinError(iHic + 1, iStave + 1, hClusterSizeMonitorOB[iLayer][iStave][iHic]->GetMean() < 1e-15 ? 1e200 : 0);
        }
      }
  }
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

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;
    // sClustersSize[iLayer]->Reset();
    hClusterSizeLayerSummary[iLayer]->Reset();
    hGroupedClusterSizeLayerSummary[iLayer]->Reset();
    hClusterTopologyLayerSummary[iLayer]->Reset();

    if (iLayer < 3) {
      hAverageClusterOccupancySummaryIB[iLayer]->Reset();
      hAverageClusterOccupancyMonitorIB[iLayer]->Reset();
      hAverageClusterSizeSummaryIB[iLayer]->Reset();
      hAverageClusterSizeMonitorIB[iLayer]->Reset();
      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterSizeSummaryIB[iLayer][iStave][iChip]->Reset();
          hClusterTopologySummaryIB[iLayer][iStave][iChip]->Reset();
          hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]->Reset();
        }
      }
    } else {
      hAverageClusterOccupancySummaryOB[iLayer]->Reset();
      hAverageClusterSizeSummaryOB[iLayer]->Reset();
      hAverageClusterOccupancyMonitorOB[iLayer]->Reset();
      hAverageClusterSizeMonitorOB[iLayer]->Reset();
      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        hClusterTopologySummaryOB[iLayer][iStave]->Reset();
        hGroupedClusterSizeSummaryOB[iLayer][iStave]->Reset();

        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hClusterSizeOB[iLayer][iStave][iHic]->Reset();
        }
      }
    }
  }
}

void ITSClusterTask::createAllHistos()
{

  hClusterVsBunchCrossing = new TH2D("BunchCrossingIDvsClusters", "BunchCrossingIDvsClusters", nBCbins, 0, 4095, 100, 0, 1000);
  hClusterVsBunchCrossing->SetTitle("#clusters vs BC id for clusters with npix > 2");
  addObject(hClusterVsBunchCrossing);
  formatAxes(hClusterVsBunchCrossing, "Bunch Crossing ID", "Number of clusters with npix > 2 in ROF", 1, 1.10);
  hClusterVsBunchCrossing->SetStats(0);

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    hClusterSizeLayerSummary[iLayer] = new TH1D(Form("Layer%d/AverageClusterSizeSummary", iLayer), Form("Layer%dAverageClusterSizeSummary", iLayer), 100, 0, 100);
    hClusterSizeLayerSummary[iLayer]->SetTitle(Form("Cluster size summary for Layer %d", iLayer));
    addObject(hClusterSizeLayerSummary[iLayer]);
    formatAxes(hClusterSizeLayerSummary[iLayer], "Cluster Size (pixels)", "counts", 1, 1.10);
    hClusterSizeLayerSummary[iLayer]->SetStats(0);

    hGroupedClusterSizeLayerSummary[iLayer] = new TH1D(Form("Layer%d/AverageGroupedClusterSizeSummary", iLayer), Form("Layer%dAverageGroupedClusterSizeSummary", iLayer), 100, 0, 100);
    hGroupedClusterSizeLayerSummary[iLayer]->SetTitle(Form("Cluster size summary for Layer %d", iLayer));
    addObject(hGroupedClusterSizeLayerSummary[iLayer]);
    formatAxes(hGroupedClusterSizeLayerSummary[iLayer], "Grouped Cluster Size (pixels)", "counts", 1, 1.10);
    hGroupedClusterSizeLayerSummary[iLayer]->SetStats(0);

    hClusterTopologyLayerSummary[iLayer] = new TH1D(Form("Layer%d/ClusterTopologySummary", iLayer), Form("Layer%dClusterTopologySummary", iLayer), 300, 0, 300);
    hClusterTopologyLayerSummary[iLayer]->SetTitle(Form("Cluster topology summary for Layer %d", iLayer));
    addObject(hClusterTopologyLayerSummary[iLayer]);
    formatAxes(hClusterTopologyLayerSummary[iLayer], "Cluster Topology (ID)", "counts", 1, 1.10);
    hClusterTopologyLayerSummary[iLayer]->SetStats(0);

    if (iLayer < 3) {

      hAverageClusterOccupancySummaryIB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOccupancySummaryIB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hAverageClusterOccupancySummaryIB[iLayer]);
      formatAxes(hAverageClusterOccupancySummaryIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterOccupancySummaryIB[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryIB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterOccupancyMonitorIB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupationForLastNROFS", iLayer), Form("Layer%dClusterOccupancyForLastNROFS", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOccupancyMonitorIB[iLayer]->SetTitle(Form("Cluster Occupancy for the last NROFs on Layer %d", iLayer));
      addObject(hAverageClusterOccupancyMonitorIB[iLayer]);
      formatAxes(hAverageClusterOccupancyMonitorIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterOccupancyMonitorIB[iLayer]->SetStats(0);
      hAverageClusterOccupancyMonitorIB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterSizeSummaryIB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeSummaryIB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterSizeSummaryIB[iLayer]);
      formatAxes(hAverageClusterSizeSummaryIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterSizeSummaryIB[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryIB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterSizeMonitorIB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSizeForLastNROFS", iLayer), Form("Layer%dAverageClusterSizeForLastNROFS", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeMonitorIB[iLayer]->SetTitle(Form("Average Cluster Size for the last NROFs on Layer %d", iLayer));
      addObject(hAverageClusterSizeMonitorIB[iLayer]);
      formatAxes(hAverageClusterSizeMonitorIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterSizeMonitorIB[iLayer]->SetStats(0);
      hAverageClusterSizeMonitorIB[iLayer]->SetBit(TH1::kIsAverage);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterSizeSummaryIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSize", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSize", iLayer, iStave, iChip), 100, 0, 100);
          hClusterSizeSummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hClusterSizeSummaryIB[iLayer][iStave][iChip]);
          formatAxes(hClusterSizeSummaryIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSizeGrouped", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSizeGroped", iLayer, iStave, iChip), 100, 0, 100);
          hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size for grouped topologies on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]);
          formatAxes(hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hClusterSizeMonitorIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSizeMonitor", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSizeMonitor", iLayer, iStave, iChip), 100, 0, 100);

          hClusterTopologySummaryIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterTopology", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterTopology", iLayer, iStave, iChip), 300, 0, 300);
          hClusterTopologySummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Topology on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hClusterTopologySummaryIB[iLayer][iStave][iChip]);
          formatAxes(hClusterTopologySummaryIB[iLayer][iStave][iChip], "Cluster topology (ID)", "Counts", 1, 1.10);
        }
      }
    } else {

      hAverageClusterOccupancySummaryOB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOccupancySummaryOB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hAverageClusterOccupancySummaryOB[iLayer]);
      formatAxes(hAverageClusterOccupancySummaryOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hAverageClusterOccupancySummaryOB[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryOB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterOccupancyMonitorOB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupationForLastNROFS", iLayer), Form("Layer%dClusterOccupancyForLastNROFS", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOccupancyMonitorOB[iLayer]->SetTitle(Form("Cluster Occupancy in last NROFS on Layer %d", iLayer));
      addObject(hAverageClusterOccupancyMonitorOB[iLayer]);
      formatAxes(hAverageClusterOccupancyMonitorOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hAverageClusterOccupancyMonitorOB[iLayer]->SetStats(0);
      hAverageClusterOccupancyMonitorOB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterSizeSummaryOB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeSummaryOB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterSizeSummaryOB[iLayer]);
      formatAxes(hAverageClusterSizeSummaryOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hAverageClusterSizeSummaryOB[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryOB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterSizeMonitorOB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSizeForLastNROFS", iLayer), Form("Layer%dAverageClusterSizeForLastNROFS", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeMonitorOB[iLayer]->SetTitle(Form("Average Cluster Size for the last NROFs on Layer %d", iLayer));
      addObject(hAverageClusterSizeMonitorOB[iLayer]);
      formatAxes(hAverageClusterSizeMonitorOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hAverageClusterSizeMonitorOB[iLayer]->SetStats(0);
      hAverageClusterSizeMonitorOB[iLayer]->SetBit(TH1::kIsAverage);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        hClusterSizeSummaryOB[iLayer][iStave] = new TH1D(Form("Layer%d/Stave%d/ClusterSize", iLayer, iStave), Form("Layer%dStave%dClusterSize", iLayer, iStave), 100, 0, 100);
        hClusterSizeSummaryOB[iLayer][iStave]->SetTitle(Form("Cluster Size summary for Layer %d Stave %d", iLayer, iStave));
        addObject(hClusterSizeSummaryOB[iLayer][iStave]);
        formatAxes(hClusterSizeSummaryOB[iLayer][iStave], "Cluster size (Pixel)", "Counts", 1, 1.10);

        hGroupedClusterSizeSummaryOB[iLayer][iStave] = new TH1D(Form("Layer%d/Stave%d/GroupedClusterSize", iLayer, iStave), Form("Layer%dStave%dGroupedClusterSize", iLayer, iStave), 100, 0, 100);
        hGroupedClusterSizeSummaryOB[iLayer][iStave]->SetTitle(Form("Grouped Cluster Size summary for Layer %d Stave %d", iLayer, iStave));
        addObject(hGroupedClusterSizeSummaryOB[iLayer][iStave]);
        formatAxes(hGroupedClusterSizeSummaryOB[iLayer][iStave], "Cluster size (Pixel)", "Counts", 1, 1.10);

        hClusterTopologySummaryOB[iLayer][iStave] = new TH1D(Form("Layer%d/Stave%d/ClusterTopology", iLayer, iStave), Form("Layer%dStave%dClusterTopology", iLayer, iStave), 300, 0, 300);
        hClusterTopologySummaryOB[iLayer][iStave]->SetTitle(Form("Cluster Toplogy summary for Layer %d Stave %d", iLayer, iStave));
        addObject(hClusterTopologySummaryOB[iLayer][iStave]);
        formatAxes(hClusterTopologySummaryOB[iLayer][iStave], "Cluster toplogy (ID)", "Counts", 1, 1.10);

        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) { // are used in TH2 construction, no need to keep on ccdb
          hClusterSizeOB[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterSize", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterSize", iLayer, iStave, iHic), 100, 0, 100);
          hClusterSizeMonitorOB[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterSizeMonitor", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterSizeMonitor", iLayer, iStave, iHic), 100, 0, 100);
        }
      }
    }
  }
}

void ITSClusterTask::getStavePoint(int layer, int stave, double* px, double* py)
{

  float stepAngle = TMath::Pi() * 2 / mNStaves[layer];            // the angle between to stave
  float midAngle = StartAngle[layer] + (stave * stepAngle);       // mid point angle
  float staveRotateAngle = TMath::Pi() / 2 - (stave * stepAngle); // how many angle this stave rotate(compare with first stave)
  px[1] = MidPointRad[layer] * TMath::Cos(midAngle);              // there are 4 point to decide this TH2Poly bin
                                                                  // 0:left point in this stave;
                                                                  // 1:mid point in this stave;
                                                                  // 2:right point in this stave;
                                                                  // 3:higher point int this stave;
  py[1] = MidPointRad[layer] * TMath::Sin(midAngle);              // 4 point calculated accord the blueprint
                                                                  // roughly calculate
  if (layer < NLayerIB) {
    px[0] = 7.7 * TMath::Cos(staveRotateAngle) + px[1];
    py[0] = -7.7 * TMath::Sin(staveRotateAngle) + py[1];
    px[2] = -7.7 * TMath::Cos(staveRotateAngle) + px[1];
    py[2] = 7.7 * TMath::Sin(staveRotateAngle) + py[1];
    px[3] = 5.623 * TMath::Sin(staveRotateAngle) + px[1];
    py[3] = 5.623 * TMath::Cos(staveRotateAngle) + py[1];
  } else {
    px[0] = 21 * TMath::Cos(staveRotateAngle) + px[1];
    py[0] = -21 * TMath::Sin(staveRotateAngle) + py[1];
    px[2] = -21 * TMath::Cos(staveRotateAngle) + px[1];
    py[2] = 21 * TMath::Sin(staveRotateAngle) + py[1];
    px[3] = 40 * TMath::Sin(staveRotateAngle) + px[1];
    py[3] = 40 * TMath::Cos(staveRotateAngle) + py[1];
  }
}

void ITSClusterTask::getJsonParameters()
{
  mNThreads = stoi(mCustomParameters.find("nThreads")->second);
  nBCbins = stoi(mCustomParameters.find("nBCbins")->second);
  mGeomPath = mCustomParameters["geomPath"];

  for (int ilayer = 0; ilayer < NLayer; ilayer++) {

    if (mCustomParameters["layer"][ilayer] != '0') {
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

} // namespace o2::quality_control_modules::its
