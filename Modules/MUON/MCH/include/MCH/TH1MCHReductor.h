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
/// \file   TH1MCHReductor.h
/// \author Piotr Konopka, Sebastien Perrin
///
#ifndef QUALITYCONTROL_TH1MCHREDUCTOR_H
#define QUALITYCONTROL_TH1MCHREDUCTOR_H

#include "QualityControl/Reductor.h"

namespace o2::quality_control_modules::muonchambers
{

/// \brief A Reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1.

class TH1MCHReductor : public quality_control::postprocessing::Reductor
{
 public:
  TH1MCHReductor() = default;
  ~TH1MCHReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

 private:
  struct {
    union {
      struct {
        Double_t val500;
        Double_t val501;
        Double_t val502;
        Double_t val503;
        Double_t val504;
        Double_t val505;
        Double_t val506;
        Double_t val507;
        Double_t val508;
        Double_t val509;
        Double_t val510;
        Double_t val511;
        Double_t val512;
        Double_t val513;
        Double_t val514;
        Double_t val515;
        Double_t val516;
        Double_t val517;
        Double_t val600;
        Double_t val601;
        Double_t val602;
        Double_t val603;
        Double_t val604;
        Double_t val605;
        Double_t val606;
        Double_t val607;
        Double_t val608;
        Double_t val609;
        Double_t val610;
        Double_t val611;
        Double_t val612;
        Double_t val613;
        Double_t val614;
        Double_t val615;
        Double_t val616;
        Double_t val617;
        Double_t val700;
        Double_t val701;
        Double_t val702;
        Double_t val703;
        Double_t val704;
        Double_t val705;
        Double_t val706;
        Double_t val707;
        Double_t val708;
        Double_t val709;
        Double_t val710;
        Double_t val711;
        Double_t val712;
        Double_t val713;
        Double_t val714;
        Double_t val715;
        Double_t val716;
        Double_t val717;
        Double_t val718;
        Double_t val719;
        Double_t val720;
        Double_t val721;
        Double_t val722;
        Double_t val723;
        Double_t val724;
        Double_t val725;
        Double_t val800;
        Double_t val801;
        Double_t val802;
        Double_t val803;
        Double_t val804;
        Double_t val805;
        Double_t val806;
        Double_t val807;
        Double_t val808;
        Double_t val809;
        Double_t val810;
        Double_t val811;
        Double_t val812;
        Double_t val813;
        Double_t val814;
        Double_t val815;
        Double_t val816;
        Double_t val817;
        Double_t val818;
        Double_t val819;
        Double_t val820;
        Double_t val821;
        Double_t val822;
        Double_t val823;
        Double_t val824;
        Double_t val825;
        Double_t val900;
        Double_t val901;
        Double_t val902;
        Double_t val903;
        Double_t val904;
        Double_t val905;
        Double_t val906;
        Double_t val907;
        Double_t val908;
        Double_t val909;
        Double_t val910;
        Double_t val911;
        Double_t val912;
        Double_t val913;
        Double_t val914;
        Double_t val915;
        Double_t val916;
        Double_t val917;
        Double_t val918;
        Double_t val919;
        Double_t val920;
        Double_t val921;
        Double_t val922;
        Double_t val923;
        Double_t val924;
        Double_t val925;
        Double_t val1000;
        Double_t val1001;
        Double_t val1002;
        Double_t val1003;
        Double_t val1004;
        Double_t val1005;
        Double_t val1006;
        Double_t val1007;
        Double_t val1008;
        Double_t val1009;
        Double_t val1010;
        Double_t val1011;
        Double_t val1012;
        Double_t val1013;
        Double_t val1014;
        Double_t val1015;
        Double_t val1016;
        Double_t val1017;
        Double_t val1018;
        Double_t val1019;
        Double_t val1020;
        Double_t val1021;
        Double_t val1022;
        Double_t val1023;
        Double_t val1024;
        Double_t val1025;
      } named;
      Double_t values[140];
    } de_values;
    union {
      struct {
        Double_t val5I;
        Double_t val5O;
        Double_t val6I;
        Double_t val6O;
        Double_t val7I;
        Double_t val7O;
        Double_t val8I;
        Double_t val8O;
        Double_t val9I;
        Double_t val9O;
        Double_t val10I;
        Double_t val10O;
      } named;
      Double_t values[12];
    } halfch_values;
    Double_t entries;
  } mStats;
  std::array<int, 9> detCH5I = { 500, 501, 502, 503, 504, 514, 515, 516, 517 };
  std::array<int, 9> detCH5O = { 505, 506, 507, 508, 509, 510, 511, 512, 513 };
  std::array<int, 9> detCH6I = { 600, 601, 602, 603, 604, 614, 615, 616, 617 };
  std::array<int, 9> detCH6O = { 605, 606, 607, 608, 609, 610, 611, 612, 613 };
  std::array<int, 13> detCH7I = { 700, 701, 702, 703, 704, 705, 706, 720, 721, 722, 723, 724, 725 };
  std::array<int, 13> detCH7O = { 707, 708, 709, 710, 711, 712, 713, 714, 715, 716, 717, 718, 719 };
  std::array<int, 13> detCH8I = { 800, 801, 802, 803, 804, 805, 806, 820, 821, 822, 823, 824, 825 };
  std::array<int, 13> detCH8O = { 807, 808, 809, 810, 811, 812, 813, 814, 815, 816, 817, 818, 819 };
  std::array<int, 13> detCH9I = { 900, 901, 902, 903, 904, 905, 906, 920, 921, 922, 923, 924, 925 };
  std::array<int, 13> detCH9O = { 907, 908, 909, 910, 911, 912, 913, 914, 915, 916, 917, 918, 919 };
  std::array<int, 13> detCH10I = { 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1020, 1021, 1022, 1023, 1024, 1025 };
  std::array<int, 13> detCH10O = { 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019 };
};

} // namespace o2::quality_control_modules::muonchambers

#endif //QUALITYCONTROL_TH1mchREDUCTOR_H
