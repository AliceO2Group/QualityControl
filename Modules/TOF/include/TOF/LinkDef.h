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
#pragma link C++ class o2::quality_control_modules::tof::CheckHitMap + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckNoise + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckSlotPartMask + ;
#pragma link C++ class o2::quality_control_modules::tof::CheckLostOrbits + ;
// PostProcessing
#pragma link C++ class o2::quality_control_modules::tof::PostProcessDiagnosticPerCrate + ;
#pragma link C++ class o2::quality_control_modules::tof::PostProcessHitMap + ;
#pragma link C++ class o2::quality_control_modules::tof::PostProcessingLostOrbits + ;
// Trending
#pragma link C++ class o2::quality_control_modules::tof::TrendingHits + ;
#pragma link C++ class o2::quality_control_modules::tof::TrendingRate + ;
#pragma link C++ class o2::quality_control_modules::tof::TrendingCalibLHCphase + ;
// Reductors
#pragma link C++ class o2::quality_control_modules::tof::TH1ReductorTOF + ;
// Utilities
#endif
