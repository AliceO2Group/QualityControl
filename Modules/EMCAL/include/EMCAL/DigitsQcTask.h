///
/// \file   DigitsQcTask.h
/// \author Markus Fasel, Cristina Terrevoli
///

#ifndef QC_MODULE_EMCAL_DIGITSQCTASK_H
#define QC_MODULE_EMCAL_DIGITSQCTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include <CCDB/TObjectWrapper.h>
#include <TProfile2D.h>

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2
{
namespace emcal
{
class Geometry;
class BadChannelMap;
class TimeCalibrationParams;
} // namespace emcal

namespace quality_control_modules
{
namespace emcal
{

/// \brief QC Task for EMCAL digits
/// \author Markus Fasel
///
/// The main monitoring component for EMCAL digits (energy and time measurement in tower).
/// Monitoring observables:
/// - Digit amplitude for different towers
/// - Digit time for different towers
class DigitsQcTask final : public TaskInterface
{
 public:
  struct DigitsHistograms {
    std::string mTriggerClass;
    std::array<TH2*, 2> mDigitAmplitude;      ///< Digit amplitude
    std::array<TH2*, 2> mDigitTime;           ///< Digit time
    std::array<TH2*, 2> mDigitAmplitudeCalib; ///< Digit amplitude calibrated
    std::array<TH2*, 2> mDigitTimeCalib;      ///< Digit time calibrated

    TH2* mDigitOccupancy = nullptr;             ///< Digit occupancy EMCAL and DCAL
    TH2* mDigitOccupancyThr = nullptr;          ///< Digit occupancy EMCAL and DCAL with Energy trheshold
    TProfile2D* mIntegratedOccupancy = nullptr; ///< Digit integrated occupancy
    TH1* mDigitAmplitudeEMCAL = nullptr;        ///< Digit amplitude in EMCAL
    TH1* mDigitAmplitudeDCAL = nullptr;         ///< Digit amplitude in DCAL
    TH1* mnumberEvents = nullptr;               ///< Number of Events for normalization

    void initForTrigger(const char* trigger);
    void startPublishing();
    void reset();
    void clean();
  };

  /// \brief Constructor
  DigitsQcTask() = default;
  /// Destructor
  ~DigitsQcTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

  void setThreshold(Double_t threshold) { mCellThreshold = threshold; }
  void setEndOfPayloadCheck(Bool_t doCheck) { mDoEndOfPayloadCheck = doCheck; }

 private:
  void startPublishing(DigitsHistograms& histos);
  Double_t mCellThreshold = 0.5;                               ///< energy cell threshold
  Bool_t mDoEndOfPayloadCheck = false;                         ///< Do old style end-of-payload check
  std::map<std::string, DigitsHistograms> mHistogramContainer; ///< Container with histograms per trigger class
  o2::emcal::Geometry* mGeometry = nullptr;                    ///< EMCAL geometry
  o2::emcal::BadChannelMap* mBadChannelMap;                    ///< EMCAL channel map
  o2::emcal::TimeCalibrationParams* mTimeCalib;                ///< EMCAL time calib
};

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_EMCAL_DIGITSQCTASK_H
