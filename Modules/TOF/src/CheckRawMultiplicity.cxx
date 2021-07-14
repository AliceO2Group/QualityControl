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
/// \file   CheckRawMultiplicity.cxx
/// \author Nicolo' Jacazio
/// \brief  Checker for the raw hit multiplicity obtained with the TaskDigits
///

// QC
#include "TOF/CheckRawMultiplicity.h"
#include "QualityControl/QcInfoLogger.h"

// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TObjArray.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

void CheckRawMultiplicity::configure(std::string)
{
  mMinRawHits = 10;
  mMaxRawHits = 150;
  if (auto param = mCustomParameters.find("MinRawHits"); param != mCustomParameters.end()) {
    mMinRawHits = ::atof(param->second.c_str());
  }
  if (auto param = mCustomParameters.find("MaxRawHits"); param != mCustomParameters.end()) {
    mMaxRawHits = ::atof(param->second.c_str());
  }
}

Quality CheckRawMultiplicity::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  // Int_t nTrgCl = 0; // AliQADataMaker::GetNTrigClasses();
  // Int_t nToDrawTrgCl;

  /*customize the summary image*/
  // bool drawRawsSumImage = kTRUE;                       //hTOF_Raws: kTRUE shows it in the summary image, kFALSE does not
  // bool drawRawsTimeSumImage = kTRUE;                   //hTOF_RawsTime: kTRUE shows it in the summary image, kFALSE does not
  // bool drawRawsToTSumImage = kTRUE;                    //hTOF_RawsToT: kTRUE shows it in the summary image, kFALSE does not
  // TString ClToDraw[] = { "kINT7", "kCalibBarell", "0" }; //trigger classes shown in the summary image (it MUST end with "0")

  // Int_t flag = AliQAv1::kNULLBit;
  // Int_t trgCl;
  // Int_t trigId = 0;
  // bool suffixTrgCl = kFALSE;

  // TString histname = h->GetName();
  // for (trgCl = 0; trgCl < nTrgCl; trgCl++)
  //   if (histname.EndsWith(AliQADataMaker::GetTrigClassName(trgCl)))
  //     suffixTrgCl = kTRUE;
  // if ((histname.EndsWith("TOFRawsMulti")) || (histname.Contains("TOFRawsMulti") && suffixTrgCl)) {

  for (auto& [moName, mo] : *moMap) {
    (void)moName;
    if (mo->getName() == "TOFRawsMulti") {
      auto* h = dynamic_cast<TH1I*>(mo->getObject());
      // if (!suffixTrgCl)
      //   h->SetBit(AliQAv1::GetImageBit(), drawRawsSumImage);
      // if (suffixTrgCl) {
      //   h->SetBit(AliQAv1::GetImageBit(), kFALSE); //clones not shown by default
      //   for (int i = 0; i < nToDrawTrgCl; i++) {
      //     if (histname.EndsWith(ClToDraw[i]))
      //       h->SetBit(AliQAv1::GetImageBit(), kTRUE);
      //   }
      // }
      if (h->GetEntries() == 0) { // Histogram is empty
        result = Quality::Medium;
        shifter_msg = "No counts!";
      } else { // Histogram is non empty
        mRawHitsMean = h->GetMean();
        mRawHitsZeroMultIntegral = h->Integral(1, 1);
        mRawHitsLowMultIntegral = h->Integral(1, 10);
        mRawHitsIntegral = h->Integral(2, h->GetNbinsX());

        if (mRawHitsIntegral == 0) { //if only "0 hits per event" bin is filled -> error
          if (h->GetBinContent(1) > 0) {
            result = Quality::Bad;
            shifter_msg = "Only events at 0 filled!";
          }
        } else {
          // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
          if (0) { // TODO: this is only for cosmics, how to check? Re: from the configuration of the checker!
            if (mRawHitsMean < 10.) {
              result = Quality::Good;
              shifter_msg = "Average within limits, OK!";
              // flag = AliQAv1::kINFO;
            } else {
              result = Quality::Medium;
              shifter_msg = "Average outside limits!";
              // flag = AliQAv1::kWARNING;
            }
          } else { // Running with collisions
            const bool isZeroBinContentHigh = (mRawHitsZeroMultIntegral > (mFractAtZeroMult * mRawHitsIntegral));
            const bool isLowMultContentHigh = (mRawHitsLowMultIntegral > (mFractAtLowMult * mRawHitsIntegral));
            const bool isINT7AverageLow = (mRawHitsMean < mMinRawHits);
            const bool isINT7AverageHigh = (mRawHitsMean > mMaxRawHits);

            // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kLowMult) {
            if (0) { // TODO: Low multiplicity running, how to check if it is pp? Probably this can be simplified in the json
              if (isZeroBinContentHigh && (mRawHitsMean > 10.)) {

              } else {
                // if (!histname.Contains("INT7") && (mRawHitsMean > 100.)) {
                if ((mRawHitsMean > 100.)) {
                  result = Quality::Medium;
                  shifter_msg = "Average outside limits!";
                } else {
                  // if (histname.Contains("INT7") && (isINT7AverageLow || isINT7AverageHigh)) {
                  if ((isINT7AverageLow || isINT7AverageHigh)) {
                    result = Quality::Medium;
                    shifter_msg = "Average outside limits!";
                  } else {
                    result = Quality::Good;
                    shifter_msg = "Average within limits, OK!";
                  }
                }
              }
            }
            // } else if ((AliRecoParam::ConvertIndex(specie) == AliRecoParam::kHighMult) && (isLowMultContentHigh || (mRawHitsMean > 500.))) {
            else { // High multiplicity running e.g. Pb-Pb
              if (isLowMultContentHigh) {
                result = Quality::Medium;
                shifter_msg = Form("Low-multiplicity counts are high\n(%.2f higher than total)!", mFractAtLowMult);
              } else if (mRawHitsMean > mMaxTOFRawHitsPbPb) {
                //assume that good range of multi in PbPb goes from 20 to 500 tracks
                result = Quality::Medium;
                shifter_msg = Form("Average higher than expected (%.2f)!", mMaxTOFRawHitsPbPb);
              } else {
                result = Quality::Good;
                shifter_msg = "Average within limits";
              }
            }
          }
        }
      }
    }
  }
  return result;
}

std::string CheckRawMultiplicity::getAcceptedType() { return "TH1I"; }

void CheckRawMultiplicity::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "TOFRawsMulti") {
    auto* h = dynamic_cast<TH1I*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "blNDC");
    h->GetListOfFunctions()->Add(msg);
    msg->SetBorderSize(1);
    msg->SetTextColor(kWhite);
    msg->SetFillColor(kBlack);
    msg->AddText(Form("Default message for %s", mo->GetName()));
    msg->SetName(Form("%s_msg", mo->GetName()));
    msg->Clear();
    TObjArray* txt_arr = shifter_msg.Tokenize("\n");
    for (Int_t i = 0; i < txt_arr->GetEntries(); i++) {
      msg->AddText(txt_arr->At(i)->GetName());
    }

    if (checkResult == Quality::Good) {
      msg->AddText(Form("Mean value = %5.2f", mRawHitsMean));
      msg->AddText(Form("Reference range: %5.2f-%5.2f", mMinRawHits, mMaxRawHits));
      msg->AddText(Form("Events with 0 hits = %5.2f%%", mRawHitsZeroMultIntegral * 100. / mRawHitsIntegral));
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
      msg->SetFillColor(kRed);
      msg->SetTextColor(kBlack);
    } else if (checkResult == Quality::Medium) {
      ILOG(Info, Support) << "Quality::medium, setting to yellow";
      msg->AddText("IF TOF IN RUN check the TOF TWiki");
      msg->SetFillColor(kYellow);
      msg->SetTextColor(kBlack);
    }
  } else {
    ILOG(Error, Support) << "Did not get correct histo from " << mo->GetName();
  }
}

} // namespace o2::quality_control_modules::tof
