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
// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/SupermoduleProjectorTask.h"
#include "QualityControl/DatabaseInterface.h"
#include <DataFormatsQualityControl/FlagTypeFactory.h>

#include <boost/algorithm/string/case_conv.hpp>

// root includes
#include "TCanvas.h"
#include "TPaveText.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TLatex.h"

#include <boost/property_tree/ptree.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

void SupermoduleProjectorTask::configure(const boost::property_tree::ptree& config)
{
  mDataSources = getDataSources(getID(), config);
  mAttributeHandler = parseCustomizations(getID(), config);
}

void SupermoduleProjectorTask::initialize(Trigger, framework::ServiceRegistryRef)
{
  ILOG(Debug, Devel) << "initialize SuperModuleProjectorTask" << ENDM;
  // create canvas objects for each plot
  for (const auto& datasource : mDataSources) {
    std::string canvasname = "PerSM_" + datasource.name,
                canvastitle = datasource.name + " per SM";
    auto plot = new TCanvas(canvasname.data(), canvastitle.data(), 1000, 800);
    plot->Divide(4, 5);
    getObjectsManager()->startPublishing(plot);
    mCanvasHandler[datasource.name] = plot;
  }
  mIndicesConverter.Initialize();
}

void SupermoduleProjectorTask::update(Trigger t, framework::ServiceRegistryRef services)
{
  auto& qcdb = services.get<quality_control::repository::DatabaseInterface>();
  for (auto& dataSource : mDataSources) {
    auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
    if (mo == nullptr) {
      ILOG(Warning, Trace) << "Could not retrieve MO '" << dataSource.name << "', skipping this data source" << ENDM;
      continue;
    }
    auto canvas = mCanvasHandler.find(dataSource.name);
    if (canvas != mCanvasHandler.end()) {
      PlotAttributes* plotCustomizations = nullptr;
      auto customiztions = mAttributeHandler.find(dataSource.name);
      if (customiztions != mAttributeHandler.end()) {
        plotCustomizations = &(customiztions->second);
      }

      // retrive associated quality object
      std::shared_ptr<QualityObject> qo;
      if (plotCustomizations && plotCustomizations->qualityPath.length()) {
        std::string qopath = dataSource.path;
        ILOG(Debug, Support) << "Looking for associated quality: " << plotCustomizations->qualityPath << ENDM;
        qo = qcdb.retrieveQO(plotCustomizations->qualityPath, t.timestamp, t.activity);
      }
      makeProjections(*mo, *(canvas->second), plotCustomizations, qo.get());
    }
  }
}

void SupermoduleProjectorTask::finalize(Trigger t, framework::ServiceRegistryRef)
{
  for (auto& [datasource, plot] : mCanvasHandler) {
    getObjectsManager()->stopPublishing(plot);
    delete plot;
    plot = nullptr;
  }
}

void SupermoduleProjectorTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;

} // namespace o2::quality_control_modules::emcal

std::vector<SupermoduleProjectorTask::DataSource> SupermoduleProjectorTask::getDataSources(std::string name, const boost::property_tree::ptree& config)
{
  std::vector<DataSource> dataSources;
  for (const auto& dataSourceConfig : config.get_child("qc.postprocessing." + name + ".dataSources")) {
    if (const auto& sourceNames = dataSourceConfig.second.get_child_optional("names"); sourceNames.has_value()) {
      for (const auto& sourceName : sourceNames.value()) {
        dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                                dataSourceConfig.second.get<std::string>("path"),
                                sourceName.second.data() });
      }
    } else if (!dataSourceConfig.second.get<std::string>("name").empty()) {
      // "name" : [ "something" ] would return an empty string here
      dataSources.push_back({ dataSourceConfig.second.get<std::string>("type", "repository"),
                              dataSourceConfig.second.get<std::string>("path"),
                              dataSourceConfig.second.get<std::string>("name") });
    } else {
      throw std::runtime_error("No 'name' value or a 'names' vector in the path 'qc.postprocessing." + name + ".dataSources'");
    }
  }
  return dataSources;
}

std::map<std::string, SupermoduleProjectorTask::PlotAttributes> SupermoduleProjectorTask::parseCustomizations(std::string name, const boost::property_tree::ptree& config)
{
  auto decodeBool = [](const std::string_view& token) -> bool {
    if (token == "true" || token == "True" || token == "TRUE") {
      return true;
    }
    return false;
  };
  std::map<std::string, PlotAttributes> customizations;
  for (const auto& customizationConfig : config.get_child("qc.postprocessing." + name + ".customizations")) {
    std::string objectname = customizationConfig.second.get<std::string>("name");
    // each setting is optional
    customizations[objectname] = {
      customizationConfig.second.get<std::string>("xtitle", ""),
      customizationConfig.second.get<std::string>("ytitle", ""),
      customizationConfig.second.get<double>("xmin", DBL_MIN),
      customizationConfig.second.get<double>("xmax", DBL_MAX),
      decodeBool(customizationConfig.second.get<std::string>("logx", "false")),
      decodeBool(customizationConfig.second.get<std::string>("logy", "false")),
      customizationConfig.second.get<std::string>("qualityPath", ""),
    };
  }
  return customizations;
}

std::map<int, std::string> SupermoduleProjectorTask::parseQuality(const quality_control::core::QualityObject& qo) const
{
  std::map<int, std::string> result;
  auto globalQuality = qo.getQuality();
  if (globalQuality == Quality::Bad || globalQuality == Quality::Medium) {
    auto flags = qo.getFlags();
    auto emflag = std::find_if(flags.begin(), flags.end(), [](const std::pair<quality_control::FlagType, std::string>& testflag) -> bool {
      bool found = testflag.first == quality_control::FlagTypeFactory::BadEMCalorimetry();
      return testflag.first == quality_control::FlagTypeFactory::BadEMCalorimetry();
    });
    if (emflag != flags.end()) {
      std::stringstream lineparser(emflag->second);
      std::string line;
      while (std::getline(lineparser, line, '\n')) {
        std::stringstream tokenizer(line);
        std::string token;
        bool found = false;
        int smID = -1;
        while (std::getline(tokenizer, token, ' ')) {
          if (found) {
            try {
              smID = std::stoi(token);
            } catch (...) {
            }
            break;
          }
          auto lowertoken = boost::algorithm::to_lower_copy(token);
          if (lowertoken == "sm" || lowertoken == "supermodule") {
            found = true;
          }
        }
        if (smID >= 0 && smID < 20) {
          result[smID] = line;
        }
      }
    }
  }

  return result;
}

void SupermoduleProjectorTask::makeProjections(quality_control::core::MonitorObject& mo, TCanvas& plot, const PlotAttributes* customizations, const quality_control::core::QualityObject* qo)
{
  auto inputhist = dynamic_cast<TH2*>(mo.getObject());
  bool xAxisSM = inputhist->GetXaxis()->GetNbins() == 20; // for the moment assume that the axis with 20 bins is the SM axis, very unlikely that we have an observable with exactly 20 bins
  if (!inputhist) {
    ILOG(Error, Support) << "Monitoring object to be projected not of type TH2" << ENDM;
    return;
  }
  std::map<int, std::string> badSMs;
  if (qo) {
    badSMs = parseQuality(*qo);
  }
  plot.Clear();
  plot.Divide(4, 5);
  for (int supermoduleID = 0; supermoduleID < 20; supermoduleID++) {
    std::string histname = std::string(inputhist->GetName()) + "_SM" + std::to_string(supermoduleID),
                histtitle = "Supermodule " + std::to_string(supermoduleID) + "  (" + mIndicesConverter.GetOnlineSMIndex(supermoduleID) + ")";
    TH1* projection = nullptr;
    if (xAxisSM) {
      projection = inputhist->ProjectionY(histname.data(), supermoduleID + 1, supermoduleID + 1);
    } else {
      projection = inputhist->ProjectionX(histname.data(), supermoduleID + 1, supermoduleID + 1);
    }
    projection->SetStats(false);
    projection->SetTitle(histtitle.data());
    projection->SetTitleSize(12);
    projection->SetTitleFont(43);
    bool logx = false;
    bool logy = false;
    if (customizations) {
      if (customizations->titleX.length()) {
        projection->SetXTitle(customizations->titleX.data());
      }
      if (customizations->titleY.length()) {
        projection->SetYTitle(customizations->titleY.data());
      }
      bool hasXrange = false;
      double xrangeMin = DBL_MIN;
      double xrangeMax = DBL_MAX;
      if (customizations->minX > DBL_MIN && customizations->minX >= projection->GetXaxis()->GetXmin()) {
        xrangeMin = customizations->minX;
      } else {
        xrangeMin = projection->GetXaxis()->GetXmin();
      }
      if (customizations->maxX < DBL_MAX && customizations->maxX <= projection->GetXaxis()->GetXmax()) {
        xrangeMax = customizations->maxX;
      } else {
        xrangeMax = projection->GetXaxis()->GetXmax();
      }
      if (hasXrange) {
        projection->GetXaxis()->SetRangeUser(xrangeMin, xrangeMax);
      }
      logx = customizations->logx;
      logy = customizations->logy;
    }
    plot.cd(supermoduleID + 1);
    if (logx) {
      gPad->SetLogx();
    }
    if (logy) {
      gPad->SetLogy();
    }
    if (qo) {
      auto bad_quality = badSMs.find(supermoduleID);
      if (bad_quality != badSMs.end()) {
        TLatex* msg = new TLatex(0.3, 0.8, Form("#color[2]{%s}", bad_quality->second.data()));
        msg->SetNDC();
        msg->SetTextSize(8);
        msg->SetTextFont(43);
        projection->GetListOfFunctions()->Add(msg);
        msg->Draw();
        projection->SetFillColor(kRed);
      } else {
        TLatex* msg = new TLatex(0.3, 0.8, "#color[418]{Data OK}");
        msg->SetNDC();
        msg->SetTextSize(8);
        msg->SetTextFont(43);
        projection->GetListOfFunctions()->Add(msg);
        msg->Draw();
        projection->SetFillColor(kGreen);
      }
    }
    projection->Draw();
    projection->SetBit(TObject::kCanDelete);
  }
}

} // namespace o2::quality_control_modules::emcal
