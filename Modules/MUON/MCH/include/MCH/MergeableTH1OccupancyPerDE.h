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

#ifndef O2_MERGEABLETH1OCCUPANCYPERDE_H
#define O2_MERGEABLETH1OCCUPANCYPERDE_H

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
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
#include "MCHMappingSegContour/CathodeSegmentationContours.h"

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableTH1OccupancyPerDE : public TH1F, public o2::mergers::MergeInterface
{
 public:
  MergeableTH1OccupancyPerDE() = default;

  MergeableTH1OccupancyPerDE(MergeableTH1OccupancyPerDE const& copymerge)
    : TH1F("DefaultName", "DefaultTitle", 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistoNum(copymerge.getNum()), mhistoDen(copymerge.getDen())
  {
  }

  MergeableTH1OccupancyPerDE(const char* name, const char* title, TH2F* histonum, TH2F* histoden)
    : TH1F(name, title, 1100, -0.5, 1099.5), o2::mergers::MergeInterface(), mhistoNum(histonum), mhistoDen(histoden)
  {
    update();
  }

  ~MergeableTH1OccupancyPerDE() override = default;

  void merge(MergeInterface* const other) override
  {
    mhistoNum->Add(dynamic_cast<const MergeableTH1OccupancyPerDE* const>(other)->getNum());
    mhistoDen->Add(dynamic_cast<const MergeableTH1OccupancyPerDE* const>(other)->getDen());

    update();
  }

  TH2F* getNum() const
  {
    return mhistoNum;
  }

  TH2F* getDen() const
  {
    return mhistoDen;
  }

  void update()
  {
    TH2F htemp = TH2F("TempNameDiv", "TempTitleDiv", mhistoNum->GetNbinsX(), 0, mhistoNum->GetNbinsX(), mhistoNum->GetNbinsY(), 0, mhistoNum->GetNbinsY());
    htemp.Divide(mhistoNum, mhistoDen);

    const char* name = this->GetName();
    const char* title = this->GetTitle();
    Reset();
    SetNameTitle(name, title);

    o2::mch::raw::Elec2DetMapper mElec2DetMapper = o2::mch::raw::createElec2DetMapper<o2::mch::raw::ElectronicMapperGenerated>();
    o2::mch::raw::Det2ElecMapper mDet2ElecMapper = o2::mch::raw::createDet2ElecMapper<o2::mch::raw::ElectronicMapperGenerated>();
    o2::mch::raw::FeeLink2SolarMapper mFeeLink2SolarMapper = o2::mch::raw::createFeeLink2SolarMapper<o2::mch::raw::ElectronicMapperGenerated>();
    o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper = o2::mch::raw::createSolar2FeeLinkMapper<o2::mch::raw::ElectronicMapperGenerated>();

    // Using OccupancyElec to get the mean occupancy per DE
    // By looking at each bin in NHits Elec histogram, getting the Elec info (fee, link, de) for each bin, computing the number of hits seen on a given DE and dividing by the number of bins
    double MeanOccupancyDE[1100];
    int NbinsDE[1100];
    auto horbits = mhistoDen;
    if (horbits) {
      for (auto de : o2::mch::raw::deIdsForAllMCH) {
        MeanOccupancyDE[de] = 0;
        NbinsDE[de] = 0;
      }
      for (int binx = 1; binx < htemp.GetXaxis()->GetNbins() + 1; binx++) {
        for (int biny = 1; biny < htemp.GetYaxis()->GetNbins() + 1; biny++) {

          int mNOrbits = horbits->GetBinContent(binx, biny);
          if (mNOrbits <= 0) {
            // no orbits detected for this channel, skip it
            continue;
          }
          // Getting Elec information based on the definition of x and y bins in ElecHistograms
          uint32_t dsAddr = (binx - 1) % 40;
          uint32_t linkId = ((binx - 1 - dsAddr) / 40) % 12;
          uint32_t feeId = (binx - 1 - dsAddr - 40 * linkId) / (12 * 40);
          uint32_t de;
          std::optional<uint16_t> solarId = mFeeLink2SolarMapper(o2::mch::raw::FeeLinkId{ static_cast<uint16_t>(feeId), static_cast<uint8_t>(linkId) });
          if (!solarId.has_value()) {
            continue;
          }
          std::optional<o2::mch::raw::DsDetId> dsDetId =
            mElec2DetMapper(o2::mch::raw::DsElecId{ solarId.value(), static_cast<uint8_t>(dsAddr / 5), static_cast<uint8_t>(dsAddr % 5) });
          if (!dsDetId.has_value()) {
            continue;
          }
          de = dsDetId->deId();

          MeanOccupancyDE[de] += htemp.GetBinContent(binx, biny);
          NbinsDE[de] += 1;
        }
      }

      for (auto i : o2::mch::raw::deIdsForAllMCH) {
        if (NbinsDE[i] > 0) {
          MeanOccupancyDE[i] /= NbinsDE[i];
        }
        SetBinContent(i + 1, MeanOccupancyDE[i]);
      }
    }

    Scale(1 / 87.5);
  }

 private:
  TH2F* mhistoNum{ nullptr };
  TH2F* mhistoDen{ nullptr };
  std::string mTreatMeAs = "TH1F";

  ClassDefOverride(MergeableTH1OccupancyPerDE, 2);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLETH1OCCUPANCYPERDE_H
