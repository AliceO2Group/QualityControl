// Copyright 2019-2026 CERN and copyright holders of ALICE O2.
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
/// \file   testTrendingMachine.cxx
/// \brief  Tests of the TrendingMachine: trend filling, plot generation (incl. customization), trend continuation.
///
/// The tests do not need any database. They use the TH1Reductor from the QcCommon module, so the module has to be
/// loadable at runtime, as in testTrendingTask.
///

#include "QualityControl/TrendingMachine.h"
#include "QualityControl/TrendingMachineConfig.h"
#include "QualityControl/Reductor.h"
#include "QualityControl/ReductorTObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QualityObject.h"

#include <TCanvas.h>
#include <TColor.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TH1.h>
#include <TH1D.h>
#include <TAxis.h>
#include <TLegend.h>
#include <TList.h>
#include <TROOT.h>
#include <TStyle.h>
#include <TTree.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <catch_amalgamated.hpp>

using namespace o2::quality_control::postprocessing;
using Catch::Approx;
using o2::quality_control::core::Quality;
using o2::quality_control::core::QualityObject;

namespace
{

constexpr UInt_t T0 = 1'700'000'000;
constexpr UInt_t TStep = 3600;
constexpr int Run0 = 100;
const std::vector<double> Means = { 10., 20., 30., 40. };

TrendingMachineConfig::DataSource makeSource(const std::string& name = "example")
{
  TrendingMachineConfig::DataSource ds;
  ds.type = "repository";
  ds.path = "TST/MO/QcTask";
  ds.name = name;
  ds.reductorName = "o2::quality_control_modules::common::TH1Reductor";
  ds.moduleName = "QcCommon";
  return ds;
}

TrendingMachineConfig::Graph makeGraph(const std::string& name, const std::string& varexp, const std::string& option = "*L")
{
  TrendingMachineConfig::Graph g;
  g.name = name;
  g.title = name + " title";
  g.varexp = varexp;
  g.option = option;
  return g;
}

TrendingMachineConfig::Plot makePlot(const std::string& name, std::vector<TrendingMachineConfig::Graph> graphs)
{
  TrendingMachineConfig::Plot p;
  p.name = name;
  p.title = name + " plot title";
  p.graphs = std::move(graphs);
  return p;
}

TrendingMachineConfig makeConfig(std::vector<TrendingMachineConfig::Plot> plots = {},
                                 std::vector<TrendingMachineConfig::DataSource> sources = { makeSource() })
{
  TrendingMachineConfig config;
  config.plots = std::move(plots);
  config.dataSources = std::move(sources);
  return config;
}

// a histogram with a known mean, stddev == 1 and two entries
std::unique_ptr<TH1D> makeHisto(double mean, const std::string& name = "example")
{
  auto h = std::make_unique<TH1D>(name.c_str(), name.c_str(), 200, 0, 200);
  h->SetDirectory(nullptr);
  h->Fill(mean - 1);
  h->Fill(mean + 1);
  return h;
}

// updates the reductors of the data sources with the provided objects, indexed by the data source name
bool updateReductors(TrendingMachine& machine, const std::map<std::string, TObject*>& objects)
{
  return machine.updateReductors([&](const TrendingMachineConfig::DataSource& dataSource, Reductor& reductor) {
    auto it = objects.find(dataSource.name);
    auto* reductorTObject = dynamic_cast<ReductorTObject*>(&reductor);
    if (it == objects.end() || it->second == nullptr || reductorTObject == nullptr) {
      return false;
    }
    reductorTObject->update(it->second);
    return true;
  });
}

void fillEntry(TrendingMachine& machine, size_t i, const std::string& source = "example")
{
  auto histo = makeHisto(Means[i], source);
  machine.setMetadata(T0 + i * TStep, Run0 + i);
  REQUIRE(updateReductors(machine, { { source, histo.get() } }));
  machine.fillTrend();
}

void fillTrend(TrendingMachine& machine, size_t n = Means.size())
{
  for (size_t i = 0; i < n; i++) {
    fillEntry(machine, i);
  }
}

std::vector<double> getColumn(TTree& tree, const std::string& varexp, const std::string& selection = "")
{
  // "goff" skips the graphics, the values are then available via GetV1(), see the TTree::Draw documentation, section
  // "How to obtain more info from TTree::Draw": https://root.cern/doc/master/classTTree.html
  // V1 in GetV1 is the first part/dimension in varexp (V1:V2), not "Version 1", as I'd thought personally.
  const auto n = tree.Draw(varexp.c_str(), selection.c_str(), "goff");
  return std::vector<double>(tree.GetV1(), tree.GetV1() + n);
}

using CanvasPtr = std::unique_ptr<TCanvas>;

// builds a trending machine, fills the trend and returns the (single) canvas generated for the plot
struct PlotFixture {
  explicit PlotFixture(TrendingMachineConfig::Plot plot, size_t nEntries = Means.size())
    : machine(makeConfig({ std::move(plot) }))
  {
    machine.initializeTrend("trend");
    fillTrend(machine, nEntries);
    auto canvases = machine.generatePlots();
    REQUIRE(canvases.size() == 1);
    canvas = std::move(canvases[0]);
  }

  template <typename T = TObject>
  T* find(const std::string& name) const
  {
    return dynamic_cast<T*>(canvas->FindObject(name.c_str()));
  }

  TH1* background() const { return find<TH1>("background"); }

  TLegend* legend() const
  {
    for (auto* primitive : *canvas->GetListOfPrimitives()) {
      if (auto* leg = dynamic_cast<TLegend*>(primitive)) {
        return leg;
      }
    }
    return nullptr;
  }

  TrendingMachine machine;
  CanvasPtr canvas;
};

// TCanvas objects are created in the tests. In batch mode ROOT does not try to open any graphics window, so the tests
// work also where there is no display (e.g. CI). This has to be set before the first TCanvas is created, hence it is
// done at static initialization.
struct BatchMode {
  BatchMode() { gROOT->SetBatch(true); }
};
const BatchMode batchMode;

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// Construction and trend filling
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("trending_machine_construction")
{
  SECTION("reductors are created for each data source")
  {
    TrendingMachine machine(makeConfig({}, { makeSource("a"), makeSource("b") }));
    REQUIRE(machine.getReductor("a") != nullptr);
    REQUIRE(machine.getReductor("b") != nullptr);
    REQUIRE(machine.getReductor("a") != machine.getReductor("b"));
    REQUIRE(machine.getReductor("c") == nullptr);
  }

  SECTION("an unknown reductor results in an exception")
  {
    auto source = makeSource();
    source.reductorName = "o2::quality_control_modules::common::ThisReductorDoesNotExist";
    REQUIRE_THROWS(TrendingMachine(makeConfig({}, { source })));
  }

  SECTION("an unknown module results in an exception")
  {
    auto source = makeSource();
    source.moduleName = "ThisModuleDoesNotExist";
    REQUIRE_THROWS(TrendingMachine(makeConfig({}, { source })));
  }
}

TEST_CASE("trending_machine_trend_structure")
{
  TrendingMachine machine(makeConfig({}, { makeSource("a"), makeSource("b") }));
  REQUIRE(machine.getTrend() == nullptr);

  machine.initializeTrend("my_trend");
  auto* trend = machine.getTrend();
  REQUIRE(trend != nullptr);
  REQUIRE(std::string(trend->GetName()) == "my_trend");
  REQUIRE(trend->GetEntries() == 0);
  // meta + time + one for each data source
  REQUIRE(trend->GetNbranches() == 4);
  REQUIRE(trend->GetBranch("meta") != nullptr);
  REQUIRE(trend->GetBranch("time") != nullptr);
  REQUIRE(trend->GetBranch("a") != nullptr);
  REQUIRE(trend->GetBranch("b") != nullptr);
}

TEST_CASE("trending_machine_filling")
{
  TrendingMachine machine(makeConfig());
  machine.initializeTrend("trend");

  SECTION("values, timestamps and run numbers end up in the trend")
  {
    fillTrend(machine);
    auto* trend = machine.getTrend();
    REQUIRE(trend->GetEntries() == 4);

    REQUIRE(getColumn(*trend, "example.mean") == Means);
    REQUIRE(getColumn(*trend, "example.entries") == std::vector<double>(4, 2.));
    for (double stddev : getColumn(*trend, "example.stddev")) {
      REQUIRE(stddev == Approx(1.));
    }

    const auto times = getColumn(*trend, "time");
    const auto runs = getColumn(*trend, "meta.runNumber");
    REQUIRE(times.size() == 4);
    REQUIRE(runs.size() == 4);
    for (size_t i = 0; i < 4; i++) {
      REQUIRE(times[i] == T0 + i * TStep);
      REQUIRE(runs[i] == Run0 + i);
    }

    // the run number is available also as a string, for evenly spaced trends against run numbers
    REQUIRE(trend->GetEntries("meta.runNumberStr==\"100\"") == 1);
    REQUIRE(trend->GetEntries("meta.runNumberStr==\"103\"") == 1);
  }

  SECTION("updateReductors calls the updater for each data source, with the matching reductor")
  {
    TrendingMachine twoSources(makeConfig({}, { makeSource("a"), makeSource("b") }));
    twoSources.initializeTrend("trend");
    std::vector<std::string> visited;
    const bool all = twoSources.updateReductors([&](const TrendingMachineConfig::DataSource& ds, Reductor& reductor) {
      visited.push_back(ds.name);
      REQUIRE(&reductor == twoSources.getReductor(ds.name));
      return true;
    });
    REQUIRE(all);
    REQUIRE(visited == std::vector<std::string>({ "a", "b" }));
  }

  SECTION("updateReductors reports a failure of any source, but still tries all of them")
  {
    TrendingMachine twoSources(makeConfig({}, { makeSource("a"), makeSource("b") }));
    twoSources.initializeTrend("trend");
    size_t calls = 0;
    const bool all = twoSources.updateReductors([&](const TrendingMachineConfig::DataSource& ds, Reductor&) {
      calls++;
      return ds.name != "a";
    });
    REQUIRE_FALSE(all);
    REQUIRE(calls == 2);
  }

  SECTION("updateReductors can fill the trend, as the pull-style callers do")
  {
    machine.setMetadata(T0, Run0);
    auto histo = makeHisto(Means[2]);
    REQUIRE(machine.updateReductors([&](const TrendingMachineConfig::DataSource&, Reductor& reductor) {
      static_cast<ReductorTObject&>(reductor).update(histo.get());
      return true;
    }));
    machine.fillTrend();
    REQUIRE(getColumn(*machine.getTrend(), "example.mean") == std::vector<double>({ Means[2] }));
  }

  SECTION("a source which was not updated keeps its previous values")
  {
    fillEntry(machine, 1);
    // no updateReductors() this time
    machine.setMetadata(T0 + TStep, Run0 + 1);
    machine.fillTrend();
    REQUIRE(getColumn(*machine.getTrend(), "example.mean") == std::vector<double>({ Means[1], Means[1] }));
  }

  SECTION("filling a not initialized machine does not crash")
  {
    TrendingMachine notInitialized(makeConfig());
    REQUIRE_NOTHROW(notInitialized.fillTrend());
    REQUIRE(notInitialized.getTrend() == nullptr);
  }

  SECTION("reset removes the entries, but keeps the structure and allows to fill again")
  {
    fillTrend(machine);
    machine.reset();
    REQUIRE(machine.getTrend()->GetEntries() == 0);
    REQUIRE(machine.getTrend()->GetNbranches() == 3);
    fillEntry(machine, 2);
    REQUIRE(getColumn(*machine.getTrend(), "example.mean") == std::vector<double>({ Means[2] }));
  }

  SECTION("reset of a not initialized trending machine does not crash")
  {
    TrendingMachine notInitialized(makeConfig());
    REQUIRE_NOTHROW(notInitialized.reset());
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// Trend continuation
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("trending_machine_trend_continuation")
{
  // prepare an existing tree, as if it was retrieved from a database
  TrendingMachine original(makeConfig());
  original.initializeTrend("trend");
  fillTrend(original);
  std::unique_ptr<TTree> existing(original.getTrend()->CloneTree());
  REQUIRE(existing->GetEntries() == 4);

  SECTION("an identical structure is continued, existing entries are kept and new ones appended")
  {
    TrendingMachine machine(makeConfig());
    REQUIRE(machine.initializeTrend("new_trend", std::move(existing)));
    REQUIRE(machine.getTrend()->GetEntries() == 4);
    REQUIRE(std::string(machine.getTrend()->GetName()) == "trend"); // the name of the existing tree is preserved

    fillEntry(machine, 0);
    REQUIRE(machine.getTrend()->GetEntries() == 5);
    REQUIRE(getColumn(*machine.getTrend(), "example.mean") == std::vector<double>({ 10., 20., 30., 40., 10. }));
  }

  SECTION("an additional data source starts a new trend")
  {
    TrendingMachine machine(makeConfig({}, { makeSource("example"), makeSource("other") }));
    REQUIRE_FALSE(machine.initializeTrend("new_trend", std::move(existing)));
    REQUIRE(machine.getTrend() != nullptr);
    REQUIRE(machine.getTrend()->GetEntries() == 0);
    REQUIRE(std::string(machine.getTrend()->GetName()) == "new_trend");
    REQUIRE(machine.getTrend()->GetNbranches() == 4);
  }

  SECTION("a missing data source starts a new trend")
  {
    TrendingMachine machine(makeConfig({}, {}));
    REQUIRE_FALSE(machine.initializeTrend("new_trend", std::move(existing)));
    REQUIRE(machine.getTrend()->GetEntries() == 0);
    REQUIRE(machine.getTrend()->GetNbranches() == 2);
  }

  SECTION("the same number of branches, but with a different name starts a new trend")
  {
    TrendingMachine machine(makeConfig({}, { makeSource("renamed") }));
    REQUIRE_FALSE(machine.initializeTrend("new_trend", std::move(existing)));
    REQUIRE(machine.getTrend()->GetEntries() == 0);
    REQUIRE(machine.getTrend()->GetBranch("renamed") != nullptr);
    REQUIRE(machine.getTrend()->GetBranch("example") == nullptr);
  }

  SECTION("no existing tree starts a new trend")
  {
    TrendingMachine machine(makeConfig());
    REQUIRE_FALSE(machine.initializeTrend("new_trend", nullptr));
    REQUIRE(machine.getTrend() != nullptr);
    REQUIRE(machine.getTrend()->GetEntries() == 0);
  }
}

// ---------------------------------------------------------------------------------------------------------------------
// Plot generation
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("trending_machine_plots_generation")
{
  SECTION("nothing is generated without a trend")
  {
    TrendingMachine machine(makeConfig({ makePlot("p", { makeGraph("g", "example.mean:time") }) }));
    REQUIRE(machine.generatePlots().empty());
  }

  SECTION("nothing is generated for an empty trend")
  {
    TrendingMachine machine(makeConfig({ makePlot("p", { makeGraph("g", "example.mean:time") }) }));
    machine.initializeTrend("trend");
    REQUIRE(machine.generatePlots().empty());
  }

  SECTION("one canvas per configured plot, with name and title")
  {
    TrendingMachine machine(makeConfig({ makePlot("first", { makeGraph("g1", "example.mean:time") }),
                                         makePlot("second", { makeGraph("g2", "example.stddev:time") }),
                                         makePlot("third", { makeGraph("g3", "example.mean") }) }));
    machine.initializeTrend("trend");
    fillTrend(machine);
    auto canvases = machine.generatePlots();
    REQUIRE(canvases.size() == 3);
    const std::vector<std::string> names = { "first", "second", "third" };
    for (size_t i = 0; i < 3; i++) {
      REQUIRE(std::string(canvases[i]->GetName()) == names[i]);
      REQUIRE(std::string(canvases[i]->GetTitle()) == names[i] + " plot title");
    }
  }

  SECTION("no plots configured means no canvases")
  {
    TrendingMachine machine(makeConfig());
    machine.initializeTrend("trend");
    fillTrend(machine);
    REQUIRE(machine.generatePlots().empty());
  }

  SECTION("plots can be generated repeatedly as the trend grows")
  {
    TrendingMachine machine(makeConfig({ makePlot("p", { makeGraph("g", "example.mean:time") }) }));
    machine.initializeTrend("trend");
    fillEntry(machine, 0);
    {
      auto canvases = machine.generatePlots();
      REQUIRE(canvases.size() == 1);
      REQUIRE(dynamic_cast<TGraph*>(canvases[0]->FindObject("g"))->GetN() == 1);
    }
    fillEntry(machine, 1);
    {
      auto canvases = machine.generatePlots();
      REQUIRE(canvases.size() == 1);
      REQUIRE(dynamic_cast<TGraph*>(canvases[0]->FindObject("g"))->GetN() == 2);
    }
  }
}

TEST_CASE("trending_machine_plot_graph")
{
  auto graph = makeGraph("g", "example.mean:time");
  PlotFixture fixture(makePlot("p", { graph }));

  SECTION("a graph with the data points from the trend, name and title")
  {
    auto* tgraph = fixture.find<TGraph>("g");
    REQUIRE(tgraph != nullptr);
    REQUIRE(std::string(tgraph->GetTitle()) == "g title");
    REQUIRE(tgraph->GetN() == 4);
    for (int i = 0; i < 4; i++) {
      REQUIRE(tgraph->GetPointY(i) == Approx(Means[i]));
      REQUIRE(tgraph->GetPointX(i) == Approx(T0 + i * TStep));
    }
  }

  SECTION("the title of the plot is applied to the axes frame")
  {
    REQUIRE(fixture.background() != nullptr);
    REQUIRE(std::string(fixture.background()->GetTitle()) == "p plot title");
  }

  SECTION("a plot with a single graph does not get a legend")
  {
    REQUIRE(fixture.legend() == nullptr);
  }

  SECTION("a time axis is used for ':time' plots")
  {
    REQUIRE(fixture.background()->GetXaxis()->GetTimeDisplay());
  }
}

TEST_CASE("trending_machine_plot_x_axis_types")
{
  SECTION("a run number axis does not use the time display and exponents")
  {
    PlotFixture fixture(makePlot("p", { makeGraph("g", "example.mean:meta.runNumber") }));
    REQUIRE_FALSE(fixture.background()->GetXaxis()->GetTimeDisplay());
    REQUIRE(fixture.background()->GetXaxis()->GetNoExponent());
  }

  SECTION("a correlation plot uses neither")
  {
    PlotFixture fixture(makePlot("p", { makeGraph("g", "example.mean:example.entries") }));
    REQUIRE_FALSE(fixture.background()->GetXaxis()->GetTimeDisplay());
    REQUIRE_FALSE(fixture.background()->GetXaxis()->GetNoExponent());
  }
}

TEST_CASE("trending_machine_plot_axes_customization")
{
  SECTION("axis labels: yLabel:xLabel")
  {
    auto plot = makePlot("p", { makeGraph("g", "example.mean:time") });
    plot.graphAxisLabel = "the y label:the x label";
    PlotFixture fixture(plot);
    REQUIRE(std::string(fixture.background()->GetYaxis()->GetTitle()) == "the y label");
    REQUIRE(std::string(fixture.background()->GetXaxis()->GetTitle()) == "the x label");
  }

  SECTION("malformed axis labels are ignored")
  {
    // without a user label, TTree::Draw sets the axis titles to the drawn expressions
    PlotFixture reference(makePlot("p", { makeGraph("g", "example.mean:time") }));
    auto plot = makePlot("p", { makeGraph("g", "example.mean:time") });
    plot.graphAxisLabel = "too:many:colons";
    PlotFixture fixture(plot);
    REQUIRE(std::string(fixture.background()->GetYaxis()->GetTitle()) == reference.background()->GetYaxis()->GetTitle());
    REQUIRE(std::string(fixture.background()->GetXaxis()->GetTitle()) == reference.background()->GetXaxis()->GetTitle());
  }

  SECTION("y axis range")
  {
    auto plot = makePlot("p", { makeGraph("g", "example.mean:time") });
    plot.graphYRange = "-5:100";
    PlotFixture fixture(plot);
    REQUIRE(fixture.background()->GetYaxis()->GetXmin() == Approx(-5.));
    REQUIRE(fixture.background()->GetYaxis()->GetXmax() == Approx(100.));
  }

  SECTION("malformed y axis range is ignored and does not throw")
  {
    PlotFixture reference(makePlot("p", { makeGraph("g", "example.mean:time") }));
    for (const std::string range : { "0:1:2", "abc:def", "0:", ":10", "10", "5:5", "10:0", "1e999:2e999" }) {
      CAPTURE(range);
      auto plot = makePlot("p", { makeGraph("g", "example.mean:time") });
      plot.graphYRange = range;
      std::unique_ptr<PlotFixture> fixture;
      REQUIRE_NOTHROW(fixture = std::make_unique<PlotFixture>(plot));
      REQUIRE(fixture->background()->GetYaxis()->GetXmin() == Approx(reference.background()->GetYaxis()->GetXmin()));
      REQUIRE(fixture->background()->GetYaxis()->GetXmax() == Approx(reference.background()->GetYaxis()->GetXmax()));
    }
  }
}

TEST_CASE("trending_machine_plot_graph_selection")
{
  auto graph = makeGraph("g", "example.mean:time");
  graph.selection = "example.mean>15";
  PlotFixture fixture(makePlot("p", { graph }));
  auto* tgraph = fixture.find<TGraph>("g");
  REQUIRE(tgraph != nullptr);
  REQUIRE(tgraph->GetN() == 3);
  REQUIRE(tgraph->GetPointY(0) == Approx(20.));
}

TEST_CASE("trending_machine_plot_graph_style")
{
  SECTION("explicit style is applied to the graph")
  {
    auto styled = makeGraph("styled", "example.mean:time", "LP");
    styled.style.lineColor = kRed;
    styled.style.lineStyle = 2;
    styled.style.lineWidth = 3;
    styled.style.markerColor = kBlue;
    styled.style.markerStyle = 21;
    styled.style.markerSize = 1.5f;
    styled.style.fillColor = kGreen;
    styled.style.fillStyle = 3001;
    auto plain = makeGraph("plain", "example.mean + 5:time", "LP");

    PlotFixture fixture(makePlot("p", { styled, plain }));
    auto* tgraph = fixture.find<TGraph>("styled");
    REQUIRE(tgraph != nullptr);
    REQUIRE(tgraph->GetLineColor() == kRed);
    REQUIRE(tgraph->GetLineStyle() == 2);
    REQUIRE(tgraph->GetLineWidth() == 3);
    REQUIRE(tgraph->GetMarkerColor() == kBlue);
    REQUIRE(tgraph->GetMarkerStyle() == 21);
    REQUIRE(tgraph->GetMarkerSize() == Approx(1.5));
    REQUIRE(tgraph->GetFillColor() == kGreen);
    REQUIRE(tgraph->GetFillStyle() == 3001);
  }

  SECTION("style parameters which are not set do not override the defaults")
  {
    auto graph = makeGraph("styled", "example.mean:time", "LP");
    graph.style.markerStyle = 22; // the only one set
    PlotFixture reference(makePlot("p", { makeGraph("g", "example.mean:time", "LP") }));
    PlotFixture fixture(makePlot("p", { graph }));
    auto* tgraph = fixture.find<TGraph>("styled");
    auto* refGraph = reference.find<TGraph>("g");
    REQUIRE(tgraph != nullptr);
    REQUIRE(refGraph != nullptr);
    REQUIRE(tgraph->GetMarkerStyle() == 22);
    REQUIRE(tgraph->GetMarkerColor() == refGraph->GetMarkerColor());
    REQUIRE(tgraph->GetLineColor() == refGraph->GetLineColor());
    REQUIRE(tgraph->GetLineWidth() == refGraph->GetLineWidth());
    REQUIRE(tgraph->GetMarkerSize() == Approx(refGraph->GetMarkerSize()));
  }
}

TEST_CASE("trending_machine_plot_graph_errors")
{
  SECTION("error bars are drawn as a separate graph with the same style")
  {
    auto graph = makeGraph("g", "example.mean:time", "P");
    graph.errors = "1:2";
    graph.style.markerColor = kMagenta;
    graph.style.lineColor = kMagenta;
    PlotFixture fixture(makePlot("p", { graph }));

    auto* main = fixture.find<TGraph>("g");
    auto* errors = fixture.find<TGraphErrors>("g_errors");
    REQUIRE(main != nullptr);
    REQUIRE(errors != nullptr);
    REQUIRE(errors->GetN() == 4);
    for (int i = 0; i < 4; i++) {
      REQUIRE(errors->GetPointY(i) == Approx(Means[i]));
      REQUIRE(errors->GetErrorX(i) == Approx(1.));
      REQUIRE(errors->GetErrorY(i) == Approx(2.));
    }
    REQUIRE(errors->GetMarkerColor() == kMagenta);
    REQUIRE(errors->GetLineColor() == kMagenta);
  }

  SECTION("errors can be expressions of the trend branches")
  {
    auto graph = makeGraph("g", "example.mean:time", "P");
    graph.errors = "1:example.mean*0.5";
    PlotFixture fixture(makePlot("p", { graph }));
    auto* errors = fixture.find<TGraphErrors>("g_errors");
    REQUIRE(errors != nullptr);
    for (int i = 0; i < 4; i++) {
      REQUIRE(errors->GetErrorY(i) == Approx(Means[i] * 0.5));
    }
  }

  SECTION("errors are ignored for histograms")
  {
    auto graph = makeGraph("h", "example.mean", "");
    graph.errors = "1:2";
    PlotFixture fixture(makePlot("p", { graph }));
    REQUIRE(fixture.find("h_errors") == nullptr);
    REQUIRE(fixture.find<TH1>("h") != nullptr);
  }
}

TEST_CASE("trending_machine_plot_histogram")
{
  PlotFixture fixture(makePlot("p", { makeGraph("h", "example.mean", "") }));
  auto* histo = fixture.find<TH1>("h");
  REQUIRE(histo != nullptr);
  // the plot title is what gets displayed on top of the pad, also for plots with histograms
  REQUIRE(std::string(histo->GetTitle()) == "p plot title");
  REQUIRE(histo->GetEntries() == 4);
  REQUIRE(histo->GetMean() == Approx(25.));
}

TEST_CASE("trending_machine_plot_multiple_graphs")
{
  auto first = makeGraph("first", "example.mean:time", "LP");
  auto second = makeGraph("second", "example.mean + 10:time", "LP");
  auto third = makeGraph("third", "example.mean + 20:time", "LP");

  SECTION("all graphs are drawn on the same canvas and listed in the legend")
  {
    PlotFixture fixture(makePlot("p", { first, second, third }));
    for (const char* name : { "first", "second", "third" }) {
      REQUIRE(fixture.find<TGraph>(name) != nullptr);
    }
    REQUIRE(fixture.find<TGraph>("second")->GetPointY(0) == Approx(Means[0] + 10));
    REQUIRE(fixture.find<TGraph>("third")->GetPointY(0) == Approx(Means[0] + 20));

    auto* legend = fixture.legend();
    REQUIRE(legend != nullptr);
    REQUIRE(legend->GetListOfPrimitives()->GetSize() == 3);
  }

  SECTION("the legend placement and number of columns can be customized (NDC)")
  {
    auto plot = makePlot("p", { first, second, third });
    plot.legend.nColumns = 3;
    plot.legend.x1 = 0.11f;
    plot.legend.y1 = 0.72f;
    plot.legend.x2 = 0.55f;
    plot.legend.y2 = 0.88f;
    PlotFixture fixture(plot);
    auto* legend = fixture.legend();
    REQUIRE(legend != nullptr);
    REQUIRE(legend->GetNColumns() == 3);
    REQUIRE(legend->GetX1NDC() == Approx(0.11));
    REQUIRE(legend->GetY1NDC() == Approx(0.72));
    REQUIRE(legend->GetX2NDC() == Approx(0.55));
    REQUIRE(legend->GetY2NDC() == Approx(0.88));
  }

  SECTION("the legend is transparent and has no border")
  {
    PlotFixture fixture(makePlot("p", { first, second }));
    auto* legend = fixture.legend();
    REQUIRE(legend != nullptr);
    REQUIRE(legend->GetBorderSize() == 0);
    REQUIRE(legend->GetFillStyle() == 0);
  }
}

TEST_CASE("trending_machine_plot_color_palette")
{
  auto first = makeGraph("first", "example.mean:time", "LP PLC PMC");
  auto second = makeGraph("second", "example.mean + 10:time", "LP PLC PMC");
  auto third = makeGraph("third", "example.mean + 20:time", "LP PLC PMC");

  auto plot = makePlot("p", { first, second, third });
  plot.colorPalette = kViridis;
  PlotFixture fixture(plot);

  // with PLC/PMC, each graph should get a different color out of the selected palette
  const auto c1 = fixture.find<TGraph>("first")->GetLineColor();
  const auto c2 = fixture.find<TGraph>("second")->GetLineColor();
  const auto c3 = fixture.find<TGraph>("third")->GetLineColor();
  REQUIRE(c1 != c2);
  REQUIRE(c2 != c3);
  REQUIRE(c1 != c3);
  REQUIRE(fixture.find<TGraph>("first")->GetMarkerColor() == c1);
}

// ---------------------------------------------------------------------------------------------------------------------
// Trending of qualities ("repository-quality" data sources)
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("trending_machine_quality_source")
{
  auto qualitySource = makeSource("QcCheck");
  qualitySource.type = "repository-quality";
  qualitySource.path = "TST/QO";
  qualitySource.reductorName = "o2::quality_control_modules::common::QualityReductor";

  auto graph = makeGraph("quality", "QcCheck.level:time", "*");
  TrendingMachine machine(makeConfig({ makePlot("p", { graph }) }, { makeSource(), qualitySource }));
  machine.initializeTrend("trend");
  REQUIRE(machine.getTrend()->GetNbranches() == 4);
  REQUIRE(machine.getReductor("QcCheck") != nullptr);

  const std::vector<Quality> qualities = { Quality::Good, Quality::Medium, Quality::Bad, Quality::Null };
  for (size_t i = 0; i < qualities.size(); i++) {
    QualityObject qo(qualities[i], "QcCheck", "TST");
    auto histo = makeHisto(Means[i]);
    machine.setMetadata(T0 + i * TStep, Run0 + i);
    REQUIRE(updateReductors(machine, { { "example", histo.get() }, { "QcCheck", &qo } }));
    machine.fillTrend();
  }

  auto* trend = machine.getTrend();
  REQUIRE(trend->GetEntries() == 4);

  SECTION("quality levels and names are trended next to other sources")
  {
    const auto levels = getColumn(*trend, "QcCheck.level");
    REQUIRE(levels.size() == 4);
    for (size_t i = 0; i < qualities.size(); i++) {
      REQUIRE(levels[i] == qualities[i].getLevel());
    }
    REQUIRE(trend->GetEntries("QcCheck.name==\"Good\"") == 1);
    REQUIRE(trend->GetEntries("QcCheck.name==\"Medium\"") == 1);
    REQUIRE(trend->GetEntries("QcCheck.name==\"Bad\"") == 1);
    REQUIRE(trend->GetEntries("QcCheck.name==\"Null\"") == 1);
    REQUIRE(getColumn(*trend, "example.mean") == Means);
  }

  SECTION("a quality trend can be plotted")
  {
    auto canvases = machine.generatePlots();
    REQUIRE(canvases.size() == 1);
    auto* tgraph = dynamic_cast<TGraph*>(canvases[0]->FindObject("quality"));
    REQUIRE(tgraph != nullptr);
    REQUIRE(tgraph->GetN() == 4);
    REQUIRE(tgraph->GetPointY(0) == Approx(Quality::Good.getLevel()));
    REQUIRE(tgraph->GetPointY(2) == Approx(Quality::Bad.getLevel()));
  }

  SECTION("an object of a wrong type does not change the quality values")
  {
    auto histo = makeHisto(1);
    // the reductor is called, but it ignores the object
    REQUIRE(updateReductors(machine, { { "example", histo.get() }, { "QcCheck", histo.get() } }));
    machine.setMetadata(T0 + 10 * TStep, Run0 + 10);
    machine.fillTrend();
    const auto levels = getColumn(*trend, "QcCheck.level");
    REQUIRE(levels.size() == 5);
    REQUIRE(levels[4] == levels[3]);
  }
}
