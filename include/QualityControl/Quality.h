///
/// \file   Quality.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_LIBS_CORE_QUALITY_H_
#define QUALITYCONTROL_LIBS_CORE_QUALITY_H_

#include <string>
#include <ostream>
#include <TObject.h>

namespace AliceO2 {
namespace QualityControl {
namespace Core {

/// \brief  Class representing the quality of a MonitorObject.
///
/// \author Barthelemy von Haller
class Quality
{
  public:
    /// Default constructor
    Quality(unsigned int level=0, std::string name="");
    /// Destructor
    virtual ~Quality();

    unsigned int getLevel() const;
    const std::string& getName() const;

    static const Quality Null;
    static const Quality Good;
    static const Quality Medium;
    static const Quality Bad;

    friend bool operator==(const Quality& lhs, const Quality& rhs)
    {
      return (lhs.getName() == rhs.getName() && lhs.getLevel() == rhs.getLevel());
    }
    friend bool operator!=(const Quality& lhs, const Quality& rhs)
    {
      return !operator==(lhs, rhs);
    }
    friend std::ostream& operator<<(std::ostream &out, const Quality& q)     //output
    {
      out << "Quality: " << q.getName() << " (level " << q.getLevel() << ")\n";
      return out;
    }

  private:
    unsigned int mLevel; /// 0 is no quality, 1 is best quality, then it only goes downhill...
    std::string mName;

    ClassDef(Quality,1);
};

} // namespace Core
} // namespace QualityControl
} // namespace AliceO2

#endif // QUALITYCONTROL_LIBS_CORE_QUALITY_H_
