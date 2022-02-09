///
/// \file   DigitsCheck.h
/// \author My Name
///

#ifndef QC_MODULE_TRD_DIGITSCHECK_H
#define QC_MODULE_TRD_DIGITSCHECK_H

#include "QualityControl/CheckInterface.h"
#include <TFile.h>

namespace o2::quality_control_modules::trd
{

/// \brief  TRD Check
/// \author My Name
class DigitsCheck : public o2::quality_control::checker::CheckInterface
{
 public:
  /// Default constructor
  DigitsCheck() = default;
  /// Destructor
  ~DigitsCheck() override = default;

  // Override interface
  void configure() override;
  Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) override;
  void beautify(std::shared_ptr<MonitorObject> mo, Quality checkResult = Quality::Null) override;
  std::string getAcceptedType() override;

  ClassDefOverride(DigitsCheck, 1);
};

} // namespace o2::quality_control_modules::trd

#endif // QC_MODULE_TRD_DIGITSCHECK_H
