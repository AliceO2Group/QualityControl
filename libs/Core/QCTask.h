///
/// \file   QCTask.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QCTASK_H_
#define QUALITYCONTROL_LIBS_CORE_QCTASK_H_

#include "DataBlock.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

///
/// Dummy class that should be removed when there is the official one
/// \author   Barthelemy von Haller
class Activity
{
  public:
    /// Default constructor
    Activity();
    /// Destructor
    virtual ~Activity();
};

/// \brief   Here you put a short description of the class
///
/// \author   Barthelemy von Haller
class QCTask
{
  public:
    /// Default constructor
    QCTask();
    /// Destructor
    virtual ~QCTask();

    // Definition of the methods for the template method pattern
    virtual void initialize() = 0;
    virtual void startOfActivity(Activity &activity) = 0;
    virtual void startOfCycle() = 0;
    virtual void monitorDataBlock(DataBlock &block) = 0;
    virtual void endOfCycle() = 0;
    virtual void endOfActivity(Activity &activity) = 0;
    virtual void Reset() = 0;

};

} /* namespace Core */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CORE_QCTASK_H_ */
