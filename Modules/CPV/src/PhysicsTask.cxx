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
/// \file   PhysicsTask.cxx
/// \author Sergey Evdokimov
///

#include <TH1.h>
#include <TH2.h>

#include "QualityControl/QcInfoLogger.h"
#include "CPV/PhysicsTask.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>
#include <Framework/DataRefUtils.h>
#include "DataFormatsCPV/Digit.h"
#include "DataFormatsCPV/Cluster.h"
#include "DataFormatsCPV/TriggerRecord.h"
#include "CPVReconstruction/RawDecoder.h"

namespace o2::quality_control_modules::cpv
{

PhysicsTask::~PhysicsTask()
{
  for (int i = 0; i < kNHist1D; i++) {
    if (mHist1D[i]) {
      mHist1D[i]->Delete();
      mHist1D[i] = nullptr;
    }
  }
  for (int i = 0; i < kNHist2D; i++) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
}

void PhysicsTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize PhysicsTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }
  initHistograms();
  mNEventsTotal = 0;
}

void PhysicsTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity() : resetting everything" << activity.mId << ENDM;
  resetHistograms();
  mNEventsTotal = 0;
}

void PhysicsTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void PhysicsTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  // Some examples:

  // 1. In a loop
  int nInputs = ctx.inputs().size();
  mHist1D[H1DNInputs]->Fill(nInputs);

  int nValidInputs = ctx.inputs().countValidInputs();
  mHist1D[H1DNValidInputs]->Fill(nValidInputs);

  bool hasClusters = false, hasDigits = false, hasCalibDigits = false, hasRawErrors = false;
  for (auto&& input : o2::framework::InputRecordWalker(ctx.inputs())) {
    // get message header
    if (input.header != nullptr && input.payload != nullptr) {
      const auto* header = o2::framework::DataRefUtils::getHeader<header::DataHeader*>(input);
      auto payloadSize = o2::framework::DataRefUtils::getPayloadSize(input);
      // LOG(info) << "monitorData() : obtained input " << header->dataOrigin.str << "/" << header->dataDescription.str;
      if ((strcmp(header->dataOrigin.str, "CPV") == 0) && (strcmp(header->dataDescription.str, "DIGITS") == 0)) {
        // LOG(info) << "monitorData() : I found digits in inputs";
        hasDigits = true;
      }
      if ((strcmp(header->dataOrigin.str, "CPV") == 0) && (strcmp(header->dataDescription.str, "CLUSTERS") == 0)) {
        // LOG(info) << "monitorData() : I found clusters in inputs";
        hasClusters = true;
      }
      if ((strcmp(header->dataOrigin.str, "CPV") == 0) && (strcmp(header->dataDescription.str, "CALIBDIGITS") == 0)) {
        // LOG(info) << "monitorData() : I found clusters in inputs";
        hasCalibDigits = true;
      }
      if ((strcmp(header->dataOrigin.str, "CPV") == 0) && (strcmp(header->dataDescription.str, "RAWHWERRORS") == 0)) {
        // LOG(info) << "monitorData() : I found clusters in inputs";
        hasRawErrors = true;
      }

      // get payload of a specific input, which is a char array.
      // const char* payload = input.payload;

      // for the sake of an example, let's fill the histogram with payload sizes
      mHist1D[H1DInputPayloadSize]->Fill(payloadSize);
    }
  }

  // 2. Using get("<binding>")
  // digits
  if (hasDigits) {
    auto digits = ctx.inputs().get<gsl::span<o2::cpv::Digit>>("digits");
    auto digitsTR = ctx.inputs().get<gsl::span<o2::cpv::TriggerRecord>>("dtrigrec");
    mHist1D[H1DNDigitsPerInput]->Fill(digits.size());
    unsigned short nDigPerEvent[3];
    for (const auto& trigRecord : digitsTR) {
      LOG(debug) << " monitorData() : digit trigger record #" << mNEventsTotal
                 << " contains " << trigRecord.getNumberOfObjects() << " objects.";
      mNEventsTotal++;
      mHist1D[H1DDigitsInEventM2M3M4]->Fill(trigRecord.getNumberOfObjects());
      memset(nDigPerEvent, 0, 3 * sizeof(short));
      if (trigRecord.getNumberOfObjects() > 0) {
        for (int iDig = trigRecord.getFirstEntry(); iDig < trigRecord.getFirstEntry() + trigRecord.getNumberOfObjects(); iDig++) {
          mHist1D[H1DDigitIds]->Fill(digits[iDig].getAbsId());
          short relId[3];
          if (Geometry::absToRelNumbering(digits[iDig].getAbsId(), relId)) {
            // reminder: relId[3]={Module, phi col, z row} where Module=2..4, phi col=0..127, z row=0..59
            mHist2D[H2DDigitMapM2 + relId[0] - 2]->Fill(relId[1], relId[2]);
            mHist1D[H1DDigitEnergyM2 + relId[0] - 2]->Fill(digits[iDig].getAmplitude());
            nDigPerEvent[relId[0] - 2]++;
          }
        }
      }
    }
    for (int iMod = 0; iMod < kNModules; iMod++) {
      mHist1D[H1DDigitsInEventM2 + iMod]->Fill(nDigPerEvent[iMod]);
      for (int iZ = 1; iZ <= 60; iZ++) {
        for (int iX = 1; iX <= 128; iX++) {
          mHist2D[H2DDigitFreqM2 + iMod]->SetBinContent(iX, iZ, mHist2D[H2DDigitMapM2 + iMod]->GetBinContent(iX, iZ) / mNEventsTotal);
        }
      }
    }
  }

  // clusters
  if (hasClusters) {
    auto clusters = ctx.inputs().get<gsl::span<o2::cpv::Cluster>>("clusters");
    auto clustersTR = ctx.inputs().get<gsl::span<o2::cpv::TriggerRecord>>("ctrigrec");
    mHist1D[H1DNClustersPerInput]->Fill(clusters.size());
    unsigned short nCluPerEvent[3];
    for (const auto& trigRecord : clustersTR) {
      mHist1D[H1DClustersInEventM2M3M4]->Fill(trigRecord.getNumberOfObjects());
      memset(nCluPerEvent, 0, 3 * sizeof(short));
      if (trigRecord.getNumberOfObjects() > 0) {
        for (int iClu = trigRecord.getFirstEntry(); iClu < trigRecord.getFirstEntry() + trigRecord.getNumberOfObjects(); iClu++) {
          // reminder: relId[3]={Module, phi col, z row} where Module=2..4, phi col=0..127, z row=0..59
          char mod = clusters[iClu].getModule();
          nCluPerEvent[mod - 2]++;
          float x, z, totEn;
          clusters[iClu].getLocalPosition(x, z);
          totEn = clusters[iClu].getEnergy();
          int mult = clusters[iClu].getMultiplicity();
          mHist2D[H2DClusterMapM2 + mod - 2]->Fill(x, z);
          mHist1D[H1DClusterTotEnergyM2 + mod - 2]->Fill(totEn);
          mHist1D[H1DNDigitsInClusterM2 + mod - 2]->Fill(mult);
        }
      }
      for (unsigned char mod = 0; mod <= 2; mod++) {
        mHist1D[H1DClustersInEventM2 + mod]->Fill(nCluPerEvent[mod]);
      }
    }
  }

  // calib digits
  if (hasCalibDigits) {
    auto calibDigits = ctx.inputs().get<gsl::span<o2::cpv::Digit>>("calibdigits");
    mHist1D[H1DNCalibDigitsPerInput]->Fill(calibDigits.size());
    for (const auto& digit : calibDigits) {
      mHist1D[H1DCalibDigitIds]->Fill(digit.getAbsId());
      short relId[3];
      if (Geometry::absToRelNumbering(digit.getAbsId(), relId)) {
        // reminder: relId[3]={Module, phi col, z row} where Module=2..4, phi col=0..127, z row=0..59
        mHist2D[H2DCalibDigitMapM2 + relId[0] - 2]->Fill(relId[1], relId[2]);
        mHist1D[H1DCalibDigitEnergyM2 + relId[0] - 2]->Fill(digit.getAmplitude());
      }
    }
  }

  // HW Raw Errors
  if (hasRawErrors) {
    auto rawErrors = ctx.inputs().get<gsl::span<o2::cpv::RawDecoderError>>("rawerrors");
    for (const auto& rawError : rawErrors) {
      mHist1D[H1DRawErrors]->Fill(rawError.errortype);
    }
  }

  // 3. Access CCDB. If it is enough to retrieve it once, do it in initialize().
  // Remember to delete the object when the pointer goes out of scope or it is no longer needed.
  //   TObject* condition = TaskInterface::retrieveCondition("QcTask/example"); // put a valid condition path here
  //   if (condition) {
  //     LOG(info) << "Retrieved " << condition->ClassName();
  //     delete condition;
  //   }
}

void PhysicsTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void PhysicsTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void PhysicsTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  resetHistograms();
  mNEventsTotal = 0;
}

void PhysicsTask::initHistograms()
{
  ILOG(Info, Devel) << "initing histograms" << ENDM;
  // 1D Histos
  if (!mHist1D[H1DInputPayloadSize]) {
    mHist1D[H1DInputPayloadSize] =
      new TH1F("InputPayloadSize", "Input Payload Size", 30000, 0, 30000000);
    getObjectsManager()->startPublishing(mHist1D[H1DInputPayloadSize]);
  } else {
    mHist1D[H1DInputPayloadSize]->Reset();
  }

  if (!mHist1D[H1DNInputs]) {
    mHist1D[H1DNInputs] = new TH1F("NInputs", "Number of inputs", 10, -0.5, 9.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNInputs]);
  } else {
    mHist1D[H1DNInputs]->Reset();
  }

  if (!mHist1D[H1DNValidInputs]) {
    mHist1D[H1DNValidInputs] =
      new TH1F("NValidInputs", "Number of valid inputs", 10, -0.5, 9.5);
    getObjectsManager()->startPublishing(mHist1D[H1DNValidInputs]);
  } else {
    mHist1D[H1DNValidInputs]->Reset();
  }

  if (!mHist1D[H1DNDigitsPerInput]) {
    mHist1D[H1DNDigitsPerInput] =
      new TH1F("NDigitsPerInput", "Number of digits per input", 30000, 0, 300000);
    getObjectsManager()->startPublishing(mHist1D[H1DNDigitsPerInput]);
  } else {
    mHist1D[H1DNDigitsPerInput]->Reset();
  }

  if (!mHist1D[H1DNCalibDigitsPerInput]) {
    mHist1D[H1DNCalibDigitsPerInput] =
      new TH1F("NCalibDigitsPerInput", "Number of CalibDigits per input", 30000, 0, 300000);
    getObjectsManager()->startPublishing(mHist1D[H1DNCalibDigitsPerInput]);
  } else {
    mHist1D[H1DNCalibDigitsPerInput]->Reset();
  }

  if (!mHist1D[H1DNClustersPerInput]) {
    mHist1D[H1DNClustersPerInput] =
      new TH1F("NClustersPerInput", "Number of clusters per input", 30000, 0, 300000);
    getObjectsManager()->startPublishing(mHist1D[H1DNClustersPerInput]);
  } else {
    mHist1D[H1DNClustersPerInput]->Reset();
  }

  if (!mHist1D[H1DDigitIds]) {
    mHist1D[H1DDigitIds] = new TH1F("DigitIds", "Digit Ids", 30000, -0.5, 29999.5);
    getObjectsManager()->startPublishing(mHist1D[H1DDigitIds]);
  } else {
    mHist1D[H1DDigitIds]->Reset();
  }

  if (!mHist1D[H1DCalibDigitIds]) {
    mHist1D[H1DCalibDigitIds] = new TH1F("CalibDigitIds", "CalibDigit Ids", 30000, -0.5, 29999.5);
    getObjectsManager()->startPublishing(mHist1D[H1DCalibDigitIds]);
  } else {
    mHist1D[H1DCalibDigitIds]->Reset();
  }

  if (!mHist1D[H1DDigitsInEventM2M3M4]) {
    mHist1D[H1DDigitsInEventM2M3M4] =
      new TH1F("NDigitsInEventM2M3M4", "Number of Digits per event", 23040, 0., 23040.);
    getObjectsManager()->startPublishing(mHist1D[H1DDigitsInEventM2M3M4]);
  } else {
    mHist1D[H1DDigitsInEventM2M3M4]->Reset();
  }

  // if (!mHist1D[H1DCalibDigitsInEventM2M3M4]) {
  //   mHist1D[H1DCalibDigitsInEventM2M3M4] =
  //     new TH1F("NCalibDigitsInEventM2M3M4", "Number of CalibDigits per event", 23040, 0., 23040.);
  //   getObjectsManager()->startPublishing(mHist1D[H1DCalibDigitsInEventM2M3M4]);
  // } else {
  //   mHist1D[H1DCalibDigitsInEventM2M3M4]->Reset();
  // }

  if (!mHist1D[H1DClustersInEventM2M3M4]) {
    mHist1D[H1DClustersInEventM2M3M4] =
      new TH1F("NClustersInEventM2M3M4", "Number of clusters per event", 23040, 0., 23040.);
    getObjectsManager()->startPublishing(mHist1D[H1DClustersInEventM2M3M4]);
  } else {
    mHist1D[H1DClustersInEventM2M3M4]->Reset();
  }

  if (!mHist1D[H1DRawErrors]) {
    mHist1D[H1DRawErrors] =
      new TH1F("RawErrors", "Raw Errors", 20, 0., 20.);
    getObjectsManager()->startPublishing(mHist1D[H1DRawErrors]);
  } else {
    mHist1D[H1DRawErrors]->Reset();
  }

  int nPadsX = Geometry::kNumberOfCPVPadsPhi;
  int nPadsZ = Geometry::kNumberOfCPVPadsZ;
  float rangeX = Geometry::kCPVPadSizePhi / 2. * nPadsX + 10.;
  float rangeZ = Geometry::kCPVPadSizeZ / 2. * nPadsZ + 10.;

  for (int mod = 0; mod < kNModules; mod++) {
    // 1D
    // Digit energy
    if (!mHist1D[H1DDigitEnergyM2 + mod]) {
      mHist1D[H1DDigitEnergyM2 + mod] =
        new TH1F(
          Form("DigitEnergyM%d", mod + 2),
          Form("Digit energy distribution M%d", mod + 2),
          1000, 0, 1000.);
      mHist1D[H1DDigitEnergyM2 + mod]->GetXaxis()->SetTitle("Digit energy");
      getObjectsManager()->startPublishing(mHist1D[H1DDigitEnergyM2 + mod]);
    } else {
      mHist1D[H1DDigitEnergyM2 + mod]->Reset();
    }
    // calib digits
    if (!mHist1D[H1DCalibDigitEnergyM2 + mod]) {
      mHist1D[H1DCalibDigitEnergyM2 + mod] =
        new TH1F(
          Form("CalibDigitEnergyM%d", mod + 2),
          Form("CalibDigit energy distribution M%d", mod + 2),
          1000, 0, 1000.);
      mHist1D[H1DCalibDigitEnergyM2 + mod]->GetXaxis()->SetTitle("CalibDigit energy");
      getObjectsManager()->startPublishing(mHist1D[H1DCalibDigitEnergyM2 + mod]);
    } else {
      mHist1D[H1DCalibDigitEnergyM2 + mod]->Reset();
    }

    // N Digits per event
    if (!mHist1D[H1DDigitsInEventM2 + mod]) {
      mHist1D[H1DDigitsInEventM2 + mod] =
        new TH1F(
          Form("DigitsInEventM%d", mod + 2),
          Form("Digits per event in M%d", mod + 2),
          Geometry::kNCHANNELS / 3, 0., float(Geometry::kNCHANNELS / 3));
      mHist1D[H1DDigitsInEventM2 + mod]->GetXaxis()->SetTitle("Number of digits");
      getObjectsManager()->startPublishing(mHist1D[H1DDigitsInEventM2 + mod]);
    } else {
      mHist1D[H1DDigitsInEventM2 + mod]->Reset();
    }

    // N CalibDigits per event
    // if (!mHist1D[H1DCalibDigitsInEventM2 + mod]) {
    //  mHist1D[H1DCalibDigitsInEventM2 + mod] =
    //    new TH1F(
    //      Form("CalibDigitsInEventM%d", mod + 2),
    //      Form("CalibDigits per event in M%d", mod + 2),
    //      Geometry::kNCHANNELS / 3, 0., float(Geometry::kNCHANNELS / 3));
    //  mHist1D[H1DCalibDigitsInEventM2 + mod]->GetXaxis()->SetTitle("Number of CalibDigits");
    //  getObjectsManager()->startPublishing(mHist1D[H1DCalibDigitsInEventM2 + mod]);
    // } else {
    //  mHist1D[H1DCalibDigitsInEventM2 + mod]->Reset();
    // }

    // Total cluster energy
    if (!mHist1D[H1DClusterTotEnergyM2 + mod]) {
      mHist1D[H1DClusterTotEnergyM2 + mod] =
        new TH1F(
          Form("ClusterTotEnergyM%d", mod + 2),
          Form("Total cluster energy distribution M%d", mod + 2),
          2000, 0, 2000.);
      mHist1D[H1DClusterTotEnergyM2 + mod]->GetXaxis()->SetTitle("cluster energy");
      getObjectsManager()->startPublishing(mHist1D[H1DClusterTotEnergyM2 + mod]);
    } else {
      mHist1D[H1DClusterTotEnergyM2 + mod]->Reset();
    }

    // Number of digits in cluster
    if (!mHist1D[H1DNDigitsInClusterM2 + mod]) {
      mHist1D[H1DNDigitsInClusterM2 + mod] =
        new TH1F(
          Form("NDigitsInClusterM%d", mod + 2),
          Form("Multiplicity of digits in cluster M%d", mod + 2),
          50, 0, 50.);
      mHist1D[H1DNDigitsInClusterM2 + mod]->GetXaxis()->SetTitle("Number of digits");
      getObjectsManager()->startPublishing(mHist1D[H1DNDigitsInClusterM2 + mod]);
    } else {
      mHist1D[H1DNDigitsInClusterM2 + mod]->Reset();
    }

    // N clusters per event
    if (!mHist1D[H1DClustersInEventM2 + mod]) {
      mHist1D[H1DClustersInEventM2 + mod] =
        new TH1F(
          Form("ClustersInEventM%d", mod + 2),
          Form("Clusters per event in M%d", mod + 2),
          Geometry::kNCHANNELS / 3, 0., float(Geometry::kNCHANNELS / 3));
      mHist1D[H1DClustersInEventM2 + mod]->GetXaxis()->SetTitle("Number of digits");
      getObjectsManager()->startPublishing(mHist1D[H1DClustersInEventM2 + mod]);
    } else {
      mHist1D[H1DClustersInEventM2 + mod]->Reset();
    }

    // 2D
    // digit map
    if (!mHist2D[H2DDigitMapM2 + mod]) {
      mHist2D[H2DDigitMapM2 + mod] =
        new TH2F(
          Form("DigitMapM%d", 2 + mod),
          Form("Digit Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DDigitMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DDigitMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DDigitMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DDigitMapM2 + mod]);
    } else {
      mHist2D[H2DDigitMapM2 + mod]->Reset();
    }

    // CalibDigit map
    if (!mHist2D[H2DCalibDigitMapM2 + mod]) {
      mHist2D[H2DCalibDigitMapM2 + mod] =
        new TH2F(
          Form("CalibDigitMapM%d", 2 + mod),
          Form("CalibDigit Map in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DCalibDigitMapM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DCalibDigitMapM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DCalibDigitMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DCalibDigitMapM2 + mod]);
    } else {
      mHist2D[H2DCalibDigitMapM2 + mod]->Reset();
    }

    // digit frequency
    if (!mHist2D[H2DDigitFreqM2 + mod]) {
      mHist2D[H2DDigitFreqM2 + mod] =
        new TH2F(
          Form("DigitFreqM%d", 2 + mod),
          Form("Digit Frequency in M%d", mod + 2),
          nPadsX, -0.5, nPadsX - 0.5,
          nPadsZ, -0.5, nPadsZ - 0.5);
      mHist2D[H2DDigitFreqM2 + mod]->GetXaxis()->SetTitle("x, pad");
      mHist2D[H2DDigitFreqM2 + mod]->GetYaxis()->SetTitle("z, pad");
      mHist2D[H2DDigitFreqM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DDigitFreqM2 + mod]);
    } else {
      mHist2D[H2DDigitFreqM2 + mod]->Reset();
    }

    // cluster map
    if (!mHist2D[H2DClusterMapM2 + mod]) {
      mHist2D[H2DClusterMapM2 + mod] =
        new TH2F(
          Form("ClusterMapM%d", 2 + mod),
          Form("Cluster Map in M%d", mod + 2),
          200, -rangeX, rangeX,
          200, -rangeZ, rangeZ);
      mHist2D[H2DClusterMapM2 + mod]->GetXaxis()->SetTitle("x, cm");
      mHist2D[H2DClusterMapM2 + mod]->GetYaxis()->SetTitle("z, cm");
      mHist2D[H2DClusterMapM2 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[H2DClusterMapM2 + mod]);
    } else {
      mHist2D[H2DClusterMapM2 + mod]->Reset();
    }
  }
}

void PhysicsTask::resetHistograms()
{
  // clean all histograms
  ILOG(Info, Support) << "Resetting the 1D Histograms" << ENDM;
  for (int itHist1D = H1DInputPayloadSize; itHist1D < kNHist1D; itHist1D++) {
    if (mHist1D[itHist1D]) {
      mHist1D[itHist1D]->Reset();
    } else {
      ILOG(Info, Support) << "1D histo " << itHist1D << " is not created yet!";
    }
  }

  ILOG(Info, Support) << "Resetting the 2D Histograms" << ENDM;
  for (int itHist2D = H2DDigitMapM2; itHist2D < kNHist2D; itHist2D++) {
    mHist2D[itHist2D]->Reset();
  }
}

} // namespace o2::quality_control_modules::cpv
