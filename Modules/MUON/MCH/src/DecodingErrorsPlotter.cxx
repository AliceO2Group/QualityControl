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
/// \file   DecodingErrorsPlotter.cxx
/// \author Andrea Ferrero
///

#include "MCH/DecodingErrorsPlotter.h"
#include "MCH/Helpers.h"
#include "MCHRawDecoder/ErrorCodes.h"
#include "MCHGlobalMapping/DsIndex.h"
#include <fmt/format.h>

using namespace o2::mch;
using namespace o2::mch::raw;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

static void setXAxisLabels(TH2F* hErrors)
{
  TAxis* ay = hErrors->GetXaxis();
  for (int i = 1; i <= 10; i++) {
    auto label = fmt::format("CH{}", i);
    ay->SetBinLabel(i, label.c_str());
  }
}

static void setYAxisLabels(TH2F* hErrors)
{
  TAxis* ax = hErrors->GetYaxis();
  for (int i = 0; i < getErrorCodesSize(); i++) {
    ax->SetBinLabel(i + 1, errorCodeAsString(1 << i).c_str());
    ax->ChangeLabel(i + 1, 45);
  }
}

DecodingErrorsPlotter::DecodingErrorsPlotter(std::string path) : mPath(path)
{
  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  //--------------------------------------------
  // Fraction of DS boards in error per DE
  //--------------------------------------------

  // Number of decoding errors, grouped by FEE ID and normalized to the number of processed TF
  mHistogramGoodBoardsPerDE = std::make_unique<TH1F>(TString::Format("%sGoodBoardsFractionPerDE", path.c_str()),
                                                     "Fraction of boards not in error", getNumDE(), 0, getNumDE());
  addDEBinLabels(mHistogramGoodBoardsPerDE.get());
  addHisto(mHistogramGoodBoardsPerDE.get(), false, "", "");

  mHistogramGoodBoardsPerSolar = std::make_unique<TH1F>(TString::Format("%sGoodBoardsFractionPerSolar", path.c_str()),
                                                        "Fraction of boards not in error per SOLAR board", getNumSolar(), 0, getNumSolar());
  addSolarBinLabels(mHistogramGoodBoardsPerSolar.get());
  addHisto(mHistogramGoodBoardsPerSolar.get(), false, "", "");

  //--------------------------------------------
  // Decoding errors per chamber, DE and FEEID
  //--------------------------------------------

  // Number of decoding errors, grouped by FEE ID and normalized to the number of processed TF
  mHistogramErrorsPerFeeId = std::make_unique<TH2F>(TString::Format("%sDecodingErrorsPerFeeId", path.c_str()),
                                                    "FEE ID vs. Error Type", 64, 0, 64,
                                                    getErrorCodesSize(), 0, getErrorCodesSize());
  setYAxisLabels(mHistogramErrorsPerFeeId.get());
  addHisto(mHistogramErrorsPerFeeId.get(), false, "colz", "gridy logz");

  // Number of decoding errors, grouped by DE ID and normalized to the number of processed TF
  mHistogramErrorsPerDE = std::make_unique<TH2F>(TString::Format("%sDecodingErrorsPerDE", path.c_str()),
                                                 "Error Type vs. DE ID",
                                                 getNumDE(), 0, getNumDE(),
                                                 getErrorCodesSize(), 0, getErrorCodesSize());
  setYAxisLabels(mHistogramErrorsPerDE.get());
  addHisto(mHistogramErrorsPerDE.get(), false, "colz", "gridy logz");

  // Number of decoding errors, grouped by chamber ID and normalized to the number of processed TF
  mHistogramErrorsPerChamber = std::make_unique<TH2F>(TString::Format("%sDecodingErrorsPerChamber", path.c_str()),
                                                      "Chamber Number vs. Error Type", 10, 1, 11,
                                                      getErrorCodesSize(), 0, getErrorCodesSize());
  setXAxisLabels(mHistogramErrorsPerChamber.get());
  setYAxisLabels(mHistogramErrorsPerChamber.get());
  addHisto(mHistogramErrorsPerChamber.get(), false, "colz", "gridx gridy logz");
}

//_________________________________________________________________________________________

static bool isWarning(int errorID)
{
  // Temporary solution, until the error code severity is implemented directly in O2
  uint32_t error = ((uint32_t)1) << errorID;
  if (error == ErrorCodes::ErrorTruncatedData) {
    return true;
  }
  if (error == ErrorCodes::ErrorTruncatedDataUL) {
    return true;
  }
  return false;
}

//_________________________________________________________________________________________

void DecodingErrorsPlotter::update(TH2F* h)
{
  if (!h) {
    return;
  }

  auto incrementBin = [&](TH2F* h, int bx, int by, float val) {
    auto entries = h->GetBinContent(bx, by);
    h->SetBinContent(bx, by, entries + val);
  };

  mHistogramErrorsPerFeeId->Reset("ICES");
  mHistogramErrorsPerDE->Reset("ICES");
  mHistogramErrorsPerChamber->Reset("ICES");

  std::array<std::pair<float, float>, getNumDE()> goodBoardsFractionPerDE;
  std::array<std::pair<float, float>, getNumSolar()> goodBoardsFractionPerSolar;

  // loop over bins in electronics coordinates, and map the channels to the corresponding cathode pads
  int nbinsx = h->GetXaxis()->GetNbins();
  int nbinsy = h->GetYaxis()->GetNbins();
  for (int i = 1; i <= nbinsx; i++) {
    // address of the DS board in detector representation
    auto dsDetId = getDsDetId(i - 1);
    auto deId = dsDetId.deId();
    auto dsId = dsDetId.dsId();

    int chamber = deId / 100;

    int deIndex = getDEindex(deId);
    if (deIndex < 0) {
      continue;
    }

    // Using the mapping to go from (deId, dsId) to Elec info (fee, link) and fill Elec Histogram,
    // where one bin is one physical pad
    // get the unique solar ID and the DS address associated to this digit
    int feeId = -1;
    int solarId = -1;
    std::optional<DsElecId> dsElecId = mDet2ElecMapper(dsDetId);
    if (dsElecId) {
      solarId = dsElecId->solarId();
      uint32_t dsAddr = dsElecId->elinkId();

      std::optional<FeeLinkId> feeLinkId = mSolar2FeeLinkMapper(solarId);
      if (feeLinkId) {
        feeId = feeLinkId->feeId();
      }
    }
    int solarIndex = (solarId >= 0) ? getSolarIndex(solarId) : -1;
    if (solarIndex < 0) {
      continue;
    }

    bool hasError = false;

    for (int j = 1; j <= nbinsy; j++) {
      auto count = h->GetBinContent(i, j);
      if (feeId >= 0) {
        incrementBin(mHistogramErrorsPerFeeId.get(), feeId + 1, j, count);
        // do not include warnings when counting the boards in error
        if (!isWarning(j - 1) && count > 0) {
          hasError = true;
        }
      }

      incrementBin(mHistogramErrorsPerDE.get(), deIndex + 1, j, count);
      incrementBin(mHistogramErrorsPerChamber.get(), chamber, j, count);
    }

    goodBoardsFractionPerDE[deIndex].first += 1;
    goodBoardsFractionPerSolar[solarIndex].first += 1;
    if (!hasError) {
      goodBoardsFractionPerDE[deIndex].second += 1;
      goodBoardsFractionPerSolar[solarIndex].second += 1;
    }
  }

  // update fraction of good boards per DE
  for (int i = 0; i < getNumDE(); i++) {
    if (goodBoardsFractionPerDE[i].first > 0) {
      float frac = goodBoardsFractionPerDE[i].second / goodBoardsFractionPerDE[i].first;
      mHistogramGoodBoardsPerDE->SetBinContent(i + 1, frac);
    } else {
      mHistogramGoodBoardsPerDE->SetBinContent(i + 1, 0);
    }
  }

  // update fraction of good boards per Solar
  for (int i = 0; i < getNumSolar(); i++) {
    if (goodBoardsFractionPerSolar[i].first > 0) {
      float frac = goodBoardsFractionPerSolar[i].second / goodBoardsFractionPerSolar[i].first;
      mHistogramGoodBoardsPerSolar->SetBinContent(i + 1, frac);
    } else {
      mHistogramGoodBoardsPerSolar->SetBinContent(i + 1, 0);
    }
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
