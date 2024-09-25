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
/// \brief   Post-processing of the ZDC ADC and TDC (Time and amplitudes) plots
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

  // TDC Time summary plot
  for (auto source : zdcConfig.dataSourcesTDC) {
    std::string histName, moName;
    splitDataSourceName(source.name, histName, moName);
    if (moName.empty()) {
      continue;
    }

    mBinLabelsTDC.emplace_back(histName);
    mMOsTDC.emplace(mBinLabelsTDC.size(), MOHelper(source.path, moName));
  }

  // TDC Amplitude summary plot
  for (auto source : zdcConfig.dataSourcesTDCA) {
    std::string histName, moName;
    splitDataSourceName(source.name, histName, moName);
    if (moName.empty()) {
      continue;
    }

    mBinLabelsTDCA.emplace_back(histName);
    mMOsTDCA.emplace(mBinLabelsTDCA.size(), MOHelper(source.path, moName));
  }

  // TDC Peak 1n summary plot
  for (auto source : zdcConfig.dataSourcesPeak1n) {
    std::string histName, moName;
    splitDataSourceName(source.name, histName, moName);
    if (moName.empty()) {
      continue;
    }

    mBinLabelsPeak1n.emplace_back(histName);
    mMOsPeak1n.emplace(mBinLabelsPeak1n.size(), MOHelper(source.path, moName));
  }

  // TDC Peak 1p summary plot
  for (auto source : zdcConfig.dataSourcesPeak1p) {
    std::string histName, moName;
    splitDataSourceName(source.name, histName, moName);
    if (moName.empty()) {
      continue;
    }

    mBinLabelsPeak1p.emplace_back(histName);
    mMOsPeak1p.emplace(mBinLabelsPeak1p.size(), MOHelper(source.path, moName));
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

void ZDCRecDataPostProcessing::createSummaryTDCAHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  mSummaryTDCAHisto.reset();
  mSummaryTDCAHisto = std::make_unique<TH1F>("h_summary_TDCA", "Summary TDC Amplitude", mBinLabelsTDCA.size(), 0, mBinLabelsTDCA.size());
  for (size_t bin = 0; bin < mBinLabelsTDCA.size(); bin++) {
    mSummaryTDCAHisto->GetXaxis()->SetBinLabel(bin + 1, mBinLabelsTDCA[bin].c_str());
  }
  publishHisto(mSummaryTDCAHisto.get(), false, "E", "");
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::createSummaryPeak1nHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  mSummaryPeak1nHisto.reset();
  mSummaryPeak1nHisto = std::make_unique<TH1F>("h_summary_Peak1n", "Summary TDC Amplitude Peak 1n", mBinLabelsPeak1n.size(), 0, mBinLabelsPeak1n.size());
  for (size_t bin = 0; bin < mBinLabelsPeak1n.size(); bin++) {
    mSummaryPeak1nHisto->GetXaxis()->SetBinLabel(bin + 1, mBinLabelsPeak1n[bin].c_str());
  }
  publishHisto(mSummaryPeak1nHisto.get(), false, "E", "");
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::createSummaryPeak1pHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  mSummaryPeak1pHisto.reset();
  mSummaryPeak1pHisto = std::make_unique<TH1F>("h_summary_Peak1p", "Summary TDC Amplitude Peak 1p", mBinLabelsPeak1p.size(), 0, mBinLabelsPeak1p.size());
  for (size_t bin = 0; bin < mBinLabelsPeak1p.size(); bin++) {
    mSummaryPeak1pHisto->GetXaxis()->SetBinLabel(bin + 1, mBinLabelsPeak1p[bin].c_str());
  }
  publishHisto(mSummaryPeak1pHisto.get(), false, "E", "");
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::initialize(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  createSummaryADCHistos(t, &qcdb);
  createSummaryTDCHistos(t, &qcdb);
  createSummaryTDCAHistos(t, &qcdb);
  createSummaryPeak1nHistos(t, &qcdb);
  createSummaryPeak1pHistos(t, &qcdb);
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

void ZDCRecDataPostProcessing::updateSummaryTDCAHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  for (auto& [bin, mo] : mMOsTDCA) {
    if (!mo.update(qcdb, t.timestamp, t.activity)) {
      continue;
    }
    TH1F* h = mo.get<TH1F>();
    if (!h) {
      continue;
    }
    mSummaryTDCAHisto->SetBinContent(bin, h->GetMean());
    mSummaryTDCAHisto->SetBinError(bin, h->GetMeanError());
  }
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::updateSummaryPeak1nHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  for (auto& [bin, mo] : mMOsPeak1n) {
    if (!mo.update(qcdb, t.timestamp, t.activity)) {
      continue;
    }
    TH1F* h = mo.get<TH1F>();
    if (!h) {
      continue;
    }
    h->GetXaxis()->SetRangeUser(1., 200.);
    mSummaryPeak1nHisto->SetBinContent(bin, h->GetBinCenter(h->GetMaximumBin()));
  }
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::updateSummaryPeak1pHistos(Trigger t, repository::DatabaseInterface* qcdb)
{
  float minBin1p = 2;
  float maxBin1p = 250;
  for (auto& [bin, mo] : mMOsPeak1p) {
    if (!mo.update(qcdb, t.timestamp, t.activity)) {
      continue;
    }
    TH1F* h = mo.get<TH1F>();
    if (!h) {
      continue;
    }
    if (auto param = mCustomParameters.find("minBin1p"); param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "Custom parameter - minBin1p: " << param->second << ENDM;
      minBin1p = atof(param->second.c_str());
    } else {
      minBin1p = 2;
    }
    if (auto param = mCustomParameters.find("maxBin1p"); param != mCustomParameters.end()) {
      ILOG(Debug, Devel) << "Custom parameter - maxBin1p: " << param->second << ENDM;
      maxBin1p = atof(param->second.c_str());
    } else {
      maxBin1p = 250;
    }
    h->GetXaxis()->SetRangeUser(2, 250);
    mSummaryPeak1pHisto->SetBinContent(bin, h->GetBinCenter(h->GetMaximumBin()));
  }
}
//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();

  updateSummaryADCHistos(t, &qcdb);
  updateSummaryTDCHistos(t, &qcdb);
  updateSummaryTDCAHistos(t, &qcdb);
  updateSummaryPeak1nHistos(t, &qcdb);
  updateSummaryPeak1pHistos(t, &qcdb);
}

//_________________________________________________________________________________________

void ZDCRecDataPostProcessing::finalize(Trigger t, framework::ServiceRegistryRef)
{
}
