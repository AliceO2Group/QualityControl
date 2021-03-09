///
/// \file   PedestalsTask.h
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H
#define QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H

#include "QualityControl/TaskInterface.h"
#include "MCHRawElecMap/Mapper.h"
#include "MCH/GlobalHistogram.h"
#include "Framework/DataRef.h"
#include "MCHCalibration/MCHChannelCalibrator.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2::quality_control_modules::muonchambers
{

/// \brief Quality Control Task for the analysis of MCH pedestal data
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class PedestalsTask final : public TaskInterface
{
 public:
  /// \brief Constructor
  PedestalsTask();
  /// Destructor
  ~PedestalsTask() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  o2::mch::raw::Solar2FeeLinkMapper mSolar2FeeLinkMapper;
  o2::mch::raw::Elec2DetMapper mElec2DetMapper;

  /// helper class that performs the actual computation of the pedestals from the input digits
  o2::mch::calibration::PedestalProcessor mPedestalProcessor;

  //Matrices [de][padid], stated an upper value for de# and padid#

  //uint64_t nhitsDigits[1100][1500];
  //double pedestalDigits[1100][1500];
  //double noiseDigits[1100][1500];

  TH2F* mHistogramPedestals;
  TH2F* mHistogramNoise;

  //std::vector<int> DEs;
  //MapFEC mMapFEC;
  std::map<int, TH2F*> mHistogramPedestalsDE;
  std::map<int, TH2F*> mHistogramNoiseDE;
  std::map<int, TH2F*> mHistogramPedestalsXY[2];
  std::map<int, TH2F*> mHistogramNoiseXY[2];

  std::map<int, TH1F*> mHistogramNoiseDistributionDE[5][2];

  GlobalHistogram* mHistogramPedestalsMCH;
  GlobalHistogram* mHistogramNoiseMCH;

  int mPrintLevel;

  void monitorDataDigits(o2::framework::ProcessingContext& ctx);
  void monitorDataPedestals(o2::framework::ProcessingContext& ctx);

  void PlotPedestal(uint16_t solarID, uint8_t dsID, uint8_t channel, double mean, double rms);
  void PlotPedestalDE(uint16_t solarID, uint8_t dsID, uint8_t channel, double mean, double rms);
  void fill_noise_distributions();
  void save_histograms();
};

} // namespace o2::quality_control_modules::muonchambers

#endif // QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H
