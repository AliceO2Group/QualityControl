#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class o2::quality_control_modules::fv0::DigitQcTask + ;
#pragma link C++ class o2::quality_control_modules::fv0::PostProcTask + ;
#pragma link C++ class o2::quality_control_modules::fv0::CFDEffCheck + ;
#pragma link C++ class o2::quality_control_modules::fv0::OutOfBunchCollCheck + ;
#pragma link C++ class o2::quality_control_modules::fv0::GenericCheck + ;
#pragma link C++ class o2::quality_control_modules::fv0::TriggersSwVsTcmCheck + ;
#pragma link C++ class o2::quality_control_modules::fv0::OutOfBunchCollFeeModulesCheck + ;
//#pragma link C++ class o2::quality_control_modules::fv0::CalibrationTask + ;
//#pragma link C++ class o2::quality_control_modules::fv0::ChannelTimeCalibrationCheck + ;

#pragma link C++ class o2::quality_control_modules::fv0::DigitQcTaskLaser + ;
#pragma link C++ class o2::quality_control_modules::fv0::TH1ReductorLaser + ;
#endif
