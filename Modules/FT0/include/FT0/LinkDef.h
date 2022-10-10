#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class o2::quality_control_modules::ft0::EventWithChannelData + ;

#pragma link C++ class o2::quality_control_modules::ft0::BasicDigitQcTask + ;
#pragma link C++ class o2::quality_control_modules::ft0::DigitQcTask + ;
#pragma link C++ class o2::quality_control_modules::ft0::DigitsCheck + ;
#pragma link C++ class o2::quality_control_modules::ft0::ChannelsCheck + ;
#pragma link C++ class o2::quality_control_modules::ft0::MergedTreeCheck + ;
#pragma link C++ class o2::quality_control_modules::ft0::TreeReaderPostProcessing + ;
//#pragma link C++ class o2::quality_control_modules::ft0::CalibrationTask + ;
//#pragma link C++ class o2::quality_control_modules::ft0::ChannelTimeCalibrationCheck + ;
#pragma link C++ class o2::quality_control_modules::ft0::PostProcTask + ;
#pragma link C++ class o2::quality_control_modules::ft0::GenericCheck+;
#pragma link C++ class o2::quality_control_modules::ft0::CFDEffCheck + ;
#pragma link C++ class o2::quality_control_modules::ft0::OutOfBunchCollCheck + ;
#pragma link C++ class o2::quality_control_modules::ft0::RecPointsQcTask + ;

#pragma link C++ class o2::quality_control_modules::ft0::DigitQcTaskLaser + ;
#pragma link C++ class o2::quality_control_modules::ft0::TH1ReductorLaser + ;
#endif
