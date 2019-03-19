///
/// \file   Dummy.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_DUMMY_DUMMY_H
#define QC_MODULE_DUMMY_DUMMY_H

#include "QualityControl/TaskInterface.h"
#include "Framework/DataProcessorSpec.h"
//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/common/reconstruction/include/ITSMFTReconstruction/DigitPixelReader.h"
//#include "/data/zhaozhong/alice/O2/Detectors/ITSMFT/ITS/QCWorkFlow/include/ITSQCWorkflow/HisAnalyzerSpec.h"
//#include "ITSMFTReconstruction/DigitPixelReader.h"

class TH1F;

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace dummy
{

/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class Dummy /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  Dummy();
  /// Destructor
  ~Dummy() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;

  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;
  void init(o2::framework::InitContext& ctx);
  void run(o2::framework::ProcessingContext& ctx);

 private:
  TH1F* mHistogram;
  framework::DataProcessorSpec getDummySpec();
	//std::unique_ptr<o2::ITSMFT::DigitPixelReader> mReaderMC;    
  //o2::ITSMFT::DigitPixelReader reader;

};


} // namespace dummy
} // namespace quality_control_modules
} // namespace o2


#endif // QC_MODULE_DUMMY_DUMMY_H

