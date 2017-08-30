///
/// \file   Consumer.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_CLIENT_CONSUMER_H_
#define QUALITYCONTROL_CLIENT_CONSUMER_H_

// QC
#include "QualityControl/ClientDataProvider.h"

using namespace o2::quality_control;

namespace o2 {
namespace quality_control {
namespace client {

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
} /* namespace o2 */

#endif /* QUALITYCONTROL_CLIENT_CONSUMER_H_ */
