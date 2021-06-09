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
/// \file   CustomTH2F.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_EXAMPLE_CUSTOMTH2F_H
#define QC_MODULE_EXAMPLE_CUSTOMTH2F_H

#include "QualityControl/TaskInterface.h"
#include <TH2F.h>
#include "Mergers/MergeInterface.h"

namespace o2::quality_control_modules::example
{

/// \brief Example of a custom class that inherits from a ROOT standard class.
/// It should be drawn by calling the standard TH2::Draw() method. In ROOT, it is transparent,
/// in QCG we state the equivalence via the member `mTreatMeAs`.
/// \author Barthelemy von Haller
class CustomTH2F : public TH2F, public o2::mergers::MergeInterface
{
 public:
  /// \brief Constructor.
  CustomTH2F(std::string name);
  CustomTH2F() = default;
  /// \brief Default destructor
  ~CustomTH2F() override = default;

  const char* GetName() const override
  {
    return TH2F::GetName();
  }

  void merge(MergeInterface* const other) override
  {
    auto otherHisto = dynamic_cast<const TH2F* const>(other);
    if (otherHisto) {
      this->Add(otherHisto);
    }
  }

 private:
  std::string mTreatMeAs = "TH2F"; // the name of the class this object should be considered as when drawing in QCG.

  ClassDefOverride(CustomTH2F, 1);
};

} // namespace o2::quality_control_modules::example

#endif //QC_MODULE_EXAMPLE_CUSTOMTH2F_H
