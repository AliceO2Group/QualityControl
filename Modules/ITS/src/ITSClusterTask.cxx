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
#include "Common/Utils.h"

#include <ITSMFTReconstruction/DigitPixelReader.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <DataFormatsITSMFT/ClusterTopology.h>
#include <Framework/InputRecord.h>

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
  delete hTFCounter;
  delete hEmptyLaneFractionGlobal;
  delete hClusterVsBunchCrossing;

  for (int iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    if (iLayer < NLayerIB) {
      delete hClusterCenterMap[iLayer];
      delete hLongClustersPerChip[iLayer];
      delete hMultPerChipWhenLongClusters[iLayer];
    }

    else {
      delete hLongClustersPerStave[iLayer - NLayerIB];
    }
    delete hClusterSizeLayerSummary[iLayer];
    delete hClusterTopologyLayerSummary[iLayer];
    delete hGroupedClusterSizeLayerSummary[iLayer];
    delete hClusterOccupancyDistribution[iLayer];

    if (mDoPublish1DSummary == 1) {
      if (iLayer < NLayerIB) {
        for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
          for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

            delete hClusterTopologySummaryIB[iLayer][iStave][iChip];
            delete hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip];
            delete hClusterSizeSummaryIB[iLayer][iStave][iChip];
          }
        }
      } else {

        for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

          delete hClusterSizeSummaryOB[iLayer][iStave];
          delete hClusterTopologySummaryOB[iLayer][iStave];
          delete hGroupedClusterSizeSummaryOB[iLayer][iStave];
        }
      }
    }
  }
}

void ITSClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  ILOG(Debug, Devel) << "initialize ITSClusterTask" << ENDM;

  getJsonParameters();

  // Create binning for fine checks
  setRphiBinningIB();
  setZBinningIB();
  setRphiBinningOB();
  setZBinningOB();

  createAllHistos();
  mGeneralOccupancy = std::make_unique<TH2DRatio>("General/General_Occupancy", "General Cluster Occupancy (max n_clusters/event/chip)", 24, -12, 12, 14, 0, 14, true);

  addObject(mGeneralOccupancy.get());
  mGeneralOccupancy->SetStats(0);
  for (int iy = 1; iy <= mGeneralOccupancy->GetNbinsY(); iy++) {
    mGeneralOccupancy->GetYaxis()->SetBinLabel(iy, mYlabels[iy - 1].c_str());
  }
  mGeneralOccupancy->GetXaxis()->SetLabelOffset(999);
  mGeneralOccupancy->GetXaxis()->SetLabelSize(0);
  mGeneralOccupancy->GetZaxis()->SetTitle("Max Avg Cluster occ (clusters/event/chip)");

  publishHistos();
}

void ITSClusterTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void ITSClusterTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void ITSClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{

  if (mTimestamp == -1) { // get dict from ccdb
    mTimestamp = std::stol(o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "dicttimestamp", "0"));
    long int ts = mTimestamp ? mTimestamp : ctx.services().get<o2::framework::TimingInfo>().creation;
    ILOG(Debug, Devel) << "Getting dictionary from ccdb - timestamp: " << ts << ENDM;
    std::map<std::string, std::string> metadata;
    mDict = TaskInterface::retrieveConditionAny<o2::itsmft::TopologyDictionary>("ITS/Calib/ClusterDictionary", metadata, ts);
    ILOG(Debug, Devel) << "Dictionary size: " << mDict->getSize() << ENDM;
    o2::its::GeometryTGeo::adopt(TaskInterface::retrieveConditionAny<o2::its::GeometryTGeo>("ITS/Config/Geometry", metadata, ts));
    mGeom = o2::its::GeometryTGeo::Instance();
    ILOG(Debug, Devel) << "Loaded new instance of mGeom" << ENDM;
  }

  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();
  ILOG(Debug, Devel) << "START DOING QC General" << ENDM;
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto clusRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");
  auto clusPatternArr = ctx.inputs().get<gsl::span<unsigned char>>("patterns");
  auto pattIt = clusPatternArr.begin();

  // Reset this histo to have the latest picture
  hEmptyLaneFractionGlobal->Reset("ICES");

#ifdef WITH_OPENMP
  omp_set_num_threads(mNThreads);
#pragma omp parallel for schedule(dynamic)
#endif

  // Filling cluster histogram for each ROF by open_mp
  for (unsigned int iROF = 0; iROF < clusRofArr.size(); iROF++) {

    const auto& ROF = clusRofArr[iROF];
    const auto bcdata = ROF.getBCData();
    int nDigits3pixLay[7] = { 0 };
    int nClusters3pixLay[7] = { 0 };
    int nClusters3pix = 0;
    int nLongClusters[ChipBoundary[NLayerIB]] = {};     // for IB
    int nHitsFromClusters[ChipBoundary[NLayerIB]] = {}; // only IB is implemented at the moment
    int nLongClustersStave[4][mNStaves[6]] = { {} };    // for OB. nLongClustersStave[n][m] means stave L<3+n>_<m>

    for (int icl = ROF.getFirstEntry(); icl < ROF.getFirstEntry() + ROF.getNEntries(); icl++) {

      auto& cluster = clusArr[icl];
      auto ChipID = cluster.getSensorID();
      int ClusterID = cluster.getPatternID(); // used for normal (frequent) cluster shapes
      int lay, sta, ssta, mod, chip, lane;

      // TODO: avoid call Geom if ChipID is the same as previous cluster
      mGeom->getChipId(ChipID, lay, sta, ssta, mod, chip);
      mod = mod + (ssta * (mNHicPerStave[lay] / 2));
      int chipIdLocal = (ChipID - ChipBoundary[lay]) % (14 * mNHicPerStave[lay]);
      lane = (chipIdLocal % (14 * mNHicPerStave[lay])) / (14 / 2);

      int npix = -1;
      int colspan = -1;
      int rowspan = -1;
      int isGrouped = -1;

      o2::math_utils::Point3D<float> locC; // local coordinates

      if (ClusterID != o2::itsmft::CompCluster::InvalidPatternID) { // Normal (frequent) cluster shapes
        if (!mDict->isGroup(ClusterID)) {
          npix = mDict->getNpixels(ClusterID);

          // TODO: is there way other than calling the pattern?
          colspan = mDict->getPattern(ClusterID).getColumnSpan();
          rowspan = mDict->getPattern(ClusterID).getRowSpan();

          if (mDoPublishDetailedSummary == 1) {
            locC = mDict->getClusterCoordinates(cluster);
          }
          isGrouped = 0;
        } else {
          o2::itsmft::ClusterPattern patt(pattIt);
          npix = patt.getNPixels();
          colspan = patt.getColumnSpan();
          rowspan = patt.getRowSpan();
          if (mDoPublishDetailedSummary == 1) {
            locC = mDict->getClusterCoordinates(cluster, patt, true);
          }
          isGrouped = 1;
        }

      } else { // invalid pattern
        o2::itsmft::ClusterPattern patt(pattIt);
        npix = patt.getNPixels();
        colspan = patt.getColumnSpan();
        rowspan = patt.getRowSpan();
        isGrouped = 0;
        if (mDoPublishDetailedSummary == 1) {
          locC = mDict->getClusterCoordinates(cluster, patt, false);
        }
      }

      if (npix > 2) {
        nClusters3pixLay[lay]++;
        nClusters3pix++;
        nDigits3pixLay[lay] += npix;
      }

      if (lay < NLayerIB) {
        nHitsFromClusters[ChipID] += npix;
      }

      if (colspan >= minColSpanLongCluster && rowspan <= maxRowSpanLongCluster) {
        // definition of long cluster
        if (lay < NLayerIB) {
          nLongClusters[ChipID]++;
        } else {
          nLongClustersStave[lay - NLayerIB][sta]++;
        }
      }

      if (lay < NLayerIB) {
        hAverageClusterOccupancySummaryIB[lay]->getNum()->Fill(chip, sta);
        hAverageClusterSizeSummaryIB[lay]->getNum()->Fill(chip, sta, (double)npix);
        hAverageClusterSizeSummaryIB[lay]->getDen()->Fill(chip, sta, 1.);
        hClusterCenterMap[lay]->Fill(cluster.getCol(), cluster.getRow());
        if (mDoPublish1DSummary == 1) {
          hClusterTopologySummaryIB[lay][sta][chip]->Fill(ClusterID);
        }

        hClusterSizeLayerSummary[lay]->Fill(npix);
        hClusterTopologyLayerSummary[lay]->Fill(ClusterID);

        if (mDoPublish1DSummary) {
          hClusterSizeSummaryIB[lay][sta][chip]->Fill(npix);
        }

        if (isGrouped) {
          if (mDoPublish1DSummary == 1) {
            hGroupedClusterSizeSummaryIB[lay][sta][chip]->Fill(npix);
          }
          hGroupedClusterSizeLayerSummary[lay]->Fill(npix);
        }
      } else {
        hAverageClusterOccupancySummaryOB[lay]->getNum()->Fill(lane, sta, 1. / (mNChipsPerHic[lay] / mNLanePerHic[lay])); // 14 To have occupation per chip -> 7 because we're considering lanes
        hAverageClusterSizeSummaryOB[lay]->getNum()->Fill(lane, sta, (double)npix);
        hAverageClusterSizeSummaryOB[lay]->getDen()->Fill(lane, sta, 1);
        if (mDoPublish1DSummary == 1) {
          hClusterTopologySummaryOB[lay][sta]->Fill(ClusterID);
          hClusterSizeSummaryOB[lay][sta]->Fill(npix);
        }
        hClusterSizeLayerSummary[lay]->Fill(npix);
        hClusterTopologyLayerSummary[lay]->Fill(ClusterID);
        if (isGrouped) {
          if (mDoPublish1DSummary == 1) {
            hGroupedClusterSizeSummaryOB[lay][sta]->Fill(npix);
          }
          hGroupedClusterSizeLayerSummary[lay]->Fill(npix);
        }
      }

      // Transformation to the local --> global
      if (mDoPublishDetailedSummary == 1) {
        auto gloC = mGeom->getMatrixL2G(ChipID) * locC;
        float phi = (float)TMath::ATan2(gloC.Y(), gloC.X());

        phi = (float)(phi * 180 / TMath::Pi());
        hAverageClusterOccupancySummaryZPhi[lay]->getNum()->Fill(gloC.Z(), phi);
        hAverageClusterSizeSummaryZPhi[lay]->getNum()->Fill(gloC.Z(), phi, (float)npix);

        hAverageClusterOccupancySummaryFine[lay]->getNum()->Fill(getHorizontalBin(locC.Z(), chip, lay, lane), getVerticalBin(locC.X(), sta, lay));
        hAverageClusterSizeSummaryFine[lay]->getNum()->Fill(getHorizontalBin(locC.Z(), chip, lay, lane), getVerticalBin(locC.X(), sta, lay), (float)npix);
      }
    }
    hClusterVsBunchCrossing->Fill(bcdata.bc, nClusters3pix); // we count only the number of clusters, not their sizes
    for (int lay = 0; lay < 7; lay++) {
      if (nClusters3pixLay[lay] > 0) {
        int nchips = mNStaves[lay] * mNHicPerStave[lay] * mNChipsPerHic[lay];
        hClusterOccupancyDistribution[lay]->Fill(1. * nClusters3pixLay[lay] / nchips, 1. * nDigits3pixLay[lay] / nchips);
      }
    }

    // filling these anomaly plots once per ROF, ignoring chips w/o long clusters -- IB
    for (int ichip = 0; ichip < ChipBoundary[NLayerIB]; ichip++) {

      int nLong = TMath::Min(nLongClusters[ichip], 40);
      if (nLong < 1) {
        continue;
      }
      int ilayer = -1;
      while (ichip >= ChipBoundary[ilayer + 1]) {
        ilayer++;
      }
      hLongClustersPerChip[ilayer]->Fill(ichip, nLong);
      hMultPerChipWhenLongClusters[ilayer]->Fill(ichip, nHitsFromClusters[ichip]);
    }
    // filling anomaly plots once per ROF, ignoring staves w/o long clusters -- OB
    for (int ilay = 3; ilay < 7; ilay++) {
      for (int ist = 0; ist < mNStaves[ilay]; ist++) {

        int nLong = TMath::Min(nLongClustersStave[ilay - NLayerIB][ist], 40);
        if (nLong < 1) {
          continue;
        }
        hLongClustersPerStave[ilay - NLayerIB]->Fill(ist, nLong);
      }
    }
  }

  if ((int)clusRofArr.size() > 0) {

    mGeneralOccupancy->getDen()->Fill(0., 0., (double)(clusRofArr.size()));

    for (int iLayer = 0; iLayer < NLayer; iLayer++) {

      if (!mEnableLayers[iLayer]) {
        continue;
      }

      if (iLayer < NLayerIB) {
        hAverageClusterOccupancySummaryIB[iLayer]->getDen()->Fill(0., 0., (double)(clusRofArr.size()));
      } else {
        hAverageClusterOccupancySummaryOB[iLayer]->getDen()->Fill(0., 0., (double)(clusRofArr.size()));
      }

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

        if (iLayer < NLayerIB) {
          float max = -1.;
          for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
            // find the chip with the max occupancy stave by stave
            if (hAverageClusterOccupancySummaryIB[iLayer]->getNum()->GetBinContent(iChip + 1, iStave + 1) > max) {
              max = hAverageClusterOccupancySummaryIB[iLayer]->getNum()->GetBinContent(iChip + 1, iStave + 1);
            }
            if (hAverageClusterOccupancySummaryIB[iLayer]->getNum()->GetBinContent(iChip + 1, iStave + 1) < 1e-10) {
              hEmptyLaneFractionGlobal->Fill(0., 1. / mNLanes[0]);
              hEmptyLaneFractionGlobal->Fill(3., 1. / mNLanes[3]);
            }
          }
          int ybin = iStave < (mNStaves[iLayer] / 2) ? 7 + iLayer + 1 : 7 - iLayer;
          int xbin = 12 - mNStaves[iLayer] / 4 + 1 + (iStave % (mNStaves[iLayer] / 2));
          mGeneralOccupancy->getNum()->SetBinContent(xbin, ybin, max);
        } else {
          float max = -1.;
          for (int iLane = 0; iLane < mNLanePerHic[iLayer] * mNHicPerStave[iLayer]; iLane++) {
            if (hAverageClusterOccupancySummaryOB[iLayer]->getNum()->GetBinContent(iLane + 1, iStave + 1) > max) {
              max = hAverageClusterOccupancySummaryOB[iLayer]->getNum()->GetBinContent(iLane + 1, iStave + 1);
            }
            if (hAverageClusterOccupancySummaryOB[iLayer]->getNum()->GetBinContent(iLane + 1, iStave + 1) < 1e-10) {
              hEmptyLaneFractionGlobal->Fill((iLayer < 5 ? 1. : 2.), (1. / mNLanes[iLayer < 5 ? 1 : 2]));
              hEmptyLaneFractionGlobal->Fill(3., (1. / mNLanes[3]));
            }
          }
          int ybin = iStave < (mNStaves[iLayer] / 2) ? 7 + iLayer + 1 : 7 - iLayer;
          int xbin = 12 - mNStaves[iLayer] / 4 + 1 + (iStave % (mNStaves[iLayer] / 2));
          mGeneralOccupancy->getNum()->SetBinContent(xbin, ybin, max);
        }
      }

      if (mDoPublishDetailedSummary == 1) {

        for (int ix = 1; ix <= hAverageClusterSizeSummaryZPhi[iLayer]->GetNbinsX(); ix++)
          for (int iy = 1; iy <= hAverageClusterSizeSummaryZPhi[iLayer]->GetNbinsY(); iy++) {
            hAverageClusterSizeSummaryZPhi[iLayer]->getDen()->SetBinContent(ix, iy, hAverageClusterOccupancySummaryZPhi[iLayer]->getNum()->GetBinContent(ix, iy));
          }
        hAverageClusterOccupancySummaryZPhi[iLayer]->getDen()->Fill(0., 0., (double)(clusRofArr.size()));

        for (int ix = 1; ix <= hAverageClusterSizeSummaryFine[iLayer]->GetNbinsX(); ix++)
          for (int iy = 1; iy <= hAverageClusterSizeSummaryFine[iLayer]->GetNbinsY(); iy++) {
            hAverageClusterSizeSummaryFine[iLayer]->getDen()->SetBinContent(ix, iy, hAverageClusterOccupancySummaryFine[iLayer]->getNum()->GetBinContent(ix, iy));
          }
        hAverageClusterOccupancySummaryFine[iLayer]->getDen()->Fill(0., 0., (double)(clusRofArr.size()));
      }
    }
  }

  hTFCounter->Fill(0);

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Debug, Devel) << "Time in QC Cluster Task:  " << difference << ENDM;
}

void ITSClusterTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (mDoPublishDetailedSummary == 1) {
      hAverageClusterSizeSummaryZPhi[iLayer]->update();
      hAverageClusterOccupancySummaryZPhi[iLayer]->update();
      hAverageClusterOccupancySummaryFine[iLayer]->update();
      hAverageClusterSizeSummaryFine[iLayer]->update();
    }
    if (iLayer < NLayerIB) {
      hAverageClusterOccupancySummaryIB[iLayer]->update();
      hAverageClusterSizeSummaryIB[iLayer]->update();
    } else {
      hAverageClusterOccupancySummaryOB[iLayer]->update();
      hAverageClusterSizeSummaryOB[iLayer]->update();
    }
  }
  mGeneralOccupancy->update();
}

void ITSClusterTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void ITSClusterTask::reset()
{
  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  hTFCounter->Reset();
  hClusterVsBunchCrossing->Reset();
  hEmptyLaneFractionGlobal->Reset("ICES");
  mGeneralOccupancy->Reset();
  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    hClusterOccupancyDistribution[iLayer]->Reset();
    hClusterSizeLayerSummary[iLayer]->Reset();
    hGroupedClusterSizeLayerSummary[iLayer]->Reset();
    hClusterTopologyLayerSummary[iLayer]->Reset();

    if (iLayer < NLayerIB) {
      hClusterCenterMap[iLayer]->Reset();
      hLongClustersPerChip[iLayer]->Reset();
      hMultPerChipWhenLongClusters[iLayer]->Reset();
      hAverageClusterOccupancySummaryIB[iLayer]->Reset();
      hAverageClusterSizeSummaryIB[iLayer]->Reset();
      if (mDoPublish1DSummary == 1) {
        for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
          for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
            hClusterTopologySummaryIB[iLayer][iStave][iChip]->Reset();
            hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]->Reset();
            hClusterSizeSummaryIB[iLayer][iStave][iChip]->Reset();
          }
        }
      }
    } else {
      hLongClustersPerStave[iLayer - NLayerIB]->Reset();
      hAverageClusterOccupancySummaryOB[iLayer]->Reset();
      hAverageClusterSizeSummaryOB[iLayer]->Reset();
      if (mDoPublish1DSummary == 1) {
        for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
          hClusterTopologySummaryOB[iLayer][iStave]->Reset();
          hGroupedClusterSizeSummaryOB[iLayer][iStave]->Reset();
          hClusterSizeSummaryOB[iLayer][iStave]->Reset();
        }
      }
    }
    if (mDoPublishDetailedSummary == 1) {
      hAverageClusterOccupancySummaryZPhi[iLayer]->Reset();
      hAverageClusterSizeSummaryZPhi[iLayer]->Reset();
      hAverageClusterOccupancySummaryFine[iLayer]->Reset();
      hAverageClusterSizeSummaryFine[iLayer]->Reset();
    }
  }
}

void ITSClusterTask::createAllHistos()
{
  hTFCounter = new TH1D("TFcounter", "TFcounter", 1, 0, 1);
  hTFCounter->SetTitle("TF counter");
  addObject(hTFCounter);
  formatAxes(hTFCounter, "", "TF", 1, 1.10);

  hClusterVsBunchCrossing = new TH2D("BunchCrossingIDvsClusters", "BunchCrossingIDvsClusters", nBCbins, 0, 4095, 150, 0, 3000);
  hClusterVsBunchCrossing->SetTitle("#clusters vs BC id for clusters with npix > 2");
  addObject(hClusterVsBunchCrossing);
  formatAxes(hClusterVsBunchCrossing, "Bunch Crossing ID", "Number of clusters with npix > 2 in ROF", 1, 1.10);
  hClusterVsBunchCrossing->SetStats(0);

  hEmptyLaneFractionGlobal = new TH1D("EmptyLaneFractionGlobal", "Empty Lane Fraction Global", 4, 0, 4);
  hEmptyLaneFractionGlobal->SetTitle("Empty Lane /All Lane ");
  addObject(hEmptyLaneFractionGlobal);
  formatAxes(hEmptyLaneFractionGlobal, "", "Fraction of empty lane", 1, 1.10);
  for (int i = 0; i < NFlags; i++) {
    hEmptyLaneFractionGlobal->GetXaxis()->SetBinLabel(i + 1, mLaneStatusFlag[i].c_str());
  }
  hEmptyLaneFractionGlobal->GetXaxis()->CenterLabels();
  hEmptyLaneFractionGlobal->SetMaximum(1);
  hEmptyLaneFractionGlobal->SetMinimum(0);
  hEmptyLaneFractionGlobal->SetBit(TH1::kIsAverage);
  hEmptyLaneFractionGlobal->SetStats(0);

  for (int iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    if (iLayer < NLayerIB) {
      hClusterCenterMap[iLayer] = new TH2D(Form("ClusterCenterMapL%d", iLayer), Form("Stacked map of cluster centers for all L%d chips;Column;Row", iLayer), 1024, -0.5, 1023.5, 512, -0.5, 511.5);
      addObject(hClusterCenterMap[iLayer]);
      formatAxes(hClusterCenterMap[iLayer], "Column", "Row", 1, 1.10);
      hClusterCenterMap[iLayer]->SetStats(0);

      hLongClustersPerChip[iLayer] = new TH2D(Form("Anomalies/Layer%d/LongClusters", iLayer), Form("Layer%d/LongClusters", iLayer), ChipBoundary[iLayer + 1] - ChipBoundary[iLayer], ChipBoundary[iLayer], ChipBoundary[iLayer + 1], 41, 0, 41);
      hMultPerChipWhenLongClusters[iLayer] = new TH2D(Form("Anomalies/Layer%d/HitsWhenLongClusters", iLayer), Form("Layer%d/HitsWhenLongClusters", iLayer), ChipBoundary[iLayer + 1] - ChipBoundary[iLayer], ChipBoundary[iLayer], ChipBoundary[iLayer + 1], 250, 0, 40000);
      addObject(hLongClustersPerChip[iLayer]);
      formatAxes(hLongClustersPerChip[iLayer], "Chip ID", "number of long clusters", 1, 1.10);
      hLongClustersPerChip[iLayer]->SetStats(0);
      addObject(hMultPerChipWhenLongClusters[iLayer]);
      formatAxes(hMultPerChipWhenLongClusters[iLayer], "Chip ID", "Sum of clusters size (events w/ long clus)", 1, 1.10);
      hMultPerChipWhenLongClusters[iLayer]->SetStats(0);
    }

    else {

      hLongClustersPerStave[iLayer - NLayerIB] = new TH2D(Form("Anomalies/Layer%d/LongClusters", iLayer), Form("Layer%d/LongClusters", iLayer), mNStaves[iLayer], 0, mNStaves[iLayer], 41, 0, 41);
      addObject(hLongClustersPerStave[iLayer - NLayerIB]);
      formatAxes(hLongClustersPerStave[iLayer - NLayerIB], "Stave", "number of long clusters", 1, 1.10);
      hLongClustersPerStave[iLayer - NLayerIB]->SetStats(0);
    }

    hClusterSizeLayerSummary[iLayer] = new TH1L(Form("Layer%d/AverageClusterSizeSummary", iLayer), Form("Layer%dAverageClusterSizeSummary", iLayer), 128 * 128, 0, 128 * 128);
    hClusterSizeLayerSummary[iLayer]->SetTitle(Form("Cluster size summary for Layer %d", iLayer));
    addObject(hClusterSizeLayerSummary[iLayer]);
    formatAxes(hClusterSizeLayerSummary[iLayer], "Cluster Size (pixels)", "counts", 1, 1.10);
    hClusterSizeLayerSummary[iLayer]->SetStats(0);

    double dynbin[7][4] = { { 300, 100, 500, 1000 }, { 300, 100, 500, 1000 }, { 300, 100, 500, 1000 }, // IB
                            { 300, 5, 500, 50 },
                            { 300, 5, 500, 50 }, // ML
                            { 300, 1.5, 500, 15 },
                            { 300, 1.5, 500, 15 } }; // OL
    hClusterOccupancyDistribution[iLayer] = new TH2D(Form("Layer%d/OccupancyPerChipPerEvt", iLayer), Form("Layer%d/OccupancyPerChipPerEvt", iLayer), (int)dynbin[iLayer][0], 0, dynbin[iLayer][1], (int)dynbin[iLayer][2], 0, dynbin[iLayer][3]);
    hClusterOccupancyDistribution[iLayer]->SetTitle(Form("hits/chip/evt, form clusters with npix>2 - Layer%d", iLayer));
    addObject(hClusterOccupancyDistribution[iLayer]);
    formatAxes(hClusterOccupancyDistribution[iLayer], "N clus", "N hit", 1, 1.10);
    hClusterOccupancyDistribution[iLayer]->SetStats(0);

    hGroupedClusterSizeLayerSummary[iLayer] = new TH1L(Form("Layer%d/AverageGroupedClusterSizeSummary", iLayer), Form("Layer%dAverageGroupedClusterSizeSummary", iLayer), 128 * 128, 0, 128 * 128);
    hGroupedClusterSizeLayerSummary[iLayer]->SetTitle(Form("Cluster size summary for Layer %d", iLayer));
    addObject(hGroupedClusterSizeLayerSummary[iLayer]);
    formatAxes(hGroupedClusterSizeLayerSummary[iLayer], "Grouped Cluster Size (pixels)", "counts", 1, 1.10);
    hGroupedClusterSizeLayerSummary[iLayer]->SetStats(0);

    hClusterTopologyLayerSummary[iLayer] = new TH1L(Form("Layer%d/ClusterTopologySummary", iLayer), Form("Layer%dClusterTopologySummary", iLayer), 300, 0, 300);
    hClusterTopologyLayerSummary[iLayer]->SetTitle(Form("Cluster topology summary for Layer %d", iLayer));
    addObject(hClusterTopologyLayerSummary[iLayer]);
    formatAxes(hClusterTopologyLayerSummary[iLayer], "Cluster Topology (ID)", "counts", 1, 1.10);
    hClusterTopologyLayerSummary[iLayer]->SetStats(0);

    if (mDoPublishDetailedSummary == 1) {
      hAverageClusterOccupancySummaryZPhi[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterOccupancyZPhi", iLayer), Form("Cluster occupancy on Layer %d;z (cm);#phi (degree);", iLayer), 2 * (int)mLength[iLayer], -1 * mLength[iLayer], mLength[iLayer], 360, -180., 180., true);
      hAverageClusterOccupancySummaryZPhi[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryZPhi[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryZPhi[iLayer]->GetXaxis()->SetLabelSize(0.02);

      addObject(hAverageClusterOccupancySummaryZPhi[iLayer].get());

      hAverageClusterSizeSummaryZPhi[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterSizeZPhi", iLayer), Form("Cluster size on Layer %d;z (cm);#phi (degree);", iLayer), 2 * (int)mLength[iLayer], -1 * mLength[iLayer], mLength[iLayer], 360, -180., 180., false);
      hAverageClusterSizeSummaryZPhi[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryZPhi[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryZPhi[iLayer]->GetXaxis()->SetLabelSize(0.02);
      addObject(hAverageClusterSizeSummaryZPhi[iLayer].get());
    }

    if (iLayer < NLayerIB) {
      hAverageClusterOccupancySummaryIB[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer], true);
      hAverageClusterOccupancySummaryIB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hAverageClusterOccupancySummaryIB[iLayer].get());
      formatAxes(hAverageClusterOccupancySummaryIB[iLayer].get(), "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterOccupancySummaryIB[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryIB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryIB[iLayer]->GetXaxis()->SetLabelSize(0.02);

      hAverageClusterSizeSummaryIB[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer], false);
      hAverageClusterSizeSummaryIB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterSizeSummaryIB[iLayer].get());
      formatAxes(hAverageClusterSizeSummaryIB[iLayer].get(), "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterSizeSummaryIB[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryIB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryIB[iLayer]->GetXaxis()->SetLabelSize(0.02);
      // Fine check
      if (mDoPublishDetailedSummary == 1) {
        hAverageClusterOccupancySummaryFine[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterOccupancyFine", iLayer), Form("Cluster occupancy on Layer %d; Pixel group (column direction); Pixel group (row direction)", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB) - 0.5, mNStaves[iLayer] * nRphiBinsIB, -0.5, mNStaves[iLayer] * nRphiBinsIB - 0.5, true);
        hAverageClusterOccupancySummaryFine[iLayer]->SetStats(0);
        addObject(hAverageClusterOccupancySummaryFine[iLayer].get());

        hAverageClusterSizeSummaryFine[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterSizeFine", iLayer), Form("Cluster size on Layer %d; Pixel group (column direction); Pixel group (row direction)", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsIB) - 0.5, mNStaves[iLayer] * nRphiBinsIB, -0.5, mNStaves[iLayer] * nRphiBinsIB - 0.5, false);
        hAverageClusterSizeSummaryFine[iLayer]->SetStats(0);
        addObject(hAverageClusterSizeSummaryFine[iLayer].get());
      }

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        hAverageClusterSizeSummaryIB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterSizeSummaryIB[iLayer]->getDen()->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterSizeSummaryIB[iLayer]->getNum()->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterOccupancySummaryIB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));

        for (int iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

          if (mDoPublish1DSummary == 1) {
            hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSizeGrouped", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSizeGroped", iLayer, iStave, iChip), 100, 0, 100);
            hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size for grouped topologies on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
            addObject(hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip]);
            formatAxes(hGroupedClusterSizeSummaryIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

            hClusterSizeSummaryIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSize", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSize", iLayer, iStave, iChip), 100, 0, 100);
            hClusterSizeSummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size for Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
            addObject(hClusterSizeSummaryIB[iLayer][iStave][iChip]);
            formatAxes(hClusterSizeSummaryIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

            hClusterTopologySummaryIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterTopology", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterTopology", iLayer, iStave, iChip), 300, 0, 300);
            hClusterTopologySummaryIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Topology on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
            addObject(hClusterTopologySummaryIB[iLayer][iStave][iChip]);
            formatAxes(hClusterTopologySummaryIB[iLayer][iStave][iChip], "Cluster topology (ID)", "Counts", 1, 1.10);
          }

          hAverageClusterSizeSummaryIB[iLayer]->GetXaxis()->SetBinLabel(iChip + 1, Form("Chip %i", iChip));
          hAverageClusterSizeSummaryIB[iLayer]->getDen()->GetXaxis()->SetBinLabel(iChip + 1, Form("Chip %i", iChip));
          hAverageClusterSizeSummaryIB[iLayer]->getNum()->GetXaxis()->SetBinLabel(iChip + 1, Form("Chip %i", iChip));
          hAverageClusterOccupancySummaryIB[iLayer]->GetXaxis()->SetBinLabel(iChip + 1, Form("Chip %i", iChip));
        }
      }
    } else {
      hAverageClusterOccupancySummaryOB[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNLanePerHic[iLayer] * mNHicPerStave[iLayer], 0, mNLanePerHic[iLayer] * mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer], true);
      hAverageClusterOccupancySummaryOB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hAverageClusterOccupancySummaryOB[iLayer].get());
      formatAxes(hAverageClusterOccupancySummaryOB[iLayer].get(), "", "Stave Number", 1, 1.10);
      hAverageClusterOccupancySummaryOB[iLayer]->SetStats(0);
      hAverageClusterOccupancySummaryOB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryOB[iLayer]->GetXaxis()->SetLabelSize(0.02);
      hAverageClusterOccupancySummaryOB[iLayer]->GetXaxis()->SetTitleOffset(1.2);

      hAverageClusterSizeSummaryOB[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNLanePerHic[iLayer] * mNHicPerStave[iLayer], 0, mNLanePerHic[iLayer] * mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterSizeSummaryOB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterSizeSummaryOB[iLayer].get());
      formatAxes(hAverageClusterSizeSummaryOB[iLayer].get(), "", "Stave Number", 1, 1.10);
      hAverageClusterSizeSummaryOB[iLayer]->SetStats(0);
      hAverageClusterSizeSummaryOB[iLayer]->SetOption("colz");
      hAverageClusterSizeSummaryOB[iLayer]->GetYaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryOB[iLayer]->GetXaxis()->SetLabelSize(0.02);
      hAverageClusterSizeSummaryOB[iLayer]->GetXaxis()->SetTitleOffset(1.2);

      // Fine check
      if (mDoPublishDetailedSummary == 1) {
        hAverageClusterOccupancySummaryFine[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterOccupancyFine", iLayer), Form("Cluster occupancy on Layer %d; Pixel group (column direction); Pixel group (row direction)", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB) - 0.5, mNStaves[iLayer] * nRphiBinsOB, -0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5, true);
        hAverageClusterOccupancySummaryFine[iLayer]->SetStats(0);
        addObject(hAverageClusterOccupancySummaryFine[iLayer].get());

        hAverageClusterSizeSummaryFine[iLayer] = std::make_shared<TH2DRatio>(Form("Layer%d/ClusterSizeFine", iLayer), Form("Cluster size on Layer %d; Pixel group (column direction); Pixel group (row direction)", iLayer), mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB, -0.5, (mNChipsPerHic[iLayer] * mNHicPerStave[iLayer] * nZBinsOB) - 0.5, mNStaves[iLayer] * nRphiBinsOB, -0.5, mNStaves[iLayer] * nRphiBinsOB - 0.5, false);
        hAverageClusterSizeSummaryFine[iLayer]->SetStats(0);
        addObject(hAverageClusterSizeSummaryFine[iLayer].get());
      }

      for (int iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        if (mDoPublish1DSummary == 1) {
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
        }

        hAverageClusterSizeSummaryOB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterSizeSummaryOB[iLayer]->getDen()->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterSizeSummaryOB[iLayer]->getNum()->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        hAverageClusterOccupancySummaryOB[iLayer]->GetYaxis()->SetBinLabel(iStave + 1, Form("Stave %i", iStave));
        for (int iLane = 0; iLane < mNLanePerHic[iLayer] * mNHicPerStave[iLayer]; iLane++) { // are used in TH2 construction, no need to keep on ccdb
          (iLayer < 5) ? (xLabel = Form("%s", OBLabel34[iLane])) : (xLabel = Form("%s", OBLabel56[iLane]));
          hAverageClusterSizeSummaryOB[iLayer]->GetXaxis()->SetBinLabel(iLane + 1, xLabel);
          hAverageClusterSizeSummaryOB[iLayer]->getDen()->GetXaxis()->SetBinLabel(iLane + 1, xLabel);
          hAverageClusterSizeSummaryOB[iLayer]->getNum()->GetXaxis()->SetBinLabel(iLane + 1, xLabel);
          hAverageClusterOccupancySummaryOB[iLayer]->GetXaxis()->SetBinLabel(iLane + 1, xLabel);
        }
      }
    }
  }
}

void ITSClusterTask::getJsonParameters()
{
  mNThreads = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nThreads", mNThreads);
  nBCbins = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "nBCbins", nBCbins);
  mDoPublish1DSummary = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "publishSummary1D", mDoPublish1DSummary);
  std::string LayerConfig = o2::quality_control_modules::common::getFromConfig<std::string>(mCustomParameters, "layer", "0000000");
  mDoPublishDetailedSummary = o2::quality_control_modules::common::getFromConfig<int>(mCustomParameters, "publishDetailedSummary", mDoPublishDetailedSummary);

  for (int ilayer = 0; ilayer < NLayer; ilayer++) {
    if (LayerConfig[ilayer] != '0') {
      mEnableLayers[ilayer] = 1;
      ILOG(Debug, Devel) << "enable layer : " << ilayer << ENDM;
    } else {
      mEnableLayers[ilayer] = 0;
    }
  }
}

void ITSClusterTask::addObject(TObject* aObject)
{
  if (!aObject) {
    ILOG(Debug, Devel) << " ERROR: trying to add non-existent object " << ENDM;
    return;
  } else
    mPublishedObjects.push_back(aObject);
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

} // namespace o2::quality_control_modules::its
