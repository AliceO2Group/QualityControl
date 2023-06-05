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
/// \file     RatioGeneratorTPC.cxx
/// \author   Marcel Lesch
///

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/QcInfoLogger.h"
#include <TPC/RatioGeneratorTPC.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <string>
#include <TAxis.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tpc;

void RatioGeneratorTPC::configure(const boost::property_tree::ptree& config)
{
  auto& id = getID();
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + id + ".ratioConfig")) {
    std::string inputNames[2];
    int counter = 0;

    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("inputObjects"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        if (counter > 1) {
          ILOG(Error, Support) << "Ratio plots should not have more than two input objects!" << ENDM;
          break;
        }
        inputNames[counter] = sourceName.second.data();
        counter++;
      }
    }
    mConfig.push_back({ dataSourceConfig.second.get<std::string>("path"),
                        { inputNames[0], inputNames[1] },
                        dataSourceConfig.second.get<std::string>("outputName"),
                        dataSourceConfig.second.get<std::string>("plotTitle", ""),
                        dataSourceConfig.second.get<std::string>("axisTitle", "") });

  } // for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".ratioConfig"))
}

void RatioGeneratorTPC::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  generateRatios(t, qcdb);
  generatePlots();
}

void RatioGeneratorTPC::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (const auto& source : mConfig) {
    if (mRatios.count(source.nameOutputObject)) {
      getObjectsManager()->stopPublishing(source.nameOutputObject);
    }
  }
  generatePlots();
}

void RatioGeneratorTPC::generateRatios(const Trigger& t,
                                       repository::DatabaseInterface& qcdb)
{
  for (const auto& source : mConfig) {
    // Delete the existing ratios before regenerating them.
    if (mRatios.count(source.nameOutputObject)) {
      getObjectsManager()->stopPublishing(source.nameOutputObject);
      delete mRatios[source.nameOutputObject];
      mRatios[source.nameOutputObject] = nullptr;
    }
    auto moNumerator = qcdb.retrieveMO(source.path, source.nameInputObjects[0], t.timestamp, t.activity);
    TH1* histoNumerator = moNumerator ? dynamic_cast<TH1*>(moNumerator->getObject()) : nullptr;
    auto moDenominator = qcdb.retrieveMO(source.path, source.nameInputObjects[1], t.timestamp, t.activity);
    TH1* histoDenominator = moDenominator ? dynamic_cast<TH1*>(moDenominator->getObject()) : nullptr;

    if (histoNumerator && histoDenominator) {
      mRatios[source.nameOutputObject] = (TH1*)histoNumerator->Clone(fmt::format("{}_over_{}", histoNumerator->GetName(), histoDenominator->GetName()).data());
      mRatios[source.nameOutputObject]->Divide(histoDenominator);
    }
  }
} // void RatioGeneratorTPC::trendValues(uint64_t timestamp, repository::DatabaseInterface& qcdb)

void RatioGeneratorTPC::generatePlots()
{
  for (const auto& source : mConfig) {
    // Beautify and publish the ratios
    const std::size_t posDivider = source.axisTitle.find(":");
    const std::string yLabel(source.axisTitle.substr(0, posDivider));
    const std::string xLabel(source.axisTitle.substr(posDivider + 1));

    mRatios[source.nameOutputObject]->SetName(source.nameOutputObject.c_str());
    mRatios[source.nameOutputObject]->GetXaxis()->SetTitle(xLabel.data());
    mRatios[source.nameOutputObject]->GetYaxis()->SetTitle(yLabel.data());
    mRatios[source.nameOutputObject]->SetTitle(source.plotTitle.data());

    getObjectsManager()->startPublishing(mRatios[source.nameOutputObject]);
  }
} // void RatioGeneratorTPC::generatePlots()
