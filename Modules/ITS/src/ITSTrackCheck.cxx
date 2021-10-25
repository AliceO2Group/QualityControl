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
/// \file   ITSTrackCheck.cxx
/// \author Artem Isakov
/// \author Jian Liu
///

#include "ITS/ITSTrackCheck.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/Quality.h"

#include <TList.h>
#include <TH1.h>
#include <TH2.h>
#include <TText.h>
#include "TMath.h"

#include <iostream>

namespace o2::quality_control_modules::its
{

void ITSTrackCheck::configure(std::string) {}

Quality ITSTrackCheck::check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap)
{
  Quality result = Quality::Null;
  std::map<std::string, std::shared_ptr<MonitorObject>>::iterator iter;
  for (iter = moMap->begin(); iter != moMap->end(); ++iter) {

    if (iter->second->getName() == "NClusters") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if (h->GetMean() <= 10)
        result = Quality::Good;
      else if (h->GetMean() > 10 && h->GetMean() < 15)
        result = Quality::Medium;
      else if (h->GetMean() >= 15)
        result = Quality::Bad;
    }

    if (iter->second->getName() == "PhiDistribution") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      Double_t ratio = abs(h->Integral(h->FindBin(0), h->FindBin(TMath::Pi())) / h->Integral(h->FindBin(TMath::Pi()), h->FindBin(TMath::TwoPi())) - 1);

      if (ratio > 0.3)
        result = Quality::Bad;
      else
        result = Quality::Good;
    }

    if (iter->second->getName() == "AngularDistribution") {
      auto* hAngluar = dynamic_cast<TH2D*>(iter->second->getObject());
      TH1D* projectPhi = hAngluar->ProjectionY("hAngluar", hAngluar->FindBin(-1.5), hAngluar->FindBin(1.5));
      Double_t ratio = abs(projectPhi->Integral(projectPhi->FindBin(0), projectPhi->FindBin(TMath::Pi())) / projectPhi->Integral(projectPhi->FindBin(TMath::Pi()), projectPhi->FindBin(TMath::TwoPi())) - 1);

      if (ratio > 0.3)
        result = Quality::Bad;
      else
        result = Quality::Good;
    }

    if (iter->second->getName() == "ClusterUsage") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if (h->GetMaximum() < 0.1)
        result = Quality::Bad;
      else
        result = Quality::Good;
    }

    if (iter->second->getName() == "EtaDistribution") {
      auto* h = dynamic_cast<TH1D*>(iter->second->getObject());
      if (abs(h->GetBinCenter(h->FindBin(h->GetMaximum()))) > 0.5)
        result = Quality::Bad;
      else
        result = Quality::Good;
    }
  }
  return result;
}

std::string ITSTrackCheck::getAcceptedType() { return "TH1D"; }

void ITSTrackCheck::beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult)
{
  auto* tInfo = new TText();

  if (mo->getName() == "NClusters") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if (checkResult == Quality::Medium) {
      tInfo->SetText(0.1, 0.8, "Info: a track(s) has between 10 and 15 clusters, inform expert on MM");
      tInfo->SetTextColor(kOrange);
    } else if (checkResult == Quality::Bad) {
      tInfo->SetText(0.1, 0.8, "Info: a track(s) has more than 15 clusters, call expert");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  }

  if (mo->getName() == "PhiDistribution") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      tInfo->SetText(0.1, 0.8, "Info: distribution asymmetric in phi, call expert");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  }

  if (mo->getName() == "AngularDistribution") {
    auto* h = dynamic_cast<TH2D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      tInfo->SetText(0.1, 0.8, "Info: distribution asymmetric in phi, call expert");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  }

  if (mo->getName() == "ClusterUsage") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      tInfo->SetText(0.1, 0.8, "Info: fraction of clusters below 0.1, call expert");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  }
  if (mo->getName() == "EtaDistribution") {
    auto* h = dynamic_cast<TH1D*>(mo->getObject());
    if (checkResult == Quality::Good) {
      tInfo->SetText(0.1, 0.8, "Quality::GOOD");
      tInfo->SetTextColor(kGreen);
    } else if (checkResult == Quality::Bad) {
      tInfo->SetText(0.1, 0.8, "Info: distribution asymmetric in eta, call expert");
      tInfo->SetTextColor(kRed);
    }
    tInfo->SetTextSize(17);
    tInfo->SetNDC();
    h->GetListOfFunctions()->Add(tInfo);
  }
}

} // namespace o2::quality_control_modules::its
