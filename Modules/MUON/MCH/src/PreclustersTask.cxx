// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PreclustersTask.cxx
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
/// \author Sebastien Perrin
///

#include "MCH/PreclustersTask.h"
#include "MCH/Helpers.h"
#ifdef MCH_HAS_MAPPING_FACTORY
#include "MCHMappingFactory/CreateSegmentation.h"
#endif
#include "MCHGlobalMapping/DsIndex.h"
#include "MCHMappingInterface/Segmentation.h"
#include "MCHMappingInterface/CathodeSegmentation.h"
// #include "MCHMappingSegContour/CathodeSegmentationContours.h"
#include <Framework/InputRecord.h>

using namespace std;
using namespace o2::mch;
using namespace o2::mch::raw;
using namespace o2::quality_control::core;

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

void PreclustersTask::initialize(o2::framework::InitContext& /*ctx*/)
{
  ILOG(Info, Devel) << "initialize PreclustersTask" << AliceO2::InfoLogger::InfoLogger::endm;

  mIsSignalDigit = o2::mch::createDigitFilter(20, true, true);

  mDet2ElecMapper = createDet2ElecMapper<ElectronicMapperGenerated>();
  mSolar2FeeLinkMapper = createSolar2FeeLinkMapper<ElectronicMapperGenerated>();

  mHistogramPreclustersPerDE = std::make_unique<MergeableTH1Ratio>("PreclustersPerDE", "Number of pre-clusters for each DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramPreclustersPerDE.get(), "hist", false);
  mHistogramPreclustersSignalPerDE = std::make_unique<MergeableTH1Ratio>("PreclustersSignalPerDE", "Number of pre-clusters (with signal) for each DE", getNumDE(), 0, getNumDE());
  publishObject(mHistogramPreclustersSignalPerDE.get(), "hist", false);

  const uint32_t nElecXbins = NumberOfDualSampas;

  // Histograms in electronics coordinates
  mHistogramPseudoeffElec = std::make_unique<MergeableTH2Ratio>("Pseudoeff_Elec", "Pseudoeff", nElecXbins, 0, nElecXbins, 64, 0, 64, true);
  publishObject(mHistogramPseudoeffElec.get(), "colz", false);

  // 256 bins, 50 ADC / bin
  mHistogramClusterCharge = std::make_unique<TH2F>("ClusterChargeHist", "Cluster Charge", getNumDE(), 0, getNumDE(), 256, 0, 256 * 50);
  publishObject(mHistogramClusterCharge.get(), "colz", false);

  mHistogramClusterSize = std::make_unique<TH2F>("ClusterSizeHist", "Cluster Size", getNumDE() * 3, 0, getNumDE() * 3, 100, 0, 100);
  publishObject(mHistogramClusterSize.get(), "colz", false);
}

//_________________________________________________________________________________________________

void PreclustersTask::startOfActivity(const Activity& /*activity*/)
{
  ILOG(Info, Devel) << "startOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

//_________________________________________________________________________________________________

void PreclustersTask::startOfCycle()
{
  ILOG(Info, Devel) << "startOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;
}

//_________________________________________________________________________________________________

static void updateTFcount(TH1D* hDen)
{
  for (int bin = 1; bin <= hDen->GetXaxis()->GetNbins(); bin++) {
    auto x = hDen->GetXaxis()->GetBinCenter(bin);
    hDen->Fill(x);
  }
}

//_________________________________________________________________________________________________

void PreclustersTask::monitorData(o2::framework::ProcessingContext& ctx)
{
  // get the input preclusters and associated digits
  auto preClusters = ctx.inputs().get<gsl::span<o2::mch::PreCluster>>("preclusters");
  auto digits = ctx.inputs().get<gsl::span<o2::mch::Digit>>("preclusterdigits");

  ILOG(Info, Devel) << fmt::format("Received {} pre-clusters and {} digits", preClusters.size(), digits.size()) << AliceO2::InfoLogger::InfoLogger::endm;

  updateTFcount(mHistogramPreclustersPerDE->getDen());
  updateTFcount(mHistogramPreclustersSignalPerDE->getDen());

  for (auto& p : preClusters) {
    plotPrecluster(p, digits);
  }
}

//_________________________________________________________________________________________________

// compute the center-of-gravity of a given pre-cluster
static void CoG(gsl::span<const o2::mch::Digit> precluster, double& Xcog, double& Ycog, bool isWide[2])
{

  double xmin = 1E9;
  double ymin = 1E9;
  double xmax = -1E9;
  double ymax = -1E9;
  double charge[] = { 0.0, 0.0 };
  int multiplicity[] = { 0, 0 };
  isWide[0] = isWide[1] = false;
  // isWide tells if a given precluster is extended enough on a given cathode. On the bending side for exemple, a wide precluster would have at least 2 pads fired in the x direction (so when clustering it, we obtain a meaningful value for x). If a precluster is not wide and on a single cathode, when clustering, one of the coordinates will not be computed properly and set to the center of the pad by default.

  double x[] = { 0.0, 0.0 };
  double y[] = { 0.0, 0.0 };

  double xsize[] = { 0.0, 0.0 };
  double ysize[] = { 0.0, 0.0 };

  std::set<double> padPos[2];

  int deId = precluster[0].getDetID();
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);

  for (ssize_t i = 0; (unsigned)i < precluster.size(); ++i) {
    const o2::mch::Digit& digit = precluster[i];
    int padid = digit.getPadID();

    // position and size of current pad
    double padPosition[2] = { segment.padPositionX(padid), segment.padPositionY(padid) };
    double padSize[2] = { segment.padSizeX(padid), segment.padSizeY(padid) };

    // update of xmin/max et ymin/max
    xmin = std::min(padPosition[0] - 0.5 * padSize[0], xmin);
    xmax = std::max(padPosition[0] + 0.5 * padSize[0], xmax);
    ymin = std::min(padPosition[1] - 0.5 * padSize[1], ymin);
    ymax = std::max(padPosition[1] + 0.5 * padSize[1], ymax);

    // cathode index
    int cathode = segment.isBendingPad(padid) ? 0 : 1;

    // update of the cluster position, size, charge and multiplicity
    x[cathode] += padPosition[0] * digit.getADC();
    y[cathode] += padPosition[1] * digit.getADC();
    xsize[cathode] += padSize[0];
    ysize[cathode] += padSize[1];
    charge[cathode] += digit.getADC();

    if (cathode == 0) {
      padPos[0].insert(padPosition[1]);
    } else if (cathode == 1) {
      padPos[1].insert(padPosition[0]);
    }

    multiplicity[cathode] += 1;
  }

  // Computation of the CoG coordinates for the two cathodes
  for (int cathode = 0; cathode < 2; ++cathode) {
    if (charge[cathode] != 0) {
      x[cathode] /= charge[cathode];
      y[cathode] /= charge[cathode];
    }
    if (multiplicity[cathode] != 0) {
      double sqrtCharge = sqrt(charge[cathode]);
      xsize[cathode] /= (multiplicity[cathode] * sqrtCharge);
      ysize[cathode] /= (multiplicity[cathode] * sqrtCharge);
    } else {
      xsize[cathode] = 1E9;
      ysize[cathode] = 1E9;
    }
  }

  isWide[0] = (padPos[0].size() > 1);
  isWide[1] = (padPos[1].size() > 1);

  // each CoG coordinate is taken from the cathode with the best precision
  Xcog = (xsize[0] < xsize[1]) ? x[0] : x[1];
  Ycog = (ysize[0] < ysize[1]) ? y[0] : y[1];
}

//_________________________________________________________________________________________________

static void getFecChannel(int deId, int padId, int& fecId, int& channel)
{
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);
  int dsId = segment.padDualSampaId(padId);
  fecId = getDsIndex(DsDetId{ deId, dsId });
  channel = segment.padDualSampaChannel(padId);
}

//_________________________________________________________________________________________________

void PreclustersTask::plotPrecluster(const o2::mch::PreCluster& preCluster, gsl::span<const o2::mch::Digit> digits)
{
  // filter out single-pad clusters
  if (preCluster.nDigits < 2) {
    return;
  }

  // get the digits of this precluster
  auto preClusterDigits = digits.subspan(preCluster.firstDigit, preCluster.nDigits);

  // whether a cathode has digits or not
  bool cathode[2] = { false, false };
  // total charge on each cathode
  float chargeSum[2] = { 0, 0 };
  // largest signal in each cathode
  float chargeMax[2] = { 0, 0 };
  // whether the pre-cluster contains at least one signal-like digit in each cathode
  float hasSignal[2] = { false, false };
  // number of digits in each cathode
  int multiplicity[2] = { 0, 0 };

  int deId = preClusterDigits[0].getDetID();
  const o2::mch::mapping::Segmentation& segment = o2::mch::mapping::segmentation(deId);

  // loop over digits and collect information on charge and multiplicity
  for (const o2::mch::Digit& digit : preClusterDigits) {
    int padid = digit.getPadID();

    // cathode index
    int cid = segment.isBendingPad(padid) ? 0 : 1;
    cathode[cid] = true;
    chargeSum[cid] += digit.getADC();
    multiplicity[cid] += 1;

    if (digit.getADC() > chargeMax[cid]) {
      chargeMax[cid] = digit.getADC();
    }

    if (mIsSignalDigit(digit)) {
      hasSignal[cid] = true;
    }
  }

  mHistogramPreclustersPerDE->getNum()->Fill(getDEindex(deId));
  if (hasSignal[0] || hasSignal[1]) {
    mHistogramPreclustersSignalPerDE->getNum()->Fill(getDEindex(deId));
  }

  // compute center-of-gravity of the charge distribution
  double Xcog, Ycog;
  bool isWide[2];
  CoG(preClusterDigits, Xcog, Ycog, isWide);

  int padIdB = -1;
  int padIdNB = -1;
  int fecIdB = -1;
  int channelB = -1;
  int fecIdNB = -1;
  int channelNB = -1;
  if (segment.findPadPairByPosition(Xcog, Ycog, padIdB, padIdNB)) {
    getFecChannel(deId, padIdB, fecIdB, channelB);
    getFecChannel(deId, padIdNB, fecIdNB, channelNB);
  }

  // criteria to define a "good" charge cluster in one cathode:
  bool isGoodDen[2] = { hasSignal[1] && isWide[1], hasSignal[0] && isWide[0] };
  bool isGoodNum[2] = { cathode[0] && isWide[0], cathode[1] };

  // Filling histograms to be used for Pseudo-efficiency computation
  if (isGoodDen[0]) {
    // good cluster on non-bending side, check if there is data from the bending side as well
    if (fecIdB >= 0 && channelB >= 0) {
      mHistogramPseudoeffElec->getDen()->Fill(fecIdB, channelB);
    }
    if (isGoodNum[0]) { // Check if associated to something on Bending
      if (fecIdB >= 0 && channelB >= 0) {
        mHistogramPseudoeffElec->getNum()->Fill(fecIdB, channelB);
      }
    }
  }

  if (isGoodDen[1]) {
    // good cluster on bending side, check if there is data from the non-bending side as well
    if (fecIdNB >= 0 && channelNB >= 0) {
      mHistogramPseudoeffElec->getDen()->Fill(fecIdNB, channelNB);
    }
    if (isGoodNum[1]) { // Check if associated to something on Non-Bending
      if (fecIdNB >= 0 && channelNB >= 0) {
        mHistogramPseudoeffElec->getNum()->Fill(fecIdNB, channelNB);
      }
    }
  }

  // This code is slow. Will be re-introduced after some optimizations
  // const o2::mch::mapping::CathodeSegmentation& csegment = segment.bending();
  // o2::mch::contour::Contour<double> envelop = o2::mch::mapping::getEnvelop(csegment);
  // std::vector<o2::mch::contour::Vertex<double>> vertices = envelop.getVertices();
  // o2::mch::contour::BBox<double> bbox = o2::mch::mapping::getBBox(csegment);

  // skip a fiducial border around the active area, so that only fully-contained clusters are considered
  // if(Xcog < (bbox.xmin() + 5)) return true;
  // if(Xcog > (bbox.xmax() - 5)) return true;
  // if(Ycog < (bbox.ymin() + 5)) return true;
  // if(Ycog > (bbox.ymax() - 5)) return true;

  if (hasSignal[0] || hasSignal[1]) {
    int deIndex = getDEindex(deId);
    // cluster size, separately on each cathode and combined
    mHistogramClusterSize->Fill(deIndex * 3, multiplicity[0]);
    mHistogramClusterSize->Fill(deIndex * 3 + 1, multiplicity[1]);
    mHistogramClusterSize->Fill(deIndex * 3 + 2, multiplicity[0] + multiplicity[1]);

    // total cluster charge
    float chargeTot = chargeSum[0] + chargeSum[1];
    mHistogramClusterCharge->Fill(deIndex, chargeTot);
  }
}

//_________________________________________________________________________________________________

void PreclustersTask::endOfCycle()
{
  ILOG(Info, Devel) << "endOfCycle" << AliceO2::InfoLogger::InfoLogger::endm;

  // update mergeable ratios
  mHistogramPseudoeffElec->update();
  mHistogramPreclustersPerDE->update();
  mHistogramPreclustersSignalPerDE->update();
}

//_________________________________________________________________________________________________

void PreclustersTask::endOfActivity(const Activity& /*activity*/)
{
  ILOG(Info, Devel) << "endOfActivity" << AliceO2::InfoLogger::InfoLogger::endm;
}

//_________________________________________________________________________________________________

void PreclustersTask::reset()
{
  // clean all the monitor objects here
  ILOG(Info, Devel) << "Resetting the histograms" << AliceO2::InfoLogger::InfoLogger::endm;

  for (auto h : mAllHistograms) {
    h->Reset("ICES");
  }
}

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2
