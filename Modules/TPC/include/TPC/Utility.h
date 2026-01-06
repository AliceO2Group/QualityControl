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
/// \file   Utility.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_TPCUTILITY_H
#define QUALITYCONTROL_TPCUTILITY_H

#include "QualityControl/ObjectsManager.h"
#include "QualityControl/CustomParameters.h"

#if __has_include("TPCBase/CalDet.h")
#include "TPCBase/CalDet.h"
#else
#include "TPCBaseRecSim/CalDet.h"
#endif
#include "DataFormatsTPC/ClusterNative.h"
#include "DataFormatsTPC/WorkflowHelper.h"
#include "Framework/ProcessingContext.h"
#include "CCDB/CcdbApi.h"

#include <TCanvas.h>

namespace o2::quality_control_modules::tpc
{
/// \brief Get boolean configurable from json file
/// This function checks if a configurable is available in the json file and makes sure that different versions of it are accepted (true, TRUE, 1, etc)
/// \param config ConfigurationInterface with prefix set to ""
/// \param id Task id
/// \param property Property name to be looked for in json
bool getPropertyBool(const boost::property_tree::ptree& config, const std::string& id, const std::string property);

/// \brief Prepare canvases to publish DalPad data
/// This function creates canvases for CalPad data and registers them to be published on the QCG.
/// \param objectsManager ObjectsManager of the underlying PostProcessingInterface
/// \param canVec Vector which holds TCanvas pointers persisting for the entire runtime of the task
/// \param canvNames Names of the canvases
/// \param metaData Optional std::map to set meta data for the publishing
void addAndPublish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, std::vector<std::unique_ptr<TCanvas>>& canVec, std::vector<std::string_view> canvNames, const std::map<std::string, std::string>& metaData = std::map<std::string, std::string>());

/// \brief Converts std::vector<std::unique_ptr<TCanvas>> to std::vector<TCanvas*>
/// \param input std::vector<std::unique_ptr<TCanvas>> to be converted to std::vector<TCanvas*>
/// \return std::vector<TCanvas*>
std::vector<TCanvas*> toVector(std::vector<std::unique_ptr<TCanvas>>& input);

/// \brief Fills std::vector<std::unique_ptr<TCanvas>> with data from calDet
/// This is a convenience function to call o2::tpc::painter::makeSummaryCanvases in QC tasks to visualize the content of a CalDet object.
/// \param calDet Object to be displayed in the canvases
/// \param canvases Vector containing three std::unique_ptr<TCanvas>, will be filled
/// \param params Information about the ranges of the histograms that will be drawn on the canvases. The params can be set via 'taskParameters' in the config file of corresponding the task.
/// \param paramName Name of the observable that is stored in calDet
void fillCanvases(const o2::tpc::CalDet<float>& calDet, std::vector<std::unique_ptr<TCanvas>>& canvases, const quality_control::core::CustomParameters& params, const std::string paramName);

/// \brief Clears all canvases
/// \param canvases Contains the canvases that will be cleared
void clearCanvases(std::vector<std::unique_ptr<TCanvas>>& canvases);

/// \brief Converts CLUSTERNATIVE from InputRecord to getWorkflowTPCInput_ret
/// Convenience funtion to make native clusters accessible when receiving them from the DPL
/// \param input InputReconrd from the ProcessingContext
/// \return getWorkflowTPCInput_ret object for easy cluster access
std::unique_ptr<o2::tpc::internal::getWorkflowTPCInput_ret> clusterHandler(o2::framework::InputRecord& inputs, int verbosity = 0, unsigned long tpcSectorMask = 0xFFFFFFFFF);

/// \brief Extracts the "Valid from" timestamp from metadata
void getTimestamp(const std::string& metaInfo, std::vector<long>& timeStamps);

/// \brief Gives a vector of timestamps for data to be processed
/// Gives a vector of time stamps of x files (x=nFiles) in path which are older than a given time stamp (limit)
/// \param url CCDB URL
/// \param path File path in the CCDB
/// \param nFiles Number of files that shall be processed
/// \param limit Most recent timestamp to be processed
std::vector<long> getDataTimestamps(const o2::ccdb::CcdbApi& cdbApi, const std::string_view path, const unsigned int nFiles, const long limit);

/// \brief Calculates mean and stddev from yValues of a TGraph. Overloaded function, actual calculation in retrieveStatistics
/// \param yValues const double* pointer to yValues of TGraph (via TGraph->GetY())
/// \param yErrors const double* pointer to y uncertainties of TGraph (via TGraph->GetEY())
/// \param useErrors bool whether uncertainties should be used in calculation of mean and stddev of mean
/// \param firstPoint const int, first point of yValues to include in calculation
/// \param lastPoint const int, last point of yValues to include in calculation
/// \param mean double&, reference to double that should store mean
/// \param stddevOfMean double&, reference to double that should store stddev of mean
void calculateStatistics(const double* yValues, const double* yErrors, bool useErrors, const int firstPoint, const int lastPoint, double& mean, double& stddevOfMean);

/// \brief Calculates mean and stddev from yValues of a TGraph. Overloaded function
/// \param yValues const double* pointer to yValues of TGraph (via TGraph->GetY())
/// \param yErrors const double* pointer to y uncertainties of TGraph (via TGraph->GetEY())
/// \param useErrors bool whether uncertainties should be used in calculation of mean and stddev of mean
/// \param firstPoint const int, first point of yValues to include in calculation
/// \param lastPoint const int, last point of yValues to include in calculation
/// \param mean double&, reference to double that should store mean
/// \param stddevOfMean double&, reference to double that should store stddev of mean
/// \param maskPoints std::vector<int>&, points of the selected TGraph-points that should be masked
void calculateStatistics(const double* yValues, const double* yErrors, bool useErrors, const int firstPoint, const int lastPoint, double& mean, double& stddevOfMean, std::vector<int>& maskPoints);

/// \brief Calculates mean and stddev from yValues of a TGraph. Overloaded function, actual calculation in retrieveStatistics
/// \param values std::vector<double>& vector that contains the data points
/// \param errors std::vector<double>& vector that contains the data errors
/// \param useErrors bool whether uncertainties should be used in calculation of mean and stddev of mean
/// \param mean double&, reference to double that should store mean
/// \param stddevOfMean double&, reference to double that should store stddev of mean
void retrieveStatistics(std::vector<double>& values, std::vector<double>& errors, bool useErrors, double& mean, double& stddevOfMean);

/// \brief Calculates mean and stddev from a vector
/// \param values std::vector<values>& vector that contains the data points
/// \param mean float&, reference to float that should store mean
/// \param stddev float&, reference to float that should store stddev of mean
void calcMeanAndStddev(const std::vector<float>& values, float& mean, float& stddev);
} // namespace o2::quality_control_modules::tpc
#endif // QUALITYCONTROL_TPCUTILITY_H
