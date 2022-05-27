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
/// \file     TrendingTaskTPC.cxx
/// \author   Marcel Lesch
/// \author   Cindy Mordasini
/// \author   Based on the work from Piotr Konopka
///

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/RootClassFactory.h"
#include "QualityControl/QcInfoLogger.h"
#include "TPC/TH1ReductorTPC.h"
#include "TPC/TH2ReductorTPC.h"
//#include "TPC/QualityReductorTPC.h"
#include "TPC/TrendingTaskTPC.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <TDatime.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TTreeReaderArray.h>
#include <fmt/format.h>
#include <TAxis.h>
#include <TH2F.h>
#include <TStyle.h>

using namespace o2::quality_control;
using namespace o2::quality_control::core;
using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control_modules::tpc;

void TrendingTaskTPC::configure(std::string name,
                                const boost::property_tree::ptree& config)
{
  mConfig = TrendingTaskConfigTPC(name, config);
}

void TrendingTaskTPC::initialize(Trigger, framework::ServiceRegistry&)
{
  // Prepare the data structure of the trending TTree.
  mTrend = std::make_unique<TTree>();
  mTrend->SetName(PostProcessingInterface::getName().c_str());
  mTrend->Branch("meta", &mMetaData, "runNumber/I");
  mTrend->Branch("time", &mTime);

  for (const auto& source : mConfig.dataSources) {
    mSources[source.name] = std::vector<SliceInfo>();
    mSourcesQuality[source.name] = SliceInfoQuality();

    std::unique_ptr<ReductorTPC> reductor(root_class_factory::create<ReductorTPC>(
      source.moduleName, source.reductorName));
    if (source.type == "repository") {
      mTrend->Branch(source.name.c_str(), &mSources[source.name]);
      mIsMoObject[source.name] = true;
    } else if (source.type == "repository-quality") {
      mTrend->Branch(source.name.c_str(), &mSourcesQuality[source.name]);
      mIsMoObject[source.name] = false;
    }
    mReductors[source.name] = std::move(reductor);
  }

  getObjectsManager()->startPublishing(mTrend.get());
}

void TrendingTaskTPC::update(Trigger t, framework::ServiceRegistry& services)
{
  auto& qcdb = services.get<repository::DatabaseInterface>();
  trendValues(t, qcdb);
  generatePlots();
}

void TrendingTaskTPC::finalize(Trigger t, framework::ServiceRegistry&)
{
  generatePlots();
}

void TrendingTaskTPC::trendValues(const Trigger& t,
                                  repository::DatabaseInterface& qcdb)
{
  mTime = t.timestamp / 1000; // ROOT expects seconds since epoch.
  mMetaData.runNumber = -1;

  for (auto& dataSource : mConfig.dataSources) {
    mSubtitles[dataSource.name] = std::vector<std::string>();
    if (dataSource.type == "repository") {
      auto mo = qcdb.retrieveMO(dataSource.path, dataSource.name, t.timestamp, t.activity);
      TObject* obj = mo ? mo->getObject() : nullptr;

      mAxisDivision[dataSource.name] = dataSource.axisDivision;

      if (obj) {
        mReductors[dataSource.name]->update(obj, mSources[dataSource.name],
                                            dataSource.axisDivision, mSubtitles[dataSource.name]);
      }

    } else if (dataSource.type == "repository-quality") {
      if (auto qo = qcdb.retrieveQO(dataSource.path + "/" + dataSource.name, t.timestamp, t.activity)) {
        mReductors[dataSource.name]->updateQuality(qo.get(), mSourcesQuality[dataSource.name], mSubtitles[dataSource.name]);
      }
    } else {
      ILOG(Error, Support) << "Data source '" << dataSource.type << "' unknown." << ENDM;
    }
  }

  mTrend->Fill();
} // void TrendingTaskTPC::trendValues(uint64_t timestamp, repository::DatabaseInterface& qcdb)

void TrendingTaskTPC::generatePlots()
{
  if (mTrend->GetEntries() < 1) {
    ILOG(Info, Support) << "No entries in the trend so far, no plot generated." << ENDM;
    return;
  }

  ILOG(Info, Support) << "Generating " << mConfig.plots.size() << " plots." << ENDM;
  for (const auto& plot : mConfig.plots) {
    // Delete the existing plots before regenerating them.
    if (mPlots.count(plot.name)) {
      getObjectsManager()->stopPublishing(plot.name);
      delete mPlots[plot.name];
    }

    // Postprocess each pad (titles, axes, flushing buffers).
    const std::size_t posEndVar = plot.varexp.find("."); // Find the end of the dataSource.
    const std::string varName(plot.varexp.substr(0, posEndVar));

    // Draw the trending on a new canvas.
    TCanvas* c = new TCanvas();
    c->SetName(plot.name.c_str());
    c->SetTitle(plot.title.c_str());

    if (mIsMoObject[varName]) {
      drawCanvasMO(c, plot.varexp, plot.name, plot.option, plot.graphErrors, mAxisDivision[varName]);
    } else {
      drawCanvasQO(c, plot.varexp, plot.name, plot.option);
    }

    int NumberPlots = 1;
    if (plot.varexp.find(":time") != std::string::npos) { // we plot vs time, multiple plots on canvas possible
      NumberPlots = mSubtitles[varName].size();
    }
    for (int p = 0; p < NumberPlots; p++) {
      c->cd(p + 1);
      if (auto histo = dynamic_cast<TGraphErrors*>(c->cd(p + 1)->GetPrimitive("Graph"))) {

        // Set the title of the graph in a proper way.
        std::string thisTitle;
        if (plot.varexp.find(":time") != std::string::npos) {
          thisTitle = fmt::format("{0:s} - {1:s}", plot.title.data(), mSubtitles[varName][p].data()); // for plots vs time slicing might be applied for the title
        } else {
          thisTitle = fmt::format("{0:s}", plot.title.data());
        }
        histo->SetTitle(thisTitle.data());

        // Set the user-defined range on the y axis if needed.
        if (!plot.graphYRange.empty()) {
          float yMin, yMax;
          getUserAxisRange(plot.graphYRange, yMin, yMax);
          histo->SetMinimum(yMin);
          histo->SetMaximum(yMax);
          histo->Draw(plot.option.data()); // redraw and update to force changes on y-axis
          c->Update();
        }

        if (!plot.graphXRange.empty()) {
          float xMin, xMax;
          getUserAxisRange(plot.graphXRange, xMin, xMax);
          histo->GetXaxis()->SetLimits(xMin, xMax);
          histo->Draw(fmt::format("{0:s} A", plot.option.data()).data());
          c->Update();
        }

        if (!plot.graphAxisLabel.empty()) {
          setUserAxisLabel(histo->GetXaxis(), histo->GetYaxis(), plot.graphAxisLabel);
          histo->Draw(fmt::format("{0:s} A", plot.option.data()).data());
          c->Update();
        }

        // Configure the time for the x axis.
        if (plot.varexp.find(":time") != std::string::npos) {
          histo->GetXaxis()->SetTimeDisplay(1);
          histo->GetXaxis()->SetNdivisions(505);
          histo->GetXaxis()->SetTimeOffset(0.0);
          histo->GetXaxis()->SetTimeFormat("%Y-%m-%d %H:%M");
        }

        if (plot.varexp.find("quality") != std::string::npos) {
          histo->SetMinimum(-0.5);
          histo->SetMaximum(3.5);

          histo->GetYaxis()->Set(4, -0.5, 3.5);
          histo->GetYaxis()->SetNdivisions(3);
          histo->GetYaxis()->SetBinLabel(1, "No Quality");
          histo->GetYaxis()->SetBinLabel(2, "Good");
          histo->GetYaxis()->SetBinLabel(3, "Medium");
          histo->GetYaxis()->SetBinLabel(4, "Bad");
          histo->GetYaxis()->ChangeLabel(2, -1., -1., -1., kGreen + 2, -1, "Good");
          histo->GetYaxis()->ChangeLabel(3, -1., -1., -1., kOrange - 3, -1, "Medium");
          histo->GetYaxis()->ChangeLabel(4, -1., -1., -1., kRed, -1, "Bad");

          histo->Draw(fmt::format("{0:s} A", plot.option.data()).data());
          c->Update();
        }

        // Manually empty the buffers before visualising the plot.
        // histo->BufferEmpty(); // TBD: Should we keep it or not? Graph does not have this method.
      } else if (auto histo = dynamic_cast<TH2F*>(c->cd(p + 1)->GetPrimitive("Graph2D"))) {

        const std::string thisTitle = fmt::format("{0:s}", plot.title.data());
        histo->SetTitle(thisTitle.data());

        if (!plot.graphAxisLabel.empty()) {
          setUserAxisLabel(histo->GetXaxis(), histo->GetYaxis(), plot.graphAxisLabel);
          histo->Draw(plot.option.data());
          c->Update();
        }

        if (!plot.graphYRange.empty()) {
          float yMin, yMax;
          getUserAxisRange(plot.graphYRange, yMin, yMax);
          histo->SetMinimum(yMin);
          histo->SetMaximum(yMax);
          histo->Draw(plot.option.data()); // redraw and update to force changes on y-axis
          c->Update();
        }

        gStyle->SetPalette(kBird);
        histo->SetStats(kFALSE);
        histo->Draw(plot.option.data());

      } else {
        ILOG(Error, Devel) << "Could not get the 'Graph' of the plot '"
                           << plot.name << "'." << ENDM;
      }
    }

    mPlots[plot.name] = c;
    getObjectsManager()->startPublishing(c);
  }
} // void TrendingTaskTPC::generatePlots()

void TrendingTaskTPC::drawCanvasMO(TCanvas* thisCanvas, const std::string& var,
                                   const std::string& name, const std::string& opt, const std::string& err, const std::vector<std::vector<float>>& axis)
{
  // Determine the order of the plot (1 - histo, 2 - graph, ...)
  const size_t plotOrder = std::count(var.begin(), var.end(), ':') + 1;

  // Prepare the strings for the dataSource and its trending quantity.
  std::string varName, typeName, trendType;
  getTrendVariables(var, varName, typeName, trendType);

  std::string errXName, errYName;
  getTrendErrors(err, errXName, errYName);

  // Divide the canvas into the correct number of pads.
  if (trendType == "time") {
    thisCanvas->DivideSquare(mSubtitles[varName].size()); // trending vs time: multiple plots per canvas possible
  } else {
    thisCanvas->DivideSquare(1);
  }

  // Delete the graph errors after the plot is saved. //To-Do check if ownership is now taken
  // Unfortunately the canvas does not take its ownership.
  TGraphErrors* graphErrors = nullptr;

  // Setup the tree reader with the needed values.
  TTreeReader myReader(mTrend.get());
  TTreeReaderValue<UInt_t> RetrieveTime(myReader, "time");
  TTreeReaderValue<std::vector<SliceInfo>> DataRetrieveVector(myReader, varName.data());

  const int nuPa = mSubtitles[varName].size();
  const int nEntries = mTrend->GetEntriesFast();

  // Fill the graph(errors) to be published.
  if (trendType == "time") {

    for (int p = 0; p < nuPa; p++) {
      thisCanvas->cd(p + 1);
      int iEntry = 0;

      graphErrors = new TGraphErrors(nEntries);

      while (myReader.Next()) {
        const double timeStamp = (double)(*RetrieveTime);
        const double dataPoint = (DataRetrieveVector->at(p)).RetrieveValue(typeName);
        double errorX = 0.;
        double errorY = 0.;

        if (!err.empty()) {
          errorX = (DataRetrieveVector->at(p)).RetrieveValue(errXName);
          errorY = (DataRetrieveVector->at(p)).RetrieveValue(errYName);
        }

        graphErrors->SetPoint(iEntry, timeStamp, dataPoint);
        graphErrors->SetPointError(iEntry, errorX, errorY); // Add Error to the last added point

        iEntry++;
      }
      myReader.Restart();

      if (!err.empty()) {
        if (plotOrder != 2) {
          ILOG(Info, Support) << "Non empty graphErrors seen for the plot '" << name
                              << "', which is not a graph, ignoring." << ENDM;
        } else {
          graphErrors->Draw(opt.data());
          // We try to convince ROOT to delete graphErrors together with the rest of the canvas.
          if (auto* pad = thisCanvas->GetPad(p + 1)) {
            if (auto* primitives = pad->GetListOfPrimitives()) {
              primitives->Add(graphErrors); // TO-DO: Is this needed?
            }
          }
        }
      }
    }
  } else if (trendType == "slices") {

    graphErrors = new TGraphErrors(nuPa);
    thisCanvas->cd(1);

    myReader.SetEntry(nEntries - 1); // set event to last entry with index nEntries-1

    int iEntry = 0;
    for (int p = 0; p < nuPa; p++) {

      const double dataPoint = (DataRetrieveVector->at(p)).RetrieveValue(typeName);
      double errorX = 0.;
      double errorY = 0.;
      if (!err.empty()) {
        errorX = (DataRetrieveVector->at(p)).RetrieveValue(errXName);
        errorY = (DataRetrieveVector->at(p)).RetrieveValue(errYName);
      }
      const double xLabel = (DataRetrieveVector->at(p)).RetrieveValue("sliceLabelX");

      graphErrors->SetPoint(iEntry, xLabel, dataPoint);
      graphErrors->SetPointError(iEntry, errorX, errorY); // Add Error to the last added point

      iEntry++;
    }

    if (myReader.Next()) {
      ILOG(Error, Devel) << "Entry beyond expected last entry" << ENDM;
    }

    myReader.Restart();

    if (!err.empty()) {
      if (plotOrder != 2) {
        ILOG(Info, Support) << "Non empty graphErrors seen for the plot '" << name
                            << "', which is not a graph, ignoring." << ENDM;
      } else {
        graphErrors->Draw(opt.data());
        // We try to convince ROOT to delete graphErrors together with the rest of the canvas.
        if (auto* pad = thisCanvas->GetPad(1)) {
          if (auto* primitives = pad->GetListOfPrimitives()) {
            primitives->Add(graphErrors); // TO-DO: Is this needed?
          }
        }
      }
    }
  } // Trending vs Slices
  else if (trendType == "slices2D") {

    thisCanvas->cd(1);
    const int xBins = axis[0].size();
    float xBoundaries[xBins];
    for (int i = 0; i < xBins; i++) {
      xBoundaries[i] = axis[0][i];
    }
    const int yBins = axis[1].size();
    float yBoundaries[yBins];
    for (int i = 0; i < yBins; i++) {
      yBoundaries[i] = axis[1][i];
    }

    TH2F* graph2D = new TH2F("Graph2D", "", xBins - 1, xBoundaries, yBins - 1, yBoundaries);
    thisCanvas->cd(1);

    myReader.SetEntry(nEntries - 1); // set event to last entry with index nEntries-1

    int iEntry = 0;
    for (int p = 0; p < nuPa; p++) {

      const double dataPoint = (double)(DataRetrieveVector->at(p)).RetrieveValue(typeName);
      double error = 0.;
      if (!err.empty()) {
        error = (double)(DataRetrieveVector->at(p)).RetrieveValue(errYName);
      }
      const double xLabel = (double)(DataRetrieveVector->at(p)).RetrieveValue("sliceLabelX");
      const double yLabel = (double)(DataRetrieveVector->at(p)).RetrieveValue("sliceLabelY");

      graph2D->Fill(xLabel, yLabel, dataPoint);
      graph2D->SetBinError(graph2D->GetXaxis()->FindBin(xLabel), graph2D->GetYaxis()->FindBin(yLabel), error);

      iEntry++;
    }

    if (myReader.Next()) {
      ILOG(Error, Devel) << "Entry beyond expected last entry" << ENDM;
    }

    myReader.Restart();
    gStyle->SetPalette(kBird);
    graph2D->Draw(opt.data());
    // We try to convince ROOT to delete graphErrors together with the rest of the canvas.
    if (auto* pad = thisCanvas->GetPad(1)) {
      if (auto* primitives = pad->GetListOfPrimitives()) {
        primitives->Add(graph2D); // TO-DO: Is this needed?
      }
    }
  } // Trending vs Slices2D
}

void TrendingTaskTPC::drawCanvasQO(TCanvas* thisCanvas, const std::string& var,
                                   const std::string& name, const std::string& opt)
{
  // Determine the order of the plot (1 - histo, 2 - graph, ...)
  const size_t plotOrder = std::count(var.begin(), var.end(), ':') + 1;

  // Prepare the strings for the dataSource and its trending quantity.
  std::string varName, typeName, trendType;
  getTrendVariables(var, varName, typeName, trendType);

  // Divide the canvas into the correct number of pads.
  if (trendType != "time") {
    ILOG(Error, Devel) << "Error in trending of Quality Object  '" << name
                       << "'Trending only possible vs time, break." << ENDM;
  }
  thisCanvas->DivideSquare(1);

  // Delete the graph errors after the plot is saved. //To-Do check if ownership is now taken
  // Unfortunately the canvas does not take its ownership.
  TGraphErrors* graphErrors = nullptr;

  // Setup the tree reader with the needed values.
  TTreeReader myReader(mTrend.get());
  TTreeReaderValue<UInt_t> RetrieveTime(myReader, "time");
  TTreeReaderValue<SliceInfoQuality> QualityRetrieveVector(myReader, varName.data());

  if (mSubtitles[varName].size() != 1)
    ILOG(Error, Devel) << "Error in trending of Quality Object  '" << name
                       << "'Quality trending should not have slicing, break." << mSubtitles[varName].size() << ENDM;

  const int nEntries = mTrend->GetEntriesFast();
  const double errorX = 0.;
  const double errorY = 0.;

  int iEntry = 0;
  graphErrors = new TGraphErrors(nEntries);

  while (myReader.Next()) {
    const double timeStamp = (double)(*RetrieveTime);
    double dataPoint = 0.;

    dataPoint = QualityRetrieveVector->RetrieveValue(typeName);

    if (dataPoint < 1. || dataPoint > 3.) { // if quality is outside standard good, medium, bad -> set to 0
      dataPoint = 0.;
    }

    graphErrors->SetPoint(iEntry, timeStamp, dataPoint);
    graphErrors->SetPointError(iEntry, errorX, errorY); // Add Error to the last added point

    iEntry++;
  }
  myReader.Restart();

  if (plotOrder != 2) {
    ILOG(Info, Support) << "Non empty graphErrors seen for the plot '" << name
                        << "', which is not a graph, ignoring." << ENDM;
  } else {
    graphErrors->Draw(opt.data());
    // We try to convince ROOT to delete graphErrors together with the rest of the canvas.
    if (auto* pad = thisCanvas->GetPad(1)) {
      if (auto* primitives = pad->GetListOfPrimitives()) {
        primitives->Add(graphErrors); // TO-DO: Is this needed?
      }
    }
  }
}

void TrendingTaskTPC::getUserAxisRange(const std::string graphAxisRange, float& limitLow, float& limitUp)
{
  const std::size_t posDivider = graphAxisRange.find(":");
  const std::string minString(graphAxisRange.substr(0, posDivider));
  const std::string maxString(graphAxisRange.substr(posDivider + 1));

  limitLow = std::stof(minString);
  limitUp = std::stof(maxString);
}

void TrendingTaskTPC::setUserAxisLabel(TAxis* xAxis, TAxis* yAxis, const std::string graphAxisLabel)
{
  const std::size_t posDivider = graphAxisLabel.find(":");
  const std::string yLabel(graphAxisLabel.substr(0, posDivider));
  const std::string xLabel(graphAxisLabel.substr(posDivider + 1));

  xAxis->SetTitle(xLabel.data());
  yAxis->SetTitle(yLabel.data());
}

void TrendingTaskTPC::getTrendVariables(const std::string& inputvar, std::string& sourceName, std::string& variableName, std::string& trend)
{
  const std::size_t posEndVar = inputvar.find(".");  // Find the end of the dataSource.
  const std::size_t posEndType = inputvar.find(":"); // Find the end of the quantity.
  sourceName = inputvar.substr(0, posEndVar);
  variableName = inputvar.substr(posEndVar + 1, posEndType - posEndVar - 1);
  trend = inputvar.substr(posEndType + 1, -1);
}

void TrendingTaskTPC::getTrendErrors(const std::string& inputvar, std::string& errorX, std::string& errorY)
{
  const std::size_t posEndType_err = inputvar.find(":"); // Find the end of the error.
  errorX = inputvar.substr(posEndType_err + 1);
  errorY = inputvar.substr(0, posEndType_err);
}
