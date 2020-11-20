#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Tasks
#pragma link C++ class o2::quality_control_modules::tof::TaskDigits+;
#pragma link C++ class o2::quality_control_modules::tof::TaskRaw+;
// Checks
#pragma link C++ class o2::quality_control_modules::tof::CheckDiagnostics+;
#pragma link C++ class o2::quality_control_modules::tof::CheckCompressedData+;
#pragma link C++ class o2::quality_control_modules::tof::CheckRawMultiplicity+;
#pragma link C++ class o2::quality_control_modules::tof::CheckRawTime+;
#pragma link C++ class o2::quality_control_modules::tof::CheckRawToT+;
// PostProcessing
#pragma link C++ class o2::quality_control_modules::tof::PostProcessDiagnosticPerCrate+;
// Utilities
// #pragma link C++ class o2::quality_control_modules::tof::Counter<const unsigned int, const char*>+;
#endif
