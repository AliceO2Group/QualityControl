///
/// \file   PedestalsTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H
#define QC_MODULE_MUONCHAMBERS_PEDESTALSTASK_H

#include "QualityControl/TaskInterface.h"
#include "MCH/Mapping.h"
#include "MCHMappingFactory/CreateSegmentation.h"
#include "MCH/Decoding.h"
#include "MCHBase/Digit.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

/// \brief Quality Control Task for the analysis of MCH pedestal data
/// \author Andrea Ferrero
/// \author Sebastien Perrin
class PedestalsTask /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
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
  void monitorDataReadout(o2::framework::ProcessingContext& ctx);
  void monitorDataDigits(const o2::framework::DataRef& input);
  void monitorData(o2::framework::ProcessingContext& ctx);
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  int count;
  Decoder mDecoder;
  uint64_t nhits[MCH_MAX_CRU_IN_FLP][24][40][64];
  double pedestal[MCH_MAX_CRU_IN_FLP][24][40][64];
  double noise[MCH_MAX_CRU_IN_FLP][24][40][64];

  //Matrices [de][padid], stated an upper value for de# and padid#

  uint64_t nhitsDigits[1100][1500];
  double pedestalDigits[1100][1500];
  double noiseDigits[1100][1500];

  MapCRU mMapCRU[MCH_MAX_CRU_IN_FLP];
  TH2F* mHistogramPedestals[MCH_MAX_CRU_IN_FLP * 24];
  TH2F* mHistogramNoise[MCH_MAX_CRU_IN_FLP * 24];

  std::vector<int> DEs;
  //MapFEC mMapFEC;
  std::map<int, TH2F*> mHistogramPedestalsDE;
  std::map<int, TH2F*> mHistogramNoiseDE;
  std::map<int, TH2F*> mHistogramPedestalsXY[2];
  std::map<int, TH2F*> mHistogramNoiseXY[2];

  std::map<int, TH1F*> mHistogramNoiseDistributionDE[5][2];

  int mPrintLevel;

  void fill_noise_distributions();
  void save_histograms();
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H
