#ifndef __O2_QC_FT0_UTILITIES_H__
#define __O2_QC_FT0_UTILITIES_H__

#include <vector>
#include "DataFormatsFT0/Digit.h"
#include "DataFormatsFT0/ChannelData.h"
#include "Rtypes.h"
#include "TObject.h"

namespace o2::quality_control_modules::ft0
{



struct DummyClassMF
{
    DummyClassMF() = default;
    DummyClassMF(int aa, int bb){ a = aa; b = bb; }

    int a = 0;
    int b = 0;

    void someFunction() const;
    ClassDefNV(DummyClassMF, 2);
};


struct EventWithChannelData
{

    EventWithChannelData() = default;
    EventWithChannelData(int pEventID, uint16_t pBC, uint32_t pOrbit, double pTimestamp, const std::vector<o2::ft0::ChannelData>& pChannels)
        : eventID(pEventID), bc(pBC), orbit(pOrbit), timestampNS(pTimestamp), channels(pChannels){} 

    int eventID = -1;
    uint16_t bc = o2::InteractionRecord::DummyBC;
    uint32_t orbit = o2::InteractionRecord::DummyOrbit;
    double timestampNS = 0; 
    std::vector<o2::ft0::ChannelData> channels;

    int getEventID() const {return eventID; }
    uint16_t getBC() const { return bc; }
    uint32_t getOrbit() const { return orbit; }
    double getTimestamp() const { return timestampNS; }
    const std::vector<o2::ft0::ChannelData>& getChannels() const { return channels; }

    ClassDefNV(EventWithChannelData, 2);
};


}




#endif