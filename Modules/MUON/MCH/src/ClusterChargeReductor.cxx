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
/// \file   ClusterChargeReductor.cxx
/// \author Piotr Konopka, Sebastien Perrin
///

#include "MCH/ClusterChargeReductor.h"
#include <TH2F.h>
#include <TH1F.h>

namespace o2::quality_control_modules::muonchambers
{

ClusterChargeReductor::ClusterChargeReductor() : quality_control::postprocessing::Reductor()
{
}

void* ClusterChargeReductor::getBranchAddress()
{
  return &mStats;
}

const char* ClusterChargeReductor::getBranchLeafList()
{
  return "DE100/D:DE101:DE102:DE103:DE200:DE201:DE202:DE203:DE300:DE301:DE302:DE303:DE400:DE401:DE402:DE403:DE500:DE501:DE502:DE503:DE504:DE505:DE506:DE507:DE508:DE509:DE510:DE511:DE512:DE513:DE514:DE515:DE516:DE517:DE600:DE601:DE602:DE603:DE604:DE605:DE606:DE607:DE608:DE609:DE610:DE611:DE612:DE613:DE614:DE615:DE616:DE617:DE700:DE701:DE702:DE703:DE704:DE705:DE706:DE707:DE708:DE709:DE710:DE711:DE712:DE713:DE714:DE715:DE716:DE717:DE718:DE719:DE720:DE721:DE722:DE723:DE724:DE725:DE800:DE801:DE802:DE803:DE804:DE805:DE806:DE807:DE808:DE809:DE810:DE811:DE812:DE813:DE814:DE815:DE816:DE817:DE818:DE819:DE820:DE821:DE822:DE823:DE824:DE825:DE900:DE901:DE902:DE903:DE904:DE905:DE906:DE907:DE908:DE909:DE910:DE911:DE912:DE913:DE914:DE915:DE916:DE917:DE918:DE919:DE920:DE921:DE922:DE923:DE924:DE925:DE1000:DE1001:DE1002:DE1003:DE1004:DE1005:DE1006:DE1007:DE1008:DE1009:DE1010:DE1011:DE1012:DE1013:DE1014:DE1015:DE1016:DE1017:DE1018:DE1019:DE1020:DE1021:DE1022:DE1023:DE1024:DE1025:entries";
}

Double_t ClusterChargeReductor::getDeValue(int deid)
{
  if (deid < 0 || deid >= sDeNum) {
    return 0;
  }
  return mStats.deValues.values[deid];
}

void ClusterChargeReductor::update(TObject* obj)
{
  auto h = dynamic_cast<TH2F*>(obj);
  if (!h) {
    return;
  }

  for (int xbin = 1; xbin <= h->GetXaxis()->GetNbins(); xbin++) {
    if (xbin > sDeNum) {
      break;
    }
    int de = xbin - 1;

    TH1F* proj = (TH1F*)h->ProjectionY("_proj", xbin, xbin);
    int binmax = proj->GetMaximumBin();
    double xmax = proj->GetXaxis()->GetBinCenter(binmax);
    mStats.deValues.values[de] = xmax;
    delete proj;
  }
}

} // namespace o2::quality_control_modules::muonchambers
