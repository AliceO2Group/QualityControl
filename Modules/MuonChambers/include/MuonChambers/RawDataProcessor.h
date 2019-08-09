///
/// \file   RawDataProcessor.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
///

#ifndef QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H
#define QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H

#include "QualityControl/TaskInterface.h"
#include "MuonChambers/sampa_header.h"

class TH1F;
class TH2F;

using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{



enum DualSampaStatus {
    notSynchronized              = 1,
    synchronized                 = 2,
    headerToRead                 = 3,
    sizeToRead                   = 4,
    timeToRead                   = 5,
    dataToRead                   = 6,
    chargeToRead                 = 7,
    OK                           = 8   // Data block filled (over a time window)
};


struct DualSampa
{
  int id;
  DualSampaStatus status;            // Status during the data filling
  uint64_t data;                // curent data
  int bit;                           // current position
  uint64_t powerMultiplier;     // power to convert to move bits
  int nsyn2Bits;                     // Nb of words waiting synchronization
  Sampa::SampaHeaderStruct header;   // current channel header
  int64_t bxc[2];
  uint32_t csize, ctime, cid, sample;
  int chan_addr[2];
  uint64_t packetsize;
  int nbHit; // incremented each time a header packet is received for this card
  int nbHitChan[64]; // incremented each time a header packet for a given packet is received for this card
  int ndata[2][32];
  int nclus[2][32];
  double pedestal[2][32], noise[2][32];
};


struct DualSampaGroup
{
  int64_t bxc;
};


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
  int hb_orbit;
  DualSampa ds[24][40];
  DualSampaGroup dsg[24][8];
  TH1F* mHistogram;
  TH2F* mHistogramPedestals[24];
  TH2F* mHistogramNoise[24];
  TH1F* mHistogramPedestalsDS[24][8];
  TH1F* mHistogramNoiseDS[24][8];
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_RAWDATAPROCESSOR_H
