///
/// \file   NonEmpty.h
/// \author flpprotodev
///

#ifndef QUALITYCONTROL_LIBS_CHECKER_NONEMPTY_H_
#define QUALITYCONTROL_LIBS_CHECKER_NONEMPTY_H_

#include "QualityControl/CheckInterface.h"

namespace AliceO2 {
namespace QualityControl {
namespace Checker {

class NonEmpty /*final*/: public CheckInterface
{
  public:
    NonEmpty();
    virtual ~NonEmpty();

    Quality check(const MonitorObject *mo) override;
    void beautify(MonitorObject *mo, Quality checkResult = Quality::Null) override;
    std::string getAcceptedType() override;
};

} // namespace Checker 
} // namespace QualityControl 
} // namespace AliceO2 

#endif // QUALITYCONTROL_LIBS_CHECKER_NONEMPTY_H_ 
