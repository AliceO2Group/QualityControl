///
/// \file   Publisher.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_PUBLISHER_PUBLISHER_H_
#define QUALITYCONTROL_LIBS_PUBLISHER_PUBLISHER_H_

#include <string>

namespace AliceO2 {
namespace QualityControl {
namespace Publisher {

/// \brief   Here you put a short description of the class
///
/// \author   Barthelemy von Haller
class Publisher
{
  public:
    /// Default constructor
    Publisher();
    /// Destructor
    virtual ~Publisher();

    void startPublishing(std::string name, void* object); // TODO what type here ?
    void setQuality(std::string name/*, Quality*/);
};

} /* namespace Publisher */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_PUBLISHER_PUBLISHER_H_ */
