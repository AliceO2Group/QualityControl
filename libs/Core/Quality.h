///
/// \file   Quality.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QUALITY_H_
#define QUALITYCONTROL_LIBS_CORE_QUALITY_H_

#include <string>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

class Quality
{
  public:
    Quality(unsigned int level, std::string name);
    virtual ~Quality();

    unsigned int getLevel() const;
    const std::string& getName() const;

    static const Quality Null;
    static const Quality Good;
    static const Quality Medium;
    static const Quality Bad;

    friend bool operator==(const Quality& lhs, const Quality& rhs);
    friend bool operator!=(const Quality& lhs, const Quality& rhs);

  private:
    unsigned int mLevel; /// 0 is no quality, 1 is best quality, then it only goes downhill...
    std::string mName;
};

bool operator==(const Quality& lhs, const Quality& rhs)
{
  return (lhs.getName() == rhs.getName() && lhs.getLevel() == rhs.getLevel());
}
bool operator!=(const Quality& lhs, const Quality& rhs)
{
  return !operator==(lhs, rhs);
}

} /* namespace Core */
} /* namespace QualityControl */
} /* namespace AliceO2 */

#endif /* QUALITYCONTROL_LIBS_CORE_QUALITY_H_ */
