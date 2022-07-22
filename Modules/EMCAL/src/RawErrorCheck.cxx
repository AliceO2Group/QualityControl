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

#include <algorithm>
#include <array>
#include <string>
#include <boost/algorithm/string.hpp>

#include "EMCALBase/Geometry.h"

#include "EMCAL/RawErrorCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>

#include <DataFormatsQualityControl/FlagReasons.h>

using namespace std;
using namespace o2::quality_control;

namespace o2::quality_control_modules::emcal
{

void RawErrorCheck::configure()
{
  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);

  // switch on/off messages on the infoLogger
  auto switchNotifyIL = mCustomParameters.find("NotifyInfologger");
  if (switchNotifyIL != mCustomParameters.end()) {
    try {
      mNotifyInfologger = decodeBool(switchNotifyIL->second);
    } catch (std::exception& e) {
      ILOG(Error, Support) << e.what() << ENDM;
    }
  }
}

Quality RawErrorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::array<std::string, 7> errorhists = { { "RawDataErrors", "PageErrors", "MajorAltroErrors", "MinorAltroError", "RawFitError", "GeometryError", "GainTypeError" } };
  std::array<std::string, 2> gainhists = { { "NoHGPerDDL", "NoLGPerDDL" } };
  std::array<std::string, 2> channelgainhists = { { "ChannelLGnoHG", "ChannelHGnoLG" } };
  Quality result = Quality::Good;

  for (auto& [moName, mo] : *moMap) {
    if (std::find(errorhists.begin(), errorhists.end(), mo->getName()) != errorhists.end()) {
      // Check for presence of error codes
      auto* errorhist = dynamic_cast<TH2*>(mo->getObject());

      for (int linkID = 0; linkID < errorhist->GetXaxis()->GetNbins(); linkID++) {
        for (int errorcode = 0; errorcode < errorhist->GetYaxis()->GetNbins(); errorcode++) {
          if (errorhist->GetBinContent(linkID + 1, errorcode + 1)) {
            // Found raw data error
            if (result != Quality::Bad) {
              result = Quality::Bad;
            }
            result.addReason(FlagReasonFactory::Unknown(), "Raw error " + std::string(errorhist->GetYaxis()->GetBinLabel(errorcode + 1)) + " found in DDL " + std::to_string(linkID));
          }
        }
      }
    } else if (std::find(gainhists.begin(), gainhists.end(), mo->GetName()) != gainhists.end()) {
      // Find FEC with gain error
      auto errorhist = dynamic_cast<TH2*>(mo->getObject());
      std::string errortype;
      std::string_view histname = mo->GetName();
      if (histname == "NoHGPerDDL") {
        errortype = "LGnoHG";
      } else {
        errortype = "HGnoLG";
      }
      for (int linkID = 0; linkID < errorhist->GetXaxis()->GetNbins(); linkID++) {
        for (int fecID = 0; fecID < errorhist->GetYaxis()->GetNbins(); fecID++) {
          if (errorhist->GetBinContent(linkID + 1, fecID + 1)) {
            if (result != Quality::Bad) {
              result = Quality::Bad;
            }
            result.addReason(FlagReasonFactory::Unknown(), "Gain error " + errortype + " in FEC " + std::to_string(fecID) + " of DDL " + std::to_string(linkID));
            ILOG(Debug, Support) << "Detected " << errortype << " in FEC " << fecID << " of DDL " << linkID << ENDM;
          }
        }
      }
    } else if (std::find(channelgainhists.begin(), channelgainhists.end(), mo->GetName()) != channelgainhists.end()) {
      auto errorhist = dynamic_cast<TH2*>(mo->getObject());
      std::string errortype;
      std::string_view histname = mo->GetName();
      if (histname == "ChannelLGnoHG") {
        errortype = "LGnoHG";
      } else {
        errortype = "HGnoLG";
      }
      for (int column = 0; column < 96; column++) {
        for (int row = 0; row < 208; row++) {
          if (errorhist->GetBinContent(column + 1, row + 1)) {
            if (result != Quality::Bad) {
              result = Quality::Bad;
            }
            // Get position in supermodule
            auto [supermoduleID, rowSM, columnSM] = mGeometry->GetPositionInSupermoduleFromGlobalRowCol(row, column);
            result.addReason(FlagReasonFactory::Unknown(), "Gain error " + errortype + " in channel col " + std::to_string(column) + " row " + std::to_string(column) + " (SM " + std::to_string(supermoduleID) + ", row " + std::to_string(rowSM) + " col " + std::to_string(columnSM) + ")");
            ILOG(Debug, Support) << "Detected " << errortype << " in column " << column << " row  " << row << " ( SM " << supermoduleID << ", " << columnSM << ", " << rowSM << ")" << ENDM;
          }
        }
      }
    } else {
      ILOG(Error, Support) << "Unsupported histogram in check: " << mo->GetName() << ENDM;
      continue;
    }
  }
  return result;
}

std::string RawErrorCheck::getAcceptedType() { return "TH2"; }

void RawErrorCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* h = dynamic_cast<TH1*>(mo->getObject());
  if (checkResult == Quality::Good) {
    TLatex* msg = new TLatex(0.2, 0.8, "#color[418]{No Error: OK}");
    msg->SetNDC();
    msg->SetTextSize(16);
    msg->SetTextFont(43);
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
  } else if (checkResult == Quality::Bad) {
    TLatex* msg = new TLatex(0.2, 0.8, "#color[2]{Presence of Error Code: call EMCAL oncall}");
    msg->SetNDC();
    msg->SetTextSize(16);
    msg->SetTextFont(43);
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    // Notify about found errors on the infoLogger:
    if (mNotifyInfologger) {
      for (const auto& reason : checkResult.getReasons()) {
        ILOG(Warning, Support) << "Raw Error in " << mo->GetName() << " found: " << reason.second << ENDM;
      }
    }
  }
}

bool RawErrorCheck::decodeBool(std::string value) const
{
  boost::algorithm::to_lower_copy(value);
  if (value == "true") {
    return true;
  }
  if (value == "false") {
    return false;
  }
  throw std::runtime_error(fmt::format("Value {} not a boolean", value.data()).data());
}

} // namespace o2::quality_control_modules::emcal
