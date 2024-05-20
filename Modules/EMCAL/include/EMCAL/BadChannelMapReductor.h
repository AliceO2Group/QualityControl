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

#ifndef QUALITYCONTROL_EMCAL_BADCHANNELMAPREDUCTOR_H
#define QUALITYCONTROL_EMCAL_BADCHANNELMAPREDUCTOR_H

#include "QualityControl/ReductorTObject.h"

namespace o2
{
namespace emcal
{
class Geometry;
}
} // namespace o2

namespace o2::quality_control_modules::emcal
{

/// \class BadChannelMapReductor
/// \brief Dedicated reductor for EMCAL bad channel map
/// \ingroup EMCALQCCReductors
/// \author Markus Fasel <markus.fasel@cern.ch>, Oak Ridge National Laboratory
/// \since November 1st, 2022
///
/// Produces entries:
/// - Bad/Dead/Non-good channels for Full acceptance/Subdetector/Supermodule
/// - Fraction Bad/Dead/Non-good channels for Full acceptance/Subdetector/Supermodule
/// - Index of the Supermodule with the highest number of Bad/Dead/Non-good channels
class BadChannelMapReductor : public quality_control::postprocessing::ReductorTObject
{
 public:
  /// \brief Constructor
  BadChannelMapReductor();
  /// \brief Destructor
  virtual ~BadChannelMapReductor() = default;

  /// \brief Get branch address of structure with data
  /// \return Branch address
  void* getBranchAddress() override;

  /// \brief Get list of variables providede by reductor
  /// \return List of variables (leaflist)
  const char* getBranchLeafList() override;

  /// \brief Extract information from bad channel histogram and fill observables
  /// \param obj Input object to get the data from
  void update(TObject* obj) override;

 private:
  /// \brief Check whether a certain position is within the PHOS region
  /// \param column Column number of the position
  /// \param row Row number of the position
  bool isPHOSRegion(int column, int row) const;

  o2::emcal::Geometry* mGeometry; ///< EMCAL geometry
  struct {
    Int_t mBadChannelsTotal;         ///< Total number of bad channels
    Int_t mDeadChannelsTotal;        ///< Total number of dead channels
    Int_t mNonGoodChannelsTotal;     ///< Total number of channels which are dead or bad
    Int_t mBadChannelsEMCAL;         ///< Number of bad channels in EMCAL
    Int_t mBadChannelsDCAL;          ///< Number of bad channels in DCAL
    Int_t mDeadChannelsEMCAL;        ///< Number of dead channels in EMCAL
    Int_t mDeadChannelsDCAL;         ///< Number of dead channels in DCAL
    Int_t mNonGoodChannelsEMCAL;     ///< Number of channels which are dead or bad in EMCAL
    Int_t mNonGoodChannelsDCAL;      ///< Number of channels which are dead or bad in DCAL
    Int_t mBadChannelsSM[20];        ///< Number of bad channels per Supermodule
    Int_t mDeadChannelsSM[20];       ///< Number of dead channels per Supermodule
    Int_t mNonGoodChannelsSM[20];    ///< Number of channels which are dead or bad per Supermodule
    Int_t mSupermoduleMaxBad;        ///< Index of the supermodule with the highest number of bad channels
    Int_t mSupermoduleMaxDead;       ///< Index of the supermodule with the highest number of dead channels
    Int_t mSupermoduleMaxNonGood;    ///< Index of the supermodule with the highest number of channels which are dead or bad
    Double_t mFractionBadTotal;      ///< Total fraction of bad channels
    Double_t mFractionDeadTotal;     ///< Total fraction of dead channels
    Double_t mFractionNonGoodTotal;  ///< Total fraction of channels which are dead or bad
    Double_t mFractionBadEMCAL;      ///< Fraction of bad channels in EMCAL
    Double_t mFractionBadDCAL;       ///< Fraction of bad channels in DCAL
    Double_t mFractionDeadEMCAL;     ///< Fraction of dead channels in EMCAL
    Double_t mFractionDeadDCAL;      ///< Fraction of dead channels in DCAL
    Double_t mFractionNonGoodEMCAL;  ///< Fraction of channels in EMCAL which are dead or bad
    Double_t mFractionNonGoodDCAL;   ///< Fraction of channels in EMCAL which are dead or bad
    Double_t mFractionBadSM[20];     ///< Fraction of bad channels per Supermodule
    Double_t mFractionDeadSM[20];    ///< Fraction of dead channels per Supermodule
    Double_t mFractionNonGoodSM[20]; ///< Fraction of channels which are dead or bad per Supermodule
  } mStats;                          ///< Trending data point
};

} // namespace o2::quality_control_modules::emcal

#endif // QUALITYCONTROL_TH2REDUCTOR_