#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class o2::quality_control_modules::muonchambers::TH2ElecMapReductor + ;
// tasks
#pragma link C++ class o2::quality_control_modules::muonchambers::PedestalsTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::DigitsTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::PreclustersTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::RofsTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::DecodingTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::TracksTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::ErrorTask + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::ClustersTask + ;
// Post-processing
#pragma link C++ class o2::quality_control_modules::muonchambers::DecodingPostProcessing + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::DigitsPostProcessing + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::PreclustersPostProcessing + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::QualityPostProcessing + ;
// Trending
#pragma link C++ class o2::quality_control_modules::muonchambers::TrendingTracks + ;
// Checks
#pragma link C++ class o2::quality_control_modules::muonchambers::PedestalsCheck + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::DecodingCheck + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::DigitsCheck + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::PreclustersCheck + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::QualityCheck + ;
// Aggregators
#pragma link C++ class o2::quality_control_modules::muonchambers::MCHAggregator + ;
// legacy tasks
#pragma link C++ class o2::quality_control_modules::muonchambers::PhysicsTaskDigits + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::PhysicsTaskRofs + ;
#pragma link C++ class o2::quality_control_modules::muonchambers::PhysicsTaskPreclusters + ;

#endif
