#ifndef QC_MODULE_FT0_FT0ChannelsCheck_H
#define QC_MODULE_FT0_FT0ChannelsCheck_H

// Quality Control
#include "QualityControl/CheckInterface.h"

namespace o2::quality_control_modules::ft0
{

/// \brief  
///
class ChannelsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  ChannelsCheck() = default;
  /// Destructor
  ~ChannelsCheck() override = default;

  // Override interface
  void configure(std::string name) override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(ChannelsCheck, 1);
};

} // namespace o2::quality_control_modules::ft0

#endif
