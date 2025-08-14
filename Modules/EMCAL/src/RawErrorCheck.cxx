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

#include "DataFormatsEMCAL/ErrorTypeFEE.h"
#include "EMCALBase/Geometry.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/CaloRawFitter.h"
#include "EMCALReconstruction/RawDecodingError.h"
#include "EMCALReconstruction/ReconstructionErrors.h"

#include "EMCAL/RawErrorCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TH1.h>
#include <TH2.h>
#include <TLatex.h>
#include <TList.h>
#include <TLine.h>

#include <DataFormatsQualityControl/FlagType.h>
#include <DataFormatsQualityControl/FlagTypeFactory.h>

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
  const std::string keyThreshRawdataErrors = "ThresholdRDE",
                    keyThreshPageError = "ThresholdPE",
                    keyThreshMajorAltroError = "ThresholdMAAE",
                    keyThreshMinorAltroError = "ThresholdMIAE",
                    keyThresRawFitError = "ThresholdRFE",
                    keyThresholdGeometryError = "ThresholdGEE",
                    keyThresholdGainTypeError = "ThresholdGTE";

  try {
    for (auto& [param, value] : mCustomParameters.getAllDefaults()) {
      if (param.find(keyThreshRawdataErrors) == 0) {
        auto errortype = param.substr(keyThreshRawdataErrors.length());
        auto errorcode = findErrorCodeRDE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram RawDataErrors: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdRDE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram RawDataErrors: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram RawDataErrors: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      // Raw data error summary hists start with RDESummaryErr for error values and RDESummaryWarn for warning values
      std::string strSummaryErr = "RDESummaryErr";
      if (param.starts_with(strSummaryErr)) {
        auto errortype = param.substr(strSummaryErr.length());
        auto errorcode = findErrorCodeRDE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram RawDataErrors Summary Errors: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdRDESummary[errorcode][0] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram RawDataErrors Summary Errors: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram RawDataErrors Summary Errors: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      std::string strSummaryWarn = "RDESummaryWarn";
      if (param.starts_with(strSummaryWarn)) {
        auto errortype = param.substr(strSummaryWarn.length());
        auto errorcode = findErrorCodeRDE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram RawDataErrors Summary Warning: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdRDESummary[errorcode][1] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram RawDataErrors Summary Warning: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram RawDataErrors Summary Warning: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      if (param.find(keyThreshPageError) == 0) {
        auto errortype = param.substr(keyThreshPageError.length());
        auto errorcode = findErrorCodePE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram PageErrors: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdPE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram PageErrors: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram PageErrors: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      if (param.find(keyThreshMajorAltroError) == 0) {
        auto errortype = param.substr(keyThreshMajorAltroError.length());
        auto errorcode = findErrorCodeMAAE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram MajorAltroErrors: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdMAAE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram MajorAltroErrors: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram MajorAltroErrors: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      if (param.find(keyThreshMinorAltroError) == 0) {
        auto errortype = param.substr(keyThreshMinorAltroError.length());
        auto errorcode = findErrorCodeMIAE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram MinorAltroError: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdMAAE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram MinorAltroError: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram MinorAltroError: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      if (param.find(keyThresRawFitError) == 0) {
        auto errortype = param.substr(keyThresRawFitError.length());
        auto errorcode = findErrorCodeRFE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram RawFitError: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdRFE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram RawFitError: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram RawFitError: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      if (param.find(keyThresholdGeometryError) == 0) {
        auto errortype = param.substr(keyThresholdGeometryError.length());
        auto errorcode = findErrorCodeGEE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram GeometryError: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdGEE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram GeometryError: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram GeometryError: Requested error type " << errortype << " not found" << ENDM;
        }
      }

      if (param.find(keyThresholdGainTypeError) == 0) {
        auto errortype = param.substr(keyThresholdGainTypeError.length());
        auto errorcode = findErrorCodeGTE(errortype);
        if (errorcode > -1) {
          try {
            auto threshold = std::stoi(value);
            ILOG(Info) << "Setting custom threshold in Histogram GainTypeError: " << errortype << " <= " << threshold << ENDM;
            mErrorCountThresholdGTE[errorcode] = threshold;
          } catch (...) {
            ILOG(Error) << "Thresholds for histogram GainTypeError: Failure in decoding threshold value (" << value << ") for error type " << errortype << ENDM;
          }
        } else {
          ILOG(Error) << "Thresholds for histogram GainTypeError: Requested error type " << errortype << " not found" << ENDM;
        }
      }
    }
  } catch (std::out_of_range& e) {
    // Nothing to be done, no parameter found.
    ILOG(Debug) << "Error in parameter extraction" << ENDM;
  }
}

Quality RawErrorCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::array<std::string, 1> errorSummaryHists = { { "RawDataErrors" } };
  std::array<std::string, 6> errorhists = { { "PageErrors", "MajorAltroErrors", "MinorAltroError", "RawFitError", "GeometryError", "GainTypeError" } };
  std::array<std::string, 2> gainhists = { { "NoHGPerDDL", "NoLGPerDDL" } };
  std::array<std::string, 2> channelgainhists = { { "ChannelLGnoHG", "ChannelHGnoLG" } };
  Quality result = Quality::Good;

  std::map<std::string, const std::map<int, int>*> thresholdConfigErrorHists = {
    { "RawDataErrors", &mErrorCountThresholdRDE },
    { "PageErrors", &mErrorCountThresholdPE },
    { "MajorAltroErrors", &mErrorCountThresholdMAAE },
    { "MinorAltroError", &mErrorCountThresholdMIAE },
    { "RawFitError", &mErrorCountThresholdRFE },
    { "GeometryError", &mErrorCountThresholdGEE },
    { "GainTypeError", &mErrorCountThresholdGTE }
  };

  for (auto& [moName, mo] : *moMap) {
    if (std::find(errorSummaryHists.begin(), errorSummaryHists.end(), mo->getName()) != errorSummaryHists.end()) {
      // Check for presence of error codes
      auto* errorhist = dynamic_cast<TH2*>(mo->getObject());

      for (int errorcode = 0; errorcode < errorhist->GetYaxis()->GetNbins(); errorcode++) {
        // try to find a threshold for the number of errors per bin
        int threshold = 0;
        auto thresholdHandler = thresholdConfigErrorHists.find(mo->getName());
        if (thresholdHandler != thresholdConfigErrorHists.end()) {
          auto thresholdFound = thresholdHandler->second->find(errorcode);
          if (thresholdFound != thresholdHandler->second->end()) {
            threshold = thresholdFound->second;
          }
        }
        // try to find the threshold for the number of links from when on it is considered to be warning ot bad
        int thresholdTotalErrBad = 0;
        int thresholdTotalErrWarn = 0;
        if (mErrorCountThresholdRDESummary.contains(errorcode)) {
          thresholdTotalErrBad = mErrorCountThresholdRDESummary[errorcode][0];
          thresholdTotalErrWarn = mErrorCountThresholdRDESummary[errorcode][1];
        }

        int numErrors = 0;
        for (int linkID = 0; linkID < errorhist->GetXaxis()->GetNbins(); linkID++) {
          int nErr = errorhist->GetBinContent(linkID + 1, errorcode + 1);
          if (nErr > threshold) {
            numErrors++;
          }
        }

        if (numErrors > thresholdTotalErrBad) { // Number of raw error exceeds the threshold and is considered to be bad
          if (result != Quality::Bad) {
            result = Quality::Bad;
          }
          result.addFlag(FlagTypeFactory::Unknown(), "Raw error " + std::string(errorhist->GetYaxis()->GetBinLabel(errorcode + 1)) + " above critical threshold: Call oncall!");

        } else if (numErrors > thresholdTotalErrWarn) { // Number of raw error exceeds the threshold but is considered to be okay. Error can be fixed at beam dump
          if (result != Quality::Medium) {
            result = Quality::Medium;
          }
          result.addFlag(FlagTypeFactory::Unknown(), "Raw error " + std::string(errorhist->GetYaxis()->GetBinLabel(errorcode + 1)) + " below critical threshold: Call oncall at beam dump");
        }
      }
    } else if (std::find(errorhists.begin(), errorhists.end(), mo->getName()) != errorhists.end()) {
      // Check for presence of error codes
      auto* errorhist = dynamic_cast<TH2*>(mo->getObject());

      for (int errorcode = 0; errorcode < errorhist->GetYaxis()->GetNbins(); errorcode++) {
        // try to find a threshold
        int threshold = 0;
        auto thresholdHandler = thresholdConfigErrorHists.find(mo->getName());
        if (thresholdHandler != thresholdConfigErrorHists.end()) {
          auto thresholdFound = thresholdHandler->second->find(errorcode);
          if (thresholdFound != thresholdHandler->second->end()) {
            threshold = thresholdFound->second;
          }
        }
        int numErrors = 0;
        for (int linkID = 0; linkID < errorhist->GetXaxis()->GetNbins(); linkID++) {
          numErrors += errorhist->GetBinContent(linkID + 1, errorcode + 1);
        }
        if (numErrors > threshold) {
          // Found number of raw data errors is above threshold for bin
          if (result != Quality::Bad) {
            result = Quality::Bad;
          }
          result.addFlag(FlagTypeFactory::Unknown(), "Raw error " + std::string(errorhist->GetYaxis()->GetBinLabel(errorcode + 1)) + " above critical threshold: Call oncall!");
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
            result.addFlag(FlagTypeFactory::Unknown(), "Gain error " + errortype + " in FEC " + std::to_string(fecID) + " of DDL " + std::to_string(linkID));
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
            result.addFlag(FlagTypeFactory::Unknown(), "Gain error " + errortype + " in channel col " + std::to_string(column) + " row " + std::to_string(column) + " (SM " + std::to_string(supermoduleID) + ", row " + std::to_string(rowSM) + " col " + std::to_string(columnSM) + ")");
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
      for (const auto& flag : checkResult.getFlags()) {
        ILOG(Warning, Devel) << "Raw Error in " << mo->GetName() << " found: " << flag.second << ENDM;
      }
    }
  } else if (checkResult == Quality::Medium) {
    TLatex* msg = new TLatex(0.2, 0.8, "#color[802]{Non-critical error codes present: call EMCAL oncall at beam dump}");
    msg->SetNDC();
    msg->SetTextSize(16);
    msg->SetTextFont(43);
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    // Notify about found errors on the infoLogger:
    if (mNotifyInfologger) {
      for (const auto& flag : checkResult.getFlags()) {
        ILOG(Warning, Devel) << "Non-critical raw Error in " << mo->GetName() << " found: " << flag.second << ENDM;
      }
    }
  }
  // SM grid
  if (mo->getName().find("Channel") != std::string::npos) {
    auto* h2D = dynamic_cast<TH2*>(mo->getObject());
    // orizontal
    TLine* l1 = new TLine(-0.5, 24, 95.5, 24);
    TLine* l2 = new TLine(-0.5, 48, 95.5, 48);
    TLine* l3 = new TLine(-0.5, 72, 95.5, 72);
    TLine* l4 = new TLine(-0.5, 96, 95.5, 96);
    TLine* l5 = new TLine(-0.5, 120, 95.5, 120);
    TLine* l6 = new TLine(-0.5, 128, 95.5, 128);

    TLine* l7 = new TLine(-0.5, 152, 31.5, 152);
    TLine* l8 = new TLine(63.5, 152, 95.5, 152);

    TLine* l9 = new TLine(-0.5, 176, 31.5, 176);
    TLine* l10 = new TLine(63.5, 176, 95.5, 176);

    TLine* l11 = new TLine(-0.5, 200, 95.5, 200);
    TLine* l12 = new TLine(47.5, 200, 47.5, 207.5);

    // vertical
    TLine* l13 = new TLine(47.5, -0.5, 47.5, 128);
    TLine* l14 = new TLine(31.5, 128, 31.5, 200);
    TLine* l15 = new TLine(63.5, 128, 63.5, 200);

    h2D->GetListOfFunctions()->Add(l1);
    h2D->GetListOfFunctions()->Add(l2);
    h2D->GetListOfFunctions()->Add(l3);
    h2D->GetListOfFunctions()->Add(l4);
    h2D->GetListOfFunctions()->Add(l5);
    h2D->GetListOfFunctions()->Add(l6);
    h2D->GetListOfFunctions()->Add(l7);
    h2D->GetListOfFunctions()->Add(l8);
    h2D->GetListOfFunctions()->Add(l9);
    h2D->GetListOfFunctions()->Add(l10);
    h2D->GetListOfFunctions()->Add(l11);
    h2D->GetListOfFunctions()->Add(l12);
    h2D->GetListOfFunctions()->Add(l13);
    h2D->GetListOfFunctions()->Add(l14);
    h2D->GetListOfFunctions()->Add(l15);

    l1->Draw();
    l2->Draw();
    l3->Draw();
    l4->Draw();
    l5->Draw();
    l6->Draw();
    l7->Draw();
    l8->Draw();
    l9->Draw();
    l10->Draw();
    l11->Draw();
    l12->Draw();
    l13->Draw();
    l14->Draw();
    l15->Draw();
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

int RawErrorCheck::findErrorCodeRDE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::ErrorTypeFEE::getNumberOfErrorTypes(); error++) {
    if (std::string_view(o2::emcal::ErrorTypeFEE::getErrorTypeName(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

int RawErrorCheck::findErrorCodePE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::RawDecodingError::getNumberOfErrorTypes(); error++) {
    if (std::string_view(o2::emcal::RawDecodingError::getErrorCodeNames(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

int RawErrorCheck::findErrorCodeMAAE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::AltroDecoderError::getNumberOfErrorTypes(); error++) {
    if (std::string_view(o2::emcal::AltroDecoderError::getErrorTypeName(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

int RawErrorCheck::findErrorCodeMIAE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::MinorAltroDecodingError::getNumberOfErrorTypes(); error++) {
    if (std::string_view(o2::emcal::MinorAltroDecodingError::getErrorTypeName(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

int RawErrorCheck::findErrorCodeRFE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::CaloRawFitter::getNumberOfErrorTypes(); error++) {
    if (std::string_view(o2::emcal::CaloRawFitter::getErrorTypeName(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

int RawErrorCheck::findErrorCodeGEE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::reconstructionerrors::getNumberOfGeometryErrorCodes(); error++) {
    if (std::string_view(o2::emcal::reconstructionerrors::getGeometryErrorName(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

int RawErrorCheck::findErrorCodeGTE(const std::string_view errorname) const
{
  int result = -1;
  for (int error = 0; error < o2::emcal::reconstructionerrors::getNumberOfGainErrorCodes(); error++) {
    if (std::string_view(o2::emcal::reconstructionerrors::getGainErrorName(error)) == errorname) {
      result = error;
      break;
    }
  }
  return result;
}

} // namespace o2::quality_control_modules::emcal
