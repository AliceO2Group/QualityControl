///
/// \file   DigitsQcTask.h
/// \author Markus Fasel, Cristina Terrevoli
///

#ifndef QC_MODULE_EMCAL_DIGITSQCTASK_H
#define QC_MODULE_EMCAL_DIGITSQCTASK_H

#include "QualityControl/TaskInterface.h"
#include <array>
#include <TProfile2D.h>

class TH1;
class TH2;

using namespace o2::quality_control::core;

namespace o2
{
namespace emcal
{
class Geometry;
}

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

 private:
  std::array<TH2*, 2> mDigitAmplitude;        ///< Digit amplitude
  std::array<TH2*, 2> mDigitTime;             ///< Digit time
  TH2* mDigitOccupancy = nullptr;             ///< Digit occupancy EMCAL and DCAL
  TProfile2D* mIntegratedOccupancy = nullptr; ///< Digit integrated occupancy
  TH1* mDigitAmplitudeEMCAL = nullptr;        ///< Digit amplitude in EMCAL
  TH1* mDigitAmplitudeDCAL = nullptr;         ///< Digit amplitude in DCAL
  o2::emcal::Geometry* mGeometry = nullptr;   ///< EMCAL geometry
};

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_EMCAL_DIGITSQCTASK_H
