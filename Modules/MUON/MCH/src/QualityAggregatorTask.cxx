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
/// \file    QualityAggregatorTask.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the MCH pre-clusters
/// \since   21/06/2022
///

#include "MCH/QualityAggregatorTask.h"
#include "MCH/Helpers.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ActivityHelpers.h"
#include "QualityControl/ReferenceUtils.h"
#include "Common/Utils.h"
#include "CCDB/CcdbObjectInfo.h"
#include "CommonUtils/MemFileHelper.h"
#include <TObjString.h>

#include <TPaveLabel.h>

#include <boost/property_tree/ptree.hpp>
#include <gsl/span>

using namespace o2::quality_control::core;
using namespace o2::quality_control::repository;
using namespace o2::quality_control_modules::common;
using namespace o2::quality_control_modules::muonchambers;

template <typename T>
o2::ccdb::CcdbObjectInfo createCcdbInfo(const T& object, uint64_t timeStamp, std::string_view reason)
{
  auto clName = o2::utils::MemFileHelper::getClassName(object);
  auto flName = o2::ccdb::CcdbApi::generateFileName(clName);
  std::map<std::string, std::string> md;
  md["upload-reason"] = reason;
  constexpr auto fiveDays = 5 * o2::ccdb::CcdbObjectInfo::DAY;
  return o2::ccdb::CcdbObjectInfo("MCH/Calib/BadDE", clName, flName, md, timeStamp, timeStamp + fiveDays);
}

/*
std::string toCSV(const std::set<int>& deIds)
{
  std::stringstream csv;
  csv << fmt::format("deid\n");

  for (auto deId : deIds) {
    csv << fmt::format("{}\n", deId);
  }

  return csv.str();
}
*/
//_________________________________________________________________________________________
// Helper function for retrieving a MonitorObject from the QCDB, in the form of a std::pair<std::shared_ptr<MonitorObject>, bool>
// A non-null MO is returned in the first element of the pair if the MO is found in the QCDB
// The second element of the pair is set to true if the MO has a time stamp more recent than a user-supplied threshold

static std::pair<std::shared_ptr<MonitorObject>, bool> getMO(DatabaseInterface& qcdb, const std::string& fullPath, Trigger trigger, long notOlderThan)
{
  // find the time-stamp of the most recent object matching the current activity
  // if ignoreActivity is true the activity matching criteria are not applied
  auto objectTimestamp = trigger.timestamp;
  const auto filterMetadata = activity_helpers::asDatabaseMetadata(trigger.activity, false);
  const auto objectValidity = qcdb.getLatestObjectValidity(trigger.activity.mProvenance + "/" + fullPath, filterMetadata);
  if (objectValidity.isValid()) {
    objectTimestamp = objectValidity.getMax() - 1;
  } else {
    ILOG(Warning, Devel) << "Could not find the object '" << fullPath << "' for activity " << trigger.activity << ENDM;
    return { nullptr, false };
  }

  auto [success, path, name] = o2::quality_control::core::RepoPathUtils::splitObjectPath(fullPath);
  if (!success) {
    return { nullptr, false };
  }
  // retrieve QO from CCDB
  auto qo = qcdb.retrieveMO(path, name, objectTimestamp, trigger.activity);
  if (!qo) {
    return { nullptr, false };
  }

  long elapsed = static_cast<long>(trigger.timestamp) - objectTimestamp;
  // check if the object is not older than a given number of milliseconds
  if (elapsed > notOlderThan) {
    ILOG(Warning, Devel) << "Object '" << fullPath << "' for activity " << trigger.activity << " is too old: " << elapsed << " > " << notOlderThan << ENDM;
    return { qo, false };
  }

  return { qo, true };
}

void QualityAggregatorTask::configure(const boost::property_tree::ptree& config)
{
  // input plots
  if (const auto& inputs = config.get_child_optional("qc.postprocessing." + getID() + ".inputsDE"); inputs.has_value()) {
    for (const auto& input : inputs.value()) {
      mDEPlotPaths.push_back(input.second.get_value<std::string>());
    }
  }
  if (const auto& inputs = config.get_child_optional("qc.postprocessing." + getID() + ".inputsSOLAR"); inputs.has_value()) {
    for (const auto& input : inputs.value()) {
      mSOLARPlotPaths.push_back(input.second.get_value<std::string>());
    }
  }
}

//_________________________________________________________________________________________

void QualityAggregatorTask::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  mCCDBpath = getFromExtendedConfig<std::string>(t.activity, mCustomParameters, "CCDBpath", mCCDBpath);
  mAPI.init(mCCDBpath);

  mObjectPathBadDE = getFromExtendedConfig<std::string>(t.activity, mCustomParameters, "objectPathBadDE", mObjectPathBadDE);
  mObjectPathBadSOLAR = getFromExtendedConfig<std::string>(t.activity, mCustomParameters, "objectPathBadSOLAR", mObjectPathBadSOLAR);

  //--------------------------------------------------
  // Quality histogram
  //--------------------------------------------------

  mHistogramQualityPerDE.reset();
  mHistogramQualityPerDE = std::make_unique<TH2F>("QualityFlagPerDE", "Quality Flag vs DE", getNumDE(), 0, getNumDE(), 3, 0, 3);
  addDEBinLabels(mHistogramQualityPerDE.get());
  addChamberDelimiters(mHistogramQualityPerDE.get());
  addChamberLabelsForDE(mHistogramQualityPerDE.get());
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(1, "Bad");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(2, "Medium");
  mHistogramQualityPerDE->GetYaxis()->SetBinLabel(3, "Good");
  mHistogramQualityPerDE->SetOption("colz");
  mHistogramQualityPerDE->SetStats(0);
  getObjectsManager()->startPublishing(mHistogramQualityPerDE.get(), core::PublicationPolicy::ThroughStop);
  getObjectsManager()->setDefaultDrawOptions(mHistogramQualityPerDE.get(), "colz");
  getObjectsManager()->setDisplayHint(mHistogramQualityPerDE.get(), "gridy");

  mHistogramQualityPerSolar.reset();
  mHistogramQualityPerSolar = std::make_unique<TH2F>("QualityFlagPerSolar", "Quality Flag vs Solar", getNumSolar(), 0, getNumSolar(), 3, 0, 3);
  addSolarBinLabels(mHistogramQualityPerSolar.get());
  addChamberDelimitersToSolarHistogram(mHistogramQualityPerSolar.get());
  addChamberLabelsForSolar(mHistogramQualityPerSolar.get());
  mHistogramQualityPerSolar->GetYaxis()->SetBinLabel(1, "Bad");
  mHistogramQualityPerSolar->GetYaxis()->SetBinLabel(2, "Medium");
  mHistogramQualityPerSolar->GetYaxis()->SetBinLabel(3, "Good");
  mHistogramQualityPerSolar->SetOption("col");
  mHistogramQualityPerSolar->SetStats(0);
  getObjectsManager()->startPublishing(mHistogramQualityPerSolar.get(), core::PublicationPolicy::ThroughStop);
  getObjectsManager()->setDefaultDrawOptions(mHistogramQualityPerSolar.get(), "col");
  getObjectsManager()->setDisplayHint(mHistogramQualityPerSolar.get(), "gridy");
}

//_________________________________________________________________________________________

void QualityAggregatorTask::update(Trigger trigger, framework::ServiceRegistryRef services)
{
  ILOG(Info, Devel) << "QualityAggregatorTask::update() called" << ENDM;
  auto& qcdb = services.get<repository::DatabaseInterface>();

  // ===================
  // Detection elements
  // ===================

  std::set<int> badDEs;
  std::array<int, getNumDE()> deQuality;
  std::fill(deQuality.begin(), deQuality.end(), 3);
  // fill vector of bad DEs
  for (auto plotPath : mDEPlotPaths) {
    auto [mo, success] = getMO(qcdb, plotPath, trigger, 600000);
    if (!success || !mo) {
      ILOG(Warning, Devel) << "Could not retrieve object " << plotPath << ENDM;
      continue;
    }

    TH2F* hist = dynamic_cast<TH2F*>(mo->getObject());
    if (!hist) {
      ILOG(Warning, Devel) << "Could not cast the object '" << plotPath << "' to TH2F" << ENDM;
      continue;
    }

    if (hist->GetXaxis()->GetNbins() != getNumDE()) {
      ILOG(Warning, Devel) << "Wrong number of bins for object '" << plotPath << "': "
                           << hist->GetXaxis()->GetNbins() << " while " << getNumDE() << " were expected" << ENDM;
      continue;
    }

    for (int index = 0; index < hist->GetXaxis()->GetNbins(); index++) {
      int q = 0;
      for (int qindex = 1; qindex <= hist->GetYaxis()->GetNbins(); qindex++) {
        if (hist->GetBinContent(index + 1, qindex) != 0) {
          q = qindex;
          break;
        }
      }
      if (q < deQuality[index]) {
        deQuality[index] = q;
      }

      bool isBad = (q == 1);
      if (!isBad) {
        continue;
      }

      int deId = getDEFromIndex(index);
      badDEs.insert(deId);

      ILOG(Debug, Devel) << "Bad detection element DE" << deId << " found in \'" << plotPath << "\'" << ENDM;
    }
  }
  mHistogramQualityPerDE->Reset();
  for (int index = 0; index < mHistogramQualityPerDE->GetXaxis()->GetNbins(); index++) {
    mHistogramQualityPerDE->SetBinContent(index + 1, deQuality[index], 1);
  }

  std::string badDEsStr;
  for (auto deId : badDEs) {
    if (badDEsStr.empty()) {
      badDEsStr = std::to_string(deId);
    } else {
      badDEsStr += std::string(" ") + std::to_string(deId);
    }
  }
  if (!badDEsStr.empty()) {
    std::string badDEList = std::string("Bad DEs: ") + badDEsStr;
    TPaveLabel* label = new TPaveLabel(0.2, 0.8, 0.8, 0.88, badDEList.c_str(), "blNDC");
    label->SetBorderSize(1);
    mHistogramQualityPerDE->GetListOfFunctions()->Add(label);

    ILOG(Warning, Support) << "[QualityAggregator] " << badDEList << ENDM;
  }

  // check if the list of bad DEs has changed
  bool changedDEs = (!mPreviousBadDEs.has_value() || mPreviousBadDEs.value() != badDEs);

  if (changedDEs) {
    std::string sBadDEs = "deid\n";
    for (auto deId : badDEs) {
      sBadDEs += fmt::format("{}\n", deId);
    }
    TObjString badDEsObject(sBadDEs.c_str());

    // time stamp
    auto uploadTS = o2::ccdb::getCurrentTimestamp();

    // CCDB object info
    auto clName = o2::utils::MemFileHelper::getClassName(badDEsObject);
    auto flName = o2::ccdb::CcdbApi::generateFileName(clName);
    constexpr auto fiveDays = 5 * o2::ccdb::CcdbObjectInfo::DAY;
    auto info = o2::ccdb::CcdbObjectInfo(mObjectPathBadDE, clName, flName, std::map<std::string, std::string>(), uploadTS, uploadTS + fiveDays);

    // CCDB object image
    auto image = o2::ccdb::CcdbApi::createObjectImage(&badDEsObject, &info);

    ILOG(Info, Devel) << "Storing object " << info.getPath()
                      << " of type " << info.getObjectType()
                      << " / " << info.getFileName()
                      << " and size " << image->size()
                      << " bytes, valid for " << info.getStartValidityTimestamp()
                      << " : " << info.getEndValidityTimestamp() << ENDM;

    int res = mAPI.storeAsBinaryFile(image->data(), image->size(), info.getFileName(), info.getObjectType(), info.getPath(), info.getMetaData(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp());
    if (res) {
      //LOGP(error, "uploading to {} / {} failed for [{}:{}]", mAPI.getURL(), info.getPath(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp());
      ILOG(Error, Ops) << fmt::format("uploading to {} / {} failed for [{}:{}]", mAPI.getURL(), info.getPath(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp()) << ENDM;
    } else {
      ILOG(Info, Devel) << fmt::format("uploading to {} / {} succeeded for [{}:{}]", mAPI.getURL(), info.getPath(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp()) << ENDM;
      mPreviousBadDEs = badDEs;
    }
  }

  // =============
  // SOLAR boards
  // =============

  std::set<int> badSolarBoards;
  std::array<int, getNumSolar()> solarQuality;
  std::fill(solarQuality.begin(), solarQuality.end(), 3);
  // fill vector of bad Solars
  for (auto plotPath : mSOLARPlotPaths) {
    auto [mo, success] = getMO(qcdb, plotPath, trigger, 600000);
    if (!success || !mo) {
      ILOG(Warning, Devel) << "Could not retrieve object " << plotPath << ENDM;
      continue;
    }

    TH2F* hist = dynamic_cast<TH2F*>(mo->getObject());
    if (!hist) {
      ILOG(Warning, Devel) << "Could not cast the object '" << plotPath << "' to TH2F" << ENDM;
      continue;
    }

    if (hist->GetXaxis()->GetNbins() != getNumSolar()) {
      ILOG(Warning, Devel) << "Wrong number of bins for object '" << plotPath << "': "
                           << hist->GetXaxis()->GetNbins() << " while " << getNumSolar() << " were expected" << ENDM;
      continue;
    }

    for (int index = 0; index < hist->GetXaxis()->GetNbins(); index++) {
      int q = 0;
      for (int qindex = 1; qindex <= hist->GetYaxis()->GetNbins(); qindex++) {
        if (hist->GetBinContent(index + 1, qindex) != 0) {
          q = qindex;
          break;
        }
      }
      if (q < solarQuality[index]) {
        solarQuality[index] = q;
      }

      bool isBad = (q == 1);
      if (!isBad) {
        continue;
      }

      int solarId = getSolarIdFromIndex(index);
      badSolarBoards.insert(solarId);

      ILOG(Debug, Devel) << "Bad SOLAR " << solarId << " found in \'" << plotPath << "\'" << ENDM;
    }
  }

  mHistogramQualityPerSolar->Reset();
  for (int index = 0; index < mHistogramQualityPerSolar->GetXaxis()->GetNbins(); index++) {
    mHistogramQualityPerSolar->SetBinContent(index + 1, solarQuality[index], 1);
  }

  std::string badSolarBoardsStr;
  for (auto solarId : badSolarBoards) {
    if (badSolarBoardsStr.empty()) {
      badSolarBoardsStr = std::to_string(solarId);
    } else {
      badSolarBoardsStr += std::string(" ") + std::to_string(solarId);
    }
  }
  if (!badSolarBoardsStr.empty()) {
    std::string badSolarList = std::string("Bad SOLAR boards: ") + badSolarBoardsStr;
    TPaveLabel* label = new TPaveLabel(0.2, 0.8, 0.8, 0.88, badSolarList.c_str(), "blNDC");
    label->SetBorderSize(1);
    mHistogramQualityPerSolar->GetListOfFunctions()->Add(label);

    ILOG(Warning, Support) << "[QualityAggregator] " << badSolarList << ENDM;
  }

  // check if the list of bad Solars has changed
  bool changedSolarBoards = (!mPreviousBadSolarBoards.has_value() || mPreviousBadSolarBoards.value() != badSolarBoards);

  if (changedSolarBoards) {
    std::string sBadSolars = "solarid\n";
    for (auto solarId : badSolarBoards) {
      sBadSolars += fmt::format("{}\n", solarId);
    }
    TObjString badSolarBoardsObject(sBadSolars.c_str());

    // time stamp
    auto uploadTS = o2::ccdb::getCurrentTimestamp();

    // CCDB object info
    auto clName = o2::utils::MemFileHelper::getClassName(badSolarBoardsObject);
    auto flName = o2::ccdb::CcdbApi::generateFileName(clName);
    constexpr auto fiveDays = 5 * o2::ccdb::CcdbObjectInfo::DAY;
    auto info = o2::ccdb::CcdbObjectInfo(mObjectPathBadSOLAR, clName, flName, std::map<std::string, std::string>(), uploadTS, uploadTS + fiveDays);

    // CCDB object image
    auto image = o2::ccdb::CcdbApi::createObjectImage(&badSolarBoardsObject, &info);

    ILOG(Info, Devel) << "Storing object " << info.getPath()
                      << " of type " << info.getObjectType()
                      << " / " << info.getFileName()
                      << " and size " << image->size()
                      << " bytes, valid for " << info.getStartValidityTimestamp()
                      << " : " << info.getEndValidityTimestamp() << ENDM;
    ILOG(Info, Devel) << badSolarBoardsObject.String().Data() << ENDM;

    int res = mAPI.storeAsBinaryFile(image->data(), image->size(), info.getFileName(), info.getObjectType(), info.getPath(), info.getMetaData(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp());
    if (res) {
      //LOGP(error, "uploading to {} / {} failed for [{}:{}]", mAPI.getURL(), info.getPath(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp());
      ILOG(Error, Ops) << fmt::format("uploading to {} / {} failed for [{}:{}]", mAPI.getURL(), info.getPath(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp()) << ENDM;
    } else {
      ILOG(Info, Devel) << fmt::format("uploading to {} / {} succeeded for [{}:{}]", mAPI.getURL(), info.getPath(), info.getStartValidityTimestamp(), info.getEndValidityTimestamp()) << ENDM;
      mPreviousBadSolarBoards = badSolarBoards;
    }
  }
}

//_________________________________________________________________________________________

void QualityAggregatorTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
