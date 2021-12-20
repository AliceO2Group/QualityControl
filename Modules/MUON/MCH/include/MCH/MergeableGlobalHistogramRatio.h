// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file MergeableGlobalHistogramRatio.h
/// \brief An example of a custom class inheriting MergeInterface
///
/// \author Piotr Konopka, piotr.jan.konopka@cern.ch, Sebastien Perrin

#ifndef O2_MERGEABLEGLOBALHISTOGRAMRATIO_H
#define O2_MERGEABLEGLOBALHISTOGRAMRATIO_H

#include <sstream>
#include <iostream>
#include <TObject.h>
#include <TH2.h>
#include <TList.h>
#include "Mergers/MergeInterface.h"
#include "MCH/GlobalHistogram.h"

using namespace std;
namespace o2::quality_control_modules::muonchambers
{

class MergeableGlobalHistogramRatio : public GlobalHistogram, public o2::mergers::MergeInterface
{
 public:
  MergeableGlobalHistogramRatio() = default;

  MergeableGlobalHistogramRatio(MergeableGlobalHistogramRatio const& copymerge)
    : GlobalHistogram(copymerge.getName(), copymerge.getTitle()), o2::mergers::MergeInterface(), mhistoNum(copymerge.getNum()), mhistoDen(copymerge.getDen()), mname(copymerge.getName()), mtitle(copymerge.getTitle())
  {
    update();
  }

  MergeableGlobalHistogramRatio(const char* name, const char* title, GlobalHistogram* histonum, GlobalHistogram* histoden)
    : GlobalHistogram(name, title), o2::mergers::MergeInterface(), mhistoNum(histonum), mhistoDen(histoden), mname(name), mtitle(title)
  {
    update();
  }

  ~MergeableGlobalHistogramRatio() override = default;

  void merge(MergeInterface* const other) override
  {
    mhistoNum->Add(dynamic_cast<const MergeableGlobalHistogramRatio* const>(other)->getNum());
    mhistoDen->Add(dynamic_cast<const MergeableGlobalHistogramRatio* const>(other)->getDen());
    update();
  }

  GlobalHistogram* getNum() const
  {
    return mhistoNum;
  }

  GlobalHistogram* getDen() const
  {
    return mhistoDen;
  }

  const char* getName() const
  {
    return mname;
  }

  const char* getTitle() const
  {
    return mtitle;
  }

  void update()
  {
    std::cout << "A1" << endl;
    Reset("MICES");
    std::cout << "A2" << endl;
    init();
    std::cout << "A3" << endl;
    Divide(mhistoNum, mhistoDen);
    std::cout << "A4" << endl;
    //  SetName(mname);
    //  SetTitle(mtitle);
    std::cout << "A5" << endl;
    Scale(1 / 87.5);
    SetOption("colz");
    std::cout << "A6" << endl;
  }

 private:
  GlobalHistogram* mhistoNum{ nullptr };
  GlobalHistogram* mhistoDen{ nullptr };
  std::string mTreatMeAs = "TH2F";
  const char* mname = "DefaultName";
  const char* mtitle = "DefaultTitle";

  ClassDefOverride(MergeableGlobalHistogramRatio, 1);
};

} // namespace o2::quality_control_modules::muonchambers

#endif //O2_MERGEABLEGLOBALHISTOGRAMRATIO_H
