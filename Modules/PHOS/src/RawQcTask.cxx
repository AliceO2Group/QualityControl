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
/// \file   RawQcTask.cxx
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#include <TCanvas.h>
#include <TH2.h>
#include <TMath.h>
#include <TSpectrum.h>
#include <cfloat>

#include "QualityControl/QcInfoLogger.h"
#include "PHOS/RawQcTask.h"
#include "Headers/RAWDataHeader.h"
#include "PHOSReconstruction/RawReaderError.h"
#include "PHOSBase/Geometry.h"
#include "PHOSBase/Mapping.h"
#include "Framework/InputRecord.h"
#include "CommonUtils/NameConf.h"
#include "CCDB/BasicCCDBManager.h"

namespace o2::quality_control_modules::phos
{

RawQcTask::~RawQcTask()
{
  for (int i = kNhist1D; i--;) {
    if (mHist1D[i]) {
      mHist1D[i]->Delete();
      mHist1D[i] = nullptr;
    }
  }
  for (int i = kNhist2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Delete();
      mHist2D[i] = nullptr;
    }
  }
  for (int i = kNhist2DMean; i--;) {
    if (mHist2DMean[i]) {
      mHist2DMean[i]->Delete();
      mHist2DMean[i] = nullptr;
    }
  }
  for (int i = kNhist2DBitmask; i--;) {
    if (mHist2DBitmask[i]) {
      mHist2DBitmask[i]->Delete();
      mHist2DBitmask[i] = nullptr;
    }
  }
  for (int i = kNratio1D; i--;) {
    if (mFractions1D[i]) {
      delete mFractions1D[i];
      mFractions1D[i] = nullptr;
    }
  }
}
void RawQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  using infoCONTEXT = AliceO2::InfoLogger::InfoLoggerContext;
  infoCONTEXT context;
  context.setField(infoCONTEXT::FieldName::Facility, "QC");
  context.setField(infoCONTEXT::FieldName::System, "QC");
  context.setField(infoCONTEXT::FieldName::Detector, "PHS");
  QcInfoLogger::GetInfoLogger().setContext(context);
  ILOG(Debug, Devel) << "initialize RawQcTask" << ENDM;

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("pedestal"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in pedestal mode " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 1;
    }
  }
  if (auto param = mCustomParameters.find("LED"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in LED mode " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 2;
    }
  }
  if (auto param = mCustomParameters.find("physics"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Working in physics mode " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mMode = 0;
    }
  }
  if (auto param = mCustomParameters.find("chi2"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Scan chi2 distributions " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mCheckChi2 = true;
    }
  }
  if (auto param = mCustomParameters.find("trignoise"); param != mCustomParameters.end()) {
    ILOG(Info, Support) << "Scan trigger ST and Dig matching " << ENDM;
    if (param->second.find("on") != std::string::npos) {
      mTrNoise = true;
    }
  }

  InitHistograms();
}

void RawQcTask::InitHistograms()
{

  // First init general histograms for any mode

  // Statistics histograms
  mHist2D[kErrorNumber] = new TH2F("NumberOfErrors", "Number of hardware errors", 32, 0, 32, 15, 0, 15.); // xaxis: FEE card number + 2 for TRU and global errors
  mHist2D[kErrorNumber]->GetXaxis()->SetTitle("FEE card");
  mHist2D[kErrorNumber]->GetYaxis()->SetTitle("DDL");
  mHist2D[kErrorNumber]->SetDrawOption("colz");
  mHist2D[kErrorNumber]->SetStats(0);
  getObjectsManager()->startPublishing(mHist2D[kErrorNumber]);

  mHist2DBitmask[kErrorType] = new TH2SBitmask("ErrorTypePerDDL", "ErrorTypePerDDL", 32, 0, 32, 15, 0, 15.);
  mHist2DBitmask[kErrorType]->GetXaxis()->SetTitle("FEE card");
  mHist2DBitmask[kErrorType]->GetYaxis()->SetTitle("DDL");
  mHist2DBitmask[kErrorType]->SetDrawOption("colz");
  mHist2DBitmask[kErrorType]->SetStats(0);
  getObjectsManager()->startPublishing(mHist2DBitmask[kErrorType]);

  mFractions1D[kErrorTypeOccurance] = new TH1Fraction("ErrorTypeOccurance", "Errors of differen types per event", 5, 0, 5);
  mFractions1D[kErrorTypeOccurance]->GetXaxis()->SetTitle("Error Type");
  mFractions1D[kErrorTypeOccurance]->SetStats(0);
  mFractions1D[kErrorTypeOccurance]->GetYaxis()->SetTitle("Occurance (event^{-1})");
  const char* errorLabel[] = { "wrong ALTRO", "mapping", "ch. header",
                               "ch. payload", "wrond hw addr" };
  for (int i = 1; i <= 5; i++) {
    mFractions1D[kErrorTypeOccurance]->GetXaxis()->SetBinLabel(i, errorLabel[i - 1]);
  }
  getObjectsManager()->startPublishing(mFractions1D[kErrorTypeOccurance]);

  mHist1D[kBadMapSummary] = new TH1F("BadMapSummary", "Number of bad channels", 4, 1., 5.); // xaxis: FEE card number + 2 for TRU and global errors
  mHist1D[kBadMapSummary]->GetXaxis()->SetTitle("module");
  mHist1D[kBadMapSummary]->GetYaxis()->SetTitle("N bad channels");
  mHist1D[kBadMapSummary]->SetDrawOption("h");
  mHist1D[kBadMapSummary]->SetStats(0);
  getObjectsManager()->startPublishing(mHist1D[kBadMapSummary]);

  if (mCheckChi2) {
    for (Int_t mod = 0; mod < 4; mod++) {
      if (!mHist2D[kChi2M1 + mod]) {
        mHist2D[kChi2M1 + mod] = new TH2F(Form("Chi2M%d", mod + 1), Form("sample fit #chi2/NDF, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChi2M1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChi2M1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChi2M1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChi2M1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChi2M1 + mod]->SetStats(0);
        mHist2D[kChi2M1 + mod]->SetMinimum(0);
        getObjectsManager()->startPublishing(mHist2D[kChi2M1 + mod]);

        mHist2D[kChi2NormM1 + mod] = new TH2F(Form("Chi2NormM%d", mod + 1), Form("sample fit #chi2/NDF normalization, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
        mHist2D[kChi2NormM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
        mHist2D[kChi2NormM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
        mHist2D[kChi2NormM1 + mod]->GetXaxis()->SetTitle("x, cells");
        mHist2D[kChi2NormM1 + mod]->GetYaxis()->SetTitle("z, cells");
        mHist2D[kChi2NormM1 + mod]->SetStats(0);
        mHist2D[kChi2NormM1 + mod]->SetMinimum(0);
        // getObjectsManager()->startPublishing(mHist2D[kChi2NormM1 + mod]);
      } else {
        mHist2D[kChi2M1 + mod]->Reset();
        mHist2D[kChi2NormM1 + mod]->Reset();
      }
    }
  }

  if (mMode == 0) { // Physics
    CreatePhysicsHistograms();
    //     CreateTRUHistograms();
  }
  if (mMode == 1) { // Pedestals
    CreatePedestalHistograms();
  }
  if (mMode == 2) { // LED
    CreatePhysicsHistograms();
    CreateLEDHistograms();
    //     CreateTRUHistograms();
  }
}

void RawQcTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Debug, Devel) << "startOfActivity" << ENDM;
  reset();
}

void RawQcTask::startOfCycle()
{
  ILOG(Debug, Devel) << "startOfCycle" << ENDM;
  if (mCheckChi2) {
    if (mFinalized) { // mean already calculated
      for (Int_t mod = 0; mod < 4; mod++) {
        if (mHist2D[kChi2M1 + mod]) {
          mHist2D[kChi2M1 + mod]->Multiply(mHist2D[kChi2NormM1 + mod]);
        }
      }
      if (mMode != 1) { // for mode 1 another check below
        mFinalized = false;
      }
    }
  }

  if (mMode == 1) {   // Pedestals
    if (mFinalized) { // means were already calculated
      for (Int_t mod = 0; mod < 4; mod++) {
        if (mHist2DMean[kHGmeanM1 + mod]) {
          mHist2DMean[kHGmeanM1 + mod]->Multiply(mHist2D[kHGoccupM1 + mod]);
          mHist2DMean[kHGrmsM1 + mod]->Multiply(mHist2D[kHGoccupM1 + mod]);
        }
        if (mHist2DMean[kLGmeanM1 + mod]) {
          mHist2DMean[kLGmeanM1 + mod]->Multiply(mHist2D[kLGoccupM1 + mod]);
          mHist2DMean[kLGrmsM1 + mod]->Multiply(mHist2D[kLGoccupM1 + mod]);
        }
      }
      mFinalized = false;
    }
  }
}

void RawQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // In this function you can access data inputs specified in the JSON config file, for example:
  //   "query": "random:ITS/RAWDATA/0"
  // which is correspondingly <binding>:<dataOrigin>/<dataDescription>/<subSpecification
  // One can also access conditions from CCDB, via separate API (see point 3)

  // Use Framework/DataRefUtils.h or Framework/InputRecord.h to access and unpack inputs (both are documented)
  // One can find additional examples at:
  // https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#using-inputs---the-inputrecord-api

  if (ctx.inputs().getPos("rawerr") >= 0) { // to be able to scan e.g. CTF data
    auto hwerrors = ctx.inputs().get<std::vector<o2::phos::RawReaderError>>("rawerr");
    for (auto e : hwerrors) {
      int ibin = mHist2D[kErrorNumber]->Fill(float(e.getFEC()), float(e.getDDL()));
      int cont = mHist2DBitmask[kErrorType]->GetBinContent(ibin);
      cont |= (1 << e.getError());
      mHist2DBitmask[kErrorType]->SetBinContent(ibin, cont);
      mFractions1D[kErrorTypeOccurance]->fillUnderlying(e.getError() - 3);
      // LOG(info) << "Error of type " << (int)e.getError();
    }
  }
  // Bad Map
  //  Read current bad map if not read yet
  if (mInitBadMap) {
    mInitBadMap = false;
    ILOG(Info, Support) << "Getting bad map" << ENDM;
    mBadMap = retrieveConditionAny<o2::phos::BadChannelsMap>("PHS/Calib/BadMap");
    if (!mBadMap) {
      ILOG(Error, Support) << "Can not get bad map" << ENDM;
      mHist1D[kBadMapSummary]->Reset();
    } else {
      unsigned short nbm[4] = { 0 };
      for (short absId = 1973; absId <= o2::phos::Mapping::NCHANNELS; absId++) {
        if (!mBadMap->isChannelGood(absId)) {
          nbm[(absId - 1) / 3584]++;
        }
      }
      for (int mod = 0; mod < 4; mod++) {
        mHist1D[kBadMapSummary]->SetBinContent(mod + 1, nbm[mod]);
      }
      ILOG(Info, Support) << "Bad channels:[" << nbm[0] << "," << nbm[1] << "," << nbm[2] << "," << nbm[3] << "]" << ENDM;
    }
  }

  // Chi2: not hardware errors but unusual/correpted sample
  if (mCheckChi2) {
    // vector contains subsequent pairs (address,chi2)
    auto chi2list = ctx.inputs().get<std::vector<short>>("fitquality");
    auto it = chi2list.begin();
    while (it != chi2list.end()) {
      short address = *it;
      it++;
      bool caloFlag = address & 1 << 14;
      address &= ~(1 << 14); // remove HG/LG bit 14
      float chi = 0.2 * (*it);
      it++;
      char relid[3];
      o2::phos::Geometry::absToRelNumbering(address, relid);
      mHist2D[kChi2M1 + relid[0] - 1]->Fill(relid[1] - 0.5, relid[2] - 0.5, chi);
      mHist2D[kChi2NormM1 + relid[0] - 1]->Fill(relid[1] - 0.5, relid[2] - 0.5);
    }
  }

  // Cells
  auto cells = ctx.inputs().get<gsl::span<o2::phos::Cell>>("cells");
  auto cellsTR = ctx.inputs().get<gsl::span<o2::phos::TriggerRecord>>("cellstr");
  mFractions1D[kErrorTypeOccurance]->increaseEventCounter(cellsTR.size());

  if (mMode == 0) { // Physics
    FillPhysicsHistograms(cells, cellsTR);
    //     FillTRUHistograms(cells, cellsTR);
  }
  if (mMode == 1) { // Pedestals
    FillPedestalHistograms(cells, cellsTR);
  }
  if (mMode == 2) { // LED
    FillPhysicsHistograms(cells, cellsTR);
    FillLEDHistograms(cells, cellsTR);
    //     FillTRUHistograms(cells, cellsTR);
  }
} // function monitor data

void RawQcTask::endOfCycle()
{
  mFractions1D[kErrorTypeOccurance]->update();
  if (mCheckChi2) {
    if (!mFinalized) { // not already calculated
      for (Int_t mod = 0; mod < 4; mod++) {
        if (mHist2D[kChi2M1 + mod]) {
          mHist2D[kChi2M1 + mod]->Divide(mHist2D[kChi2NormM1 + mod]);
        }
      }
    }
  }

  if (mMode == 1) {   // Pedestals
    if (mFinalized) { // means were already calculated
      return;
    }
    for (Int_t mod = 0; mod < 4; mod++) {
      if (mHist2DMean[kHGmeanM1 + mod]) {
        mHist2DMean[kHGmeanM1 + mod]->Divide(mHist2D[kHGoccupM1 + mod]);
        mHist2DMean[kHGrmsM1 + mod]->Divide(mHist2D[kHGoccupM1 + mod]);
        mHist1D[kHGmeanSummaryM1 + mod]->Reset();
        mHist1D[kHGrmsSummaryM1 + mod]->Reset();
        double occMin = 1.e+9;
        double occMax = 0.;
        for (int iz = 1; iz <= 64; iz++) {
          for (int ix = 1; ix <= 56; ix++) {
            float a = mHist2DMean[kHGmeanM1 + mod]->GetBinContent(iz, ix);
            if (a > 0) {
              mHist1D[kHGmeanSummaryM1 + mod]->Fill(a);
            }
            a = mHist2DMean[kHGrmsM1 + mod]->GetBinContent(iz, ix);
            if (a > 0) {
              mHist1D[kHGrmsSummaryM1 + mod]->Fill(a);
            }
            a = mHist2D[kHGoccupM1 + mod]->GetBinContent(iz, ix);
            if (a > 0) {
              if (a < occMin)
                occMin = a;
              if (a > occMax)
                occMax = a;
            }
          }
        }
        mHist2D[kHGoccupM1 + mod]->SetMinimum(occMin);
        mHist2D[kHGoccupM1 + mod]->SetMaximum(occMax);
      }
      if (mHist2DMean[kLGmeanM1 + mod]) {
        mHist2DMean[kLGmeanM1 + mod]->Divide(mHist2D[kLGoccupM1 + mod]);
        mHist2DMean[kLGrmsM1 + mod]->Divide(mHist2D[kLGoccupM1 + mod]);
        mHist1D[kLGmeanSummaryM1 + mod]->Reset();
        mHist1D[kLGrmsSummaryM1 + mod]->Reset();
        double occMin = 1.e+9;
        double occMax = 0.;
        for (int iz = 1; iz <= 64; iz++) {
          for (int ix = 1; ix <= 56; ix++) {
            float a = mHist2DMean[kLGmeanM1 + mod]->GetBinContent(iz, ix);
            if (a > 0) {
              mHist1D[kLGmeanSummaryM1 + mod]->Fill(a);
            }
            a = mHist2DMean[kLGrmsM1 + mod]->GetBinContent(iz, ix);
            if (a > 0) {
              mHist1D[kLGrmsSummaryM1 + mod]->Fill(a);
            }
            a = mHist2D[kLGoccupM1 + mod]->GetBinContent(iz, ix);
            if (a > 0) {
              if (a < occMin)
                occMin = a;
              if (a > occMax)
                occMax = a;
            }
          }
        }
        mHist2D[kLGoccupM1 + mod]->SetMinimum(occMin);
        mHist2D[kLGoccupM1 + mod]->SetMaximum(occMax);
      }
    }
    mFinalized = true;
  }
  //==========LED===========
  if (mMode == 2) { // LED
    ILOG(Info, Support) << " Caclulating number of peaks" << ENDM;
    for (unsigned int absId = 1793; absId <= o2::phos::Mapping::NCHANNELS; absId++) {
      int npeaks = mSpSearcher->Search(&(mSpectra[absId - 1793]), 2, "goff", 0.1);
      char relid[3];
      o2::phos::Geometry::absToRelNumbering(absId, relid);
      short mod = relid[0] - 1;
      int ibin = mHist2DMean[kLEDNpeaksM1 + mod]->FindBin(relid[1] - 0.5, relid[2] - 0.5);
      mHist2DMean[kLEDNpeaksM1 + mod]->SetBinContent(ibin, npeaks);
    }
    ILOG(Info, Support) << " Caclulating number of peaks done" << ENDM;
  }
}

void RawQcTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
  endOfCycle();
}

void RawQcTask::reset()
{
  eventCounter = 0;
  // clean all the monitor objects here
  mFinalized = false;

  ILOG(Debug, Devel) << "Resetting the histograms" << ENDM;
  for (int i = kNhist1D; i--;) {
    if (mHist1D[i]) {
      mHist1D[i]->Reset();
    }
  }
  for (int i = kNhist2D; i--;) {
    if (mHist2D[i]) {
      mHist2D[i]->Reset();
    }
  }
}
void RawQcTask::FillLEDHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& cellsTR)
{

  // Fill intermediate histograms
  for (const auto& tr : cellsTR) {
    int firstCellInEvent = tr.getFirstEntry();
    int lastCellInEvent = firstCellInEvent + tr.getNumberOfObjects();
    for (int i = firstCellInEvent; i < lastCellInEvent; i++) {
      const o2::phos::Cell c = cells[i];
      if (!c.getTRU() && c.getHighGain()) {
        mSpectra[c.getAbsId() - 1793].Fill(c.getEnergy());
      }
    }
  }
}
void RawQcTask::FillTRUHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& cellsTR)
{
  std::vector<int> triggerSTTiles;
  std::vector<int> triggerDGTiles;
  char relId[3] = { 0 };
  for (const auto tr : cellsTR) {
    triggerSTTiles.clear();
    triggerDGTiles.clear();
    int firstCellInEvent = tr.getFirstEntry();
    int lastCellInEvent = firstCellInEvent + tr.getNumberOfObjects();
    for (int i = firstCellInEvent; i < lastCellInEvent; i++) {
      const o2::phos::Cell& c = cells[i];
      if (c.getTRU()) {
        if (c.getType() == o2::phos::TRU4x4) {
          o2::phos::Geometry::truAbsToRelNumbering(c.getTRUId(), 1, relId);
          //   ILOG(Info, Support) << "TRU4x4 [" <<(int)relId[0]<< ","<<(int)relId[1]<< ","<< (int)relId[2] <<"]" << ENDM;
          mHist2D[kTRUSTOccupM1 + relId[0] - 1]->Fill(relId[1] - 0.5, relId[2] - 0.5);
          triggerSTTiles.push_back(relId[0] + (relId[1] << 3) + (relId[2] << 10));
        } else { // 2x2
          o2::phos::Geometry::truAbsToRelNumbering(c.getTRUId(), 0, relId);
          //   ILOG(Info, Support) << "TRU2x2 [" <<(int)relId[0]<< ","<<(int)relId[1]<< ","<< (int)relId[2] <<"]" << ENDM;
          mHist2D[kTRUDGOccupM1 + relId[0] - 1]->Fill(relId[1] - 0.5, relId[2] - 0.5);
          triggerDGTiles.push_back(relId[0] + (relId[1] << 3) + (relId[2] << 10));
        }
      }
    }

    //   ILOG(Info, Support) << " triggerSTTiles=" <<triggerSTTiles.size()<< ", triggerDGTiles="<<triggerDGTiles.size()<< ENDM;
    //
    for (int aST : triggerSTTiles) {
      bool matched = false;
      for (int bDG : triggerDGTiles) {
        //   ILOG(Info, Support) << " modules=" <<(aST&0x7) <<" : " << (bDG&0x7) << ENDM;
        if ((aST & 0x7) == (bDG & 0x7)) { // same module
          int dx = ((bDG >> 3) & 0x7F) - ((aST >> 3) & 0x7F);
          int dz = ((bDG >> 10) & 0x7F) - ((aST >> 10) & 0x7F);
          //   ILOG(Info, Support) << " dx=" <<dx<< ", dz="<<dz<< ENDM;

          if (dx >= 0 && dx <= 2 && dz >= 0 && dz <= 2) {
            //   ILOG(Info, Support) << "matched [" <<(aST&0x7)<< ","<<((aST>>3)&0x7F)<< ","<< ((aST>>10)&0x7F) <<"]" << ENDM;
            mHist2D[kTRUSTMatchM1 + (aST & 0x7) - 1]->Fill(((aST >> 3) & 0x7F) - 0.5, ((aST >> 10) & 0x7F) - 0.5);
            matched = true;
            break;
          }
        }
      }
      if (!matched) {
        //   ILOG(Info, Support) << " Not matched [" <<(aST&0x7) << ","<<((aST>>3)&0x7F)<< ","<< ((aST>>10)&0x7F) <<"]" << ENDM;
        mHist2D[kTRUSTFakeM1 + (aST & 0x7) - 1]->Fill(((aST >> 3) & 0x7F) - 0.5, ((aST >> 10) & 0x7F) - 0.5);
        //   ILOG(Info, Support) << "Filled" << ENDM;
      }
    }
    //   ILOG(Info, Support) << "Filled done" << ENDM;
    // now vise versa
    for (int bDG : triggerDGTiles) {
      bool matched = false;
      for (int aST : triggerSTTiles) {
        if ((aST & 0x7) == (bDG & 0x7)) { // same module
          int dx = ((bDG >> 3) & 0x7F) - ((aST >> 3) & 0x7F);
          int dz = ((bDG >> 10) & 0x7F) - ((aST >> 10) & 0x7F);
          if (dx >= 0 && dx <= 2 && dz >= 0 && dz <= 2) {
            matched = true;
            break;
          }
        }
      }
      if (!matched) {
        //   ILOG(Info, Support) << "Dig Not matched [" <<(bDG&0x7)<< ","<<((bDG>>3)&0x7F)<< ","<< ((bDG>>10)&0x7F) <<"]" << ENDM;
        mHist2D[kTRUDGFakeM1 + (bDG & 0x7) - 1]->Fill(((bDG >> 3) & 0x7F) - 0.5, ((bDG >> 10) & 0x7F) - 0.5);
      }
    }
    //   ILOG(Info, Support) << "Filled2 done" << ENDM;
  }
}
void RawQcTask::FillPhysicsHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& cellsTR)
{
  for (const auto& tr : cellsTR) {
    int firstCellInEvent = tr.getFirstEntry();
    int lastCellInEvent = firstCellInEvent + tr.getNumberOfObjects();
    for (int i = firstCellInEvent; i < lastCellInEvent; i++) {
      const o2::phos::Cell c = cells[i];
      if (c.getTRU()) {
        continue;
      }
      // short cell, float amplitude, float time, int label
      short address = c.getAbsId();
      float e = c.getEnergy();
      if (e > kOcccupancyTh) {
        // Converts the absolute numbering into the following array
        //  relid[0] = PHOS Module number 1,...4:module
        //  relid[1] = Row number inside a PHOS module (Phi coordinate)
        //  relid[2] = Column number inside a PHOS module (Z coordinate)
        char relid[3];
        o2::phos::Geometry::absToRelNumbering(address, relid);
        short mod = relid[0] - 1;
        int ibin = 0;
        float emean = e;
        if (c.getHighGain()) {
          ibin = mHist2D[kHGoccupM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5);
          float n = mHist2D[kHGoccupM1 + mod]->GetBinContent(ibin) - 1;
          if (n > 0) {
            emean = (e + mHist2DMean[kCellEM1 + mod]->GetBinContent(ibin) * n) / (n + 1);
          }
          mHist2DMean[kCellEM1 + mod]->SetBinContent(ibin, emean);
          mHist1D[kCellHGSpM1 + mod]->Fill(e);
          mHist2D[kTimeEM1 + mod]->Fill(e, c.getTime());
        } else {
          mHist2D[kLGoccupM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5);
          mHist1D[kCellLGSpM1 + mod]->Fill(e);
        }
      }
    }
  }
}

void RawQcTask::FillPedestalHistograms(const gsl::span<const o2::phos::Cell>& cells, const gsl::span<const o2::phos::TriggerRecord>& cellsTR)
{
  if (mFinalized) {
    for (Int_t mod = 0; mod < 4; mod++) {
      mHist2DMean[kHGmeanM1 + mod]->Multiply(mHist2D[kHGoccupM1 + mod]);
      mHist2DMean[kHGrmsM1 + mod]->Multiply(mHist2D[kHGoccupM1 + mod]);
      mHist2DMean[kLGmeanM1 + mod]->Multiply(mHist2D[kLGoccupM1 + mod]);
      mHist2DMean[kLGrmsM1 + mod]->Multiply(mHist2D[kLGoccupM1 + mod]);
    }
    mFinalized = false;
  }

  for (const auto& tr : cellsTR) {
    int firstCellInEvent = tr.getFirstEntry();
    int lastCellInEvent = firstCellInEvent + tr.getNumberOfObjects();
    for (int i = firstCellInEvent; i < lastCellInEvent; i++) {
      const o2::phos::Cell c = cells[i];
      short address = c.getAbsId();
      char relid[3];
      o2::phos::Geometry::absToRelNumbering(address, relid);
      short mod = relid[0] - 1;
      if (c.getHighGain()) {
        mHist2DMean[kHGmeanM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5, c.getEnergy());
        mHist2DMean[kHGrmsM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5, 1.e+7 * c.getTime()); // to store in Cells format
        mHist2D[kHGoccupM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5);
      } else {
        mHist2DMean[kLGmeanM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5, c.getEnergy());
        mHist2DMean[kLGrmsM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5, 1.e+7 * c.getTime());
        mHist2D[kLGoccupM1 + mod]->Fill(relid[1] - 0.5, relid[2] - 0.5);
      }
    }
  }
}

void RawQcTask::CreatePedestalHistograms()
{
  // Prepare historams for pedestal run QA

  for (Int_t mod = 0; mod < 4; mod++) {
    if (!mHist2DMean[kHGmeanM1 + mod]) {
      mHist2DMean[kHGmeanM1 + mod] = new TH2FMean(Form("PedHGmean%d", mod + 1), Form("Pedestal mean High Gain, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2DMean[kHGmeanM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2DMean[kHGmeanM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2DMean[kHGmeanM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2DMean[kHGmeanM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2DMean[kHGmeanM1 + mod]->SetStats(0);
      mHist2DMean[kHGmeanM1 + mod]->SetMinimum(0);
      mHist2DMean[kHGmeanM1 + mod]->SetMaximum(100);
      getObjectsManager()->startPublishing(mHist2DMean[kHGmeanM1 + mod]);
    } else {
      mHist2DMean[kHGmeanM1 + mod]->Reset();
    }
    if (!mHist2DMean[kHGrmsM1 + mod]) {
      mHist2DMean[kHGrmsM1 + mod] = new TH2FMean(Form("PedHGrms%d", mod + 1), Form("Pedestal RMS High Gain, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2DMean[kHGrmsM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2DMean[kHGrmsM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2DMean[kHGrmsM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2DMean[kHGrmsM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2DMean[kHGrmsM1 + mod]->SetStats(0);
      mHist2DMean[kHGrmsM1 + mod]->SetMinimum(0);
      mHist2DMean[kHGrmsM1 + mod]->SetMaximum(2.);
      getObjectsManager()->startPublishing(mHist2DMean[kHGrmsM1 + mod]);
    } else {
      mHist2DMean[kHGrmsM1 + mod]->Reset();
    }
    if (!mHist2D[kHGoccupM1 + mod]) {
      mHist2D[kHGoccupM1 + mod] = new TH2F(Form("HGOccupancyM%d", mod + 1), Form("High Gain occupancy, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2D[kHGoccupM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kHGoccupM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kHGoccupM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kHGoccupM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kHGoccupM1 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[kHGoccupM1 + mod]);
    } else {
      mHist2D[kHGoccupM1 + mod]->Reset();
    }
    if (!mHist2DMean[kLGmeanM1 + mod]) {
      mHist2DMean[kLGmeanM1 + mod] = new TH2FMean(Form("PedLGmean%d", mod + 1), Form("Pedestal mean Low Gain, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2DMean[kLGmeanM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2DMean[kLGmeanM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2DMean[kLGmeanM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2DMean[kLGmeanM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2DMean[kLGmeanM1 + mod]->SetStats(0);
      mHist2DMean[kLGmeanM1 + mod]->SetMinimum(0);
      mHist2DMean[kLGmeanM1 + mod]->SetMaximum(100);
      getObjectsManager()->startPublishing(mHist2DMean[kLGmeanM1 + mod]);
    } else {
      mHist2DMean[kLGmeanM1 + mod]->Reset();
    }
    if (!mHist2DMean[kLGrmsM1 + mod]) {
      mHist2DMean[kLGrmsM1 + mod] = new TH2FMean(Form("PedLGrms%d", mod + 1), Form("Pedestal RMS Low Gain, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2DMean[kLGrmsM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2DMean[kLGrmsM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2DMean[kLGrmsM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2DMean[kLGrmsM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2DMean[kLGrmsM1 + mod]->SetStats(0);
      mHist2DMean[kLGrmsM1 + mod]->SetMinimum(0);
      mHist2DMean[kLGrmsM1 + mod]->SetMaximum(2.);
      getObjectsManager()->startPublishing(mHist2DMean[kLGrmsM1 + mod]);
    } else {
      mHist2DMean[kLGrmsM1 + mod]->Reset();
    }
    if (!mHist2D[kLGoccupM1 + mod]) {
      mHist2D[kLGoccupM1 + mod] = new TH2F(Form("LGOccupancyM%d", mod + 1), Form("Low Gain occupancy, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2D[kLGoccupM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kLGoccupM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kLGoccupM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kLGoccupM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kLGoccupM1 + mod]->SetStats(0);
      getObjectsManager()->startPublishing(mHist2D[kLGoccupM1 + mod]);
    } else {
      mHist2D[kLGoccupM1 + mod]->Reset();
    }
    if (!mHist1D[kHGmeanSummaryM1 + mod]) {
      mHist1D[kHGmeanSummaryM1 + mod] = new TH1F(Form("PedHGMeanSum%d", mod + 1), Form("Pedestal HG mean summary, mod %d", mod + 1), 100, 0., 100.);
      mHist1D[kHGmeanSummaryM1 + mod]->GetXaxis()->SetTitle("ADC channels");
      // mHist1D[kHGmeanSummaryM1 + mod]->SetStats(0);
      mHist1D[kHGmeanSummaryM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kHGmeanSummaryM1 + mod]);
    } else {
      mHist1D[kHGmeanSummaryM1 + mod]->Reset();
    }
    if (!mHist1D[kHGrmsSummaryM1 + mod]) {
      mHist1D[kHGrmsSummaryM1 + mod] = new TH1F(Form("PedHGRMSSum%d", mod + 1), Form("Pedestal HG RMS summary, mod %d", mod + 1), 100, 0., 10.);
      mHist1D[kHGrmsSummaryM1 + mod]->GetXaxis()->SetTitle("ADC channels");
      // mHist1D[kHGrmsSummaryM1 + mod]->SetStats(0);
      mHist1D[kHGrmsSummaryM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kHGrmsSummaryM1 + mod]);
    } else {
      mHist1D[kHGrmsSummaryM1 + mod]->Reset();
    }
    if (!mHist1D[kLGmeanSummaryM1 + mod]) {
      mHist1D[kLGmeanSummaryM1 + mod] = new TH1F(Form("PedLGMeanSum%d", mod + 1), Form("Pedestal LG mean summary, mod %d", mod + 1), 100, 0., 100.);
      mHist1D[kLGmeanSummaryM1 + mod]->GetXaxis()->SetTitle("ADC channels");
      // mHist1D[kLGmeanSummaryM1 + mod]->SetStats(0);
      mHist1D[kLGmeanSummaryM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kLGmeanSummaryM1 + mod]);
    } else {
      mHist1D[kLGmeanSummaryM1 + mod]->Reset();
    }
    if (!mHist1D[kLGrmsSummaryM1 + mod]) {
      mHist1D[kLGrmsSummaryM1 + mod] = new TH1F(Form("PedLGRMSSum%d", mod + 1), Form("Pedestal LG RMS summary, mod %d", mod + 1), 100, 0., 10.);
      mHist1D[kLGrmsSummaryM1 + mod]->GetXaxis()->SetTitle("ADC channels");
      // mHist1D[kLGrmsSummaryM1 + mod]->SetStats(0);
      mHist1D[kLGrmsSummaryM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kLGrmsSummaryM1 + mod]);
    } else {
      mHist1D[kLGrmsSummaryM1 + mod]->Reset();
    }
  }
}
void RawQcTask::CreatePhysicsHistograms()
{
  // Prepare historams for pedestal run QA

  for (Int_t mod = 0; mod < 4; mod++) {
    if (!mHist2D[kHGoccupM1 + mod]) {
      mHist2D[kHGoccupM1 + mod] = new TH2F(Form("CellHGOccupancyM%d", mod + 1), Form("Cell HG occupancy, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2D[kHGoccupM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kHGoccupM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kHGoccupM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kHGoccupM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kHGoccupM1 + mod]->SetStats(0);
      mHist2D[kHGoccupM1 + mod]->SetMinimum(0);
      // mHist2D[kHGoccupM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kHGoccupM1 + mod]);
    } else {
      mHist2D[kHGoccupM1 + mod]->Reset();
    }

    if (!mHist2D[kLGoccupM1 + mod]) {
      mHist2D[kLGoccupM1 + mod] = new TH2F(Form("CellLGOccupancyM%d", mod + 1), Form("Cell LG occupancy, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2D[kLGoccupM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kLGoccupM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kLGoccupM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kLGoccupM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kLGoccupM1 + mod]->SetStats(0);
      mHist2D[kLGoccupM1 + mod]->SetMinimum(0);
      // mHist2D[kLGoccupM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kLGoccupM1 + mod]);
    } else {
      mHist2D[kLGoccupM1 + mod]->Reset();
    }

    if (!mHist2DMean[kCellEM1 + mod]) {
      mHist2DMean[kCellEM1 + mod] = new TH2FMean(Form("CellEmean%d", mod + 1), Form("Cell mean energy, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2DMean[kCellEM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2DMean[kCellEM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2DMean[kCellEM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2DMean[kCellEM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2DMean[kCellEM1 + mod]->SetStats(0);
      mHist2DMean[kCellEM1 + mod]->SetMinimum(0);
      // mHist2DMean[kCellEM1+mod]->SetMaximum(1.) ;
      getObjectsManager()->startPublishing(mHist2DMean[kCellEM1 + mod]);
    } else {
      mHist2DMean[kCellEM1 + mod]->Reset();
    }

    if (!mHist2D[kTimeEM1 + mod]) {
      mHist2D[kTimeEM1 + mod] = new TH2F(Form("TimevsE%d", mod + 1), Form("Cell time vs energy, mod %d", mod + 1), 50, 0., 1000., 50, -5.e-7, 5.e-7);
      mHist2D[kTimeEM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTimeEM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTimeEM1 + mod]->GetXaxis()->SetTitle("Amp");
      mHist2D[kTimeEM1 + mod]->GetYaxis()->SetTitle("Time (ns)");
      mHist2D[kTimeEM1 + mod]->SetStats(0);
      mHist2D[kTimeEM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist2D[kTimeEM1 + mod]);
    } else {
      mHist2D[kTimeEM1 + mod]->Reset();
    }

    if (!mHist1D[kCellHGSpM1 + mod]) {
      mHist1D[kCellHGSpM1 + mod] = new TH1F(Form("CellHGSpectrumM%d", mod + 1), Form("Cell HG spectrum in mod %d", mod + 1), 100, 0., 5000.);
      mHist1D[kCellHGSpM1 + mod]->GetXaxis()->SetTitle("ADC channels");
      mHist1D[kCellHGSpM1 + mod]->SetStats(0);
      mHist1D[kCellHGSpM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kCellHGSpM1 + mod]);
    } else {
      mHist1D[kCellHGSpM1 + mod]->Reset();
    }
    if (!mHist1D[kCellLGSpM1 + mod]) {
      mHist1D[kCellLGSpM1 + mod] = new TH1F(Form("CellLGSpectrumM%d", mod + 1), Form("Cell LG spectrum in mod %d", mod + 1), 100, 0., 5000.);
      mHist1D[kCellLGSpM1 + mod]->GetXaxis()->SetTitle("ADC channels");
      mHist1D[kCellLGSpM1 + mod]->SetStats(0);
      mHist1D[kCellLGSpM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist1D[kCellLGSpM1 + mod]);
    } else {
      mHist1D[kCellLGSpM1 + mod]->Reset();
    }
  }
}
void RawQcTask::CreateLEDHistograms()
{
  // Occupancy+mean+spectra
  for (Int_t mod = 0; mod < 4; mod++) {
    if (!mHist2DMean[kLEDNpeaksM1 + mod]) {
      mHist2DMean[kLEDNpeaksM1 + mod] = new TH2FMean(Form("NLedPeaksM%d", mod + 1), Form("Number of LED peaks, mod %d", mod + 1), 64, 0., 64., 56, 0., 56.);
      mHist2DMean[kLEDNpeaksM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2DMean[kLEDNpeaksM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2DMean[kLEDNpeaksM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2DMean[kLEDNpeaksM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2DMean[kLEDNpeaksM1 + mod]->SetStats(0);
      mHist2DMean[kLEDNpeaksM1 + mod]->SetMinimum(0);
      getObjectsManager()->startPublishing(mHist2DMean[kLEDNpeaksM1 + mod]);
    } else {
      mHist2DMean[kLEDNpeaksM1 + mod]->Reset();
    }
  }
  // Prepare internal array of histos and final plot with number of peaks per channel
  mSpSearcher = std::make_unique<TSpectrum>(20);
  for (unsigned int absId = 1793; absId <= o2::phos::Mapping::NCHANNELS; absId++) {
    mSpectra.emplace_back(Form("SpChannel%d", absId), "", 487, 50., 1024.);
  }
}
void RawQcTask::CreateTRUHistograms()
{
  // Prepare historams for TRU QA
  for (Int_t mod = 0; mod < 4; mod++) {
    if (!mHist2D[kTRUSTOccupM1 + mod]) {
      mHist2D[kTRUSTOccupM1 + mod] = new TH2F(Form("TRUSumTableOccupancyM%d", mod + 1), Form("TRU summary table occupancy, mod %d", mod + 1), 32, 0., 64., 28, 0., 56.);
      mHist2D[kTRUSTOccupM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTRUSTOccupM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTRUSTOccupM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kTRUSTOccupM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kTRUSTOccupM1 + mod]->SetStats(0);
      mHist2D[kTRUSTOccupM1 + mod]->SetMinimum(0);
      // mHist2D[kTRUSTOccupM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kTRUSTOccupM1 + mod]);
    } else {
      mHist2D[kTRUSTOccupM1 + mod]->Reset();
    }
    if (!mHist2D[kTRUDGOccupM1 + mod]) {
      mHist2D[kTRUDGOccupM1 + mod] = new TH2F(Form("TRUDigOccupancyM%d", mod + 1), Form("TRU digits occupancy, mod %d", mod + 1), 32, 0., 64., 28, 0., 56.);
      mHist2D[kTRUDGOccupM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTRUDGOccupM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTRUDGOccupM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kTRUDGOccupM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kTRUDGOccupM1 + mod]->SetStats(0);
      mHist2D[kTRUDGOccupM1 + mod]->SetMinimum(0);
      // mHist2D[kTRUDGOccupM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kTRUDGOccupM1 + mod]);
    } else {
      mHist2D[kTRUDGOccupM1 + mod]->Reset();
    }
    if (!mHist2D[kTRUSTMatchM1 + mod]) {
      mHist2D[kTRUSTMatchM1 + mod] = new TH2F(Form("TRUMatchedOccupancyM%d", mod + 1), Form("TRU ST+dig matched, mod %d", mod + 1), 32, 0., 64., 28, 0., 56.);
      mHist2D[kTRUSTMatchM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTRUSTMatchM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTRUSTMatchM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kTRUSTMatchM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kTRUSTMatchM1 + mod]->SetStats(0);
      mHist2D[kTRUSTMatchM1 + mod]->SetMinimum(0);
      // mHist2D[kTRUSTMatchM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kTRUSTMatchM1 + mod]);
    } else {
      mHist2D[kTRUSTMatchM1 + mod]->Reset();
    }
    if (!mHist2D[kTRUSTFakeM1 + mod]) {
      mHist2D[kTRUSTFakeM1 + mod] = new TH2F(Form("TRUFakeSTOccupancyM%d", mod + 1), Form("TRU ST without digit, mod %d", mod + 1), 32, 0., 64., 28, 0., 56.);
      mHist2D[kTRUSTFakeM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTRUSTFakeM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTRUSTFakeM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kTRUSTFakeM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kTRUSTFakeM1 + mod]->SetStats(0);
      mHist2D[kTRUSTFakeM1 + mod]->SetMinimum(0);
      // mHist2D[kTRUSTFakeM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kTRUSTFakeM1 + mod]);
    } else {
      mHist2D[kTRUSTFakeM1 + mod]->Reset();
    }

    if (!mHist2D[kTRUDGFakeM1 + mod]) {
      mHist2D[kTRUDGFakeM1 + mod] = new TH2F(Form("TRUFakeDGOccupancyM%d", mod + 1), Form("TRU dig without ST, mod %d", mod + 1), 32, 0., 64., 28, 0., 56.);
      mHist2D[kTRUDGFakeM1 + mod]->GetXaxis()->SetNdivisions(508, kFALSE);
      mHist2D[kTRUDGFakeM1 + mod]->GetYaxis()->SetNdivisions(514, kFALSE);
      mHist2D[kTRUDGFakeM1 + mod]->GetXaxis()->SetTitle("x, cells");
      mHist2D[kTRUDGFakeM1 + mod]->GetYaxis()->SetTitle("z, cells");
      mHist2D[kTRUDGFakeM1 + mod]->SetStats(0);
      mHist2D[kTRUDGFakeM1 + mod]->SetMinimum(0);
      // mHist2D[kTRUDGFakeM1+mod]->SetMaximum(100) ;
      getObjectsManager()->startPublishing(mHist2D[kTRUDGFakeM1 + mod]);
    } else {
      mHist2D[kTRUDGFakeM1 + mod]->Reset();
    }
  }
}

} // namespace o2::quality_control_modules::phos
