#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ namespace o2::quality_control::core;

#pragma link C++ class o2::quality_control::core::MonitorObject + ;
#pragma link C++ class o2::quality_control::core::QualityObject + ;
#pragma link C++ class o2::quality_control::core::Quality + ;

#pragma read                                              \
    sourceClass="o2::quality_control::core::QualityObject"                                  \
    source="std::map<std::string, std::string> mUserMetadata"           \
    version="[1-2]"                               \
    targetClass="o2::quality_control::core::QualityObject"                                  \
    embed="true"                                          \
    include="iostream,cstdlib"                            \
    code="{mQuality.addMetadata(onfile.mUserMetadata);}" \

#endif
