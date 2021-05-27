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

#ifdef WITH_OPENMP
#include <omp.h>
#endif

using o2::itsmft::Digit;
using namespace o2::itsmft;
using namespace o2::its;

namespace o2::quality_control_modules::its
{

ITSClusterTask::ITSClusterTask() : TaskInterface()
{
  o2::base::GeometryManager::loadGeometry(mGeomPath.c_str());
  mGeom = o2::its::GeometryTGeo::Instance();
}

ITSClusterTask::~ITSClusterTask()
{

  for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

    if (!mEnableLayers[iLayer])
      continue;

    if (iLayer < 3) {

      delete hOccupancyIB[iLayer];
      delete hAverageClusterIB[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

          delete hClusterSizeIB[iLayer][iStave][iChip];
          delete hClusterTopologyIB[iLayer][iStave][iChip];
        }
      }
    } else {

      delete hOccupancyOB[iLayer];
      delete hAverageClusterOB[iLayer];

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {

          delete hClusterSizeOB[iLayer][iStave][iHic];
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
  createAllHistos();

  publishHistos();
  //std::string dictfile = o2::base::NameConf::getDictionaryFileName(o2::detectors::DetID::ITS, "", ".bin");
  std::ifstream file(mDictPath.c_str());

  if (file.good()) {
    LOG(INFO) << "Running with dictionary: " << mDictPath;
    mDict.readBinaryFile(mDictPath);
  } else {
    LOG(INFO) << "Running without dictionary !";
  }
}

void ITSClusterTask::startOfActivity(Activity& /*activity*/)
{
  QcInfoLogger::GetInstance() << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
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
        mClasterOccupancyIB[lay][sta][chip]++;
        if (ClusterID < dictSize) {
          hClusterTopologyIB[lay][sta][chip]->Fill(ClusterID);
          hClusterSizeIB[lay][sta][chip]->Fill(mDict.getNpixels(ClusterID));
        }
      } else {
        mClasterOccupancyOB[lay][sta][mod]++;
        if (ClusterID < dictSize) {
          hClusterSizeOB[lay][sta][mod]->Fill(mDict.getNpixels(ClusterID));
          hClusterTopologyOB[lay][sta][mod]->Fill(ClusterID);
        }
      }
    }
  }

  mNRofs += clusRofArr.size();
  if (mNRofs >= mOccUpdateFrequency) {
    for (Int_t iLayer = 0; iLayer < NLayer; iLayer++) {

      if (!mEnableLayers[iLayer])
        continue;
      if (iLayer < 3) {
        for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
          for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {

            hOccupancyIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, 1. * mClasterOccupancyIB[iLayer][iStave][iChip] / mNRofs);
            mClasterOccupancyIB[iLayer][iStave][iChip] = 0;
            hAverageClusterIB[iLayer]->SetBinContent(iChip + 1, iStave + 1, hClusterSizeIB[iLayer][iStave][iChip]->GetMean());
          }
        }
      } else {
        for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
          for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
            hOccupancyOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, 1. * mClasterOccupancyOB[iLayer][iStave][iHic] / mNRofs);
            mClasterOccupancyOB[iLayer][iStave][iHic] = 0;
            hAverageClusterOB[iLayer]->SetBinContent(iHic + 1, iStave + 1, hClusterSizeOB[iLayer][iStave][iHic]->GetMean());
          }
        }
      }
    }
    mNRofs = 0;
  }
  end = std::chrono::high_resolution_clock::now();
  difference = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  ILOG(Info) << "Time in QC Cluster Task:  " << difference << ENDM;
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
    if (iLayer < 3) {
      hOccupancyIB[iLayer]->Reset();
      hAverageClusterIB[iLayer]->Reset();
      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterSizeIB[iLayer][iStave][iChip]->Reset();
          hClusterTopologyIB[iLayer][iStave][iChip]->Reset();
        }
      }
    } else {
      hOccupancyOB[iLayer]->Reset();
      hAverageClusterOB[iLayer]->Reset();
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
    if (iLayer < 3) {
      hOccupancyIB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyIB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hOccupancyIB[iLayer]);
      formatAxes(hOccupancyIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hOccupancyIB[iLayer]->SetStats(0);
      hAverageClusterIB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNChipsPerHic[iLayer], 0, mNChipsPerHic[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);

      hAverageClusterIB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterIB[iLayer]);
      formatAxes(hAverageClusterIB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterIB[iLayer]->SetStats(0);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iChip = 0; iChip < mNChipsPerHic[iLayer]; iChip++) {
          hClusterSizeIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterSize", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterSize", iLayer, iStave, iChip), 50, 0, 50);
          hClusterSizeIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Size on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hClusterSizeIB[iLayer][iStave][iChip]);
          formatAxes(hClusterSizeIB[iLayer][iStave][iChip], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hClusterTopologyIB[iLayer][iStave][iChip] = new TH1D(Form("Layer%d/Stave%d/CHIP%d/ClusterTopology", iLayer, iStave, iChip), Form("Layer%dStave%dCHIP%dClusterTopology", iLayer, iStave, iChip), 300, 0, 300);
          hClusterTopologyIB[iLayer][iStave][iChip]->SetTitle(Form("Cluster Toplogy on Layer %d Stave %d Chip %d", iLayer, iStave, iChip));
          addObject(hClusterTopologyIB[iLayer][iStave][iChip]);
          formatAxes(hClusterTopologyIB[iLayer][iStave][iChip], "Cluster toplogy (ID)", "Counts", 1, 1.10);
        }
      }
    } else {
      hOccupancyOB[iLayer] = new TH2D(Form("Layer%d/ClusterOccupation", iLayer), Form("Layer%dClusterOccupancy", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hOccupancyOB[iLayer]->SetTitle(Form("Cluster Occupancy on Layer %d", iLayer));
      addObject(hOccupancyOB[iLayer]);
      formatAxes(hOccupancyOB[iLayer], "HIC Number", "Stave Number", 1, 1.10);
      hOccupancyOB[iLayer]->SetStats(0);

      hAverageClusterOB[iLayer] = new TH2D(Form("Layer%d/AverageClusterSize", iLayer), Form("Layer%dAverageClusterSize", iLayer), mNHicPerStave[iLayer], 0, mNHicPerStave[iLayer], mNStaves[iLayer], 0, mNStaves[iLayer]);
      hAverageClusterOB[iLayer]->SetTitle(Form("Average Cluster Size  on Layer %d", iLayer));
      addObject(hAverageClusterOB[iLayer]);
      formatAxes(hAverageClusterOB[iLayer], "Chip Number", "Stave Number", 1, 1.10);
      hAverageClusterOB[iLayer]->SetStats(0);

      for (Int_t iStave = 0; iStave < mNStaves[iLayer]; iStave++) {
        for (Int_t iHic = 0; iHic < mNHicPerStave[iLayer]; iHic++) {
          hClusterSizeOB[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterSize", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterSize", iLayer, iStave, iHic), 50, 0, 50);
          hClusterSizeOB[iLayer][iStave][iHic]->SetTitle(Form("Cluster Size on Layer %d Stave %d HIC %d", iLayer, iStave, iHic));
          addObject(hClusterSizeOB[iLayer][iStave][iHic]);
          formatAxes(hClusterSizeOB[iLayer][iStave][iHic], "Cluster size (Pixel)", "Counts", 1, 1.10);

          hClusterTopologyOB[iLayer][iStave][iHic] = new TH1D(Form("Layer%d/Stave%d/HIC%d/ClusterTopology", iLayer, iStave, iHic), Form("Layer%dStave%dHIC%dClusterTopology", iLayer, iStave, iHic), 300, 0, 300);
          hClusterTopologyOB[iLayer][iStave][iHic]->SetTitle(Form("Cluster Toplogy on Layer %d Stave %d HIC %d", iLayer, iStave, iHic));
          addObject(hClusterTopologyOB[iLayer][iStave][iHic]);
          formatAxes(hClusterTopologyOB[iLayer][iStave][iHic], "Cluster toplogy (ID)", "Counts", 1, 1.10);
        }
      }
    }
  }
}

void ITSClusterTask::getJsonParameters()
{
  mDictPath = mCustomParameters["clusterDictionaryPath"];
  mRunNumberPath = mCustomParameters["runNumberPath"];
  mGeomPath = mCustomParameters["geomPath"];
  mNThreads = stoi(mCustomParameters.find("nThreads")->second);
  LOG(INFO) << "#################### mNThreads : " << mNThreads;
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
    LOG(INFO) << " Object will be published: " << mPublishedObjects.at(iObj)->GetName();
  }
}

} // namespace o2::quality_control_modules::its
