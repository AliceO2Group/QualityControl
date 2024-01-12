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
/// \file   RawDataReaderCheck.cxx
/// \author Lucia Anna Tarasovicova
///

#include "CTP/RawDataReaderCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsCTP/RunManager.h"
// ROOT
#include <TH1.h>
#include <TLatex.h>
#include "DataFormatsCTP/Configuration.h"
#include "DataFormatsParameters/GRPLHCIFData.h"

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::ctp;

namespace o2::quality_control_modules::ctp
{

void RawDataReaderCheck::configure() {}

Quality RawDataReaderCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  // you can get details about the activity via the object mActivity:
  ILOG(Debug, Devel) << "Run " << getActivity()->mId << ", type: " << getActivity()->mType << ", beam: " << getActivity()->mBeamType << ENDM;
  // and you can get your custom parameters:
  ILOG(Debug, Devel) << "custom param physics.pp.myOwnKey1 : " << mCustomParameters.atOrDefaultValue("myOwnKey1", "physics", "pp", "default_value") << ENDM;

  mRunNumber = getActivity()->mId;

  map<string, string> metadata; // can be empty
  auto mo = moMap->begin()->second;
  mTimestamp = mo->getValidity().getMin();
  auto lhcifdata = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF", metadata, mTimestamp);
  auto bfilling = lhcifdata->getBunchFilling();
  std::vector<int> bcs = bfilling.getFilledBCs();
  o2::ctp::BCMask* bcmask;
  std::bitset<o2::constants::lhc::LHCMaxBunches> lhcBC_bitset = bcmask->BCmask;
  lhcBC_bitset.reset();

  for (auto const& bc : bcs) {
    lhcBC_bitset.set(bc, 1);
  }

  int threshold = 10;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "histobc") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());

      result = Quality::Good;
      result.addMetadata("BC_id", "good");
      for (int i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        if (lhcBC_bitset[i] && h->GetBinContent(i + 1) < threshold) {
          result = Quality::Bad;
          result.updateMetadata(Form("BC_id%d", i + 1), "bad");
          result.addReason(quality_control::FlagReasonFactory::Unknown(), "Bunch crossing is expected, but not measured");
          break;
        }
        if (lhcBC_bitset[i] && h->GetBinContent(i + 1) == threshold) {
          result = Quality::Medium;
          result.updateMetadata(Form("BC_id%d", i + 1), "medium");
          result.addReason(quality_control::FlagReasonFactory::Unknown(), "Bunch crossing is expected, at the threshold of background");
        }
        if (!lhcBC_bitset[i] && h->GetBinContent(i + 1) == threshold) {
          result = Quality::Medium;
          result.updateMetadata(Form("BC_id%d", i + 1), "medium");
          result.addReason(quality_control::FlagReasonFactory::Unknown(), "Bunch crossing not expected, at the threshold of background");
        }
        if (!lhcBC_bitset[i] && h->GetBinContent(i + 1) > threshold) {
          result = Quality::Bad;
          result.updateMetadata(Form("BC_id%d", i + 1), "bad");
          result.addReason(quality_control::FlagReasonFactory::Unknown(), "Bunch crossing not expected, but measured");
          break;
        }
      }
    }
  }
  return result;
}

std::string RawDataReaderCheck::getAcceptedType() { return "TH1"; }

void RawDataReaderCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::shared_ptr<TLatex> msg;
  if (mo->getName() == "histobc") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetLineColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      ILOG(Debug, Devel) << "Quality::Bad, setting to red" << ENDM;
      msg = std::make_shared<TLatex>(0.15, 0.8, Form("Reason: %s", (std::get<1>(checkResult.getReasons()[0])).c_str()));
      msg->SetTextColor(kRed);
      msg->SetTextSize(0.06);
      msg->SetTextFont(43);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      h->SetLineColor(kRed);
    } else if (checkResult == Quality::Medium) {
      ILOG(Debug, Devel) << "Quality::medium, setting to orange" << ENDM;
      msg = std::make_shared<TLatex>(0.15, 0.8, Form("Reason: %s", (std::get<1>(checkResult.getReasons()[0])).c_str()));
      msg->SetTextColor(kOrange);
      msg->SetTextSize(0.06);
      msg->SetTextFont(43);
      msg->SetNDC();
      h->GetListOfFunctions()->Add(msg->Clone());
      h->SetLineColor(kOrange);
    }
  }
}

} // namespace o2::quality_control_modules::ctp
