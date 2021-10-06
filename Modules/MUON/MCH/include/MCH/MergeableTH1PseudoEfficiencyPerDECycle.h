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

#ifndef O2_MERGEABLETH1PSEUDOEFFICIENCYPERDECYCLE_H
#define O2_MERGEABLETH1PSEUDOEFFICIENCYPERDECYCLE_H

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

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH1PseudoEfficiencyPerDECycle : public TH1F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH1PseudoEfficiencyPerDECycle() = default;

  MergeableTH1PseudoEfficiencyPerDECycle(MergeableTH1PseudoEfficiencyPerDECycle const& copymerge)
    : TH1F("DefaultNameCycle", "DefaultTitleCycle", 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistosNum(copymerge.getNum()), mhistosDen(copymerge.getDen())
  {
  }

  MergeableTH1PseudoEfficiencyPerDECycle(const char* name, const char* title, std::map<int, DetectorHistogram*> histosnum, std::map<int, DetectorHistogram*> histosden)
    : TH1F(name, title, 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistosNum(histosnum), mhistosDen(histosden)
  {
    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      LastMeanNumDE[de] = 0;
      LastMeanDenDE[de] = 0;
    }
    update();
  }

  ~MergeableTH1PseudoEfficiencyPerDECycle() override = default;

  void merge(MergeInterface* const other) override
  { //FIXME
    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      auto hnum = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getNum().find(de);
      auto hden = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getDen().find(de);
      if ((hden != dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getDen().end()) && (hden->second != NULL) && (hnum != dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getNum().end()) && (hnum->second != NULL)) {

        auto hnumfinal = mhistosNum.find(de);
        auto hdenfinal = mhistosDen.find(de);
        if ((hdenfinal != mhistosDen.end()) && (hdenfinal->second != NULL) && (hnumfinal != mhistosNum.end()) && (hnumfinal->second != NULL)) {
          hnumfinal->second->Add(hnum->second);
          hdenfinal->second->Add(hden->second);
        }
      }
    }

    const double* ptrotherLastMeanNumDE = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getLastMeanNumDE();
    const double* ptrotherLastMeanDenDE = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getLastMeanDenDE();

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      LastMeanNumDE[de] += ptrotherLastMeanNumDE[de];
      LastMeanDenDE[de] += ptrotherLastMeanDenDE[de];
    }

    const double* ptrotherNewMeanNumDE = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getNewMeanNumDE();
    const double* ptrotherNewMeanDenDE = dynamic_cast<const MergeableTH1PseudoEfficiencyPerDECycle* const>(other)->getNewMeanDenDE();

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      NewMeanNumDE[de] += ptrotherNewMeanNumDE[de];
      NewMeanDenDE[de] += ptrotherNewMeanDenDE[de];
    }

    updateAfterMerge();
  }

  std::map<int, DetectorHistogram*> getNum() const
  {
    return mhistosNum;
  }

  std::map<int, DetectorHistogram*> getDen() const
  {
    return mhistosDen;
  }

  const double* getLastMeanNumDE() const
  {
    const double* ptrLastMeanNumDE = LastMeanNumDE;
    return ptrLastMeanNumDE;
  }

  const double* getLastMeanDenDE() const
  {
    const double* ptrLastMeanDenDE = LastMeanDenDE;
    return ptrLastMeanDenDE;
  }

  const double* getNewMeanNumDE() const
  {
    const double* ptrNewMeanNumDE = NewMeanNumDE;
    return ptrNewMeanNumDE;
  }

  const double* getNewMeanDenDE() const
  {
    const double* ptrNewMeanDenDE = NewMeanDenDE;
    return ptrNewMeanDenDE;
  }

  void update()
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    for (auto de : o2::mch::raw::deIdsForAllMCH) {

      LastMeanNumDE[de] = NewMeanNumDE[de];
      LastMeanDenDE[de] = NewMeanDenDE[de];
      NewMeanNumDE[de] = 0;
      NewMeanDenDE[de] = 0;

      auto hnum = mhistosNum.find(de);
      auto hden = mhistosDen.find(de);
      if ((hden != mhistosDen.end()) && (hden->second != NULL) && (hnum != mhistosNum.end()) && (hnum->second != NULL)) {
        for (int binx = 1; binx < hden->second->GetXaxis()->GetNbins() + 1; binx++) {
          for (int biny = 1; biny < hden->second->GetYaxis()->GetNbins() + 1; biny++) {
            NewMeanDenDE[de] += hden->second->GetBinContent(binx, biny);
          }
        }
        for (int binx = 1; binx < hnum->second->GetXaxis()->GetNbins() + 1; binx++) {
          for (int biny = 1; biny < hnum->second->GetYaxis()->GetNbins() + 1; biny++) {
            NewMeanNumDE[de] += hnum->second->GetBinContent(binx, biny);
          }
        }
      }
    }

    for (auto de : o2::mch::raw::deIdsForAllMCH) {
      double MeanPseudoeffDECycle = 0;
      if ((NewMeanNumDE[de] - LastMeanNumDE[de]) > 0) {
        MeanPseudoeffDECycle = (NewMeanNumDE[de] - LastMeanNumDE[de]) / (NewMeanDenDE[de] - LastMeanDenDE[de]);
      }
      SetBinContent(de + 1, MeanPseudoeffDECycle);
    }
  }

  void updateAfterMerge()
  {
    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    // Using NHitsElec and Norbits to get the mean occupancy per DE on last cycle
    // Similarly, by looking at NHits and NOrbits Elec histogram, for each bin incrementing the corresponding DE total hits and total orbits, and from the values obtained at the end of a cycle compared to the beginning of the cycle, computing the occupancy during the cycle.
    for (auto i : o2::mch::raw::deIdsForAllMCH) {
      auto hnum = mhistosNum.find(i);
      auto hden = mhistosDen.find(i);
      if ((hden != mhistosDen.end()) && (hden->second != NULL) && (hnum != mhistosNum.end()) && (hnum->second != NULL)) {
        double MeanPseudoEfficiencyDECycle = 0;
        if ((NewMeanDenDE[i] - LastMeanDenDE[i]) > 0) {
          MeanPseudoEfficiencyDECycle = (NewMeanNumDE[i] - LastMeanNumDE[i]) / (NewMeanDenDE[i] - LastMeanDenDE[i]);
        }
        SetBinContent(i + 1, MeanPseudoEfficiencyDECycle);
      }
    }
  }

 private:
  std::map<int, DetectorHistogram*> mhistosNum;
  std::map<int, DetectorHistogram*> mhistosDen;
  std::string mTreatMeAs = "TH1F";
  double NewMeanNumDE[1100]{ 0 };
  double NewMeanDenDE[1100]{ 0 };
  double LastMeanNumDE[1100]{ 0 };
  double LastMeanDenDE[1100]{ 0 };

  ClassDefOverride(MergeableTH1PseudoEfficiencyPerDECycle, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH1PSEUDOEFFICIENCYPERDECYCLE_H
