// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   RawErrorTask.cxx
/// \author My Name
///

#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <boost/algorithm/string.hpp>

#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/RawErrorTask.h"
#include "EMCALBase/Geometry.h"
#include "EMCALBase/Mapper.h"
#include "EMCALReconstruction/AltroDecoder.h"
#include "EMCALReconstruction/CaloRawFitter.h"
#include "EMCALReconstruction/RawDecodingError.h"
#include "EMCALReconstruction/ReconstructionErrors.h"
#include "DataFormatsEMCAL/ErrorTypeFEE.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

namespace o2::quality_control_modules::emcal
{

RawErrorTask::~RawErrorTask()
{
  // delete mHistogram;
  if (mErrorTypeAltro)
    delete mErrorTypeAltro;

  if (mErrorTypePage)
    delete mErrorTypePage;

  if (mErrorTypeMinAltro)
    delete mErrorTypeMinAltro;

  if (mErrorTypeFit)
    delete mErrorTypeFit;

  if (mErrorTypeGeometry)
    delete mErrorTypeGeometry;

  if (mErrorTypeGain)
    delete mErrorTypeGain;

  if (mErrorTypeUnknown)
    delete mErrorTypeUnknown;

  if (mErrorGainLow)
    delete mErrorGainLow;

  if (mErrorGainHigh)
    delete mErrorGainHigh;

  if (mFecIdMinorAltroError)
    delete mFecIdMinorAltroError;

  // histo per categoty with details
  // histo summary with error per category
}

void RawErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Debug, Devel) << "initialize RawErrorTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  auto get_bool = [](const std::string_view input) -> bool {
    return input == "true";
  };
  mExcludeGainErrorsFromOverview = get_bool(getConfigValueLower("excludeGainErrorFromSummary"));

  constexpr float binshift = -0.5; // shift bin centers of error codes in order to avoid edge effects
  mErrorTypeAll = new TH2F("RawDataErrors", "Raw data errors", 40, 0, 40, o2::emcal::ErrorTypeFEE::getNumberOfErrorTypes(), binshift, o2::emcal::ErrorTypeFEE::getNumberOfErrorTypes() + binshift);
  mErrorTypeAll->GetXaxis()->SetTitle("Link");
  mErrorTypeAll->GetYaxis()->SetTitle("Error type");
  for (int ierror = 0; ierror < o2::emcal::ErrorTypeFEE::getNumberOfErrorTypes(); ierror++) {
    mErrorTypeAll->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::ErrorTypeFEE::getErrorTypeTitle(ierror));
  }
  mErrorTypeAll->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeAll);

  mErrorTypeAltro = new TH2F("MajorAltroErrors", "Major ALTRO decoding errors", 40, 0, 40, o2::emcal::AltroDecoderError::getNumberOfErrorTypes(), binshift, o2::emcal::AltroDecoderError::getNumberOfErrorTypes() + binshift);
  mErrorTypeAltro->GetXaxis()->SetTitle("Link");
  mErrorTypeAltro->GetYaxis()->SetTitle("Error Type");
  for (auto ierror = 0; ierror < o2::emcal::AltroDecoderError::getNumberOfErrorTypes(); ierror++) {
    mErrorTypeAltro->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::AltroDecoderError::getErrorTypeTitle(ierror));
  }
  mErrorTypeAltro->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeAltro);

  mErrorTypePage = new TH2F("PageErrors", "DMA page decoding errors", 40, 0, 40, o2::emcal::RawDecodingError::getNumberOfErrorTypes(), binshift, o2::emcal::RawDecodingError::getNumberOfErrorTypes() + binshift);
  mErrorTypePage->GetXaxis()->SetTitle("Link");
  mErrorTypePage->GetYaxis()->SetTitle("Page Error Type");
  for (int ierror = 0; ierror < o2::emcal::RawDecodingError::getNumberOfErrorTypes(); ierror++) {
    mErrorTypePage->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::RawDecodingError::getErrorCodeTitles(ierror));
  }
  mErrorTypePage->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypePage);

  mErrorTypeMinAltro = new TH2F("MinorAltroError", "Minor ALTRO decoding error", 40, 0, 40, o2::emcal::MinorAltroDecodingError::getNumberOfErrorTypes(), binshift, o2::emcal::MinorAltroDecodingError::getNumberOfErrorTypes() + binshift);
  mErrorTypeMinAltro->GetXaxis()->SetTitle("Link");
  mErrorTypeMinAltro->GetYaxis()->SetTitle("MinorAltro Error Type");
  for (int ierror = 0; ierror < o2::emcal::MinorAltroDecodingError::getNumberOfErrorTypes(); ierror++) {
    mErrorTypeMinAltro->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::MinorAltroDecodingError::getErrorTypeTitle(ierror));
  }
  mErrorTypeMinAltro->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeMinAltro);

  mErrorTypeFit = new TH2F("RawFitError", "Error in raw fitting ", 40, 0, 40, o2::emcal::CaloRawFitter::getNumberOfErrorTypes(), binshift, o2::emcal::CaloRawFitter::getNumberOfErrorTypes() + binshift);
  mErrorTypeFit->GetXaxis()->SetTitle("Link");
  mErrorTypeFit->GetYaxis()->SetTitle("Fit Error Type");
  for (int ierror = 0; ierror < o2::emcal::CaloRawFitter::getNumberOfErrorTypes(); ierror++) {
    mErrorTypeFit->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::CaloRawFitter::getErrorTypeTitle(ierror));
  }
  mErrorTypeFit->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeFit);

  mErrorTypeGeometry = new TH2F("GeometryError", "Geometry error", 40, 0, 40, o2::emcal::reconstructionerrors::getNumberOfGeometryErrorCodes(), binshift, o2::emcal::reconstructionerrors::getNumberOfGeometryErrorCodes() + binshift);
  mErrorTypeGeometry->GetXaxis()->SetTitle("Link");
  mErrorTypeGeometry->GetYaxis()->SetTitle("Geometry Error Type");
  for (int ierror = 0; ierror < o2::emcal::reconstructionerrors::getNumberOfGeometryErrorCodes(); ierror++) {
    mErrorTypeGeometry->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::reconstructionerrors::getGeometryErrorTitle(ierror));
  }
  mErrorTypeGeometry->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeGeometry);

  mErrorTypeGain = new TH2F("GainTypeError", "Gain type error", 40, 0, 40, o2::emcal::reconstructionerrors::getNumberOfGainErrorCodes(), binshift, o2::emcal::reconstructionerrors::getNumberOfGainErrorCodes() + binshift);
  mErrorTypeGain->GetXaxis()->SetTitle("Link");
  mErrorTypeGain->GetYaxis()->SetTitle("Gain Error Type");
  for (int ierror = 0; ierror < o2::emcal::reconstructionerrors::getNumberOfGainErrorCodes(); ierror++) {
    mErrorTypeGain->GetYaxis()->SetBinLabel(ierror + 1, o2::emcal::reconstructionerrors::getGainErrorTitle(ierror));
  }
  mErrorTypeGain->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeGain);

  mErrorGainLow = new TH2F("NoHGPerDDL", "High Gain bunch missing", 40, 0, 40, 40, 0, 40);
  mErrorGainLow->GetYaxis()->SetTitle("fecID");
  mErrorGainLow->GetXaxis()->SetTitle("Link");
  mErrorGainLow->SetStats(0);
  getObjectsManager()->startPublishing(mErrorGainLow);

  mErrorGainHigh = new TH2F("NoLGPerDDL", "Low Gain bunch missing for saturated High Gain", 40, 0, 40, 40, 0, 40);
  mErrorGainHigh->GetYaxis()->SetTitle("fecID");
  mErrorGainHigh->GetXaxis()->SetTitle("Link");
  mErrorGainHigh->SetStats(0);
  getObjectsManager()->startPublishing(mErrorGainHigh);

  mFecIdMinorAltroError = new TH2F("FecIDMinorAltroError", "FecID Minor Altro Error", 40, 0, 40, 40, 0, 40);
  mFecIdMinorAltroError->GetYaxis()->SetTitle("fecID");
  mFecIdMinorAltroError->GetXaxis()->SetTitle("Link");
  mFecIdMinorAltroError->SetStats(0);
  getObjectsManager()->startPublishing(mFecIdMinorAltroError);

  mChannelGainLow = new TH2F("ChannelLGnoHG", "Channel with HG bunch missing", 96, -0.5, 95.5, 208, -0.5, 207.5);
  mChannelGainLow->GetXaxis()->SetTitle("Column");
  mChannelGainLow->GetYaxis()->SetTitle("Row");
  mChannelGainLow->SetStats(0);
  getObjectsManager()->startPublishing(mChannelGainLow);

  mChannelGainHigh = new TH2F("ChannelHGnoLG", "Channel with LG bunch missing", 96, -0.5, 95.5, 208, -0.5, 207.5);
  mChannelGainHigh->GetXaxis()->SetTitle("Column");
  mChannelGainHigh->GetYaxis()->SetTitle("Row");
  mChannelGainHigh->SetStats(0);
  getObjectsManager()->startPublishing(mChannelGainHigh);

  mErrorTypeUnknown = new TH1F("UnknownErrorType", "Unknown error types", 40, 0., 40);
  mErrorTypeUnknown->GetXaxis()->SetTitle("Link");
  mErrorTypeUnknown->GetYaxis()->SetTitle("Number of errors");
  getObjectsManager()->startPublishing(mErrorTypeUnknown);

  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
  mMapper = std::make_unique<o2::emcal::MappingHandler>();
}

void RawErrorTask::startOfActivity(const Activity& activity)
{
  ILOG(Debug, Devel) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void RawErrorTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
}

void RawErrorTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  constexpr auto originEMC = o2::header::gDataOriginEMC;

  std::vector<framework::InputSpec> filter{ { "filter", framework::ConcreteDataTypeMatcher(originEMC, "DECODERERR") } };
  int firstEntry = 0;
  for (const auto& rawErrorData : framework::InputRecordWalker(ctx.inputs(), filter)) {
    auto errorcont = o2::framework::DataRefUtils::as<o2::emcal::ErrorTypeFEE>(rawErrorData); // read error message
    LOG(debug) << "Received " << errorcont.size() << " errors";
    for (auto& error : errorcont) {
      auto feeid = error.getFEEID();
      if (error.getErrorType() != o2::emcal::ErrorTypeFEE::ErrorSource_t::GAIN_ERROR || !mExcludeGainErrorsFromOverview)
        mErrorTypeAll->Fill(feeid, error.getErrorType());
      auto errorCode = error.getErrorCode();
      TH2* errorhist = nullptr;
      switch (error.getErrorType()) {
        case o2::emcal::ErrorTypeFEE::ErrorSource_t::PAGE_ERROR:
          errorhist = mErrorTypePage;
          break;
        case o2::emcal::ErrorTypeFEE::ErrorSource_t::ALTRO_ERROR:
          errorhist = mErrorTypeAltro;
          break;
        case o2::emcal::ErrorTypeFEE::ErrorSource_t::MINOR_ALTRO_ERROR:
          errorhist = mErrorTypeMinAltro;
          break;
        case o2::emcal::ErrorTypeFEE::ErrorSource_t::FIT_ERROR:
          errorhist = mErrorTypeFit;
          break;
        case o2::emcal::ErrorTypeFEE::ErrorSource_t::GEOMETRY_ERROR:
          errorhist = mErrorTypeGeometry;
          break;
        case o2::emcal::ErrorTypeFEE::ErrorSource_t::GAIN_ERROR:
          errorhist = mErrorTypeGain;
          break;
        default:
          // Error type is unknown - this should never happen
          // In order to monitor such messages fill a dedicated
          // counter histogram
          mErrorTypeUnknown->Fill(feeid);
          break;
      }; // switch errorCode
      if (errorhist) {
        errorhist->Fill(feeid, errorCode);
      }

      if (error.getErrorType() == o2::emcal::ErrorTypeFEE::ErrorSource_t::GAIN_ERROR) {
        // Fill Histogram with FEC ID
        auto fecID = error.getSubspecification();
        auto gainerrortype = o2::emcal::reconstructionerrors::getGainErrorFromErrorCode(errorCode);
        switch (gainerrortype) {
          case o2::emcal::reconstructionerrors::GainError_t::LGNOHG:
            mErrorGainLow->Fill(feeid, fecID); // LGnoHG
            break;
          case o2::emcal::reconstructionerrors::GainError_t::HGNOLG:
            mErrorGainHigh->Fill(feeid, fecID); // HGnoLG
            break;
          default:
            break; // Unknown gain error - not handel
        }
        // Fill histogram with tower position
        if (error.getHarwareAddress() >= 0) {
          auto supermoduleID = feeid / 2;
          try {
            auto& mapping = mMapper->getMappingForDDL(feeid);
            auto colOnline = mapping.getColumn(error.getHarwareAddress());
            auto rowOnline = mapping.getRow(error.getHarwareAddress());
            auto [rowCorrected, colCorrected] = mGeometry->ShiftOnlineToOfflineCellIndexes(supermoduleID, rowOnline, colOnline);
            auto cellID = mGeometry->GetAbsCellIdFromCellIndexes(supermoduleID, rowCorrected, colCorrected);
            auto [globalRow, globalColumn] = mGeometry->GlobalRowColFromIndex(cellID);
            if (errorCode == 0) {
              mChannelGainLow->Fill(globalColumn, globalRow); // LGnoHG
            } else {
              mChannelGainHigh->Fill(globalColumn, globalRow); // HGnoLG
            }
          } catch (o2::emcal::MappingHandler::DDLInvalid& e) {
            ILOG(Warning, Support) << e.what() << ENDM;
          } catch (o2::emcal::Mapper::AddressNotFoundException& e) {
            ILOG(Warning, Support) << e.what() << ENDM;
          } catch (o2::emcal::InvalidCellIDException& e) {
            ILOG(Warning, Support) << e.what() << ENDM;
          }
        }
      }
      if (error.getErrorType() == o2::emcal::ErrorTypeFEE::ErrorSource_t::MINOR_ALTRO_ERROR) {
        auto fecID = error.getSubspecification(); // check: hardware address, or tower id (after markus implementation)
        mFecIdMinorAltroError->Fill(feeid, fecID);
      }
    } // end for error in errorcont
  }   // end of loop on raw error data
} // end of monitorData

void RawErrorTask::endOfCycle()
{
  ILOG(Debug, Devel) << "endOfCycle" << ENDM;
}

void RawErrorTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "endOfActivity" << ENDM;
}

void RawErrorTask::reset()
{
  // clean all the monitor objects here

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  mErrorTypeAll->Reset();
  mErrorTypeAltro->Reset();
  mErrorTypePage->Reset();
  mErrorTypeMinAltro->Reset();
  mErrorTypeFit->Reset();
  mErrorTypeGeometry->Reset();
  mErrorTypeGain->Reset();
  mErrorTypeUnknown->Reset();
  mErrorGainLow->Reset();
  mErrorGainHigh->Reset();
  mFecIdMinorAltroError->Reset();
}
std::string RawErrorTask::getConfigValue(const std::string_view key)
{
  std::string result;
  if (auto param = mCustomParameters.find(key.data()); param != mCustomParameters.end()) {
    result = param->second;
  }
  return result;
}

std::string RawErrorTask::getConfigValueLower(const std::string_view key)
{
  auto input = getConfigValue(key);
  std::string result;
  if (input.length()) {
    result = boost::algorithm::to_lower_copy(input);
  }
  return result;
}
} // namespace o2::quality_control_modules::emcal
