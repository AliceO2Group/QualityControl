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
/// \file    ZDCRecDataPostProcessing.cxx
/// \author  Andrea Ferrero andrea.ferrero@cern.ch
/// \brief   Post-processing of the ZDC ADC and TDC plots
/// \since   30/08/2023
///

#include "ZDC/ZDCRecDataPostProcessing.h"
#include "ZDC/PostProcessingConfigZDC.h"
#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/ObjectMetadataKeys.h"
#include "QualityControl/QcInfoLogger.h"

using namespace o2::quality_control_modules::zdc;

//_________________________________________________________________________________________

static void splitDataSourceName(std::string s, std::string& histName, std::string& moName)
{
  std::string delimiter = ":";
  size_t pos = s.find(delimiter);
  if (pos != std::string::npos) {
    histName = s.substr(0, pos);
    s.erase(0, pos + delimiter.length());
  }

  moName = s;
}

//_________________________________________________________________________________________

static long getMoTimeStamp(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  long timeStamp{ 0 };

  auto iter = mo->getMetadataMap().find(repository::metadata_keys::created);
  if (iter != mo->getMetadataMap().end()) {
    timeStamp = std::stol(iter->second);
  }

  return timeStamp;
}

//_________________________________________________________________________________________

static bool checkMoTimeStamp(std::shared_ptr<o2::quality_control::core::MonitorObject> mo, uint64_t& timeStampOld)
{
  auto timeStamp = getMoTimeStamp(mo);
  bool result = timeStamp != timeStampOld;
  timeStampOld = timeStamp;
  return result;
}

//_________________________________________________________________________________________

template <typename T>
T* getPlotFromMO(std::shared_ptr<o2::quality_control::core::MonitorObject> mo)
{
  // Get ROOT object
  TObject* obj = mo ? mo->getObject() : nullptr;
  if (!obj) {
    return nullptr;
  }

  // Get histogram object
  T* h = dynamic_cast<T*>(obj);
  return h;
}

//_________________________________________________________________________________________

MOHelper::MOHelper()
{
  setStartIme();
}

//_________________________________________________________________________________________

MOHelper::MOHelper(std::string p, std::string n) : mPath(p), mName(n)
{
  setStartIme();
}

//_________________________________________________________________________________________

void MOHelper::setStartIme()
{
  mTimeStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

//_________________________________________________________________________________________

bool MOHelper::update(repository::DatabaseInterface* qcdb, long timeStamp, const core::Activity& activity)
{
  mObject = qcdb->retrieveMO(mPath, mName, timeStamp, activity);
  if (!mObject) {
    return false;
  }

  if (timeStamp > mTimeStart) {
    // we are requesting a new object, so check that the retrieved one
    // was created after the start of the processing
    if (getMoTimeStamp(mObject) < mTimeStart) {
      return false;
    }
  }

  if (!checkMoTimeStamp(mObject, mTimeStamp)) {
    return false;
  }

  return true;
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::configure(const boost::property_tree::ptree& config)
{
  PostProcessingConfigZDC zdcConfig(getID(), config);

  // ADC summary plot
  for (auto source : zdcConfig.dataSourcesADC) {
    std::string histName, moName;
    splitDataSourceName(source.name, histName, moName);
    if (moName.empty()) {
      continue;
    }

    mBinLabelsADC.emplace_back(histName);
    mMOsADC.emplace(mBinLabelsADC.size(), MOHelper(source.path, moName));
  }

  // TDC summary plot
  for (auto source : zdcConfig.dataSourcesTDC) {
    std::string histName, moName;
    splitDataSourceName(source.name, histName, moName);
    if (moName.empty()) {
      continue;
    }

    mBinLabelsTDC.emplace_back(histName);
    mMOsTDC.emplace(mBinLabelsTDC.size(), MOHelper(source.path, moName));
  }
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::createSummaryADCHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  mSummaryADCHisto.reset();
  mSummaryADCHisto = std::make_unique<TH1F>("h_summary_ADC", "Summary ADC", mBinLabelsADC.size(), 0, mBinLabelsADC.size());
  for (size_t bin = 0; bin < mBinLabelsADC.size(); bin++) {
    mSummaryADCHisto->GetXaxis()->SetBinLabel(bin + 1, mBinLabelsADC[bin].c_str());
  }
  publishHisto(mSummaryADCHisto.get(), false, "E", "");
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::createSummaryTDCHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  mSummaryTDCHisto.reset();
  mSummaryTDCHisto = std::make_unique<TH1F>("h_summary_TDC", "Summary TDC", mBinLabelsTDC.size(), 0, mBinLabelsTDC.size());
  for (size_t bin = 0; bin < mBinLabelsTDC.size(); bin++) {
    mSummaryTDCHisto->GetXaxis()->SetBinLabel(bin + 1, mBinLabelsTDC[bin].c_str());
  }
  publishHisto(mSummaryTDCHisto.get(), false, "E", "");
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  createSummaryADCHistos(t, &qcdb);
  createSummaryTDCHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::updateSummaryADCHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  for (auto& [bin, mo] : mMOsADC) {
    if (!mo.update(qcdb, t.timestamp, t.activity)) {
      continue;
    }
    TH1F* h = mo.get<TH1F>();
    if (!h) {
      continue;
    }
    mSummaryADCHisto->SetBinContent(bin, h->GetMean());
    mSummaryADCHisto->SetBinError(bin, h->GetMeanError());
  }
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::updateSummaryTDCHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  for (auto& [bin, mo] : mMOsTDC) {
    if (!mo.update(qcdb, t.timestamp, t.activity)) {
      continue;
    }
    TH1F* h = mo.get<TH1F>();
    if (!h) {
      continue;
    }
    mSummaryTDCHisto->SetBinContent(bin, h->GetMean());
    mSummaryTDCHisto->SetBinError(bin, h->GetMeanError());
  }
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateSummaryADCHistos(t, &qcdb);
  updateSummaryTDCHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
