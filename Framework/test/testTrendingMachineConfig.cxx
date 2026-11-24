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
/// \file   testTrendingMachineConfig.cxx
/// \brief  Tests of the parsing of TrendingMachineConfig and its use in TrendingTaskConfig.
///

#include "QualityControl/TrendingMachineConfig.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sstream>

#include <catch_amalgamated.hpp>

using namespace o2::quality_control::postprocessing;
using Catch::Approx;

namespace
{
boost::property_tree::ptree parse(const std::string& json)
{
  boost::property_tree::ptree tree;
  std::stringstream ss(json);
  boost::property_tree::read_json(ss, tree);
  return tree;
}
} // namespace

TEST_CASE("trending_machine_config_data_sources")
{
  SECTION("a single name, with defaults")
  {
    auto tree = parse(R"json([{
      "name": "example",
      "reductorName": "my::Reductor",
      "moduleName": "MyModule"
    }])json");
    auto sources = TrendingMachineConfig::parseDataSources(tree);
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].name == "example");
    REQUIRE(sources[0].type == "repository");
    REQUIRE(sources[0].path.empty());
    REQUIRE(sources[0].reductorName == "my::Reductor");
    REQUIRE(sources[0].moduleName == "MyModule");
    REQUIRE_FALSE(sources[0].reductorParameters.atOptional("key").has_value());
  }

  SECTION("a list of names is expanded into separate sources which share the rest of the configuration")
  {
    auto tree = parse(R"json([{
      "type": "repository-quality",
      "path": "TST/QO",
      "names": [ "a", "b", "c" ],
      "reductorName": "my::Reductor",
      "moduleName": "MyModule",
      "reductorParameters": { "default": { "default": { "key": "value" } } }
    }])json");
    auto sources = TrendingMachineConfig::parseDataSources(tree);
    REQUIRE(sources.size() == 3);
    const std::vector<std::string> names = { "a", "b", "c" };
    for (size_t i = 0; i < 3; i++) {
      REQUIRE(sources[i].name == names[i]);
      REQUIRE(sources[i].type == "repository-quality");
      REQUIRE(sources[i].path == "TST/QO");
      REQUIRE(sources[i].reductorName == "my::Reductor");
      REQUIRE(sources[i].reductorParameters.atOptional("key").value_or("") == "value");
    }
  }

  SECTION("neither name nor names")
  {
    auto tree = parse(R"json([{ "reductorName": "r", "moduleName": "m" }])json");
    REQUIRE_THROWS(TrendingMachineConfig::parseDataSources(tree));
  }

  SECTION("an empty name")
  {
    auto tree = parse(R"json([{ "name": "", "reductorName": "r", "moduleName": "m" }])json");
    REQUIRE_THROWS(TrendingMachineConfig::parseDataSources(tree));
  }

  SECTION("a missing reductor or module")
  {
    REQUIRE_THROWS(TrendingMachineConfig::parseDataSources(parse(R"json([{ "name": "a", "moduleName": "m" }])json")));
    REQUIRE_THROWS(TrendingMachineConfig::parseDataSources(parse(R"json([{ "name": "a", "reductorName": "r" }])json")));
  }
}

TEST_CASE("trending_machine_config_plots")
{
  SECTION("the full format with defaults")
  {
    auto tree = parse(R"json([{
      "name": "plot", "graphs": [ { "varexp": "a.b:time" } ]
    }])json");
    auto plots = TrendingMachineConfig::parsePlots(tree);
    REQUIRE(plots.size() == 1);
    REQUIRE(plots[0].name == "plot");
    REQUIRE(plots[0].title.empty());
    REQUIRE(plots[0].graphAxisLabel.empty());
    REQUIRE(plots[0].graphYRange.empty());
    REQUIRE(plots[0].colorPalette == 0);
    REQUIRE(plots[0].legend.nColumns == 1);
    REQUIRE(plots[0].legend.x1 < 0);
    REQUIRE(plots[0].legend.y1 < 0);
    REQUIRE(plots[0].legend.x2 < 0);
    REQUIRE(plots[0].legend.y2 < 0);
    REQUIRE(plots[0].graphs.size() == 1);

    const auto& graph = plots[0].graphs[0];
    REQUIRE(graph.varexp == "a.b:time");
    REQUIRE(graph.name == "plot"); // falls back to the plot name
    REQUIRE(graph.title.empty());
    REQUIRE(graph.selection.empty());
    REQUIRE(graph.option.empty());
    REQUIRE(graph.errors.empty());
    REQUIRE(graph.style.lineColor == -1);
    REQUIRE(graph.style.lineStyle == -1);
    REQUIRE(graph.style.lineWidth == -1);
    REQUIRE(graph.style.markerColor == -1);
    REQUIRE(graph.style.markerStyle == -1);
    REQUIRE(graph.style.markerSize == Approx(-1.f));
    REQUIRE(graph.style.fillColor == -1);
    REQUIRE(graph.style.fillStyle == -1);
  }

  SECTION("everything specified")
  {
    auto tree = parse(R"json([{
      "name": "plot", "title": "Plot title", "graphAxisLabel": "y:x", "graphYRange": "0:10", "colorPalette": 112,
      "legend": { "nColumns": 2, "x1": 0.1, "y1": 0.2, "x2": 0.3, "y2": 0.4 },
      "graphs": [
        { "name": "g1", "title": "G1", "varexp": "a.b:time", "selection": "a.b>0", "option": "*L PLC", "graphErrors": "1:2",
          "style": { "lineColor": 2, "lineStyle": 3, "lineWidth": 4, "markerColor": 5, "markerStyle": 6, "markerSize": 1.5,
                     "fillColor": 7, "fillStyle": 8 } },
        { "title": "second", "varexp": "a.b" }
      ]
    }])json");
    auto plots = TrendingMachineConfig::parsePlots(tree);
    REQUIRE(plots.size() == 1);
    const auto& plot = plots[0];
    REQUIRE(plot.title == "Plot title");
    REQUIRE(plot.graphAxisLabel == "y:x");
    REQUIRE(plot.graphYRange == "0:10");
    REQUIRE(plot.colorPalette == 112);
    REQUIRE(plot.legend.nColumns == 2);
    REQUIRE(plot.legend.x1 == Approx(0.1f));
    REQUIRE(plot.legend.y1 == Approx(0.2f));
    REQUIRE(plot.legend.x2 == Approx(0.3f));
    REQUIRE(plot.legend.y2 == Approx(0.4f));

    REQUIRE(plot.graphs.size() == 2);
    const auto& g1 = plot.graphs[0];
    REQUIRE(g1.name == "g1");
    REQUIRE(g1.title == "G1");
    REQUIRE(g1.varexp == "a.b:time");
    REQUIRE(g1.selection == "a.b>0");
    REQUIRE(g1.option == "*L PLC");
    REQUIRE(g1.errors == "1:2");
    REQUIRE(g1.style.lineColor == 2);
    REQUIRE(g1.style.lineStyle == 3);
    REQUIRE(g1.style.lineWidth == 4);
    REQUIRE(g1.style.markerColor == 5);
    REQUIRE(g1.style.markerStyle == 6);
    REQUIRE(g1.style.markerSize == Approx(1.5f));
    REQUIRE(g1.style.fillColor == 7);
    REQUIRE(g1.style.fillStyle == 8);

    // without a name, the graph is named after its title
    REQUIRE(plot.graphs[1].name == "second");
  }

  SECTION("the old format: the plot is also the graph")
  {
    auto tree = parse(R"json([{
      "name": "old",
      "title": "Old title",
      "varexp": "a.b:time",
      "selection": "a.b>1",
      "option": "*",
      "graphErrors": "1:2"
    }])json");
    auto plots = TrendingMachineConfig::parsePlots(tree);
    REQUIRE(plots.size() == 1);
    REQUIRE(plots[0].name == "old");
    REQUIRE(plots[0].graphs.size() == 1);
    REQUIRE(plots[0].graphs[0].name == "old");
    REQUIRE(plots[0].graphs[0].varexp == "a.b:time");
    REQUIRE(plots[0].graphs[0].selection == "a.b>1");
    REQUIRE(plots[0].graphs[0].option == "*");
    REQUIRE(plots[0].graphs[0].errors == "1:2");
  }

  SECTION("a plot without a name")
  {
    REQUIRE_THROWS(TrendingMachineConfig::parsePlots(parse(R"json([{ "graphs": [ { "varexp": "a" } ] }])json")));
  }

  SECTION("a graph without a varexp")
  {
    REQUIRE_THROWS(TrendingMachineConfig::parsePlots(parse(R"json([{ "name": "p", "graphs": [ { "name": "g" } ] }])json")));
  }

  SECTION("an empty plot list")
  {
    REQUIRE(TrendingMachineConfig::parsePlots(parse(R"json([])json")).empty());
  }
}

TEST_CASE("trending_machine_config_from_tree")
{
  SECTION("plots and data sources are read from the given node")
  {
    auto tree = parse(R"json({
      "dataSources": [ { "name": "a", "reductorName": "r", "moduleName": "m" } ],
      "plots": [ { "name": "p", "graphs": [ { "varexp": "a.x" } ] } ],
      "somethingElse": "ignored"
    })json");
    TrendingMachineConfig config(tree);
    REQUIRE(config.dataSources.size() == 1);
    REQUIRE(config.dataSources[0].name == "a");
    REQUIRE(config.plots.size() == 1);
    REQUIRE(config.plots[0].name == "p");
  }

  SECTION("both plots and data sources are required")
  {
    REQUIRE_THROWS(TrendingMachineConfig(parse(R"json({ "plots": [] })json")));
    REQUIRE_THROWS(TrendingMachineConfig(parse(R"json({ "dataSources": [] })json")));
  }
}