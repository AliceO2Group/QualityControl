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
#include <DataFormatsCTP/Configuration.h>
#include <DataFormatsParameters/GRPLHCIFData.h>

#include <DataFormatsQualityControl/FlagReasons.h>

#include <DetectorsBase/GRPGeomHelper.h>

using namespace std;
using namespace o2::quality_control;
using namespace o2::ctp;

namespace o2::quality_control_modules::ctp
{

void RawDataReaderCheck::configure() {}

Quality RawDataReaderCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;

  map<string, string> metadata; // can be empty

  ILOG(Info, Support) << "Check timeStamp.   " << mTimestamp << ENDM;
  auto lhcifdata = UserCodeInterface::retrieveConditionAny<o2::parameters::GRPLHCIFData>("GLO/Config/GRPLHCIF", metadata, mTimestamp);
  auto bfilling = lhcifdata->getBunchFilling();

  std::vector<int> bcs = bfilling.getFilledBCs();
  o2::ctp::BCMask* bcmask;
  std::bitset<o2::constants::lhc::LHCMaxBunches> lhcBC_bitset = bcmask->BCmask;
  lhcBC_bitset.reset();
  vBadBC.clear();
  vMediumBC.clear();
  vGoodBC.clear();

  for (auto const& bc : bcs) {
    lhcBC_bitset.set(bc, 1);
  }
  ILOG(Info, Support) << "Number of BC " << bcs.size() << ENDM;
  ILOG(Info, Support) << "Filling Scheme" << bfilling.getPattern() << ENDM;

  for (auto& [moName, mo] : *moMap) {

    (void)moName;
    if (mo->getName() == "bcMTVX") {
      auto* h = dynamic_cast<TH1F*>(mo->getObject());
      mThreshold = h->GetEntries() / getNumberFilledBins(h);
      mThreshold = mThreshold - sqrt(mThreshold);

      for (int i = 0; i < o2::constants::lhc::LHCMaxBunches; i++) {
        if (lhcBC_bitset[i])
          ILOG(Info, Support) << i << " ";
        if (lhcBC_bitset[i] && h->GetBinContent(i + 1) <= mThreshold) {
          vMediumBC.push_back(i);
        } else if (!lhcBC_bitset[i] && h->GetBinContent(i + 1) > mThreshold) {
          vBadBC.push_back(i);
        } else if (lhcBC_bitset[i] && h->GetBinContent(i + 1) > mThreshold) {
          vGoodBC.push_back(i);
        }
      }
    }
  }

  if (vBadBC.size() > 0)
    result = Quality::Bad;
  else if (vMediumBC.size() > 0)
    result = Quality::Medium;
  else
    result = Quality::Good;

  return result;
}

std::string RawDataReaderCheck::getAcceptedType() { return "TH1"; }

void RawDataReaderCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  std::shared_ptr<TLatex> msg;
  if (mo->getName() == "bcMTVX") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    msg = std::make_shared<TLatex>(0.5, 0.8, Form("Overall Quality: %s", (checkResult.getName()).c_str()));
    if (checkResult == Quality::Bad)
      msg->SetTextColor(kRed);
    else if (checkResult == Quality::Medium)
      msg->SetTextColor(kOrange);
    else
      msg->SetTextColor(kGreen + 1);
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.75, Form("Number of good BC: %lu", vGoodBC.size()));
    msg->SetTextColor(kBlack);
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.7, Form("Number of medium BC: %lu", vMediumBC.size()));
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.65, Form("Number of bad BC: %lu", vBadBC.size()));
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    msg = std::make_shared<TLatex>(0.5, 0.6, Form("Threshold : %f", mThreshold));
    msg->SetTextSize(0.03);
    msg->SetNDC();
    h->GetListOfFunctions()->Add(msg->Clone());

    h->GetYaxis()->SetRangeUser(0, h->GetMaximum() * 1.5);
  }
}

int RawDataReaderCheck::getNumberFilledBins(TH1F* hist)
{

  int nBins = hist->GetXaxis()->GetNbins();
  int filledBins = 0;
  for (int i = 0; i < nBins; i++) {
    if (hist->GetBinContent(i + 1) > 0)
      filledBins++;
  }
  return filledBins;
}

void RawDataReaderCheck::startOfActivity(const core::Activity& activity)
{
  ILOG(Debug, Devel) << "RawDataReaderCheck::start : " << activity.mId << ENDM;
  mRunNumber = activity.mId;
  mTimestamp = activity.mValidity.getMin();
}

} // namespace o2::quality_control_modules::ctp
