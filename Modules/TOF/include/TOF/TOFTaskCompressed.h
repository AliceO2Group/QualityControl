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
/// \file   TOFTaskCompressed.h
/// \author Nicolo' Jacazio
///

#ifndef QC_MODULE_TOF_TOFTASKCOMPRESSED_H
#define QC_MODULE_TOF_TOFTASKCOMPRESSED_H

#include "QualityControl/TaskInterface.h"

class TH1F;
class TH2F;
class TH1I;
class TH2I;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control DPL Task
/// \author Nicolo' Jacazio
class TOFTaskCompressed            /*final*/
  : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  TOFTaskCompressed();
  /// Destructor
  ~TOFTaskCompressed() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  static Int_t fgNbinsMultiplicity;         /// Number of bins in multiplicity plot
  static Int_t fgRangeMinMultiplicity;      /// Min range in multiplicity plot
  static Int_t fgRangeMaxMultiplicity;      /// Max range in multiplicity plot
  static Int_t fgNbinsTime;                 /// Number of bins in time plot
  static const Float_t fgkNbinsWidthTime;   /// Width of bins in time plot
  static Float_t fgRangeMinTime;            /// Range min in time plot
  static Float_t fgRangeMaxTime;            /// Range max in time plot
  static Int_t fgCutNmaxFiredMacropad;      /// Cut on max number of fired macropad
  static const Int_t fgkFiredMacropadLimit; /// Limit on cut on number of fired macropad

 private:
  std::shared_ptr<TH1F> mHisto;          /// Number of fired TOF macropads per event
  std::shared_ptr<TH1F> mTime;          /// Number of fired TOF macropads per event
  std::shared_ptr<TH1F> mTOT;          /// Number of fired TOF macropads per event
  std::shared_ptr<TH1F> mIndexE;          /// Number of fired TOF macropads per event
  std::shared_ptr<TH2F> mSlotEnableMask;          /// Number of fired TOF macropads per event
  std::shared_ptr<TH2F> mDiagnostic;          /// Number of fired TOF macropads per event

};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTASKCOMPRESSED_H
