#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ namespace o2::quality_control::core;
#pragma link C++ namespace o2::quality_control::checker;
#pragma link C++ namespace o2::quality_control::gui;

#pragma link C++ class o2::quality_control::core::MonitorObject+;
#pragma link C++ class o2::quality_control::core::Quality+;
#pragma link C++ class o2::quality_control::checker::CheckInterface+;
#pragma link C++ class o2::quality_control::core::CheckDefinition+;
#pragma link C++ class o2::quality_control::gui::SpyMainFrame+;
//#pragma link C++ class o2::quality_control::gui::SpyDevice+;

#pragma link C++ class std::pair<std::string, o2::quality_control::core::CheckDefinition>;
#pragma link C++ class std::map<std::string, o2::quality_control::core::CheckDefinition>;

#endif
