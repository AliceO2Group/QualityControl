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
/// \file    Calculators.cxx
/// \author  Piotr Konopka
///
/// \brief Bunch of formulas for theoretical calculations for finding optimal QC topologies.
#include "QualityControl/Calculators.h"
#include "QualityControl/QcInfoLogger.h"
#include <cmath>

namespace o2::quality_control::calculators
{

// average M/D/1 queue size, rho is server utilisation ( input rate / processing rate )
double averageMD1Queue(double rho)
{
  return rho < 1 ? rho * rho / 2.0 / (1.0 - rho) : std::numeric_limits<double>::infinity();
}

// average M/G/1 queue size
// rho is server utilisation ( input rate / processing rate )
// mean is the mean processing time
// stddev is the standard deviation of the processing time
double averageMG1Queue(double rho, double mean, double stddev)
{
  return rho < 1 ? rho * rho / 2.0 / (1.0 - rho) * (1 + (stddev * stddev) / (mean * mean)) : std::numeric_limits<double>::infinity();
}

// number of merger layes, M0 is number of producers, R is max reduction factor
size_t numberOfMergerLayers(size_t M0, size_t R)
{
  // we benefit from the fact that log_a(b) = log_c(b) / log_c(a)
  return std::ceil(std::log((double)M0) / std::log((double)R));
}

double mergersMemoryUsage(size_t R, size_t M0, size_t objSize, double T, std::function<double(double)> performance)
{
  const size_t layers = numberOfMergerLayers(M0, R);

  double averageObjects = 0;
  size_t Mi = M0;

  for (size_t layer = 1; layer <= layers; layer++) {
    const size_t Mi_prev = Mi;
    const size_t Mi = std::ceil(Mi_prev / (double)R);
    const double Ri = Mi_prev / (double)Mi;
    const double rho = Ri / (double)T / performance(Ri);

    if (rho >= 1) {
      // if utilisation becomes > 1, then the queue will grow infinitely.
      averageObjects = std::numeric_limits<double>::infinity();
      break;
    }

    averageObjects += Mi * (averageMD1Queue(rho) + rho + 1); // should it be "2" actually? Average entities in the system + merged object is better
  }

  double memory = averageObjects * objSize;
  return memory;
}

double mergersCpuUsage(size_t R, size_t M0, double T, std::function<double(double)> performance)
{
  const size_t layers = numberOfMergerLayers(M0, R);
  double cores = 0.0;

  size_t Mi = M0;
  for (size_t layer = 1; layer <= layers; layer++) {
    const size_t Mi_prev = Mi;
    const size_t Mi = std::ceil(Mi_prev / (double)R);
    const double Ri = Mi_prev / (double)Mi;
    const double rho = Ri / (double)T / performance(Ri);

    if (rho >= 1) {
      cores = std::numeric_limits<double>::infinity();
      break;
    }

    cores += Mi * rho;
  }

  return cores;
}

// returns the cost of CPU and RAM of the full merger topology
std::tuple<double, double> mergerCosts(double costCPU, double costRAM, size_t R, int parallelism, int mosSize,
                                       double cycleDuration, std::function<double(double)> performance)
{
  double mergersCPUCost = costCPU * mergersCpuUsage(R, parallelism, cycleDuration, performance);
  double mergersfMemoryCost = costRAM * mergersMemoryUsage(R, parallelism, mosSize, cycleDuration, performance);
  return { mergersCPUCost, mergersfMemoryCost };
}

// Returns the best Reduction factor (R) for given conditions and total cost of CPU and RAM.
// If there is a range of equally good reduction factors, it will return the highest.
std::tuple<size_t, double, double> cheapestMergers(double costCPU, double costRAM, int parallelism, int mosSize,
                                                   double cycleDuration, std::function<double(double)> performance)
{
  size_t bestR = -1;
  double lowestCPUCost = std::numeric_limits<double>::max();
  double lowestRAMCost = std::numeric_limits<double>::max();
  double lowestTotalCost = std::numeric_limits<double>::max();
  for (size_t R = 2; R <= (size_t)parallelism; R++) {
    auto [costOfCPU, costOfMemory] = mergerCosts(costCPU, costRAM, R, parallelism, mosSize, cycleDuration, performance);
    double totalCost = costOfMemory + costOfCPU;
    //    ILOG(Info) << "R: " << R << " total cost: " << totalCost << ENDM;

    if (totalCost <= lowestTotalCost) {
      lowestTotalCost = totalCost;
      lowestCPUCost = costOfCPU;
      lowestRAMCost = costOfMemory;
      bestR = R;
    }
  }
  return { bestR, lowestCPUCost, lowestRAMCost };
}

double qcTaskInputMemory(double utilisation, double avgInputMessage, double stddevInputMessage)
{
  // we can use avgInputMessage and stddevInputMessage (which are in Bytes) instead of processing times,
  // because we assume that processing time is proportional to message sizes, this task throughput would cancel out,
  // being both in numerator and denominator.
  return avgInputMessage * (averageMG1Queue(utilisation, avgInputMessage, stddevInputMessage) + utilisation);
}

double qcTaskCost(double costCPU, double costRAM, double qcTaskCPU, size_t qcTaskRAM,
                  double parallelData, double avgInputMessage, double stddevInputMessage)
{
  auto utilisation = parallelData * qcTaskCPU;
  auto inputMemory = qcTaskInputMemory(utilisation, avgInputMessage, stddevInputMessage);

  return costCPU * utilisation + costRAM * (inputMemory + qcTaskRAM);
}

} // namespace o2::quality_control::calculators