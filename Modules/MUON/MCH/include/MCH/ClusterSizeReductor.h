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
/// \file   ClusterSizeReductor.h
/// \author Andrea Ferrero
///
#ifndef QUALITYCONTROL_CLUSTERSIZEREDUCTOR_H
#define QUALITYCONTROL_CLUSTERSIZEREDUCTOR_H

#include "QualityControl/Reductor.h"
#include "MCH/Helpers.h"
#include "MCHRawCommon/DataFormats.h"
#include "MCHRawElecMap/Mapper.h"
#include <array>

namespace o2::quality_control_modules::muonchambers
{

/// \brief A Reductor which obtains the most popular characteristics of TH1.
///
/// A Reductor which obtains the most popular characteristics of TH1.

class ClusterSizeReductor : public quality_control::postprocessing::Reductor
{
 public:
  ClusterSizeReductor();
  ~ClusterSizeReductor() = default;

  void* getBranchAddress() override;
  const char* getBranchLeafList() override;
  void update(TObject* obj) override;

  Double_t getDeValue(int deid, int cathode = 2);

 private:
  static constexpr int sDeNum{ getNumDE() };

  struct {
    union {
      struct {
        // CH 01
        Double_t val100;
        Double_t val101;
        Double_t val102;
        Double_t val103;
        // CH 02
        Double_t val200;
        Double_t val201;
        Double_t val202;
        Double_t val203;
        // CH 03
        Double_t val300;
        Double_t val301;
        Double_t val302;
        Double_t val303;
        // CH 04
        Double_t val400;
        Double_t val401;
        Double_t val402;
        Double_t val403;
        // CH 05
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
        // CH 05
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
        // CH 05
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
        // CH 05
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
        // CH 05
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
        // CH 05
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
      Double_t values[sDeNum];
    } deValues[3];
    Double_t entries;
  } mStats;
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QUALITYCONTROL_CLUSTERSIZEREDUCTOR_H
