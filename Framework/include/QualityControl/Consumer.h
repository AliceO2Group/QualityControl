///
/// \file   Consumer.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CLIENT_CONSUMER_H_
#define QUALITYCONTROL_CLIENT_CONSUMER_H_

// QC
#include "QualityControl/ClientDataProvider.h"

using namespace AliceO2::QualityControl;

namespace AliceO2 {
namespace QualityControl {
namespace Client {

/// \brief A special client that polls and consumes all available data.
/// It is used to stress the system for benchmarks.
///
/// \author Barthélémy von Haller
class Consumer
{
  public:
    /// Default constructor
    Consumer();
    /// Destructor
    virtual ~Consumer();

    void consume();
    void print();

  private:
    ClientDataProvider mDataProvider;
    unsigned int mNumberCycles, mNumberObjects, mNumberTasks;
};

} /* namespace Client */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_CLIENT_CONSUMER_H_ */
