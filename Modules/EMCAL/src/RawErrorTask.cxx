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

#include "QualityControl/QcInfoLogger.h"
#include "EMCAL/RawErrorTask.h"
#include "EMCALBase/Geometry.h"
#include "EMCALBase/Mapper.h"
#include "EMCALReconstruction/AltroDecoder.h"
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

  if (mErrorGainLow)
    delete mErrorGainLow;

  if (mErrorGainHigh)
    delete mErrorGainHigh;

  // histo per categoty with details
  // histo summary with error per category
}

void RawErrorTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize RawErrorTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  mErrorTypeAll = new TH2F("RawDataErrors", "Raw data errors", 40, 0, 40, 6, -0.5, 5.5);
  mErrorTypeAll->GetXaxis()->SetTitle("Link");
  mErrorTypeAll->GetYaxis()->SetTitle("Error type");
  mErrorTypeAll->GetYaxis()->SetBinLabel(1, "Page");
  mErrorTypeAll->GetYaxis()->SetBinLabel(2, "Major ALTRO");
  mErrorTypeAll->GetYaxis()->SetBinLabel(3, "Minor ALTRO");
  mErrorTypeAll->GetYaxis()->SetBinLabel(4, "Fit");
  mErrorTypeAll->GetYaxis()->SetBinLabel(5, "Geometry");
  mErrorTypeAll->GetYaxis()->SetBinLabel(6, "Gain");
  mErrorTypeAll->SetStats(0);
  getObjectsManager()->startPublishing(mErrorTypeAll);

  mErrorTypeAltro = new TH2F("MajorAltroErrors", "Major ALTRO decoding errors", 40, 0, 40, 10, 0, 10);
  mErrorTypeAltro->GetXaxis()->SetTitle("Link");
  mErrorTypeAltro->GetYaxis()->SetTitle("Error Type");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(1, "RCU Trailer");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(2, "RCU Version");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(3, "RCU Trailer Size");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(4, "ALTRO Bunch Header");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(5, "ALTRO Bunch Length");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(6, "ALTRO Payload");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(7, "ALTRO Mapping");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(8, "Channel");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(9, "Mapper HWAddress");
  mErrorTypeAltro->GetYaxis()->SetBinLabel(10, "Geometry InvalidCell");
  mErrorTypeAltro->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypeAltro);

  mErrorTypePage = new TH2F("PageErrors", "DMA page decoding errors", 40, 0, 40, 7, 0, 7);
  mErrorTypePage->GetXaxis()->SetTitle("Link");
  mErrorTypePage->GetYaxis()->SetTitle("Page Error Type");
  mErrorTypePage->GetYaxis()->SetBinLabel(1, "Page not found");
  mErrorTypePage->GetYaxis()->SetBinLabel(2, "Header decoding");
  mErrorTypePage->GetYaxis()->SetBinLabel(3, "Payload decoding");
  mErrorTypePage->GetYaxis()->SetBinLabel(4, "Header corruption");
  mErrorTypePage->GetYaxis()->SetBinLabel(5, "Page start invalid");
  mErrorTypePage->GetYaxis()->SetBinLabel(6, "Payload corruption");
  mErrorTypePage->GetYaxis()->SetBinLabel(7, "Trailer decoding");
  mErrorTypePage->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypePage);

  mErrorTypeMinAltro = new TH2F("MinorAltroError", "Minor ALTRO decoding error", 40, 0, 40, 4, 0, 4);
  mErrorTypeMinAltro->GetXaxis()->SetTitle("Link");
  mErrorTypeMinAltro->GetYaxis()->SetTitle("MinorAltro Error Type");
  mErrorTypeMinAltro->GetYaxis()->SetBinLabel(1, "Channel end unexpected");
  mErrorTypeMinAltro->GetYaxis()->SetBinLabel(2, "Channel exceed");
  mErrorTypeMinAltro->GetYaxis()->SetBinLabel(3, "Bunch header null");
  mErrorTypeMinAltro->GetYaxis()->SetBinLabel(4, "Bunch length exceed");
  mErrorTypeMinAltro->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypeMinAltro);

  mErrorTypeFit = new TH2F("RawFitError", "Error in raw fitting ", 40, 0, 40, 4, 0, 4);
  mErrorTypeFit->GetXaxis()->SetTitle("Link");
  mErrorTypeFit->GetYaxis()->SetTitle("Fit Error Type");
  mErrorTypeFit->GetYaxis()->SetBinLabel(1, "sample uninitalized");
  mErrorTypeFit->GetYaxis()->SetBinLabel(2, "No convergence");
  mErrorTypeFit->GetYaxis()->SetBinLabel(3, "Chi2 error");
  // mErrorTypeFit->GetYaxis()->SetBinLabel(4, "bunch_not_ok");
  mErrorTypeFit->GetYaxis()->SetBinLabel(4, "Low signal");
  mErrorTypeFit->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypeFit);

  mErrorTypeGeometry = new TH2F("GeometryError", "Geometry error", 40, 0, 40, 2, 0, 2);
  mErrorTypeGeometry->GetXaxis()->SetTitle("Link");
  mErrorTypeGeometry->GetYaxis()->SetTitle("Geometry Error Type");
  mErrorTypeGeometry->GetYaxis()->SetBinLabel(1, "Cell ID outside range");
  mErrorTypeGeometry->GetYaxis()->SetBinLabel(2, "Cell ID corrupted");
  mErrorTypeGeometry->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypeGeometry);

  mErrorTypeGain = new TH2F("GainTypeError", "Gain type error", 40, 0, 40, 2, 0, 2);
  mErrorTypeGain->GetXaxis()->SetTitle("Link");
  mErrorTypeGain->GetYaxis()->SetTitle("Gain Error Type");
  mErrorTypeGain->GetYaxis()->SetBinLabel(1, "High Gain missing");
  mErrorTypeGain->GetYaxis()->SetBinLabel(2, "Low Gain missing");
  mErrorTypeGain->SetStats(0);
  getObjectsManager()
    ->startPublishing(mErrorTypeGain);

  mErrorGainLow = new TH2F("NoHGPerDDL", "High Gain bunch missing", 40, 0, 40, 40, 0, 40);
  mErrorGainLow->GetYaxis()->SetTitle("FECid");
  mErrorGainLow->GetXaxis()->SetTitle("Link");
  mErrorGainLow->SetStats(0);
  getObjectsManager()->startPublishing(mErrorGainLow);

  mErrorGainHigh = new TH2F("NoLGPerDDL", "Low Gain bunch missing for saturated High Gain", 40, 0, 40, 40, 0, 40);
  mErrorGainHigh->GetYaxis()->SetTitle("FECid");
  mErrorGainHigh->GetXaxis()->SetTitle("Link");
  mErrorGainHigh->SetStats(0);
  getObjectsManager()->startPublishing(mErrorGainHigh);

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

  mGeometry = o2::emcal::Geometry::GetInstanceFromRunNumber(300000);
  mMapper = std::make_unique<o2::emcal::MappingHandler>();
}

void RawErrorTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  reset();
}

void RawErrorTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
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
        // case UNDEFINED:
        //   errorhist = mErrorTypeUndefined;
        //     break;
        default:
          break;
      }; // switch errorCode
      errorhist->Fill(feeid, errorCode);

      if (o2::emcal::ErrorTypeFEE::ErrorSource_t::GAIN_ERROR) {
        // Fill Histogram with FEC ID
        auto FECid = error.getSubspecification();
        if (errorCode == 0) {
          mErrorGainLow->Fill(feeid, FECid); // LGnoHG
        } else {
          mErrorGainHigh->Fill(feeid, FECid); // HGnoLG
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
    } // end for error in errorcont
  }   // end of loop on raw error data
} // end of monitorData

void RawErrorTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void RawErrorTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void RawErrorTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mErrorTypeAltro->Reset();
  mErrorTypePage->Reset();
  mErrorTypeMinAltro->Reset();
  mErrorTypeFit->Reset();
  mErrorTypeGeometry->Reset();
  mErrorTypeGain->Reset();
  mErrorGainLow->Reset();
  mErrorGainHigh->Reset();
}

} // namespace o2::quality_control_modules::emcal
