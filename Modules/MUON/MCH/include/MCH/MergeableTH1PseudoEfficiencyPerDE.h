// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file MergeableTH1OccupancyPerDE.h
/// \brief An example of a custom TH2Quotient inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin

#ifndef O2_MERGEABLETH1PSEUDOEFFICIENCYPERDE_H
#define O2_MERGEABLETH1PSEUDOEFFICIENCYPERDE_H

#include <sstream>
#include <iostream>
#include <TObject.h>
#include <TH1.h>
#include <TH2.h>
#include <TList.h>
#include <algorithm>

#include "Mergers/MergeInterface.h"
#include "MCHRawElecMap/Mapper.h"
#ifdef HAVE_DIGIT_IN_DATAFORMATS
#include "DataFormatsMCH/Digit.h"
#else
#include "MCHBase/Digit.h"
#endif
#include "MCH/GlobalHistogram.h"

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH1PseudoEfficiencyPerDE : public TH1F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH1PseudoEfficiencyPerDE() = default;

  MergeableTH1PseudoEfficiencyPerDE(MergeableTH1PseudoEfficiencyPerDE const& copymerge)
    : TH1F("DefaultName", "DefaultTitle", 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistosNum(copymerge.getNum()), mhistosDen(copymerge.getDen())
  {
  }

  MergeableTH1PseudoEfficiencyPerDE(const char* name, const char* title, std::map<int, DetectorHistogram*> histosnum, std::map<int, DetectorHistogram*> histosden)
    : TH1F(name, title, 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistosNum(histosnum), mhistosDen(histosden)
  {
    update();
  }

  ~MergeableTH1PseudoEfficiencyPerDE() override = default;

  void merge(MergeInterface* const other) override
  {

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      auto hnum = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDE* const>(other)->getNum().find(de);
      auto hden = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDE* const>(other)->getDen().find(de);
      if ((hden != dynamic_cast<const MergeableTH1PseudoEfficiencyPerDE* const>(other)->getDen().end()) && (hden->second != NULL) && (hnum != dynamic_cast<const MergeableTH1PseudoEfficiencyPerDE* const>(other)->getNum().end()) && (hnum->second != NULL)) {

        auto hnumfinal = mhistosNum.find(de);
        auto hdenfinal = mhistosDen.find(de);
        if ((hdenfinal != mhistosDen.end()) && (hdenfinal->second != NULL) && (hnumfinal != mhistosNum.end()) && (hnumfinal->second != NULL)) {
          hnumfinal->second->Add(hnum->second);
          hdenfinal->second->Add(hden->second);
        }
      }
    }

    update();
  }

  std::map<int, DetectorHistogram*> getNum() const
  {
    return mhistosNum;
  }

  std::map<int, DetectorHistogram*> getDen() const
  {
    return mhistosDen;
  }

  void update()
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    double NewPreclNumDE[1100];
    double NewPreclDenDE[1100];
    double MeanPseudoeffDE;

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      auto hnum = mhistosNum.find(de);
      auto hden = mhistosDen.find(de);
      if ((hden != mhistosDen.end()) && (hden->second != NULL) && (hnum != mhistosNum.end()) && (hnum->second != NULL)) {
        NewPreclNumDE[de] = 0;
        NewPreclDenDE[de] = 0;
        for (int binx = 1; binx < hden->second->GetXaxis()->GetNbins() + 1; binx++) {
          for (int biny = 1; biny < hden->second->GetYaxis()->GetNbins() + 1; biny++) {
            NewPreclDenDE[de] += hden->second->GetBinContent(binx, biny);
          }
        }
        for (int binx = 1; binx < hnum->second->GetXaxis()->GetNbins() + 1; binx++) {
          for (int biny = 1; biny < hnum->second->GetYaxis()->GetNbins() + 1; biny++) {
            NewPreclNumDE[de] += hnum->second->GetBinContent(binx, biny);
          }
        }
      }
    }

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      MeanPseudoeffDE = 0;
      if (NewPreclDenDE[de] > 0) {
        MeanPseudoeffDE = NewPreclNumDE[de] / NewPreclDenDE[de];
      }
      SetBinContent(de + 1, MeanPseudoeffDE);
    }
  }

 private:
  std::map<int, DetectorHistogram*> mhistosNum;
  std::map<int, DetectorHistogram*> mhistosDen;
  std::string mTreatMeAs = "TH1F";

  ClassDefOverride(MergeableTH1PseudoEfficiencyPerDE, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH1PSEUDOEFFICIENCYPERDE_H
