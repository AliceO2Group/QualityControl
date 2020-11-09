// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    runMergerCalculator.cxx
/// \author  Piotr Konopka
///
/// \brief This is a calculator for finding an optimal reduction factor for Mergers

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/Calculators.h"
#include <boost/program_options.hpp>

using namespace o2::quality_control::core;
using namespace o2::quality_control::calculators;
namespace bpo = boost::program_options;

int main(int argc, const char* argv[])
{
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()                                                                                                  //
      ("help,h", "Help screen")                                                                                         //
      ("cost-cpu,cp", bpo::value<double>()->default_value(118.0), "Cost of CPU [currency/CPU]")                         //
      ("cost-ram,cm", bpo::value<double>()->default_value(0.0065), "Cost of RAM [currency/MB]")                         //
      ("parallelism,p", bpo::value<int>()->default_value(2500), "Number of parallel nodes []")                          //
      ("mos-size,mo", bpo::value<int>()->default_value(500), "Size of all MonitorObjects produced by one QC Task [MB]") //
      ("cycle-duration,T", bpo::value<double>()->default_value(60.0), "Cycle duration [s]")                             //
      ("merger-performance,mu", bpo::value<double>()->default_value(25.0), "Number of objects per second which can be merged by one Merger");

    bpo::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    const auto costCPU = vm["cost-cpu"].as<double>();
    const auto costRAM = vm["cost-ram"].as<double>();
    const auto parallelism = vm["parallelism"].as<int>();
    const auto mosSize = vm["mos-size"].as<int>();
    const auto cycleDuration = vm["cycle-duration"].as<double>();
    const auto mergerPerformance = vm["merger-performance"].as<double>();

    std::cout << "PARAMETERS" << std::endl;
    std::cout << "costCPU,           " << costCPU << std::endl;
    std::cout << "costRAM,           " << costRAM << std::endl;
    std::cout << "parallelism,       " << parallelism << std::endl;
    std::cout << "mosSize,           " << mosSize << std::endl;
    std::cout << "cycleDuration,     " << cycleDuration << std::endl;
    std::cout << "mergerPerformance, " << mergerPerformance << std::endl;

    auto performance = [=](double /* Ri */) {
      // We assume the same performance regardless of the number of inputs,
      // but we could do something like this:
      // return -0.002 * Ri + 24;
      return mergerPerformance;
    };

    std::cout << "RESULTS" << std::endl;
    std::cout << "R                , costOfMemory        , costOfCPU" << std::endl;
    for (size_t R = 2; R <= (size_t)parallelism; R++) {
      double costOfMemory = costRAM * mergersMemoryUsage(R, parallelism, mosSize, cycleDuration, performance);
      double costOfCPU = costCPU * mergersCpuUsage(R, parallelism, cycleDuration, performance);
      double totalCost = costOfMemory + costOfCPU;
      std::cout << R << "  ,  " << costOfMemory << "  ,   " << costOfCPU << "  ,  " << totalCost << std::endl;
    }

    return 0;
  } catch (const bpo::error& ex) {
    std::cerr << "Exception caught: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}