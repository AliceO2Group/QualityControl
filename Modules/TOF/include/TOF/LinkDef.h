#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Tasks
#pragma link C++ class o2::quality_control_modules::tof::TaskDigits + ;
#pragma link C++ class o2::quality_control_modules::tof::TaskCosmics + ;
#pragma link C++ class o2::quality_control_modules::tof::TaskRaw + ;
#pragma link C++ class o2::quality_control_modules::tof::TOFMatchedTracks + ;
// Checks
#pragma link C++ class o2::quality_control_modules::tof::CheckDiagnostics + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckDRMDiagnostics + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckCompressedData + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckRawMultiplicity + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckRawTime + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckRawToT + ;
// PostProcessing
#pragma link C++ class o2::quality_control_modules::tof::PostProcessDiagnosticPerCrate + ;
// Trending
#pragma link C++ class o2::quality_control_modules::tof::TrendingHits + ;
// Utilities
#endif
