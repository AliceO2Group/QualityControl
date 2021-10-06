#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class o2::quality_control_modules::tpc::PID+;
#pragma link C++ class o2::quality_control_modules::tpc::Tracking+;
#pragma link C++ class o2::quality_control_modules::tpc::Tracks+;
#pragma link C++ class o2::quality_control_modules::tpc::PIDClusterCheck+;
#pragma link C++ class o2::quality_control_modules::tpc::TrackClusterCheck+;
#pragma link C++ class o2::quality_control_modules::tpc::ROCReductor+;
#pragma link C++ class o2::quality_control_modules::tpc::Clusters+;
#pragma link C++ class o2::quality_control_modules::tpc::CalDetPublisher+;
#pragma link C++ class o2::quality_control_modules::tpc::RawDigits+;
#pragma link C++ class o2::quality_control_modules::tpc::CheckOfTrendings+;
#pragma link C++ class o2::quality_control_modules::tpc::PadCalibrationCheck2D+;

#pragma link C++ function o2::quality_control_modules::tpc::addAndPublish + ;
#pragma link C++ function o2::quality_control_modules::tpc::toVector + ;
#pragma link C++ function o2::quality_control_modules::tpc::clusterHandler + ;
#endif
