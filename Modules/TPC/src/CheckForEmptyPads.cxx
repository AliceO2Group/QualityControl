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
/// \file   CheckForEmptyPads.cxx
/// \author Laura Serksnyte
///

#include "TPC/CheckForEmptyPads.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"
#include "QualityControl/QcInfoLogger.h"
#include <fairlogger/Logger.h>
#include "Common/Utils.h"

// ROOT
#include <TCanvas.h>
#include <TH1.h>
#include <TH2.h>
#include <TPad.h>
#include <TList.h>
#include <TPaveText.h>

#include <iostream>

namespace o2::quality_control_modules::tpc
{
void CheckForEmptyPads::configure()
{
  mMetadataComment = common::getFromConfig<std::string>(mCustomParameters, "MetadataComment", "");
  if (auto param = mCustomParameters.find("mediumQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mMediumQualityLimit = std::atof(param->second.c_str());
  } else {
    mMediumQualityLimit = 0.7;
    ILOG(Warning, Support) << "Chosen check requires mediumQualityPercentageOfWorkingPads which is not given. Setting to default 0.7 ." << ENDM;
  }
  if (auto param = mCustomParameters.find("badQualityPercentageOfWorkingPads"); param != mCustomParameters.end()) {
    mBadQualityLimit = std::atof(param->second.c_str());
  } else {
    mBadQualityLimit = 0.3;
    ILOG(Warning, Support) << "Chosen check requires badQualityPercentageOfWorkingPads which is not given. Setting to default 0.3 ." << ENDM;
  }
  if (auto param = mCustomParameters.find("MOsNames2D"); param != mCustomParameters.end()) {
    auto temp = param->second.c_str();
    std::istringstream ss(temp);
    std::string token;
    while (std::getline(ss, token, ',')) {
      mMOsToCheck2D.emplace_back(token);
    }
  }
}

//______________________________________________________________________________
Quality CheckForEmptyPads::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  std::vector<std::string> checkMessage;
  std::string message;

  Quality result = Quality::Null;
  for (auto const& moObj : *moMap) {
    auto mo = moObj.second;
    if (!mo) {
      continue;
    }
    auto moName = mo->getName();
    std::string histName, histNameS;
    int padsTotal = 0, padsstart = 1000;
    if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
      size_t end = moName.find("_2D");
      auto histSubName = moName.substr(7, end - 7);
      result = Quality::Good;
      auto* canv = dynamic_cast<TCanvas*>(mo->getObject());
      if (!canv)
        continue;
      // Check all histograms in the canvas
      for (int tpads = 1; tpads <= 72; tpads++) {
        const auto padName = fmt::format("{:s}_{:d}", moName, tpads);
        const auto histName = fmt::format("h_{:s}_ROC_{:02d}", histSubName, tpads - 1);
        TPad* pad = (TPad*)canv->GetListOfPrimitives()->FindObject(padName.data());
        if (!pad) {
          mSectorsName.push_back("notitle");
          mSectorsQuality.push_back(Quality::Null);
          message = "TPad has no title!";
          checkMessage.push_back(message);
          continue;
        }
        TH2F* h = (TH2F*)pad->GetListOfPrimitives()->FindObject(histName.data());
        if (!h) {
          mSectorsName.push_back("notitle");
          mSectorsQuality.push_back(Quality::Null);
          message = "THist has no title!";
          checkMessage.push_back(message);
          continue;
        }
        const std::string titleh = h->GetTitle();

        // check if we are dealing with IROC or OROC
        int totalPads = 0;
        if (titleh.find("IROC") != std::string::npos) {
          totalPads = 5280;
        } else if (titleh.find("OROC") != std::string::npos) {
          totalPads = 9280;
        } else {
          return Quality::Null;
        }
        const int NX = h->GetNbinsX();
        const int NY = h->GetNbinsY();
        // Check how many of the pads are non zero
        int sum = 0;
        for (int i = 1; i <= NX; i++) {
          for (int j = 1; j <= NY; j++) {
            float val = h->GetBinContent(i, j);
            if (val > 0.) {
              sum += 1;
            }
          }
        }
        // Check how many are off
        if (sum > mBadQualityLimit * totalPads && sum < mMediumQualityLimit * totalPads) {
          if (result == Quality::Good) {
            result = Quality::Medium;
          }
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Medium);
        } else if (sum < mBadQualityLimit * totalPads) {
          result = Quality::Bad;
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Bad);
        } else {
          mSectorsName.push_back(titleh);
          mSectorsQuality.push_back(Quality::Good);
        }
        message = fmt::format("Fraction of filled pads: {:.2f}", (float)sum / (float)totalPads);
        checkMessage.push_back(message);
      }
    }
  } // end of loop over moMap

  // For writing to metadata and drawing later
  mBadString = "";
  mMediumString = "";
  mGoodString = "";
  mNullString = "";

  // Aggregation of quality strings used for MO
  for (int qualityindex = 0; qualityindex < mSectorsQuality.size(); qualityindex++) {
    Quality q = mSectorsQuality.at(qualityindex);
    if (q == Quality::Bad) {
      mBadString = mBadString + checkMessage.at(qualityindex) + "\n";
    } else if (q == Quality::Medium) {
      mMediumString = mMediumString + checkMessage.at(qualityindex) + "\n";
    } else if (q == Quality::Good) {
      mGoodString = mGoodString + checkMessage.at(qualityindex) + "\n";
    } else {
      mNullString = mNullString + checkMessage.at(qualityindex) + "\n";
    }
  }

  // Condensing the quality in concise strings used for QO
  mBadStringMeta = "";
  mMediumStringMeta = "";
  mGoodStringMeta = "";
  mNullStringMeta = "";

  mBadStringMeta = summarizeMetaData(Quality::Bad);
  mMediumStringMeta = summarizeMetaData(Quality::Medium);
  mGoodStringMeta = summarizeMetaData(Quality::Good);
  mNullStringMeta = summarizeMetaData(Quality::Null);

  result.addMetadata(Quality::Bad.getName(), mBadStringMeta);
  result.addMetadata(Quality::Medium.getName(), mMediumStringMeta);
  result.addMetadata(Quality::Good.getName(), mGoodStringMeta);
  result.addMetadata(Quality::Null.getName(), mNullStringMeta);
  result.addMetadata("Comment", mMetadataComment);

  return result;
}

//______________________________________________________________________________
std::string CheckForEmptyPads::summarizeMetaData(Quality quality)
{
  std::string sumMetaData = "";

  // reset
  std::string mBadStringtemp = "";
  std::string mMediumStringtemp = "";
  std::string mGoodStringtemp = "";
  std::string mNullStringtemp = "";

  for (int qualityindex = 0; qualityindex < mSectorsQuality.size(); qualityindex++) {
    Quality q = mSectorsQuality.at(qualityindex);
    if (q == Quality::Bad) {
      mBadStringtemp = mBadStringtemp + fmt::format("{:d} ", qualityindex);
    } else if (q == Quality::Medium) {
      mMediumStringtemp = mMediumStringtemp + fmt::format("{:d} ", qualityindex);
    } else if (q == Quality::Good) {
      mGoodStringtemp = mGoodStringtemp + fmt::format("{:d} ", qualityindex);
    } else {
      mNullStringtemp = mNullStringtemp + fmt::format("{:d} ", qualityindex);
    }
  }
  if (quality == Quality::Bad) {
    sumMetaData += "Pads are empty: " + mBadStringtemp + "\n";
  } else if (quality == Quality::Medium) {
    sumMetaData += "Pads have medium quality: " + mMediumStringtemp + "\n";
  } else if (quality == Quality::Good) {
    sumMetaData += "Pads have good quality: " + mGoodStringtemp + "\n";
  } else {
    sumMetaData += "Pads have no quality status: " + mNullStringtemp + "\n";
  }

  return sumMetaData;
}

//______________________________________________________________________________
std::string CheckForEmptyPads::getAcceptedType()
{
  return "TCanvas";
}

//______________________________________________________________________________
void CheckForEmptyPads::beautify(std::shared_ptr<MonitorObject> mo, Quality)
{
  std::string checkMessage;

  auto moName = mo->getName();
  if (auto it = std::find(mMOsToCheck2D.begin(), mMOsToCheck2D.end(), moName); it != mMOsToCheck2D.end()) {
    int padsTotal = 0, padsstart = 1000;
    auto* tcanv = dynamic_cast<TCanvas*>(mo->getObject());
    std::string histNameS, histName;
    padsstart = 1;
    padsTotal = 72;
    size_t end = moName.find("_2D");
    auto histSubName = moName.substr(7, end - 7);
    histNameS = fmt::format("h_{:s}_ROC", histSubName);
    for (int tpads = padsstart; tpads <= padsTotal; tpads++) {
      const std::string padName = fmt::format("{:s}_{:d}", moName, tpads);
      TPad* pad = (TPad*)tcanv->GetListOfPrimitives()->FindObject(padName.data());
      if (!pad) {
        continue;
      }
      pad->cd();
      TH1F* h = nullptr;
      histName = fmt::format("{:s}_{:02d}", histNameS, tpads - 1);
      h = (TH1F*)pad->GetListOfPrimitives()->FindObject(histName.data());
      if (!h) {
        continue;
      }
      const std::string titleh = h->GetTitle();
      auto it = std::find(mSectorsName.begin(), mSectorsName.end(), titleh);
      if (it == mSectorsName.end()) {
        continue;
      }
      const int index = std::distance(mSectorsName.begin(), it);
      TPaveText* msgQuality = new TPaveText(0.1, 0.82, 0.5, 0.9, "NDC");
      msgQuality->SetBorderSize(1);
      Quality qualitySpecial = mSectorsQuality[index];
      msgQuality->SetName(Form("%s_msg", mo->GetName()));
      if (qualitySpecial == Quality::Good) {
        msgQuality->Clear();
        msgQuality->AddText(Quality::Good.getName().data());
        msgQuality->SetFillColor(kGreen);
        checkMessage = mGoodString;
      } else if (qualitySpecial == Quality::Bad) {
        msgQuality->Clear();
        msgQuality->AddText(Quality::Bad.getName().data());
        msgQuality->SetFillColor(kRed);
        checkMessage = mBadString;
      } else if (qualitySpecial == Quality::Medium) {
        msgQuality->Clear();
        msgQuality->AddText(Quality::Medium.getName().data());
        msgQuality->SetFillColor(kOrange);
        checkMessage = mMediumString;
      } else if (qualitySpecial == Quality::Null) {
        msgQuality->AddText(Quality::Null.getName().data());
        h->SetFillColor(0);
        checkMessage = mNullString;
      }

      h->SetLineColor(kBlack);

      // Split lines by hand as \n does not work with TPaveText
      std::string delimiter = "\n";
      size_t pos = 0;
      std::string subText;
      pos = checkMessage.find(delimiter);
      subText = checkMessage.substr(0, pos);
      msgQuality->AddText(subText.c_str());
      checkMessage.erase(0, pos + delimiter.length());
      msgQuality->AddText(qualitySpecial.getMetadata("Comment", "").c_str());

      msgQuality->SetTextSize(32);
      msgQuality->Draw("same");
    }
    auto savefileNAme = fmt::format("/home/ge56luj/Desktop/ThisIsBeautifyObject{:s}.pdf", moName);
    tcanv->SaveAs(savefileNAme.c_str());
    mSectorsName.clear();
    mSectorsQuality.clear();
  }
}

} // namespace o2::quality_control_modules::tpc