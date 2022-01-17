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
/// \file   RawDataQcTask.cxx
/// \author Marek Bombara
///

#include <TCanvas.h>
#include <TH1.h>

#include "QualityControl/QcInfoLogger.h"
#include "CTP/RawDataQcTask.h"
#include "DetectorsRaw/RDHUtils.h"
#include "Headers/RAWDataHeader.h"
#include "DPLUtils/DPLRawParser.h"
#include "DataFormatsCTP/Digits.h"
#include <Framework/InputRecord.h>
#include <Framework/InputRecordWalker.h>

namespace o2::quality_control_modules::ctp
{

RawDataQcTask::~RawDataQcTask()
{
  delete mHistoBC;
  delete mHistoInputs;
  delete mHistoClasses;
}

void RawDataQcTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Support) << "initialize RawDataQcTask" << ENDM; // QcInfoLogger is used. FairMQ logs will go to there as well.

  // this is how to get access to custom parameters defined in the config file at qc.tasks.<task_name>.taskParameters
  if (auto param = mCustomParameters.find("myOwnKey"); param != mCustomParameters.end()) {
    ILOG(Info, Devel) << "Custom parameter - myOwnKey: " << param->second << ENDM;
  }

  mHistoBC = new TH1F("histobc", "BC distribution", 3564, 0, 3564);
  mHistoInputs = new TH1F("inputs", "Inputs distribution", 48, 0, 48);
  mHistoClasses = new TH1F("classes", "Classes distribution", 64, 0, 64);
  getObjectsManager()->startPublishing(mHistoBC);
  getObjectsManager()->startPublishing(mHistoInputs);
  getObjectsManager()->startPublishing(mHistoClasses);
  try {
    getObjectsManager()->addMetadata(mHistoBC->GetName(), "custom", "34");
    getObjectsManager()->addMetadata(mHistoInputs->GetName(), "custom", "34");
    getObjectsManager()->addMetadata(mHistoClasses->GetName(), "custom", "34");
  } catch (...) {
    // some methods can throw exceptions, it is indicated in their doxygen.
    // In case it is recoverable, it is recommended to catch them and do something meaningful.
    // Here we don't care that the metadata was not added and just log the event.
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistoBC->GetName() << ENDM;
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistoInputs->GetName() << ENDM;
    ILOG(Warning, Support) << "Metadata could not be added to " << mHistoClasses->GetName() << ENDM;
  }
}

void RawDataQcTask::startOfActivity(Activity& activity)
{
  ILOG(Info, Support) << "startOfActivity " << activity.mId << ENDM;
  mHistoBC->Reset();
  mHistoInputs->Reset();
  mHistoClasses->Reset();
}

void RawDataQcTask::startOfCycle()
{
  ILOG(Info, Support) << "startOfCycle" << ENDM;
}

void RawDataQcTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input
  o2::framework::DPLRawParser parser(ctx.inputs());
  o2::ctp::gbtword80_t remnant = 0;
  uint32_t size_gbt = 0;
  uint32_t orbit0 = 0;
  bool first = true;
  const o2::ctp::gbtword80_t bcidmask = 0xfff;
  o2::ctp::gbtword80_t pldmask = 0;

  // loop over input
  for (auto it = parser.begin(), end = parser.end(); it != end; ++it) {
    // get the header
    auto const* rdh = it.get_if<o2::header::RAWDataHeader>();
    auto triggerOrbit = o2::raw::RDHUtils::getTriggerOrbit(rdh);
    if (first) {
      orbit0 = triggerOrbit;
      first = false;
    }

    //LOG(info) << "trigger orbit = " << triggerOrbit;
    //mHistogram->Fill(triggerOrbit);
    //mHistogram2->Fill(triggerBC);

    uint32_t payloadCTP;
    auto feeID = o2::raw::RDHUtils::getFEEID(rdh); // 0 = IR, 1 = TCR
    auto linkCRU = (feeID & 0xf00) >> 8;
    if (linkCRU == o2::ctp::GBTLinkIDIntRec) {
      payloadCTP = o2::ctp::NIntRecPayload;
    } else if (linkCRU == o2::ctp::GBTLinkIDClassRec) {
      payloadCTP = o2::ctp::NClassPayload;
    } else {
      LOG(error) << "Unxpected  CTP CRU link:" << linkCRU;
    }
    //LOG(info) << "RDH FEEid: " << feeID << " CTP CRU link:" << linkCRU << " Orbit:" << triggerOrbit << " payloadCTP = " << payloadCTP;
    pldmask = 0;
    for (uint32_t i = 0; i < payloadCTP; i++) {
      pldmask[12 + i] = 1;
    }
    gsl::span<const uint8_t> payload(it.data(), it.size());
    o2::ctp::gbtword80_t gbtWord = 0;
    int wordCount = 0;
    std::vector<o2::ctp::gbtword80_t> diglets;

    // === according to O2: Detectors/CTP/workflow/src/RawToDigitConverterSpec.cxx ========
    for (auto payloadWord : payload) 
    {
      if (wordCount == 15) {
        wordCount = 0;
      } else if (wordCount > 9) {
        wordCount++;
      } else if (wordCount == 9) {
        for (int i = 0; i < 8; i++) {
          gbtWord[wordCount * 8 + i] = bool(int(payloadWord) & (1 << i));
        }
        wordCount++;
        diglets.clear();
        o2::ctp::gbtword80_t diglet = remnant;
        uint32_t k = 0;
        const uint32_t nGBT = o2::ctp::NGBT;
        while (k < (nGBT - payloadCTP)) {
          std::bitset<nGBT> masksize = 0;
          for (uint32_t j = 0; j < (payloadCTP - size_gbt); j++) {
            masksize[j] = 1;
          }
          diglet |= (gbtWord & masksize) << (size_gbt);
          diglets.push_back(diglet);
          diglet = 0;
          k += payloadCTP - size_gbt;
          gbtWord = gbtWord >> (payloadCTP - size_gbt);
          size_gbt = 0;
        }
        size_gbt = nGBT - k;
        remnant = gbtWord;
        // ==========================================
        //LOG(info) << " gbtword after:" << gbtWord;
        for (auto diglet : diglets) {
          //LOG(info) << " diglet:" << diglet;
          //LOG(info) << " pldmask:" << pldmask;
          o2::ctp::gbtword80_t pld = (diglet & pldmask);
          if (pld.count() == 0) {
            continue;
          }
          //LOG(info) << "    pld:" << pld;
          pld >>= 12;
          uint32_t bcid = (diglet & bcidmask).to_ulong();
          //LOG(info) << " diglet:" << diglet;
          //LOG(info) << " bcid:" << bcid;
          mHistoBC->Fill(bcid);

          o2::ctp::gbtword80_t InputMask = 0;
          if (linkCRU == o2::ctp::GBTLinkIDIntRec)
          {
            InputMask = pld;
            //LOG(info) << " InputMask:" << InputMask;
            for (Int_t i = 0; i<payloadCTP; i++)
            {
              if (InputMask[i]!=0) {mHistoInputs->Fill(i);}
            }
          }

          o2::ctp::gbtword80_t ClassMask = 0;
          if (linkCRU == o2::ctp::GBTLinkIDClassRec)
          {
            ClassMask = pld;
            //LOG(info) << " ClassMask:" << ClassMask;
            for (Int_t i = 0; i<payloadCTP; i++)
            {
              if (ClassMask[i]!=0) {mHistoClasses->Fill(i);}
            }
          }

        }
        gbtWord = 0;
      } else {
        for (int i = 0; i < 8; i++) {
            gbtWord[wordCount * 8 + i] = bool(int(payloadWord) & (1 << i));
        }
        wordCount++;
      }
    }

  }
}

void RawDataQcTask::endOfCycle()
{
  ILOG(Info, Support) << "endOfCycle" << ENDM;
}

void RawDataQcTask::endOfActivity(Activity& /*activity*/)
{
  ILOG(Info, Support) << "endOfActivity" << ENDM;
}

void RawDataQcTask::reset()
{
  // clean all the monitor objects here

  ILOG(Info, Support) << "Resetting the histogram" << ENDM;
  mHistoBC->Reset();
  mHistoInputs->Reset();
  mHistoClasses->Reset();
}

} // namespace o2::quality_control_modules::ctp
