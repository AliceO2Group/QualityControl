#include "FT0/Utilities.h"


using namespace o2::quality_control_modules::ft0;

// ClassImp(DummyClassMF)
// ClassImp(EventWithChannelData)

std::ostream& operator<<(std::ostream& ostream, const DummyClassMF& cl)
{
    ostream << " OK ";
    return ostream;
}

std::ostream& operator<<(std::ostream& ostream, const EventWithChannelData& cl)
{
    ostream << " OK ";
    return ostream;
}

void DummyClassMF::someFunction() const
{
    int a;
    (void)a;
}