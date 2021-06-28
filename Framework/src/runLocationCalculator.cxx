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
/// \file    runLocationCalculator.cxx
/// \author  Piotr Konopka
///
/// \brief This is a calculator for the tasks cost when it is run locally and remotely

#include "QualityControl/Calculators.h"
#include <iostream>

#include <boost/program_options.hpp>

using namespace o2::quality_control::calculators;
namespace bpo = boost::program_options;

int main(int argc, const char* argv[])
{
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()                                                                                                       //
      ("help,h", "Help screen")                                                                                              //
      ("cost-cpu,cp", bpo::value<double>()->default_value(118.0), "Cost of CPU [currency/CPU]")                              //
      ("cost-bandwidth,cb", bpo::value<double>()->default_value(0.76), "Cost of bandwidth [currency/MB/s]")                  //
      ("cost-ram,cm", bpo::value<double>()->default_value(0.005), "Cost of RAM [currency/MB]")                               //
      ("parallelism,p", bpo::value<int>()->default_value(500), "Number of parallel nodes []")                                //
      ("parallel-data,D", bpo::value<double>()->default_value(0.19), "Parallel data stream size (QC Task input) [MB/s]")     //
      ("avg-data-message,Dm", bpo::value<double>()->default_value(1), "Average data message size [MB]")                      //
      ("stddev-data-message,Ds", bpo::value<double>()->default_value(1), "Std dev of data message size [MB]")                //
      ("mos-size,mo", bpo::value<int>()->default_value(100), "Size of all MonitorObjects produced by one QC Task [MB]")      //
      ("cycle-duration,T", bpo::value<double>()->default_value(60.0), "Cycle duration [s]")                                  //
      ("qc-task-cpu,qp", bpo::value<double>()->default_value(0.01), "CPU usage of a QC Task per data throughput [CPU/MB/s]") //
      ("qc-task-ram,qm", bpo::value<int>()->default_value(250), "RAM usage of an idle QC Task [MB]")                         //
      ("merger-performance,mu", bpo::value<double>()->default_value(25.0), "Number of objects per second which can be merged by one Merger");

    bpo::variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    const auto costCPU = vm["cost-cpu"].as<double>();
    const auto costBandwidth = vm["cost-bandwidth"].as<double>();
    const auto costRAM = vm["cost-ram"].as<double>();
    const auto parallelism = vm["parallelism"].as<int>();
    const auto parallelData = vm["parallel-data"].as<double>();
    const auto avgDataMessage = vm["avg-data-message"].as<double>();
    const auto stdDevDataMessage = vm["stddev-data-message"].as<double>();
    const auto mosSize = vm["mos-size"].as<int>();
    const auto cycleDuration = vm["cycle-duration"].as<double>();
    const auto qcTaskCPU = vm["qc-task-cpu"].as<double>();
    const auto qcTaskRAM = vm["qc-task-ram"].as<int>();
    const auto mergerPerformance = vm["merger-performance"].as<double>();

    std::cout << "PARAMETERS" << std::endl;
    std::cout << "costCPU,               " << costCPU << std::endl;
    std::cout << "costBandwidth,         " << costBandwidth << std::endl;
    std::cout << "costRAM,               " << costRAM << std::endl;
    std::cout << "parallelism,           " << parallelism << std::endl;
    std::cout << "parallelData,          " << parallelData << std::endl;
    std::cout << "avgDataMessage,        " << avgDataMessage << std::endl;
    std::cout << "stdDevDataMessage,     " << stdDevDataMessage << std::endl;
    std::cout << "mosSize,               " << mosSize << std::endl;
    std::cout << "cycleDuration,         " << cycleDuration << std::endl;
    std::cout << "qcTaskCPU,             " << qcTaskCPU << std::endl;
    std::cout << "qcTaskRAM,             " << qcTaskRAM << std::endl;
    std::cout << "mergerPerformance,     " << mergerPerformance << std::endl;
    std::cout << std::endl;

    // Computing the local variant
    {
      auto localQCTaskCost = qcTaskCost(costCPU, costRAM, qcTaskCPU, qcTaskRAM, parallelData, avgDataMessage, stdDevDataMessage);
      auto localCost = parallelism * localQCTaskCost;

      auto transportCost = costBandwidth * parallelism * mosSize / cycleDuration;

      // todo: allow to specify R in arguments
      auto [R, costCPUMergers, costRAMMergers] = cheapestMergers(costCPU, costRAM, parallelism, mosSize, cycleDuration, [mergerPerformance](double) { return mergerPerformance; });
      auto remoteCost = costCPUMergers + costRAMMergers;
      auto totalCost = localCost + transportCost + remoteCost;

      std::cout << "RESULTS LOCAL" << std::endl;
      std::cout << "R,                " << R << std::endl;
      std::cout << "localCost,        " << localCost << std::endl;
      std::cout << "transportCost,    " << transportCost << std::endl;
      std::cout << "remoteCost,       " << remoteCost << std::endl;
      std::cout << "totalCost,        " << totalCost << std::endl;
    }

    // Computing the remote variant
    {
      auto localCost = 0.0;
      auto transportCost = costBandwidth * parallelism * parallelData;
      auto remoteCost = qcTaskCost(costCPU, costRAM, qcTaskCPU, qcTaskRAM, parallelism * parallelData, avgDataMessage, stdDevDataMessage);
      auto totalCost = localCost + transportCost + remoteCost;

      std::cout << "RESULTS REMOTE" << std::endl;
      std::cout << "localCost,        " << localCost << std::endl;
      std::cout << "transportCost,    " << transportCost << std::endl;
      std::cout << "remoteCost,       " << remoteCost << std::endl;
      std::cout << "totalCost,        " << totalCost << std::endl;
    }

    return 0;
  } catch (const bpo::error& ex) {
    std::cerr << "Exception caught: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}

// -0.378347x + 2782.30