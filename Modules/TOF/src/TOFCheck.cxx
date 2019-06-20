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
/// \file   TOFCheck.cxx
/// \author Nicolo' Jacazio
///

#include "TOF/TOFCheck.h"

// ROOT
#include <FairLogger.h>
#include <TH1.h>
#include <TPaveText.h>
#include <TMath.h>

using namespace std;

namespace o2::quality_control_modules::tof
{

TOFCheck::TOFCheck() {}

TOFCheck::~TOFCheck() {}

void TOFCheck::configure(std::string) {}

Quality TOFCheck::check(const MonitorObject* mo)
{
  Quality result = Quality::Null;

  // if (mo->getName() == "example") {
  //   auto* h = dynamic_cast<TH1F*>(mo->getObject());

  //   result = Quality::Good;

  //   for (int i = 0; i < h->GetNbinsX(); i++) {
  //     if (i > 0 && i < 8 && h->GetBinContent(i) == 0) {
  //       result = Quality::Bad;
  //       break;
  //     } else if ((i == 0 || i > 7) && h->GetBinContent(i) > 0) {
  //       result = Quality::Medium;
  //     }
  //   }
  // }

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

  Double_t binWidthTOFrawTime = 2.44;
  Float_t minTOFrawTime = 175.f;
  Float_t maxTOFrawTime = 250.f;
  // if (AliRecoParam::ConvertIndex(specie) == AliRecoParam::kCosmic) {
  //   minTOFrawTime = 150.; //ns
  //   maxTOFrawTime = 250.; //ns
  // }
  Float_t minTOFrawTot = 10.;  //ns
  Double_t maxTOFrawTot = 15.; //ns

  Float_t minTOFrawINT7hits = 10;
  Float_t maxTOFrawINT7hits = 150; //for pA at 5 and 8 TeV; = 70 for pp 13 TeV

  // TString histname = h->GetName();
  // for (trgCl = 0; trgCl < nTrgCl; trgCl++)
  //   if (histname.EndsWith(AliQADataMaker::GetTrigClassName(trgCl)))
  //     suffixTrgCl = kTRUE;
  // if ((histname.EndsWith("TOFRawsMulti")) || (histname.Contains("TOFRawsMulti") && suffixTrgCl)) {
  if (mo->getName().find("TOFRawsMulti") != std::string::npos) {
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
      Float_t multiMean = h->GetMean();
      Float_t zeroBinIntegral = h->Integral(1, 1);
      Float_t lowMIntegral = h->Integral(1, 10);
      Float_t totIntegral = h->Integral(2, h->GetNbinsX());

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
          if (multiMean < minTOFrawINT7hits)
            isINT7AverageLow = kTRUE;
          if (multiMean > maxTOFrawINT7hits)
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

  // if ((histname.EndsWith("RawsTime")) || (histname.Contains("RawsTime") && suffixTrgCl)) {
  if (mo->getName().find("RawsTime") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // if (!suffixTrgCl)
    //   h->SetBit(AliQAv1::GetImageBit(), drawRawsTimeSumImage);
    // if (suffixTrgCl) {
    //   h->SetBit(AliQAv1::GetImageBit(), kFALSE);
    //   for (int i = 0; i < nToDrawTrgCl; i++) {
    //     if (histname.EndsWith(ClToDraw[i]))
    //       h->SetBit(AliQAv1::GetImageBit(), kTRUE);
    //   }
    // }
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
      // flag = AliQAv1::kWARNING;
    } else {
      Float_t timeMean = h->GetMean();
      Int_t lowBinId = TMath::Nint(minTOFrawTime / binWidthTOFrawTime);
      Int_t highBinId = TMath::Nint(maxTOFrawTime / binWidthTOFrawTime);
      Float_t peakIntegral = h->Integral(lowBinId, highBinId);
      Float_t totIntegral = h->Integral(1, h->GetNbinsX());
      if ((timeMean > minTOFrawTime) && (timeMean < maxTOFrawTime)) {

        result = Quality::Good;
        // flag = AliQAv1::kINFO;
      } else {
        if (peakIntegral / totIntegral > 0.20) {
          LOG(WARNING) << Form("Raw time: peak/total integral = %5.2f, mean = %5.2f ns -> Check filling scheme...", peakIntegral / totIntegral, timeMean);
          result = Quality::Medium;
          // flag = AliQAv1::kWARNING;
        } else {
          LOG(WARNING) << Form("Raw time peak/total integral = %5.2f, mean = %5.2f ns", peakIntegral / totIntegral, timeMean);
          result = Quality::Bad;
          // flag = AliQAv1::kERROR;
        }
      }
    }
  }

  // if ((histname.EndsWith("RawsToT")) || (histname.Contains("RawsToT") && suffixTrgCl)) {
  if (mo->getName().find("RawsToT") != std::string::npos) {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());
    // if (!suffixTrgCl)
    //   h->SetBit(AliQAv1::GetImageBit(), drawRawsToTSumImage);
    // if (suffixTrgCl) {
    //   h->SetBit(AliQAv1::GetImageBit(), kFALSE);
    //   for (int i = 0; i < nToDrawTrgCl; i++) {
    //     if (histname.EndsWith(ClToDraw[i]))
    //       h->SetBit(AliQAv1::GetImageBit(), kTRUE);
    //   }
    // }
    if (h->GetEntries() == 0) {
      result = Quality::Medium;
      // flag = AliQAv1::kWARNING;
    } else {
      Float_t timeMean = h->GetMean();
      if ((timeMean > minTOFrawTot) && (timeMean < maxTOFrawTot)) {
        result = Quality::Good;
        // flag = AliQAv1::kINFO;
      } else {
        LOG(WARNING) << Form("ToT mean = %5.2f ns", timeMean);
        result = Quality::Bad;
        // flag = AliQAv1::kERROR;
      }
    }
  }

  return result;
}

std::string TOFCheck::getAcceptedType() { return "TH1"; }

void TOFCheck::beautify(MonitorObject* mo, Quality checkResult)
{
  if (mo->getName() == "example") {
    auto* h = dynamic_cast<TH1F*>(mo->getObject());

    if (checkResult == Quality::Good) {
      h->SetFillColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      LOG(INFO) << "Quality::Bad, setting to red";
      h->SetFillColor(kRed);
    } else if (checkResult == Quality::Medium) {
      LOG(INFO) << "Quality::medium, setting to orange";
      h->SetFillColor(kOrange);
    }
    h->SetLineColor(kBlack);
  }
}

} // namespace o2::quality_control_modules::tof
