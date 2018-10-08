///
/// \file   DaqTask.h
/// \author Barthelemy von Haller
///

#ifndef QC_MODULE_DAQ_DAQTASK_H
#define QC_MODULE_DAQ_DAQTASK_H

#include <TCanvas.h>
#include <TPaveText.h>
#include "QualityControl/TaskInterface.h"

class TH1F;
class TGraph;

using namespace o2::quality_control::core;

namespace o2 {
namespace quality_control_modules {
namespace daq {

/// \brief Example Quality Control Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
class DaqTask /*final*/: public TaskInterface // todo add back the "final" when doxygen is fixed
{
  public:
    /// \brief Constructor
    DaqTask();
    /// Destructor
    ~DaqTask() override;

    // Definition of the methods for the template method pattern
    void initialize(o2::framework::InitContext& ctx) override;
    void startOfActivity(Activity &activity) override;
    void startOfCycle() override;
    void monitorData(o2::framework::ProcessingContext& ctx) override;
    void endOfCycle() override;
    void endOfActivity(Activity &activity) override;
    void reset() override;

  private:

    TH1F *mPayloadSize;
    TGraph *mIds;
    int mNPoints;
    TH1F *mNumberSubblocks;
    TH1F *mSubPayloadSize;
    UInt_t mTimeLastRecord;
    TObjString *mObjString;
    TCanvas *mCanvas;
    TPaveText *mPaveText;
};

} // namespace daq
} // namespace quality_control_modules
} // namespace o2

#endif //QC_MODULE_DAQ_DAQTASK_H
