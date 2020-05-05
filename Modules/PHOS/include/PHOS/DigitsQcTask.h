///
/// \file   DigitsQcTask.h
/// \author Dmitri Peresunko
/// \author Dmitry Blau
///

#ifndef QC_MODULE_PHOS_DIGITSQCTASK_H
#define QC_MODULE_PHOS_DIGITSQCTASK_H

#include "QualityControl/TaskInterface.h"

class TH1F;
class TH2F;
class TH2C;

using namespace o2::quality_control::core;

namespace o2
{
namespace phos
{
class Geometry;
class Digit;
} // namespace phos

namespace quality_control_modules
{
namespace phos
{

/// \brief QC Task for PHOS digits
///
/// The main monitoring component for PHOS digits (energy and time measurement in cell).
/// Monitoring observables:
/// - Digit amplitude
/// - Digit time
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

 protected:
  void publishPhysicsObjects();
  void publishPedestalObjects();
  void publishLEDObjects();
  void processPhysicsEvent(gsl::span<const o2::phos::Digit> event);
  void getBadMap();

 private:
  static const int nMod = 5;
  float mAcceptanceCorrection[nMod];

  //Physics runs
  // TH2F *mTRGoccupancy[nMod];
  // TH2F *mTRGsignal[nMod];

  // TH2F *mL1occupancy[nMod];
  // TH2F *mL1signal[nMod];

  // TH2F *mFakeTrig[nMod]; //Triggers fired without cluster
  TH2C* mBadMap[nMod] = { nullptr };
  TH2F* mTimeE[nMod] = { nullptr };
  TH1F* mCellN[nMod] = { nullptr };
  TH1F* mCellMeanEnergy[nMod] = { nullptr };
  TH2F* mCellN2D[nMod] = { nullptr };
  TH2F* mCellEmean2D[nMod] = { nullptr };
  TH1F* mCellSp[nMod] = { nullptr };

  // TH1F *mCellSpTrig[nMod];
  // TH1F *mCellSpFake[nMod];

  o2::phos::Geometry* mGeometry = nullptr; ///< PHOS geometry
};

} // namespace phos
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_PHOS_DIGITSQCTASK_H
