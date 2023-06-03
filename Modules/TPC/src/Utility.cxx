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
#include "TPCBase/Painter.h"
#include "CommonUtils/StringUtils.h"

// QC includes
#include "TPC/Utility.h"
#include "QualityControl/QcInfoLogger.h"

// external includes
#include <Framework/Logger.h>
#include <bitset>

namespace o2::quality_control_modules::tpc
{

void addAndPublish(std::shared_ptr<o2::quality_control::core::ObjectsManager> objectsManager, std::vector<std::unique_ptr<TCanvas>>& canVec, std::vector<std::string_view> canvNames, const std::map<std::string, std::string>& metaData)
{
  for (const auto& canvName : canvNames) {
    canVec.emplace_back(std::make_unique<TCanvas>(canvName.data()));
    auto canvas = canVec.back().get();
    objectsManager->startPublishing(canvas);
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
  }
  if (recvMask != tpcSectorMask) {
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

  std::vector<std::string> fileList = o2::utils::Str::tokenize(cdbApi.list(path.data()), '\n');

  if (limit == -1) {
    for (const auto& metaData : fileList) {
      getTimestamp(metaData, outVec);
      if (outVec.size() == nFiles) {
        break;
      }
    }
  } else {
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

  double sum = 0.;
  double sumSquare = 0.;
  double sumOfWeights = 0.;        // sum w_i
  double sumOfSquaredWeights = 0.; // sum (w_i)^2
  double weight = 0.;

  const std::vector<double> v(yValues + firstPoint, yValues + lastPoint);
  if (!useErrors) {
    // In case of no errors, we set our weights equal to 1
    sum = std::accumulate(v.begin(), v.end(), 0.0);
    sumOfWeights = v.size();
    sumOfSquaredWeights = v.size();
  } else {
    // In case of errors, we set our weights equal to 1/sigma_i^2
    const std::vector<double> vErr(yErrors + firstPoint, yErrors + lastPoint);
    for (size_t i = 0; i < v.size(); i++) {
      weight = 1. / std::pow(vErr[i], 2.);
      sum += v[i] * weight;
      sumSquare += v[i] * v[i] * weight;
      sumOfWeights += weight;
      sumOfSquaredWeights += weight * weight;
    }
  }

  mean = sum / sumOfWeights;

  if (v.size() == 1) { // we only have one point, we keep it's uncertainty
    if (!useErrors) {
      stddevOfMean = 0.;
    } else {
      stddevOfMean = sqrt(1. / sumOfWeights);
    }
  } else { // for >= 2 points, we calculate the spread
    if (!useErrors) {
      std::vector<double> diff(v.size());
      std::transform(v.begin(), v.end(), diff.begin(), [mean](double x) { return x - mean; });
      double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
      stddevOfMean = std::sqrt(sq_sum / (v.size() * (v.size() - 1.)));
    } else {
      double ratioSumWeight = sumOfSquaredWeights / (sumOfWeights * sumOfWeights);
      stddevOfMean = sqrt((sumSquare / sumOfWeights - mean * mean) * (1. / (1. - ratioSumWeight)) * ratioSumWeight);
    }
  }
}

} // namespace o2::quality_control_modules::tpc
