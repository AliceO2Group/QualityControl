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
/// \file   TH1MCHReductor.cxx
/// \author Piotr Konopka, Sebastien Perrin
///

#include <TH1.h>
#include "MCH/TH1MCHReductor.h"
#include <iostream>
#include <string>
#include <regex>
#include <gsl/gsl>

namespace o2::quality_control_modules::muonchambers
{

void* TH1MCHReductor::getBranchAddress()
{
  return &mStats;
}

const char* TH1MCHReductor::getBranchLeafList()
{
  return "val500/D:val501:val502:val503:val504:val505:val506:val507:val508:val509:val510:val511:val512:val513:val514:val515:val516:val517:val600:val601:val602:val603:val604:val605:val606:val607:val608:val609:val610:val611:val612:val613:val614:val615:val616:val617:val700:val701:val702:val703:val704:val705:val706:val707:val708:val709:val710:val711:val712:val713:val714:val715:val716:val717:val718:val719:val720:val721:val722:val723:val724:val725:val800:val801:val802:val803:val804:val805:val806:val807:val808:val809:val810:val811:val812:val813:val814:val815:val816:val817:val818:val819:val820:val821:val822:val823:val824:val825:val900:val901:val902:val903:val904:val905:val906:val907:val908:val909:val910:val911:val912:val913:val914:val915:val916:val917:val918:val919:val920:val921:val922:val923:val924:val925:val1000:val1001:val1002:val1003:val1004:val1005:val1006:val1007:val1008:val1009:val1010:val1011:val1012:val1013:val1014:val1015:val1016:val1017:val1018:val1019:val1020:val1021:val1022:val1023:val1024:val1025:val5I:val5O:val6I:val6O:val7I:val7O:val8I:val8O:val9I:val9O:val10I:val10O:entries";
}

void TH1MCHReductor::update(TObject* obj)
{
  auto histo = dynamic_cast<TH1*>(obj);
  if (!histo)
    return;

  mStats.entries = histo->GetEntries();

  // Get value from histo for each DE
  int ivec[7] = { 0, 18, 36, 62, 88, 114, 140 };
  int deMin = 500;
  for (int k = 0; k < 6; k++) {
    for (int i = ivec[k]; i < ivec[k + 1]; i++) {
      mStats.de_values.values[i] = histo->GetBinContent(deMin + (i - ivec[k]) + 1);
    }
    deMin += 100;
  }

  // compute mean value within one half-chamber
  auto computeMean = [&](gsl::span<int> deIDs, int idx) {
    double mean = 0;
    for (int i : deIDs) {
      mean += histo->GetBinContent(i + 1);
    }
    mean /= deIDs.size();
    mStats.halfch_values.values[idx] = mean;
  };

  int index = 0;
  computeMean(detCH5I, index++);
  computeMean(detCH5O, index++);
  computeMean(detCH6I, index++);
  computeMean(detCH6O, index++);
  computeMean(detCH7I, index++);
  computeMean(detCH7O, index++);
  computeMean(detCH8I, index++);
  computeMean(detCH8O, index++);
  computeMean(detCH9I, index++);
  computeMean(detCH9O, index++);
  computeMean(detCH10I, index++);
  computeMean(detCH10O, index++);
}

} // namespace o2::quality_control_modules::muonchambers
