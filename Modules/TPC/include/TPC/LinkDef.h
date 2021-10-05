#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class o2::quality_control_modules::tpc::PID+;
#pragma link C++ class o2::quality_control_modules::tpc::Tracking + ;
#pragma link C++ class o2::quality_control_modules::tpc::Tracks+;
#pragma link C++ class o2::quality_control_modules::tpc::PIDClusterCheck+;
#pragma link C++ class o2::quality_control_modules::tpc::TrackClusterCheck+;
#pragma link C++ class o2::quality_control_modules::tpc::ROCReductor+;
#pragma link C++ class o2::quality_control_modules::tpc::Clusters+;
#pragma link C++ class o2::quality_control_modules::tpc::CalDetPublisher+;
#pragma link C++ class o2::quality_control_modules::tpc::RawDigits+;
#pragma link C++ class o2::quality_control_modules::tpc::TrendingTaskTPC+;
#pragma link C++ class o2::quality_control_modules::tpc::TrendingTaskConfigTPC+;
#pragma link C++ class o2::quality_control_modules::tpc::ReductorTPC+;
#pragma link C++ class o2::quality_control_modules::tpc::TH1ReductorTPC+;
#pragma link C++ class o2::quality_control_modules::tpc::sliceInfo+;
#pragma link C++ class std::vector<o2::quality_control_modules::tpc::sliceInfo>+;
#pragma link C++ class o2::quality_control_modules::tpc::TH2ReductorTPC+;

#pragma link C++ function o2::quality_control_modules::tpc::addAndPublish+;
#pragma link C++ function o2::quality_control_modules::tpc::toVector+;
#pragma link C++ function o2::quality_control_modules::tpc::clusterHandler+;
#endif
