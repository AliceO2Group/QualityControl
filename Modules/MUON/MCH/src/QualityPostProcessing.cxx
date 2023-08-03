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
/// \file    QualityPostProcessing.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH quality flags
/// \since   21/06/2022
///

#include "MCH/QualityPostProcessing.h"
#include "QualityControl/DatabaseInterface.h"
#include <TPaveText.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::muonchambers;

void QualityPostProcessing::configure(const boost::property_tree::ptree& config)
{
  PostProcessingConfigMCH mchConfig(getID(), config);

  mAggregatedQualityName = mchConfig.getParameter<std::string>("AggregatedQualityName");
  mMessageGood = mchConfig.getParameter<std::string>("MessageGood");
  mMessageMedium = mchConfig.getParameter<std::string>("MessageMedium");
  mMessageBad = mchConfig.getParameter<std::string>("MessageBad");
  mMessageNull = mchConfig.getParameter<std::string>("MessageNull");

  for (auto source : mchConfig.dataSources) {
    mCcdbObjects.emplace_back(source.path, source.name);
  }
}

//_________________________________________________________________________________________

static void setQualityLabels(TH1F* h)
{
  TAxis* ax = h->GetXaxis();
  ax->SetBinLabel(1, "Null");
  ax->ChangeLabel(1, 45);
  ax->SetBinLabel(2, "Bad");
  ax->ChangeLabel(2, 45);
  ax->SetBinLabel(3, "Medium");
  ax->ChangeLabel(3, 45);
  ax->SetBinLabel(4, "Good");
  ax->ChangeLabel(4, 45);
}

void QualityPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  for (auto& obj : mCcdbObjects) {
    auto iter1 = mHistogramsQuality.emplace(std::make_pair(obj.mName, std::make_unique<TH1F>(obj.mName.c_str(), obj.mName.c_str(), 4, 0, 4)));
    setQualityLabels(iter1.first->second.get());
    publishHisto(iter1.first->second.get(), false, "hist", "");

    auto iter2 = mTrendsQuality.emplace(std::make_pair(obj.mName, std::make_unique<QualityTrendGraph>((std::string("Trends/") + obj.mName).c_str(), obj.mName.c_str())));
    getObjectsManager()->startPublishing(iter2.first->second.get());
    getObjectsManager()->setDisplayHint(iter2.first->second.get(), "gridy");
  }

  mCanvasCheckerMessages = std::make_unique<TCanvas>("CheckerMessages", "Checker Messages", 800, 600);
  getObjectsManager()->startPublishing(mCanvasCheckerMessages.get());
}

//_________________________________________________________________________________________

void QualityPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  mCheckerMessages.clear();

  Quality mchQuality = Quality::Null;

  for (auto& obj : mCcdbObjects) {
    if (obj.update(&qcdb, t.timestamp, t.activity)) {
      auto qo = obj.mObject;
      if (qo) {
        auto time = obj.getTimeStamp() / 1000; // ROOT expects seconds since epoch
        int Q = 0;
        std::string Qstr = fmt::format("#color[{}]", kViolet - 6) + "{Null}";
        if (qo->getQuality() == Quality::Bad) {
          Q = 1;
          Qstr = fmt::format("#color[{}]", kRed) + "{Bad}";
        }
        if (qo->getQuality() == Quality::Medium) {
          Q = 2;
          Qstr = fmt::format("#color[{}]", kOrange - 3) + "{Medium}";
        }
        if (qo->getQuality() == Quality::Good) {
          Q = 3;
          Qstr = fmt::format("#color[{}]", kGreen + 2) + "{Good}";
        }

        // only update plots/trends when the object is updated
        if (obj.mUpdated) {
          auto iter1 = mHistogramsQuality.find(obj.mName);
          if (iter1 != mHistogramsQuality.end()) {
            iter1->second->Fill(Q + 0.5);
          }

          auto iter2 = mTrendsQuality.find(obj.mName);
          if (iter2 != mTrendsQuality.end()) {
            iter2->second->update(time, qo->getQuality());
          }
        }

        if (qo->getName() == mAggregatedQualityName) {
          mchQuality = qo->getQuality();
        } else {
          mCheckerMessages.emplace_back(fmt::format("{} = {}", qo->getName(), Qstr));
        }
      }
    }
  }

  mCheckerMessages.insert(mCheckerMessages.begin(), "");
  if (mchQuality == Quality::Good) {
    if (!mMessageGood.empty()) {
      mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("#color[{}]", kGreen + 2) + "{" + mMessageGood + "}");
    }
    mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("MCH Quality = #color[{}]", kGreen + 2) + "{Good}");
  } else if (mchQuality == Quality::Medium) {
    if (!mMessageMedium.empty()) {
      mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("#color[{}]", kOrange - 3) + "{" + mMessageMedium + "}");
    }
    mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("MCH Quality = #color[{}]", kOrange - 3) + "{Medium}");
  } else if (mchQuality == Quality::Bad) {
    if (!mMessageBad.empty()) {
      mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("#color[{}]", kRed) + "{" + mMessageBad + "}");
    }
    mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("MCH Quality = #color[{}]", kRed) + "{Bad}");
  } else if (mchQuality == Quality::Null) {
    if (!mMessageNull.empty()) {
      mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("#color[{}]", kViolet - 6) + "{" + mMessageNull + "}");
    }
    mCheckerMessages.insert(mCheckerMessages.begin(), fmt::format("MCH Quality = #color[{}]", kViolet - 6) + "{Null}");
  }

  mCanvasCheckerMessages->Clear();
  mCanvasCheckerMessages->cd();

  TPaveText* msg = new TPaveText(0.1, 0.1, 0.9, 0.9, "NDC");
  for (auto s : mCheckerMessages) {
    msg->AddText(s.c_str());
  }
  msg->SetBorderSize(0);
  msg->SetFillColor(kWhite);
  msg->Draw();
}

//_________________________________________________________________________________________

void QualityPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
