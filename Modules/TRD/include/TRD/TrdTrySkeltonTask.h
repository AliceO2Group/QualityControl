#ifndef QC_MODULE_TRD_TRDTRYSKELTONTASK_H
#define QC_MODULE_TRD_TRDTRYSKELTONTASK_H

#include "QualityControl/TaskInterface.h"
#include <memory>

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{

/// \brief Example Quality Control Task
/// \author My Name
class TrdTrySkeltonTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  TrdTrySkeltonTask() = default;
  /// Destructor
  ~TrdTrySkeltonTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  // std::unique_ptr<TH1F> histTracklet;  // number of tracklets per event
  std::unique_ptr<TH1F> histTrackletsTF;     // Tracklets per timeframe
  std::unique_ptr<TH1F> histTrackletsEvent;     // Tracklets per event
  std::unique_ptr<TH1F> histQ0;        // Q0 per tracklet
  std::unique_ptr<TH1F> histQ1;        // Q1 per tracklet
  std::unique_ptr<TH1F> histQ2;        // Q2 per tracklet
  std::unique_ptr<TH1F> histChamber;   // tracklets per chamber
  std::unique_ptr<TH1F> histPadRow;    // tracklets per padrow
  std::unique_ptr<TH1F> histMCM;       // tracklets per MCM (non-zero)
  std::unique_ptr<TH2F> histPadRowVsDet;
  std::unique_ptr<TH1F> histMCMOccupancy;

};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDTRYSKELTONTASK_H
