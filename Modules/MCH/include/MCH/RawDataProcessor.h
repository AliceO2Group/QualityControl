///
/// \file   RawDataProcessor.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H
#define QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H

#include "QualityControl/TaskInterface.h"
#include "MCH/MuonChambersMapping.h"
#include "MCH/MuonChambersDataDecoder.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{


/// \brief Example Quality Control DPL Task
/// It is final because there is no reason to derive from it. Just remove it if needed.
/// \author Barthelemy von Haller
/// \author Piotr Konopka
class RawDataProcessor /*final*/ : public TaskInterface // todo add back the "final" when doxygen is fixed
{
 public:
  /// \brief Constructor
  RawDataProcessor();
  /// Destructor
  ~RawDataProcessor() override;

  // Definition of the methods for the template method pattern
  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(Activity& activity) override;
  void reset() override;

 private:
  int count;
  MuonChambersDataDecoder mDecoder;
  uint64_t nhits[24][40][64];
  double pedestal[24][40][64];
  double noise[24][40][64];
  MapCRU mMapCRU[MCH_MAX_CRU_IN_FLP];
  TH1F* mHistogram;
  TH2F* mHistogramPedestals[24];
  TH2F* mHistogramNoise[24];
  TH1F* mHistogramPedestalsDS[24][8];
  TH1F* mHistogramNoiseDS[24][8];

  std::map<int, TH2F*> mHistogramPedestalsDE;
  std::map<int, TH2F*> mHistogramNoiseDE;
  std::map<int, TH2F*> mHistogramPedestalsXY[2];
  std::map<int, TH2F*> mHistogramNoiseXY[2];
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H
