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
/// \file   Utility.cxx
/// \author Thomas Klemenz
///

// O2 includes
#include "Framework/ProcessingContext.h"
#include "Framework/InputRecordWalker.h"
#include "DataFormatsTPC/ClusterNativeHelper.h"
#include "DataFormatsTPC/TPCSectorHeader.h"
#include "TPCBase/CalDet.h"
#if __has_include("TPCBase/Painter.h")
#include "TPCBase/Painter.h"
#else
#include "TPCBaseRecSim/Painter.h"
#endif
#include "CommonUtils/StringUtils.h"

// QC includes
#include "TPC/Utility.h"
#include "QualityControl/QcInfoLogger.h"

// external includes
#include <Framework/Logger.h>
#include <bitset>
#include <algorithm>
#include <boost/property_tree/ptree.hpp>

namespace o2::quality_control_modules::tpc
{

bool getPropertyBool(const boost::property_tree::ptree& config, const std::string& id, const std::string property)
{
  const auto propertyFullName = fmt::format("qc.postprocessing.{}.{}", id, property);
  const boost::optional<const boost::property_tree::ptree&> propertyExists = config.get_child_optional(propertyFullName);
  if (propertyExists) {
    const auto doProperty = config.get<std::string>(propertyFullName);
    if (doProperty == "1" || doProperty == "true" || doProperty == "True" || doProperty == "TRUE" || doProperty == "yes") {
      return true;
    } else if (doProperty == "0" || doProperty == "false" || doProperty == "False" || doProperty == "FALSE" || doProperty == "no") {
      return false;
    } else {
      ILOG(Warning, Support) << fmt::format("No valid input for '{}'. Using default value 'false'.", property) << ENDM;
    }
  } else {
    ILOG(Warning, Support) << fmt::format("Option '{}' is missing. Using default value 'false'.", property) << ENDM;
  }
  return false;
}

void addAndPublish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, std::vector<std::unique_ptr<TCanvas>>& canVec, std::vector<std::string_view> canvNames, const std::map<std::string, std::string>& metaData)
{
  for (const auto& canvName : canvNames) {
    canVec.emplace_back(std::make_unique<TCanvas>(canvName.data()));
    auto canvas = canVec.back().get();
    objectsManager->startPublishing<true>(canvas);
    if (metaData.size() != 0) {
      for (const auto& [key, value] : metaData) {
        objectsManager->addMetadata(canvas->GetName(), key, value);
      }
    }
  }
}

std::vector<TCanvas*> toVector(std::vector<std::unique_ptr<TCanvas>>& input)
{
  std::vector<TCanvas*> output;
  for (auto& in : input) {
    output.emplace_back(in.get());
  }
  return output;
}

void fillCanvases(const o2::tpc::CalDet<float>& calDet, std::vector<std::unique_ptr<TCanvas>>& canvases, const quality_control::core::CustomParameters& params, const std::string paramName)
{
  const std::string parNBins = paramName + "NBins";
  const std::string parXMin = paramName + "XMin";
  const std::string parXMax = paramName + "XMax";
  int nbins = 300;
  float xmin = 0;
  float xmax = 0;
  const auto last = params.end();
  const auto itNBins = params.find(parNBins);
  const auto itXMin = params.find(parXMin);
  const auto itXMax = params.find(parXMax);
  if ((itNBins == last) || (itXMin == last) || (itXMax == last)) {
    LOGP(warning, "missing parameter {}, {} or {}, falling back to auto scaling", parNBins, parXMin, parXMax);
    LOGP(warning, "Please add '{}': '<value>', '{}': '<value>', '{}': '<value>' to the 'taskParameters'.", parNBins, parXMin, parXMax);
  } else {
    nbins = std::stoi(itNBins->second);
    xmin = std::stof(itXMin->second);
    xmax = std::stof(itXMax->second);
  }
  auto vecPtr = toVector(canvases);
  o2::tpc::painter::makeSummaryCanvases(calDet, nbins, xmin, xmax, false, &vecPtr);
}

void clearCanvases(std::vector<std::unique_ptr<TCanvas>>& canvases)
{
  for (const auto& canvas : canvases) {
    canvas->Clear();
  }
}

std::unique_ptr<o2::tpc::internal::getWorkflowTPCInput_ret> clusterHandler(o2::framework::InputRecord& inputs, int verbosity, unsigned long tpcSectorMask)
{
  auto retVal = std::make_unique<o2::tpc::internal::getWorkflowTPCInput_ret>();

  std::vector<o2::framework::InputSpec> filter = {
    { "input", o2::framework::ConcreteDataTypeMatcher{ "TPC", "CLUSTERNATIVE" }, o2::framework::Lifetime::Timeframe },
  };
  bool sampledData = true;
  for ([[maybe_unused]] auto const& ref : o2::framework::InputRecordWalker(inputs, filter)) {
    sampledData = false;
    break;
  }
  if (sampledData) {
    filter = {
      { "sampled-data", o2::framework::ConcreteDataTypeMatcher{ "DS", "CLUSTERNATIVE" }, o2::framework::Lifetime::Timeframe },
    };
    LOG(info) << "Using sampled data.";
  }

  unsigned long recvMask = 0;
  bool hasData = false;
  for (auto const& ref : o2::framework::InputRecordWalker(inputs, filter)) {
    auto const* sectorHeader = o2::framework::DataRefUtils::getHeader<o2::tpc::TPCSectorHeader*>(ref);
    if (sectorHeader == nullptr) {
      throw std::runtime_error("sector header missing on header stack");
    }
    const int sector = sectorHeader->sector();
    if (sector < 0) {
      continue;
    }
    if (recvMask & sectorHeader->sectorBits) {
      throw std::runtime_error("can only have one cluster data set per sector");
    }
    recvMask |= (sectorHeader->sectorBits & tpcSectorMask);
    retVal->internal.inputrefs[sector].data = ref;
    hasData = true;
  }
  if (hasData && (recvMask != tpcSectorMask)) {
    throw std::runtime_error("Incomplete set of clusters/digits received");
  }

  for (auto const& refentry : retVal->internal.inputrefs) {
    auto& sector = refentry.first;
    auto& ref = refentry.second.data;
    if (ref.payload == nullptr) {
      // skip zero-length message
      continue;
    }
    if (!(tpcSectorMask & (1ul << sector))) {
      continue;
    }
    if (refentry.second.labels.header != nullptr && refentry.second.labels.payload != nullptr) {
      retVal->internal.mcInputs.emplace_back(o2::dataformats::ConstMCLabelContainerView(inputs.get<gsl::span<char>>(refentry.second.labels)));
    }
    retVal->internal.inputs.emplace_back(gsl::span(ref.payload, o2::framework::DataRefUtils::getPayloadSize(ref)));
    if (verbosity > 1) {
      LOG(info) << "received " << *(ref.spec) << ", size " << o2::framework::DataRefUtils::getPayloadSize(ref) << " for sector " << sector;
    }
  }

  memset(&retVal->clusterIndex, 0, sizeof(retVal->clusterIndex));
  o2::tpc::ClusterNativeHelper::Reader::fillIndex(retVal->clusterIndex, retVal->internal.clusterBuffer, retVal->internal.clustersMCBuffer, retVal->internal.inputs, retVal->internal.mcInputs, tpcSectorMask);

  return std::move(retVal);
}

void getTimestamp(const std::string& metaInfo, std::vector<long>& timeStamps)
{
  std::string result_str;
  long result;
  std::string token = "Validity: ";
  if (metaInfo.find(token) != std::string::npos) {
    int start = metaInfo.find(token) + token.size();
    int end = metaInfo.find(" -", start);
    result_str = metaInfo.substr(start, end - start);
    std::string::size_type sz;
    result = std::stol(result_str, &sz);
    timeStamps.emplace_back(result);
  }
}

std::vector<long> getDataTimestamps(const o2::ccdb::CcdbApi& cdbApi, const std::string_view path, const unsigned int nFiles, const long limit)
{
  std::vector<long> outVec{};
  std::vector<long> tmpVec{};
  std::vector<long> tmpVec2{};

  if (limit == -1) {

    // get the list of files for the latest timestamps up to 1 day ago from the moment the code is running
    // added some seconds to upper timestamp limit as the ccdbapi uses creation timestamp, not validity!
    const auto to = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() + std::chrono::minutes(1)).time_since_epoch()).count();
    const auto from = std::chrono::duration_cast<std::chrono::milliseconds>((std::chrono::system_clock::now() + std::chrono::weeks(-2)).time_since_epoch()).count();
    std::vector<std::string> fileList = o2::utils::Str::tokenize(cdbApi.list(path.data(), false, "text/plain", to, from), '\n');
    for (const auto& metaData : fileList) {
      getTimestamp(metaData, outVec);
      if (outVec.size() == nFiles) {
        break;
      }
    }
  } else {
    // get file list for requested timestamp minus three days
    // added some seconds to upper timestamp limit as the ccdbapi uses creation timestamp, not validity!
    std::vector<std::string> fileList = o2::utils::Str::tokenize(cdbApi.list(path.data(), false, "text/plain", limit + 600000, limit - 86400000), '\n');
    for (const auto& metaData : fileList) {
      if (outVec.size() < nFiles) {
        getTimestamp(metaData, tmpVec);
        if (tmpVec.size() > 0 && tmpVec.back() <= limit) {
          if (outVec.size() == 0 || tmpVec.back() != outVec.back()) {
            outVec.emplace_back(tmpVec.back());
          }
        }
      } else {
        break;
      }
    }
  }
  std::sort(outVec.begin(), outVec.end());

  return std::move(outVec);
}

void calculateStatistics(const double* yValues, const double* yErrors, bool useErrors, const int firstPoint, const int lastPoint, double& mean, double& stddevOfMean)
{
  // yErrors returns nullptr for TGraph (no errors)
  if (lastPoint - firstPoint <= 0) {
    ILOG(Error, Support) << "In calculateStatistics(), the first and last point of the range have to differ!" << ENDM;
    return;
  }

  if (useErrors && !yErrors) {
    ILOG(Error, Support) << "In calculateStatistics(): requested to use errors of data but TGraph does not contain errors." << ENDM;
    useErrors = false;
  }

  std::vector<double> v(yValues + firstPoint, yValues + lastPoint);
  std::vector<double> vErr;

  if (useErrors) {
    const std::vector<double> vErr_temp(yErrors + firstPoint, yErrors + lastPoint);
    for (int i = 0; i < vErr_temp.size(); i++) {
      vErr.push_back(vErr_temp[i]);
    }
  }

  retrieveStatistics(v, vErr, useErrors, mean, stddevOfMean);
}

void calculateStatistics(const double* yValues, const double* yErrors, bool useErrors, const int firstPoint, const int lastPoint, double& mean, double& stddevOfMean, std::vector<int>& maskPoints)
{
  // yErrors returns nullptr for TGraph (no errors)
  if (lastPoint - firstPoint <= 0) {
    ILOG(Error, Support) << "In calculateStatistics(), the first and last point of the range have to differ!" << ENDM;
    return;
  }

  if (useErrors && !yErrors) {
    ILOG(Error, Support) << "In calculateStatistics(): requested to use errors of data but TGraph does not contain errors." << ENDM;
    useErrors = false;
  }

  std::vector<double> v;
  const std::vector<double> v_temp(yValues + firstPoint, yValues + lastPoint);
  for (int i = 0; i < v_temp.size(); i++) {
    if (std::find(maskPoints.begin(), maskPoints.end(), i) == maskPoints.end()) { // i is not in the masked points
      v.push_back(v_temp[i]);
    }
  }

  std::vector<double> vErr;
  if (useErrors) {
    const std::vector<double> vErr_temp(yErrors + firstPoint, yErrors + lastPoint);
    for (int i = 0; i < vErr_temp.size(); i++) {
      if (std::find(maskPoints.begin(), maskPoints.end(), i) == maskPoints.end()) { // i is not in the masked points
        vErr.push_back(vErr_temp[i]);
      }
    }
  }

  retrieveStatistics(v, vErr, useErrors, mean, stddevOfMean);
}

void retrieveStatistics(std::vector<double>& values, std::vector<double>& errors, bool useErrors, double& mean, double& stddevOfMean)
{
  if ((errors.size() != values.size()) && useErrors) {
    ILOG(Error, Support) << "In retrieveStatistics(): errors do not match data points, omitting errors" << ENDM;
    useErrors = false;
  }

  double sum = 0.;
  double sumSquare = 0.;
  double sumOfWeights = 0.;        // sum w_i
  double sumOfSquaredWeights = 0.; // sum (w_i)^2
  double weight = 0.;

  if (!useErrors) {
    // In case of no errors, we set our weights equal to 1
    sum = std::accumulate(values.begin(), values.end(), 0.0);
    sumOfWeights = values.size();
    sumOfSquaredWeights = values.size();
  } else {
    // In case of errors, we set our weights equal to 1/sigma_i^2
    for (size_t i = 0; i < values.size(); i++) {
      weight = 1. / std::pow(errors[i], 2.);
      sum += values[i] * weight;
      sumSquare += values[i] * values[i] * weight;
      sumOfWeights += weight;
      sumOfSquaredWeights += weight * weight;
    }
  }

  mean = sum / sumOfWeights;

  if (values.size() == 1) { // we only have one point, we keep it's uncertainty
    if (!useErrors) {
      stddevOfMean = 0.;
    } else {
      stddevOfMean = sqrt(1. / sumOfWeights);
    }
  } else { // for >= 2 points, we calculate the spread
    if (!useErrors) {
      std::vector<double> diff(values.size());
      std::transform(values.begin(), values.end(), diff.begin(), [mean](double x) { return x - mean; });
      double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      stddevOfMean = std::sqrt(sq_sum / (values.size() * (values.size() - 1.)));
    } else {
      double ratioSumWeight = sumOfSquaredWeights / (sumOfWeights * sumOfWeights);
      stddevOfMean = sqrt((sumSquare / sumOfWeights - mean * mean) * (1. / (1. - ratioSumWeight)) * ratioSumWeight);
    }
  }
}

void calcMeanAndStddev(const std::vector<float>& values, float& mean, float& stddev)
{
  if (values.size() == 0) {
    mean = 0.;
    stddev = 0.;
    return;
  }

  // Mean
  const float sum = std::accumulate(values.begin(), values.end(), 0.0);
  mean = sum / values.size();

  // Stddev
  if (values.size() == 1) { // we only have one point -> no stddev
    stddev = 0.;
  } else { // for >= 2 points, we calculate the spread
    std::vector<float> diff(values.size());
    std::transform(values.begin(), values.end(), diff.begin(), [mean](auto x) { return x - mean; });
    const auto sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.f);
    stddev = std::sqrt(sq_sum / (values.size() * (values.size() - 1.)));
  }
}

} // namespace o2::quality_control_modules::tpc
