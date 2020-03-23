#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Tasks
#pragma link C++ class o2::quality_control_modules::tof::TOFTask+;
#pragma link C++ class o2::quality_control_modules::tof::TOFTaskCompressed+;
#pragma link C++ class o2::quality_control_modules::tof::TaskDiagnostics+;
// Checks
#pragma link C++ class o2::quality_control_modules::tof::TOFCheckCompressedCounter+;
#pragma link C++ class o2::quality_control_modules::tof::TOFCheckDiagnostic+;
#pragma link C++ class o2::quality_control_modules::tof::TOFCheckRawsMulti+;
#pragma link C++ class o2::quality_control_modules::tof::TOFCheckRawsTime+;
#pragma link C++ class o2::quality_control_modules::tof::TOFCheckRawsToT+;
// Utilities
#pragma link C++ class o2::quality_control_modules::tof::Diagnostics+;
#pragma link C++ class o2::quality_control_modules::tof::TOFDecoderCompressed+;
#endif
