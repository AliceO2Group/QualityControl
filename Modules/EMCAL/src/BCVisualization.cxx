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
/// \file
/// \author Markus Fasel
///

// QC includes
#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/BCVisualization.h"
#include "QualityControl/DatabaseInterface.h"
#include "CommonConstants/LHCConstants.h"

// root includes
#include "TCanvas.h"
#include "TLegend.h"
#include "TPaveText.h"
#include "TH1D.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace o2::quality_control::postprocessing;
using namespace o2::quality_control::core;

namespace o2::quality_control_modules::emcal
{

void BCVisualization::configure(const boost::property_tree::ptree& config)
{
  ILOG(Info, Support) << "Configuring BC visualization" << ENDM;
  auto taskConfig = config.get_child_optional("qc.postprocessing." + getID() + ".configuration");
  if (taskConfig) {
    auto cfgMethod = taskConfig.get().get_child_optional("Method");
    if (cfgMethod) {
      mShiftEvaluation = getMethod(cfgMethod->get_value<std::string>());
      ILOG(Info, Support) << "Applying method for BC shift evaluation: " << getMethodString(mShiftEvaluation) << ENDM;
      if (mShiftEvaluation == MethodBCShift_t::UNKNOWN) {
        ILOG(Warning, Support) << "Unknown method for shift evaluation - BC shift cannot be determined" << ENDM;
      }
    }

    auto cfgMinEntries = taskConfig.get().get_child_optional("MinEntries");
    if (cfgMinEntries) {
      mMinNumberOfEntriesBCShift = cfgMinEntries.get().get_value<int>();
      ILOG(Info, Support) << "Request min. entries in control bunch(es): " << mMinNumberOfEntriesBCShift << ENDM;
    }
  }
  auto dataSources = config.get_child("qc.postprocessing." + getID() + ".dataSources");
  for (auto& dataSourceConfig : dataSources) {
    mDataPath = dataSourceConfig.second.get<std::string>("path");
  }
  ILOG(Info) << "Using data path: " << mDataPath << ENDM;
  ILOG(Info) << "Configuration done" << ENDM;
}

void BCVisualization::initialize(Trigger, framework::ServiceRegistryRef)
{
  QcInfoLogger::setDetector("EMC");
  ILOG(Debug, Devel) << "initialize BC plotter" << ENDM;
  mOutputCanvas = new TCanvas("BCCompEMCvsCTP", "Comparison BC EMC vs CTP", 800, 600);
  getObjectsManager()->startPublishing(mOutputCanvas);
}

void BCVisualization::update(Trigger t, framework::ServiceRegistryRef services)
{
  constexpr unsigned int LHC_MAX_BC = o2::constants::lhc::LHCMaxBunches;
  auto& qcdb = services.get<quality_control::repository::DatabaseInterface>();
  auto moBCEMC = qcdb.retrieveMO(mDataPath, "BCEMCALReadout", t.timestamp, t.activity),
       moBCCTP = qcdb.retrieveMO(mDataPath, "BCCTPEMCALAny", t.timestamp, t.activity);

  if (moBCCTP == nullptr || moBCEMC == nullptr) {
    ILOG(Error, Support) << "at least one of the expected objects (BCEMCALReadout and BCCTPEMCALAny) could not be "
                         << "retrieved, skipping this update" << ENDM;
    return;
  }
  mOutputCanvas->Clear();
  mOutputCanvas->cd();
  auto histBCEMC = static_cast<TH1*>(moBCEMC->getObject()->Clone()),
       histBCCTP = static_cast<TH1*>(moBCCTP->getObject()->Clone());
  histBCEMC->SetDirectory(nullptr);
  histBCCTP->SetDirectory(nullptr);
  double yrange = 1.5 * std::max(histBCCTP->GetBinContent(histBCCTP->GetMaximumBin()), histBCEMC->GetBinContent(histBCEMC->GetMaximumBin()));
  if (!mFrame) {
    mFrame = new TH1F("frameBCPlot", "Trigger BC comparison CTP - EMC; BC ID; Number of BCs", LHC_MAX_BC, -0.5, LHC_MAX_BC - 0.5);
    mFrame->SetStats(false);
    mFrame->GetYaxis()->SetRangeUser(0, yrange);
  } else {
    mFrame->GetYaxis()->SetRangeUser(0, yrange);
  }
  mFrame->Draw("axis");

  histBCEMC->SetStats(false);
  histBCEMC->SetMarkerColor(kBlue);
  histBCEMC->SetLineColor(kBlue);
  histBCEMC->SetMarkerStyle(20);
  histBCEMC->Draw("epsame");

  histBCCTP->SetStats(false);
  histBCCTP->SetMarkerColor(kRed);
  histBCCTP->SetLineColor(kRed);
  histBCCTP->SetMarkerStyle(25);
  histBCCTP->Draw("epsame");

  auto leg = new TLegend(0.6, 0.7, 0.89, 0.89);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->SetTextFont(42);
  leg->AddEntry(histBCEMC, "EMC triggers, EMC data", "lep");
  leg->AddEntry(histBCCTP, "EMC triggers, CTP data", "lep");
  leg->Draw();

  std::string message = "Shift cannot be obtained";
  Color_t textcolor = kBlack;
  if (mShiftEvaluation != MethodBCShift_t::UNKNOWN && histBCEMC->GetEntries() && histBCCTP->GetEntries()) {
    auto [status, bcshift] = determineBCShift(histBCEMC, histBCCTP, mShiftEvaluation);
    if (status) {
      message = "BC shift EMC vs CTP: " + std::to_string(bcshift) + " BCs";
      if (bcshift == 0) {
        textcolor = kGreen + 2;
      } else {
        textcolor = kRed;
      }
    }
  }

  auto shiftlabel = new TPaveText(0.15, 0.8, 0.5, 0.89, "NDC");
  shiftlabel->SetBorderSize(0);
  shiftlabel->SetFillStyle(0);
  shiftlabel->SetTextFont(42);
  shiftlabel->SetTextColor(textcolor);
  shiftlabel->AddText(message.data());
  shiftlabel->Draw();
}

void BCVisualization::finalize(Trigger t, framework::ServiceRegistryRef)
{
  getObjectsManager()->stopPublishing(mOutputCanvas);
  delete mOutputCanvas;
  mOutputCanvas = nullptr;
}

void BCVisualization::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Support) << "Resetting the histogram" << ENDM;
}

std::tuple<bool, int> BCVisualization::determineBCShift(const TH1* emchist, const TH1* ctphist, MethodBCShift_t method) const
{
  switch (method) {
    case MethodBCShift_t::ISOLATED_BC:
      return determineBCShiftIsolated(emchist, ctphist);
    case MethodBCShift_t::LEADING_BC:
      return determineBCShiftLeading(emchist, ctphist);
    default:
      break;
  }
  return std::make_tuple(false, 0);
}
std::tuple<bool, int> BCVisualization::determineBCShiftIsolated(const TH1* emchist, const TH1* ctphist) const
{
  auto isolatedEMC = getIsolatedBCs(emchist),
       isolatedCTP = getIsolatedBCs(ctphist);

  std::stringstream emcmaker;
  bool first = true;
  for (auto bc : isolatedEMC) {
    if (first) {
      first = false;
    } else {
      emcmaker << ", ";
    }
    emcmaker << bc;
  }
  ILOG(Debug, Support) << "isolated bcs emc: " << emcmaker.str() << ENDM;
  std::stringstream ctpmaker;
  first = true;
  for (auto bc : isolatedCTP) {
    if (first) {
      first = false;
    } else {
      ctpmaker << ", ";
    }
    ctpmaker << bc;
  }
  ILOG(Debug, Support) << "isolated bcs ctp: " << emcmaker.str() << ENDM;

  int shift = 0;
  bool status = false;
  if (isolatedEMC.size() && isolatedCTP.size()) {
    int currentshift = 0;
    int currentmax = 0;
    for (auto opt : isolatedCTP) {
      currentshift = opt - isolatedEMC[0];
      auto shiftedEMC = getShifted(isolatedEMC, currentshift);
      auto entriesOverlapping = getNMatching(shiftedEMC, isolatedCTP);
      if (entriesOverlapping > currentmax) {
        currentmax = entriesOverlapping;
        shift = currentshift;
      }
    }
    status = true;
  }
  return std::make_tuple(status, shift);
}

std::tuple<bool, int> BCVisualization::determineBCShiftLeading(const TH1* emchist, const TH1* ctphist) const
{
  auto leadingBCEMC = getLeadingBC(emchist),
       leadingBCCTP = getLeadingBC(ctphist);
  bool status = false;
  int shift = 0;
  if (emchist->GetBinContent(leadingBCEMC + 1) > mMinNumberOfEntriesBCShift && ctphist->GetBinContent(leadingBCCTP + 1) > mMinNumberOfEntriesBCShift) {
    status = true;
    shift = leadingBCCTP - leadingBCEMC;
  }
  return std::make_tuple(status, shift);
}

std::vector<int> BCVisualization::getIsolatedBCs(const TH1* bchist) const
{
  std::vector<int> bcs;
  for (int ibc = 1; ibc < bchist->GetXaxis()->GetNbins(); ibc++) {
    auto nentriesBefore = bchist->GetBinContent(ibc),
         nentriesBC = bchist->GetBinContent(ibc + 1),
         nentriesAfter = bchist->GetBinContent(ibc + 2);
    if (nentriesBC < mMinNumberOfEntriesBCShift) {
      continue;
    }
    if (nentriesBefore < 0.1 * nentriesBC && nentriesAfter < nentriesBC * 0.1) {
      bcs.emplace_back(ibc);
    }
  }
  return bcs;
}

std::vector<int> BCVisualization::getShifted(const std::vector<int>& bcEMC, int shift) const
{
  std::vector<int> result;
  for (auto bc : bcEMC) {
    auto shifted = bc - shift;
    if (shifted < 0) {
      shifted += o2::constants::lhc::LHCMaxBunches;
    }
    if (shifted >= o2::constants::lhc::LHCMaxBunches) {
      shifted -= o2::constants::lhc::LHCMaxBunches;
    }
    result.emplace_back(shifted);
  }
  return result;
}

int BCVisualization::getNMatching(const std::vector<int>& bcShiftedEMC, const std::vector<int>& bcCTP) const
{
  int entriesOverlapping = 0;
  for (auto bcEMC : bcShiftedEMC) {
    if (std::find(bcCTP.begin(), bcCTP.end(), bcEMC) != bcCTP.end()) {
      entriesOverlapping++;
    }
  }
  return entriesOverlapping;
}

int BCVisualization::getLeadingBC(const TH1* hist) const
{
  int currentBC = 0;
  double currentEntries = -1;
  for (int ibc = 0; ibc < o2::constants::lhc::LHCMaxBunches; ibc++) {
    auto counts = hist->GetBinContent(ibc + 1);
    if (counts > currentEntries) {
      currentBC = ibc;
      currentEntries = counts;
    }
  }
  return currentBC;
}

BCVisualization::MethodBCShift_t BCVisualization::getMethod(const std::string_view methodstring) const
{
  auto methodstringUpper = boost::algorithm::to_upper_copy(std::string(methodstring));
  if (methodstringUpper == "LEADING_BC") {
    return MethodBCShift_t::LEADING_BC;
  }
  if (methodstringUpper == "ISOLATED_BC") {
    return MethodBCShift_t::ISOLATED_BC;
  }
  return MethodBCShift_t::UNKNOWN;
}

std::string BCVisualization::getMethodString(MethodBCShift_t method) const
{
  switch (method) {
    case MethodBCShift_t::LEADING_BC:
      return "Leading_BC";

    case MethodBCShift_t::ISOLATED_BC:
      return "Isolated_BC";

    default:
      break;
  }
  return "Unknown";
}

} // namespace o2::quality_control_modules::emcal
