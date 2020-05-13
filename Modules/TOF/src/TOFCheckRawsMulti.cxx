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
      if (h->GetEntries() == 0) { // Histogram is empty
        result = Quality::Medium;
        // flag = AliQAv1::kWARNING;
        shifter_msg = "No counts!";
      } else { // Histogram is non empty
        multiMean = h->GetMean();
        zeroBinIntegral = h->Integral(1, 1);
        lowMIntegral = h->Integral(1, 10);
        totIntegral = h->Integral(2, h->GetNbinsX());

        if (totIntegral == 0) { //if only "0 hits per event" bin is filled -> error
          if (h->GetBinContent(1) > 0) {
            result = Quality::Bad;
            // flag = AliQAv1::kERROR;
            shifter_msg = "Only events at 0 filled!";
          }
        } else {
          // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
          if (0) { // TODO: this is only for cosmics, how to check?
            if (multiMean < 10.) {
              result = Quality::Good;
              shifter_msg = "Average within limits, OK!";
              // flag = AliQAv1::kINFO;
            } else {
              result = Quality::Medium;
              shifter_msg = "Average outside limits!";
              // flag = AliQAv1::kWARNING;
            }
          } else { // Running with collisions
            const Bool_t isZeroBinContentHigh = (zeroBinIntegral > fracAtZeroMult * totIntegral);
            const Bool_t isLowMultContentHigh = (lowMIntegral > fracAtLowMult * totIntegral);
            const Bool_t isINT7AverageLow = (multiMean < minTOFrawhits);
            const Bool_t isINT7AverageHigh = (multiMean > maxTOFrawhits);

            // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kLowMult) {
            if (0) { // TODO: Low multiplicity running, how to check if it is pp? Probably this can be simplified in the json
              if (isZeroBinContentHigh && (multiMean > 10.)) {

              } else {
                // if (!histname.Contains("INT7") && (multiMean > 100.)) {
                if ((multiMean > 100.)) {
                  result = Quality::Medium;
                  shifter_msg = "Average outside limits!";
                  // flag = AliQAv1::kWARNING;
                } else {
                  // if (histname.Contains("INT7") && (isINT7AverageLow || isINT7AverageHigh)) {
                  if ((isINT7AverageLow || isINT7AverageHigh)) {
                    result = Quality::Medium;
                    shifter_msg = "Average outside limits!";
                    // flag = AliQAv1::kWARNING;
                  } else {
                    result = Quality::Good;
                    shifter_msg = "Average within limits, OK!";
                    // flag = AliQAv1::kINFO;
                  }
                }
              }
            }
            // } else if ((AliRecoParam::ConvertIndex(specie) == AliRecoParam::kHighMult) && (isLowMultContentHigh || (multiMean > 500.))) {
            else { // High multiplicity running e.g. Pb-Pb
              if (isLowMultContentHigh) {
                result = Quality::Medium;
                shifter_msg = Form("Low-multiplicity counts are high\n(%.2f higher than total)!", fracAtLowMult);
              } else if (multiMean > maxTOFrawhitsPbPb) {
                //assume that good range of multi in PbPb goes from 20 to 500 tracks
                result = Quality::Medium;
                shifter_msg = Form("Average higher than expected (%.2f)!", maxTOFrawhitsPbPb);
                // flag = AliQAv1::kWARNING;
              } else {
                result = Quality::Good;
                shifter_msg = "Average within limits";
                // flag = AliQAv1::kINFO;
              }
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
    msg->Clear();
    TObjArray* txt_arr = shifter_msg.Tokenize("\n");
    for (Int_t i = 0; i < txt_arr->GetEntries(); i++) {
      msg->AddText(txt_arr->At(i)->GetName());
    }

    if (checkResult == Quality::Good) {
      msg->AddText(Form("Mean value = %5.2f", multiMean));
      msg->AddText(Form("Reference range: %5.2f-%5.2f", minTOFrawhits, maxTOFrawhits));
      msg->AddText(Form("Events with 0 hits = %5.2f%%", zeroBinIntegral * 100. / totIntegral));
      msg->AddText("OK!");
      msg->SetFillColor(kGreen);
      //
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      msg->AddText("Call TOF on-call.");
      msg->SetFillColor(kRed);
      //
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      msg->AddText("IF TOF IN RUN check the TOF TWiki");
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
