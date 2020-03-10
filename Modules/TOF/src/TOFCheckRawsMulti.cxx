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
/// \file   TOFCheckRawsMulti.cxx
/// \author Nicolo' Jacazio
///

// QC
#include "TOF/TOFCheckRawsMulti.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <fairlogger/Logger.h>
// ROOT
#include <TH1.h>
#include <TPaveText.h>
#include <TList.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

TOFCheckRawsMulti::TOFCheckRawsMulti() : minTOFrawhits(10),
                                         maxTOFrawhits(150),
                                         multiMean(0),
                                         zeroBinIntegral(0),
                                         lowMIntegral(0),
                                         totIntegral(0)
{
}

TOFCheckRawsMulti::~TOFCheckRawsMulti() {}

void TOFCheckRawsMulti::configure(std::string) {}

Quality TOFCheckRawsMulti::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{

  Quality result = Quality::Null;

  // Int_t nTrgCl = 0; // AliQADataMaker::GetNTrigClasses();
  // Int_t nToDrawTrgCl;

  /*customize the summary image*/
  // Bool_t drawRawsSumImage = kTRUE;                       //hTOF_Raws: kTRUE shows it in the summary image, kFALSE does not
  // Bool_t drawRawsTimeSumImage = kTRUE;                   //hTOF_RawsTime: kTRUE shows it in the summary image, kFALSE does not
  // Bool_t drawRawsToTSumImage = kTRUE;                    //hTOF_RawsToT: kTRUE shows it in the summary image, kFALSE does not
  // TString ClToDraw[] = { "kINT7", "kCalibBarell", "0" }; //trigger classes shown in the summary image (it MUST end with "0")

  // Int_t flag = AliQAv1::kNULLBit;
  // Int_t trgCl;
  // Int_t trigId = 0;
  // Bool_t suffixTrgCl = kFALSE;

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
      if (h->GetEntries() == 0) {
        result = Quality::Medium;
        // flag = AliQAv1::kWARNING;
      } else {
        multiMean = h->GetMean();
        zeroBinIntegral = h->Integral(1, 1);
        lowMIntegral = h->Integral(1, 10);
        totIntegral = h->Integral(2, h->GetNbinsX());

        if (totIntegral == 0) { //if only "0 hits per event" bin is filled -> error
          if (h->GetBinContent(1) > 0) {
            result = Quality::Medium;
            // flag = AliQAv1::kERROR;
          }
        } else {
          // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
          if (0) {
            if (multiMean < 10.) {
              result = Quality::Good;
              // flag = AliQAv1::kINFO;
            } else {
              result = Quality::Medium;
              // flag = AliQAv1::kWARNING;
            }
          } else {
            Bool_t isZeroBinContentHigh = kFALSE;
            Bool_t isLowMultContentHigh = kFALSE;
            Bool_t isINT7AverageLow = kFALSE;
            Bool_t isINT7AverageHigh = kFALSE;

            if (zeroBinIntegral > 0.75 * totIntegral)
              isZeroBinContentHigh = kTRUE;
            if (lowMIntegral > 0.75 * totIntegral)
              isLowMultContentHigh = kTRUE;
            if (multiMean < minTOFrawhits)
              isINT7AverageLow = kTRUE;
            if (multiMean > maxTOFrawhits)
              isINT7AverageHigh = kTRUE;

            // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kLowMult) {
            if (0) {
              if (isZeroBinContentHigh && (multiMean > 10.)) {

              } else {
                // if (!histname.Contains("INT7") && (multiMean > 100.)) {
                if ((multiMean > 100.)) {
                  result = Quality::Medium;
                  // flag = AliQAv1::kWARNING;
                } else {
                  // if (histname.Contains("INT7") && (isINT7AverageLow || isINT7AverageHigh)) {
                  if ((isINT7AverageLow || isINT7AverageHigh)) {
                    result = Quality::Medium;
                    // flag = AliQAv1::kWARNING;
                  } else {
                    result = Quality::Good;
                    // flag = AliQAv1::kINFO;
                  }
                }
              }
              // } else if ((AliRecoParam::ConvertIndex(specie) == AliRecoParam::kHighMult) && (isLowMultContentHigh || (multiMean > 500.))) {
            } else if ((isLowMultContentHigh || (multiMean > 500.))) {
              //assume that good range of multi in PbPb goes from 20 to 500 tracks
              result = Quality::Medium;
              // flag = AliQAv1::kWARNING;
            } else {
              result = Quality::Good;
              // flag = AliQAv1::kINFO;
            }
          }
        }
      }
    }
  }
  return result;
}

std::string TOFCheckRawsMulti::getAcceptedType() { return "TH1I"; }

void TOFCheckRawsMulti::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  if (mo->getName() == "TOFRawsMulti") {
    auto* h = dynamic_cast<TH1I*>(mo->getObject());
    TPaveText* msg = new TPaveText(0.5, 0.5, 0.9, 0.75, "NDC");
    h->GetListOfFunctions()->Add(msg);
    msg->Draw();
    msg->SetName(Form("%s_msg", mo->GetName()));

    if (checkResult == Quality::Good) {
      LOG(INFO) << "Quality::Good, setting to green";
      msg->Clear();
      msg->AddText(Form("Mean value = %5.2f", multiMean));
      msg->AddText(Form("Reference range: %5.2f-%5.2f", minTOFrawhits, maxTOFrawhits));
      msg->AddText(Form("Events with 0 hits = %5.2f%%", zeroBinIntegral * 100. / totIntegral));
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      //
      msg->Clear();
      msg->AddText("No TOF hits for all events.");
      msg->AddText("Call TOF on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      //
      msg->Clear();
      msg->AddText("No entries. IF TOF IN RUN");
      msg->AddText("check the TOF TWiki");
      msg->SetFillColor(kYellow);
      //
      h->SetFillColor(kOrange);
    } else {
      LOG(INFO) << "Quality::Null, setting to black background";
      msg->SetFillColor(kBlack);
    }
  } else
    LOG(ERROR) << "Did not get correct histo from " << mo->GetName();
}

} // namespace o2::quality_control_modules::tof
