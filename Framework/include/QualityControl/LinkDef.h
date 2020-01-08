#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ namespace o2::quality_control::core;
#pragma link C++ namespace o2::quality_control::checker;
#pragma link C++ namespace o2::quality_control::postprocessing;

#pragma link C++ class o2::quality_control::checker::CheckInterface+;
#pragma link C++ class o2::quality_control::core::TaskInterface+;
#pragma link C++ class o2::quality_control::postprocessing::PostProcessingInterface+;
#pragma link C++ class o2::quality_control::postprocessing::TrendingTask+;

#endif
