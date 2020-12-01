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
/// \file   TaskDigits.h
/// \author Nicolo' Jacazio
/// \brief  Task to monitor quantities in TOF digits in both data and MC
///

#ifndef QC_MODULE_TOF_TOFTASK_H
#define QC_MODULE_TOF_TOFTASK_H

#include "QualityControl/TaskInterface.h"
#include "Base/Counter.h"

class TH1F;
class TH2F;
class TH1I;
class TH2I;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::tof
{

/// \brief TOF Quality Control DPL Task
/// \author Nicolo' Jacazio
class TaskDigits final : public TaskInterface
{
 public:
  /// \brief Constructor
  TaskDigits();
  /// Destructor
  ~TaskDigits() override;

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
  // Event info
  std::shared_ptr<TH2F> mOrbitID = nullptr;      /// Orbits seen
  std::shared_ptr<TH2F> mTimeBC = nullptr;       /// Bunch crossings seen
  std::shared_ptr<TH2F> mEventCounter = nullptr; /// Event counters seen

  //  Multiplicity
  std::shared_ptr<TH1I> mTOFRawsMulti = nullptr;   /// TOF raw hit multiplicity per event
  std::shared_ptr<TH1I> mTOFRawsMultiIA = nullptr; /// TOF raw hit multiplicity per event - I/A side
  std::shared_ptr<TH1I> mTOFRawsMultiOA = nullptr; /// TOF raw hit multiplicity per event - O/A side
  std::shared_ptr<TH1I> mTOFRawsMultiIC = nullptr; /// TOF raw hit multiplicity per event - I/C side
  std::shared_ptr<TH1I> mTOFRawsMultiOC = nullptr; /// TOF raw hit multiplicity per event- O/C side

  // Time
  std::shared_ptr<TH1F> mTOFRawsTime = nullptr;   /// TOF Raws - Hit time (ns)
  std::shared_ptr<TH1F> mTOFRawsTimeIA = nullptr; /// TOF Raws - Hit time (ns) - I/A side
  std::shared_ptr<TH1F> mTOFRawsTimeOA = nullptr; /// TOF Raws - Hit time (ns) - O/A side
  std::shared_ptr<TH1F> mTOFRawsTimeIC = nullptr; /// TOF Raws - Hit time (ns) - I/C side
  std::shared_ptr<TH1F> mTOFRawsTimeOC = nullptr; /// TOF Raws - Hit time (ns) - O/C side

  // ToT
  std::shared_ptr<TH1F> mTOFRawsToT = nullptr;   /// TOF Raws - Hit ToT (ns)
  std::shared_ptr<TH1F> mTOFRawsToTIA = nullptr; /// TOF Raws - Hit ToT (ns) - I/A side
  std::shared_ptr<TH1F> mTOFRawsToTOA = nullptr; /// TOF Raws - Hit ToT (ns) - O/A side
  std::shared_ptr<TH1F> mTOFRawsToTIC = nullptr; /// TOF Raws - Hit ToT (ns) - I/C side
  std::shared_ptr<TH1F> mTOFRawsToTOC = nullptr; /// TOF Raws - Hit ToT (ns) - O/C side

  std::shared_ptr<TH1F> mTOFRawsLTMHits = nullptr; /// LTMs OR signals
  std::shared_ptr<TH2F> mTOFrefMap = nullptr;      /// TOF enabled channel reference map
  std::shared_ptr<TH2F> mTOFRawHitMap = nullptr;   /// TOF raw hit map (1 bin = 1 FEA = 24 channels)

  std::shared_ptr<TH2I> mTOFDecodingErrors = nullptr; /// Decoding error monitoring

  std::shared_ptr<TH1F> mTOFOrphansTime = nullptr;          /// TOF Raws - Orphans time (ns)
  std::shared_ptr<TH2F> mTOFRawTimeVsTRM035 = nullptr;      /// TOF raws - Hit time vs TRM - crates 0 to 35
  std::shared_ptr<TH2F> mTOFRawTimeVsTRM3671 = nullptr;     /// TOF raws - Hit time vs TRM - crates 36 to 72
  std::shared_ptr<TH2F> mTOFTimeVsStrip = nullptr;          /// TOF raw hit time vs. MRPC (along z axis)
  std::shared_ptr<TH2F> mTOFtimeVsBCID = nullptr;           /// TOF time vs BCID
  std::shared_ptr<TH2F> mTOFchannelEfficiencyMap = nullptr; /// TOF channels (HWok && efficient && !noisy && !problematic)
  std::shared_ptr<TH2F> mTOFhitsCTTM = nullptr;             /// Map of hit pads according to CTTM numbering
  std::shared_ptr<TH2F> mTOFmacropadCTTM = nullptr;         /// Map of hit macropads according to CTTM numbering
  std::shared_ptr<TH2F> mTOFmacropadDeltaPhiTime = nullptr; /// #Deltat vs #Delta#Phi of hit macropads
  std::shared_ptr<TH2I> mBXVsCttmBit = nullptr;             /// BX ID in TOF matching window vs trg channel
  std::shared_ptr<TH2F> mTimeVsCttmBit = nullptr;           /// TOF raw time vs trg channel
  std::shared_ptr<TH2F> mTOFRawHitMap24 = nullptr;          /// TOF average raw hits/channel map (1 bin = 1 FEA = 24 channels)
  std::shared_ptr<TH2I> mHitMultiVsDDL = nullptr;           /// TOF raw hit multiplicity per event vs DDL
  std::shared_ptr<TH1I> mNfiredMacropad = nullptr;          /// Number of fired TOF macropads per event

  // Counters
  Counter<72, nullptr> mHitCounterPerStrip[91];         /// Hit map counter in the crate, one per strip
  Counter<72 * 91 * 24, nullptr> mHitCounterPerChannel; /// Hit map counter in the single channel
};

} // namespace o2::quality_control_modules::tof

#endif // QC_MODULE_TOF_TOFTASK_H
