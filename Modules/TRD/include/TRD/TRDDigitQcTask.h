
/// \file   TRDDigitQcTask.h
/// \author Tokozani Mtetwa
///

#ifndef QC_MODULE_TRD_TRDDIGITQCTASK_H
#define QC_MODULE_TRD_TRDDIGITQCTASK_H

#include "QualityControl/TaskInterface.h"
#include "DataFormatsTRD/RawData.h"
#include <cstdint>
#include <array>
#include <fstream>
#include <string>

class TH1F;
class TH2F;
class TProfile;


using namespace o2::quality_control::core;

namespace o2::quality_control_modules::trd
{
  class TRDDigitQcTask final : public TaskInterface
  {
  public:
    /// \brief Constructor
    TRDDigitQcTask() = default;
    /// Destructor
    ~TRDDigitQcTask() override;

    // Definition of the methods for the template method pattern
    void initialize(o2::framework::InitContext& ctx) override;
    void startOfActivity(Activity& activity) override;
    void startOfCycle() override;
    void monitorData(o2::framework::ProcessingContext& ctx) override;
    void endOfCycle() override;
    void endOfActivity(Activity& activity) override;
    void reset() override;

  private:

    TH1F* mADC = nullptr;
    TH2F* mADCperTimeBinAllDetectors = nullptr;
    TProfile* mprofADCperTimeBinAllDetectors;
  };
} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_TRDDIGITQCTASK_H
