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
/// \file    Calculators.h
/// \author  Piotr Konopka
///
/// \brief Bunch of formulas for theoretical calculations for finding optimal QC topologies.

#ifndef QUALITYCONTROL_CALCULATORS_H
#define QUALITYCONTROL_CALCULATORS_H

#include <tuple>
#include <functional>

namespace o2::quality_control::calculators
{

// average M/D/1 queue size, rho is server utilisation ( input rate / processing rate )
double averageMD1Queue(double rho);

// average M/G/1 queue size
// rho is server utilisation ( input rate / processing rate )
// mean is the mean processing time
// stddev is the standard deviation of the processing time
double averageMG1Queue(double rho, double mean, double stddev);

// number of merger layes, M0 is number of producers, R is max reduction factor
size_t numberOfMergerLayers(size_t M0, size_t R);
double mergersMemoryUsage(size_t R, size_t M0, size_t objSize, double T, std::function<double(double)> performance);

double mergersCpuUsage(size_t R, size_t M0, double T, std::function<double(double)> performance);

// returns the cost of CPU and RAM of the full merger topology
std::tuple<double, double> mergerCosts(double costCPU, double costRAM, size_t R, int parallelism, int mosSize,
                                       double cycleDuration, std::function<double(double)> performance);

// Returns the best Reduction factor (R) for given conditions and total cost of CPU and RAM.
// If there is a range of equally good reduction factors, it will return the highest.
std::tuple<size_t, double, double> cheapestMergers(double costCPU, double costRAM, int parallelism, int mosSize,
                                                   double cycleDuration, std::function<double(double)> performance);

double qcTaskInputMemory(double utilisation, double avgInputMessage, double stddevInputMessage);

double qcTaskCost(double costCPU, double costRAM, double qcTaskCPU, size_t qcTaskRAM, double parallelData, double avgInputMessage, double stddevInputMessage);

} // namespace o2::quality_control::calculators
#endif //QUALITYCONTROL_CALCULATORS_H
