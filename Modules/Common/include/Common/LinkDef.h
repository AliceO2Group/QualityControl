#ifdef __CLING__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ class o2::quality_control_modules::common::NonEmpty + ;
#pragma link C++ class o2::quality_control_modules::common::MeanIsAbove + ;
#pragma link C++ class o2::quality_control_modules::common::TH1Ratio < TH1F> + ;
#pragma link C++ class o2::quality_control_modules::common::TH1Ratio < TH1D> + ;
#pragma link C++ class o2::quality_control_modules::common::TH2Ratio < TH2F> + ;
#pragma link C++ class o2::quality_control_modules::common::TH2Ratio < TH2D> + ;
#pragma link C++ class o2::quality_control_modules::common::TH1Reductor + ;
#pragma link C++ class o2::quality_control_modules::common::TH2Reductor + ;
#pragma link C++ class o2::quality_control_modules::common::THnSparse5Reductor + ;
#pragma link C++ class o2::quality_control_modules::common::QualityReductor + ;
#pragma link C++ class o2::quality_control_modules::common::EverIncreasingGraph + ;
#pragma link C++ class o2::quality_control_modules::common::QualityTask + ;
#pragma link C++ class o2::quality_control_modules::common::BigScreen + ;
#pragma link C++ class o2::quality_control_modules::common::CcdbInspectorTask + ;
#pragma link C++ class o2::quality_control_modules::common::CcdbInspectorCheck + ;
#pragma link C++ class o2::quality_control_modules::common::ObjectComparatorDeviation + ;
#pragma link C++ class o2::quality_control_modules::common::ObjectComparatorBinByBinDeviation + ;
#pragma link C++ class o2::quality_control_modules::common::ObjectComparatorChi2 + ;
#pragma link C++ class o2::quality_control_modules::common::ObjectComparatorKolmogorov + ;
#pragma link C++ class o2::quality_control_modules::common::ReferenceComparatorTask + ;
#pragma link C++ class o2::quality_control_modules::common::ReferenceComparatorCheck + ;
#pragma link C++ class o2::quality_control_modules::common::TrendCheck + ;
#pragma link C++ class o2::quality_control_modules::common::WorstOfAllAggregator + ;
#pragma link C++ class o2::quality_control_modules::common::IncreasingEntries + ;
#pragma link C++ class o2::quality_control_modules::common::TH1SliceReductor + ;
#pragma link C++ class o2::quality_control_modules::common::TH2SliceReductor + ;
#pragma link C++ class o2::quality_control_modules::common::LHCClockPhaseReductor + ;

#pragma link C++ function o2::quality_control_modules::common::getFromConfig + ;

#endif
