///
/// \file   DigitsQcTask.h
/// \author Markus Fasel
///

#ifndef QC_MODULE_EMCAL_DIGITSQCTASK_H
#define QC_MODULE_EMCAL_DIGITSQCTASK_H

#include "QualityControl/TaskInterface.h"

class TH2F;

using namespace o2::quality_control::core;

namespace o2
{
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
class DigitsQcTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
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
  TH2F* mDigitAmplitude; ///< Digit amplitude
  TH2F* mDigitTime;      ///< Digit time
};

} // namespace emcal
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_EMCAL_DIGITSQCTASK_H
