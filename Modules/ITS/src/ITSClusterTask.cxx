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
#include <DetectorsCommonDataFormats/NameConf.h>
#include <DataFormatsITSMFT/ROFRecord.h>
#include <ITSMFTReconstruction/ChipMappingITS.h>
#include <DataFormatsITSMFT/ClusterTopology.h>
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

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    //delete sClustersSize[iLayer];
    delete hAverageClusterSummary[iLayer];

    if (iLayer < 3) {

      delete hOccupancyIB[iLayer];
      delete hOccupancyIBmonitor[iLayer];
      delete hAverageClusterIB[iLayer];
      delete hAverageClusterIBmonitor[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

          delete hClusterSizeIB[iLayer][iStave][iChip];
          delete hClusterSizeIBmonitor[iLayer][iStave][iChip];
          delete hClusterTopologyIB[iLayer][iStave][iChip];
        }
      }
    } else {

      delete hOccupancyOB[iLayer];
      delete hOccupancyOBmonitor[iLayer];
      delete hAverageClusterOB[iLayer];
      delete hAverageClusterOBmonitor[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {

          delete hClusterSizeOB[iLayer][iStave][iHic];
          delete hClusterSizeOBmonitor[iLayer][iStave][iHic];
          delete hClusterTopologyOB[iLayer][iStave][iHic];
        }
      }
    }
  }
}

void ITSClusterTask::initialize(o2::framework::InitContext& /*ctx*/)
{

  QcInfoLogger::GetInstance() << "initialize ITSClusterTask" << AliceO2::InfoLogger::InfoLogger::endm;

  getJsonParameters();

  o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  mGeom = o2::its::GeometryTGeo::Instance();

  createAllHistos();

  mGeneralOccupancy = new TH2Poly();
  mGeneralOccupancy->SetTitle("General Occupancy;mm;mm");
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

  publishHistos();
  //std::string dictfile = o2::base::NameConf::getDictionaryFileName(o2::detectors::DetID::ITS, "", ".bin");
  std::ifstream file(mDictPath.c_str());

  if (file.good()) {
    mDict.readBinaryFile(mDictPath);
    LOG(INFO) << "Running with dictionary: " << mDictPath << " with size: " << mDict.getSize();

  } else {
    LOG(INFO) << "Running without dictionary !";
  }
}

void ITSClusterTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
  reset();
}

void ITSClusterTask::startOfCycle()
{
  QcInfoLogger::GetInstance() << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSClusterTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  std::chrono::time_point<std::chrono::high_resolution_clock> start;
  std::chrono::time_point<std::chrono::high_resolution_clock> end;
  int difference;
  start = std::chrono::high_resolution_clock::now();

  QcInfoLogger::GetInstance() << "START DOING QC General" << AliceO2::InfoLogger::InfoLogger::endm;
  auto clusArr = ctx.inputs().get<gsl::span<o2::itsmft::CompClusterExt>>("compclus");
  auto clusRofArr = ctx.inputs().get<gsl::span<o2::itsmft::ROFRecord>>("clustersrof");

  int dictSize = mDict.getSize();

#ifdef WITH_OPENMP
  omp_set_num_threads(mNThreads);
#pragma omp parallel for schedule(dynamic)
#endif
  //Filling cluster histogram for each ROF by open_mp

  for (unsigned int iROF = 0; iROF < clusRofArr.size(); iROF++) {
    const auto& ROF = clusRofArr[iROF];
    for (int icl = ROF.getFirstEntry(); icl < ROF.getFirstEntry() + ROF.getNEntries(); icl++) {
      auto& cluster = clusArr[icl];

      auto ChipID = cluster.getSensorID();
      int ClusterID = cluster.getPatternID();
      int lay, sta, ssta, mod, chip;

      mGeom->getChipId(ChipID, lay, sta, ssta, mod, chip);
      mod = mod + (ssta * (mNHicPerStave[lay] / 2));

      if (lay < 3) {

        mClusterOccupancyIB[lay][sta][chip]++;
        mClusterOccupancyIBmonitor[lay][sta][chip]++;
        if (ClusterID < dictSize) {

          //Double_t ClusterSizeFill[3] = {1.*sta, 1.*chip,1.* mDict.getNpixels(ClusterID)};
          //sClustersSize[lay]->Fill(ClusterSizeFill, 1.);

          hClusterTopologyIB[lay][sta][chip]->Fill(ClusterID);
          hClusterSizeIB[lay][sta][chip]->Fill(mDict.getNpixels(ClusterID));
          hAverageClusterSummary[lay]->Fill(mDict.getNpixels(ClusterID));
          hClusterSizeIBmonitor[lay][sta][chip]->Fill(mDict.getNpixels(ClusterID));
        }
      } else {

        mClusterOccupancyOB[lay][sta][mod]++;
        mClusterOccupancyOBmonitor[lay][sta][mod]++;
        if (ClusterID < dictSize) {
          //Double_t ClusterSizeFill[3] = {1.*sta, 1.*mod, 1.*mDict.getNpixels(ClusterID)};
          //sClustersSize[lay]->Fill(ClusterSizeFill, 1.);

          hClusterSizeOB[lay][sta][mod]->Fill(mDict.getNpixels(ClusterID));
          hClusterTopologyOB[lay][sta][mod]->Fill(ClusterID);
          hClusterSizeOBmonitor[lay][sta][mod]->Fill(mDict.getNpixels(ClusterID));
          hAverageClusterSummary[lay]->Fill(mDict.getNpixels(ClusterID));
        }
      }
    }
  }

  mNRofs += clusRofArr.size();        //USED to calculate occupancy for the whole run
  mNRofsMonitor += clusRofArr.size(); // Occupancy in the last N ROFs

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {

      if (iLayer < 3) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hOccupancyIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, 1. * mClusterOccupancyIB[iLayer][iStave][iChip] / mNRofs);
          hAverageClusterIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, hClusterSizeIB[iLayer][iStave][iChip]->GetMean());
        }
        mGeneralOccupancy->SetBinContent(iStave + 1 + StaveBoundary[iLayer], *(std::max_element(mClusterOccupancyIB[iLayer][iStave], mClusterOccupancyIB[iLayer][iStave] + mNChipsPerHic[iLayer])) / mNRofs);
      } else {

        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hOccupancyOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, 1. * mClusterOccupancyOB[iLayer][iStave][iHic] / mNRofs / 14); //14 To have occupation per chip and HIC has 14 chips
          hAverageClusterOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, hClusterSizeOB[iLayer][iStave][iHic]->GetMean());
        }

        mGeneralOccupancy->SetBinContent(iStave + 1 + StaveBoundary[iLayer], *(std::max_element(mClusterOccupancyOB[iLayer][iStave], mClusterOccupancyOBmonitor[iLayer][iStave] + mNHicPerStave[iLayer])) / mNRofs / 14);
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
            hClusterSizeIBmonitor[iLayer][iStave][iChip]->Reset();

        else
          for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++)
            hClusterSizeOBmonitor[iLayer][iStave][iHic]->Reset();
      }
    }
  }

  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "Time in QC Cluster Task:  " << difference << ENDM;
}

void ITSClusterTask::updateOccMonitorPlots()
{

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++)
      if (iLayer < 3) {

        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hOccupancyIBmonitor[iLayer]->SetBinContent(iChip + 1, iStave + 1, 1. * mClusterOccupancyIBmonitor[iLayer][iStave][iChip] / mNRofsMonitor);
          hAverageClusterIBmonitor[iLayer]->SetBinContent(iChip + 1, iStave + 1, hClusterSizeIBmonitor[iLayer][iStave][iChip]->GetMean());
        }
      } else {

        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hOccupancyOBmonitor[iLayer]->SetBinContent(iHic + 1, iStave + 1, 1. * mClusterOccupancyOBmonitor[iLayer][iStave][iHic] / mNRofsMonitor / 14); //14 To have occupation per chip and HIC has 14 chips
          hAverageClusterOBmonitor[iLayer]->SetBinContent(iHic + 1, iStave + 1, hClusterSizeOBmonitor[iLayer][iStave][iHic]->GetMean());
        }
      }
  }
}

void ITSClusterTask::endOfCycle()
{

  std::ifstream runNumberFile(mRunNumberPath.c_str()); //catching ITS run number in commissioning; to be redesinged for the final version
  if (runNumberFile) {
    std::string runNumber;
    runNumberFile >> runNumber;
    if (runNumber != mRunNumber) {
      for (unsigned int iObj = 0; iObj < mPublishedObjects.size(); iObj++)
        getObjectsManager()->addMetadata(mPublishedObjects.at(iObj)->GetName(), "Run", runNumber);
      mRunNumber = runNumber;
    }
    QcInfoLogger::GetInstance() << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
  }
}

void ITSClusterTask::endOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

void ITSClusterTask::reset()
{
  QcInfoLogger::GetInstance() << "Resetting the histogram" << AliceO2::InfoLogger::InfoLogger::endm;

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;
    // sClustersSize[iLayer]->Reset();
    hAverageClusterSummary[iLayer]->Reset();

    if (iLayer < 3) {
      hOccupancyIB[iLayer]->Reset();
      hOccupancyIBmonitor[iLayer]->Reset();
      hAverageClusterIB[iLayer]->Reset();
      hAverageClusterIBmonitor[iLayer]->Reset();
      //hAverageClusterIBsummary[iLayer]->Reset();
      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterSizeIB[iLayer][iStave][iChip]->Reset();
          hClusterTopologyIB[iLayer][iStave][iChip]->Reset();
        }
      }
    } else {
      hOccupancyOB[iLayer]->Reset();
      hAverageClusterOB[iLayer]->Reset();
      hOccupancyOBmonitor[iLayer]->Reset();
      hAverageClusterOBmonitor[iLayer]->Reset();
      //hAverageClusterOBsummary[iLayer]->Reset();
      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hClusterSizeOB[iLayer][iStave][iHic]->Reset();
          hClusterTopologyOB[iLayer][iStave][iHic]->Reset();
        }
      }
    }
  }
}

void ITSClusterTask::createAllHistos()
{

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {
    if (!mEnableLayers[iLayer])
      continue;

    hAverageClusterSummary[iLayer] = new TH1D(Form("Layer%d/AverageClusterSizeSummary", iLayer), Form("Layer%dAverageClusterSizeSummary", iLayer), 50, 0, 50);
    hAverageClusterSummary[iLayer]->SetTitle(Form("Summary of average Cluster on one Layer %d", iLayer));
    addObject(hAverageClusterSummary[iLayer]);
    formatAxes(hAverageClusterSummary[iLayer], "Cluster Size (pixels)", "counts", 1, 1.10);
    hAverageClusterSummary[iLayer]->SetStats(0);

    if (iLayer < 3) {
      /*
     Int_t bins[3]= {mNStaves[iLayer],mNChipsPerHic[iLayer],50};
     Double_t xmin[3] = {0., 0., 0.};
     Double_t xmax[3] = {1.*mNStaves[iLayer],1.* mNChipsPerHic[iLayer],50.};
     sClustersSize[iLayer] = new THnSparseD( Form("Layer%d/THnSparseClusterSize", iLayer), Form("Layer%d/THnSparseClusterSize", iLayer),3, bins, xmin, xmax);
     addObject(sClustersSize[iLayer]);
*/

      hOccupancyIB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyIB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hOccupancyIB[iLayer]);
      formatAxes(hOccupancyIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hOccupancyIB[iLayer]->SetStats(0);
      hOccupancyIB[iLayer]->SetBit(TH1::kIsAverage);

      hOccupancyIBmonitor[iLayer] = new TH2D(Form("Layer%d/ClusterOccupationForLastNROFS", iLayer), Form("Layer%dClusterOccupancyForLastNROFS", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyIBmonitor[iLayer]->SetTitle(Form("Cluster Occupancy for the last NROFs on Layer %d", iLayer));
      addObject(hOccupancyIBmonitor[iLayer]);
      formatAxes(hOccupancyIBmonitor[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hOccupancyIBmonitor[iLayer]->SetStats(0);
      hOccupancyIBmonitor[iLayer]->SetBit(TH1::kIsAverage);


      hAverageClusterIB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);

      hAverageClusterIB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterIB[iLayer]);
      formatAxes(hAverageClusterIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterIB[iLayer]->SetStats(0);
      hAverageClusterIB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterIBmonitor[iLayer] = new TH2D(Form("Layer%d/AverageClusterSizeForLastNROFS", iLayer), Form("Layer%dAverageClusterSizeForLastNROFS", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterIBmonitor[iLayer]->SetTitle(Form("Average Cluster Size for the last NROFs on Layer %d", iLayer));
      addObject(hAverageClusterIBmonitor[iLayer]);
      formatAxes(hAverageClusterIBmonitor[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterIBmonitor[iLayer]->SetStats(0);
      hAverageClusterIBmonitor[iLayer]->SetBit(TH1::kIsAverage);


      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterSizeIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSize", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSize", iLayer, iStave, iChip), 50, 0, 50);
          hClusterSizeIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hClusterSizeIB[iLayer][iStave][iChip]);
          formatAxes(hClusterSizeIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hClusterSizeIBmonitor[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSizeMonitor", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSizeMonitor", iLayer, iStave, iChip), 50, 0, 50);

          hClusterTopologyIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterTopology", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterTopology", iLayer, iStave, iChip), 300, 0, 300);
          hClusterTopologyIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Toplogy on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hClusterTopologyIB[iLayer][iStave][iChip]);
          formatAxes(hClusterTopologyIB[iLayer][iStave][iChip], "Cluster toplogy (ID)", "Counts", 1, 1.10);
        }
      }
    } else {
      /* 
     Int_t bins[3]= {mNStaves[iLayer], mNHicPerStave[iLayer],50};
     Double_t xmin[3] = {0., 0., 0.};
     Double_t xmax[3] = {1.*mNStaves[iLayer], 1.*mNHicPerStave[iLayer],50.};
     sClustersSize[iLayer] = new THnSparseD( Form("Layer%d/THnSparseClusterSize", iLayer), Form("Layer%d/THnSparseClusterSize", iLayer),3, bins, xmin, xmax);
     addObject(sClustersSize[iLayer]);
*/

      hOccupancyOB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyOB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hOccupancyOB[iLayer]);
      formatAxes(hOccupancyOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hOccupancyOB[iLayer]->SetStats(0);
      hOccupancyOB[iLayer]->SetBit(TH1::kIsAverage);


      hOccupancyOBmonitor[iLayer] = new TH2D(Form("Layer%d/ClusterOccupationForLastNROFS", iLayer), Form("Layer%dClusterOccupancyForLastNROFS", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyOBmonitor[iLayer]->SetTitle(Form("Cluster Occupancy in last NROFS on Layer %d", iLayer));
      addObject(hOccupancyOBmonitor[iLayer]);
      formatAxes(hOccupancyOBmonitor[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hOccupancyOBmonitor[iLayer]->SetStats(0);
      hOccupancyOBmonitor[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterOB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterOB[iLayer]);
      formatAxes(hAverageClusterOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hAverageClusterOB[iLayer]->SetStats(0);
      hAverageClusterOB[iLayer]->SetBit(TH1::kIsAverage);

      hAverageClusterOBmonitor[iLayer] = new TH2D(Form("Layer%d/AverageClusterSizeForLastNROFS", iLayer), Form("Layer%dAverageClusterSizeForLastNROFS", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOBmonitor[iLayer]->SetTitle(Form("Average Cluster Size for the last NROFs on Layer %d", iLayer));
      addObject(hAverageClusterOBmonitor[iLayer]);
      formatAxes(hAverageClusterOBmonitor[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hAverageClusterOBmonitor[iLayer]->SetStats(0);
      hAverageClusterOBmonitor[iLayer]->SetBit(TH1::kIsAverage);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hClusterSizeOB[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterSize", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterSize", iLayer, iStave, iHic), 50, 0, 50);
          hClusterSizeOB[iLayer][iStave][iHic]->SetTitle(Form("Cluster Size on Layer %d Stave %d HIC %d", iLayer, iStave, iHic));
          addObject(hClusterSizeOB[iLayer][iStave][iHic]);
          formatAxes(hClusterSizeOB[iLayer][iStave][iHic], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hClusterSizeOBmonitor[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterSizeMonitor", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterSizeMonitor", iLayer, iStave, iHic), 50, 0, 50);

          hClusterTopologyOB[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterTopology", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterTopology", iLayer, iStave, iHic), 300, 0, 300);
          hClusterTopologyOB[iLayer][iStave][iHic]->SetTitle(Form("Cluster Toplogy on Layer %d Stave %d HIC %d", iLayer, iStave, iHic));
          addObject(hClusterTopologyOB[iLayer][iStave][iHic]);
          formatAxes(hClusterTopologyOB[iLayer][iStave][iHic], "Cluster toplogy (ID)", "Counts", 1, 1.10);
        }
      }
    }
  }
}

void ITSClusterTask::getStavePoint(int layer, int stave, double* px, double* py)
{

  float stepAngle = TMath::Pi() * 2 / mNStaves[layer];            //the angle between to stave
  float midAngle = StartAngle[layer] + (stave * stepAngle);       //mid point angle
  float staveRotateAngle = TMath::Pi() / 2 - (stave * stepAngle); //how many angle this stave rotate(compare with first stave)
  px[1] = MidPointRad[layer] * TMath::Cos(midAngle);              //there are 4 point to decide this TH2Poly bin
                                                                  //0:left point in this stave;
                                                                  //1:mid point in this stave;
                                                                  //2:right point in this stave;
                                                                  //3:higher point int this stave;
  py[1] = MidPointRad[layer] * TMath::Sin(midAngle);              //4 point calculated accord the blueprint
                                                                  //roughly calculate
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
  mDictPath = mCustomParameters["clusterDictionaryPath"];
  mRunNumberPath = mCustomParameters["runNumberPath"];
  mGeomPath = mCustomParameters["geomPath"];
  mNThreads = stoi(mCustomParameters.find("nThreads")->second);

  for (int ilayer = 0; ilayer < NLayer; ilayer++) {

    if (mCustomParameters["layer"][ilayer] != '0') {
      mEnableLayers[ilayer] = 1;
      LOG(INFO) << "enable layer : " << ilayer;
    } else {
      mEnableLayers[ilayer] = 0;
    }
  }
}

void ITSClusterTask::addObject(TObject* aObject)
{
  if (!aObject) {
    LOG(INFO) << " ERROR: trying to add non-existent object ";
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
    //LOG(INFO) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName();
  }
}

} // namespace o2::quality_control_modules::its
