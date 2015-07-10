//
// Created by flpprotodev on 7/10/15.
//

#ifndef QUALITY_CONTROL_BASICTASK_H
#define QUALITY_CONTROL_BASICTASK_H

#include "QCTask.h"

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class BasicTask : public QCTask
{
  public:
    BasicTask();
    virtual ~BasicTask();

    // Definition of the methods for the template method pattern
    virtual void initialize();
    virtual void startOfActivity(Activity &activity);
    virtual void startOfCycle();
    virtual void monitorDataBlock(DataBlock &block);
    virtual void endOfCycle();
    virtual void endOfActivity(Activity &activity);
    virtual void Reset();

  private:


};

}
}
}

#endif //QUALITY_CONTROL_BASICTASK_H
