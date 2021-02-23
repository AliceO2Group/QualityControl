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
/// \file   CalDetPublisher.h
/// \author Thomas Klemenz
///

#ifndef QUALITYCONTROL_CALDETPUBLISHER_H
#define QUALITYCONTROL_CALDETPUBLISHER_H

// QC includes
#include "QualityControl/PostProcessingInterface.h"

#include <boost/property_tree/ptree_fwd.hpp>
#include <map>

class TCanvas;
class TPaveText;

namespace o2::quality_control_modules::tpc
{

/// \brief Valid CalDet objects
/// This struct contains the valid CalDet objects to be fetched from the CCDB
enum struct outputCalPad { Pedestal,
                           Noise
};

/// \brief Valid vectors of CalDet objects
/// This struct contains the valid std::unordered_maps of CalDet objects to be fetched from the CCDB
enum struct outputCalPadMap { NoPe,
                              Pulser,
                              CE
};

/// \brief Quality Control task for the calibration data of the TPC
/// \author Thomas Klemenz
class CalDetPublisher final : public quality_control::postprocessing::PostProcessingInterface
{
 public:
  /// \brief Constructor
  CalDetPublisher() = default;
  /// \brief Destructor
  ~CalDetPublisher() = default;

  /// \brief Configuration of a post-processing task.
  /// Configuration of a post-processing task. Can be overridden if user wants to retrieve the configuration of the task.
  /// \param name     Name of the task
  /// \param config   ConfigurationInterface with prefix set to ""
  void configure(std::string name, const boost::property_tree::ptree& config) override;
  /// \brief Initialization of a post-processing task.
  /// Initialization of a post-processing task. User receives a Trigger which caused the initialization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::SOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void initialize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  /// \brief Update of a post-processing task.
  /// Update of a post-processing task. User receives a Trigger which caused the update and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::Period
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void update(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;
  /// \brief Finalization of a post-processing task.
  /// Finalization of a post-processing task. User receives a Trigger which caused the finalization and a service
  /// registry with singleton interfaces.
  /// \param trigger  Trigger which caused the initialization, for example Trigger::EOR
  /// \param services Interface containing optional interfaces, for example DatabaseInterface
  void finalize(quality_control::postprocessing::Trigger, framework::ServiceRegistry&) override;

 private:
  std::vector<std::string> mOutputList{};                                ///< list of CalDet objects to be processed
  std::vector<std::string> mOutputListMap{};                             ///< list of vectors of CalDet objects to be processed
  std::vector<std::vector<std::unique_ptr<TCanvas>>> mCalDetCanvasVec{}; ///< vector containing a vector of summary canvases for every CalDet object
  std::vector<long> mTimestamps{};                                       ///< timestamps to look for specific data in the CCDB
  std::vector<std::map<std::string, std::string>> mLookupMaps{};         ///< meta data to look for data in the CCDB
  std::vector<std::map<std::string, std::string>> mStoreMaps{};          ///< meta data to be stored with the output in the QCDB
  bool mCheckZSCalib;                                                    ///< shall the calib data used for ZS be compared to the latest pedestal and noise files
  bool mCheckZSPrereq = true;                                            ///< is pedestal and noise in the outputList in the config file
  std::unique_ptr<o2::tpc::CalDet<float>> mRefPedestal;                  ///< reference pedestal file used for ZS at the moment
  std::unique_ptr<o2::tpc::CalDet<float>> mRefNoise;                     ///< reference noise file used for ZS at the moment
  long mInitRefCalibTimestamp;                                           ///< timestamp of the pedestal/noise map used at init of the task
  long mInitRefPedestalTimestamp;                                        ///< timestamp of the pedestal data used at init of the task
  long mInitRefNoiseTimestamp;                                           ///< timestamp of the noise data used at init of the task
  TPaveText* mNewZSCalibMsg = nullptr;                                   ///< badge to indicate the necessity to upload new calibration data for ZS

  std::unordered_map<std::string, std::vector<int>> mRanges{ ///< histogram ranges configurable via config file
                                                             { "Pedestals", {} },
                                                             { "Noise", {} },
                                                             { "PulserQtot", {} },
                                                             { "PulserT0", {} },
                                                             { "PulserWidth", {} },
                                                             { "CEQtot", {} },
                                                             { "CET0", {} },
                                                             { "CEWidth", {} }
  };
};

} // namespace o2::quality_control_modules::tpc

#endif //QUALITYCONTROL_CALDETPUBLISHER_H