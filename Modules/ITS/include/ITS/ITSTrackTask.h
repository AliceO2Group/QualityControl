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
/// \file   ITSTrackTask.h
/// \author Artem Isakov
///

#ifndef QC_MODULE_ITS_ITSTRACKTASK_H
#define QC_MODULE_ITS_ITSTRACKTASK_H

#include "QualityControl/TaskInterface.h"
#include <DataFormatsITSMFT/TopologyDictionary.h>
#include <ITSBase/GeometryTGeo.h>
#include <Framework/TimingInfo.h>
#include <TLine.h>
#include <TTree.h>
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include <Fit/Fitter.h>
#include <Math/Functor.h>
#include <TMatrixD.h>
#include <TVector3.h>
#include "TMath.h"
#include <cassert>
#include "ITS/ITSHelpers.h"

class TH1;
class TH2;

using namespace o2::quality_control::core;
using namespace o2::quality_control_modules::common;

namespace o2::quality_control_modules::its
{

class ITSTrackTask : public TaskInterface
{

 public:
  ITSTrackTask();
  ~ITSTrackTask() override;

  void initialize(o2::framework::InitContext& ctx) override;
  void startOfActivity(const Activity& activity) override;
  void startOfCycle() override;
  void monitorData(o2::framework::ProcessingContext& ctx) override;
  void endOfCycle() override;
  void endOfActivity(const Activity& activity) override;
  void reset() override;

 private:
  void publishHistos();
  void formatAxes(TH1* h, const char* xTitle, const char* yTitle, float xOffset = 1., float yOffset = 1.);
  void addObject(TObject* aObject);
  void createAllHistos();
  template <class T>
  void formatAxes(T* obj, const char* xTitle, const char* yTitle, float xOffset, float yOffset)
  {
    obj->GetXaxis()->SetTitle(xTitle);
    obj->GetYaxis()->SetTitle(yTitle);
    obj->GetXaxis()->SetTitleOffset(xOffset);
    obj->GetYaxis()->SetTitleOffset(yOffset);
  }

  static constexpr int NLayer = 7;
  static constexpr int NLayerIB = 3;

  const int ChipBoundary[NLayer + 1] = { 0, 108, 252, 432, 3120, 6480, 14712, 24120 };

  std::vector<TObject*> mPublishedObjects;
  std::unique_ptr<TH1DRatio> hNClusters;
  std::unique_ptr<TH1DRatio> hTrackEta;
  std::unique_ptr<TH1DRatio> hTrackPhi;
  TH1D* hVerticesRof;
  std::unique_ptr<TH2DRatio> hAngularDistribution;
  TH2D* hVertexCoordinates;
  TH2D* hVertexRvsZ;
  TH1D* hVertexZ;
  TH1D* hVertexContributors;
  TH1D* hAssociatedClusterFraction;
  TH1D* hNtracks;
  std::unique_ptr<TH2DRatio> hNClustersPerTrackEta;
  std::unique_ptr<TH2DRatio> hNClustersPerTrackPhi;
  std::unique_ptr<TH2DRatio> hNClustersPerTrackPt;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhiAll;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi4cls;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi5cls;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi6cls;
  std::unique_ptr<TH2DRatio> hHitFirstLayerPhi7cls;
  TH2D* hClusterVsBunchCrossing;
  TH2D* hNClusterVsChipITS;
  // Histograms for inv mass k0s, lambda
  TH1D* hInvMassK0s;
  TH1D* hInvMassLambda;
  TH1D* hInvMassLambdaBar;
  TH2D* hTrackPtVsEta;
  TH2D* hTrackPtVsPhi;
  float mPiInvMass = 0.14;
  float mProtonInvMass = 0.938;
  Int_t mInvMasses = 0; // switch for the V0 invariant mass computation, 1 (default) - on, 0 - off

  int mPublishMore = 1;
  float mVertexXYsize = 0.5;
  float mVertexZsize = 15.;
  float mVertexRsize = 0.8;
  Int_t mNtracksMAX = 100;
  Int_t mDoTTree = 0;
  // mDoNorm: 0 = no normalization, 1 = normalization by nVertices, 2 = normalization by nRofs
  Int_t mDoNorm = 1;
  Int_t mNRofs = 0;
  int nBCbins = 103;
  long int mTimestamp = -1;
  int nVertices = 0;
  double mChipBins[2125]; // x bins for cos(lambda) plot
  double mCoslBins[25];   // y bins for cos(lambda) plot
  double ptBins[141];     // pt bins

  o2::itsmft::TopologyDictionary* mDict;

 private:
  // analysis for its-only residual
  o2::its::GeometryTGeo* mGeom;

  std::vector<double> FitStepSize{ 0.3, 1.0e-5, 1.0e-5, 1.0e-5 };

  double FitTolerance = 1.0e-8;
  double ITS_AbsBz = 0.5; // T

  double inputG_C[3 * NLayer];
  double FitparXY[6];
  double FitparDZ[2];
  ROOT::Fit::Fitter fitterA;
  ROOT::Fit::Fitter fitterB;
  Int_t mAlignmentMonitor = 0;
  Int_t mDefaultMomResPar = 0;
  Int_t mResMonNclMin = 0;
  float mResMonTrackMinPt = 0;

  std::array<std::unique_ptr<TH2F>, NLayer> hResidualXY{}; //[NLayer];
  std::array<std::unique_ptr<TH2F>, NLayer> hResidualZD{}; //[NLayer];

  void circleFitXY(double* input, double* par, double& MSEvalue, std::vector<bool> hitUpdate, int step = 0);

  // default setting
  //  function Object to be minimized
  struct se_circlefitXY {
    // the TGraph is a data member of the object
    std::vector<TVector3> fHits;
    double thetaR;
    double Bz;
    double sigma_meas[2][NLayer] = { { 45, 45, 45, 55, 55, 55, 55 }, { 40, 40, 40, 40, 40, 40, 40 } };    // um unit
    double sigma_msc[2][NLayer] = { { 30, 30, 30, 110, 110, 110, 110 }, { 25, 25, 25, 75, 75, 75, 75 } }; // um unit

    se_circlefitXY() = default;
    se_circlefitXY(std::vector<TVector3>& h, double tR, double bz)
    {
      fHits = h;
      thetaR = tR;
      Bz = bz;
    };

    void loadparameters(double arrpar_meas[][NLayer], double arrpar_msc[][NLayer])
    {
      for (int a = 0; a < 2; a++) {
        for (int l = 0; l < NLayer; l++) {
          sigma_meas[a][l] = arrpar_meas[a][l];
          sigma_msc[a][l] = arrpar_msc[a][l];
        }
      }
    };

    void init()
    {
      fHits.clear();
      thetaR = 0;
      Bz = 0;
    };

    void set(std::vector<TVector3>& h, double tR, double bz)
    {
      fHits = h;
      thetaR = tR;
      Bz = bz;
    };

    double getsigma(double R, int L, double B, int axis)
    {
      // R : cm
      // B : T
      if (L < 0 || L >= NLayer)
        return 1;
      double aL = sigma_meas[axis][L] * 1e-4; // um -> cm
      double bL = sigma_msc[axis][L] * 1e-4;  // um -> cm
      double Beff = 0.299792458 * B;
      double sigma = std::sqrt(std::pow(aL, 2) + std::pow(bL, 2) / (std::pow(Beff, 2) * std::pow(R * 1e-2, 2)));

      return sigma;
    };

    // calculate distance line-point
    double distance2(double x, double y, const double* p, double tR, int charge)
    {

      double R = std::abs(1 / p[0]);

      double Xc = p[0] > 0 ? R * std::cos(p[1] + tR + 0.5 * TMath::Pi()) : R * std::cos(p[1] + tR - 0.5 * TMath::Pi());
      double Yc = p[0] > 0 ? R * std::sin(p[1] + tR + 0.5 * TMath::Pi()) : R * std::sin(p[1] + tR - 0.5 * TMath::Pi());

      double dx = x - (Xc + p[2]);
      double dy = y - (Yc + p[3]);
      double dxy = R - std::sqrt(dx * dx + dy * dy);

      double d2 = dxy * dxy;
      return d2;
    };

    // implementation of the function to be minimized
    double operator()(const double* par)
    { // const double -> double
      assert(fHits != 0);

      int nhits = fHits.size();
      double sum = 0;

      double charge = par[0] > 0 ? +1 : -1;
      double RecRadius = std::abs(1 / par[0]);

      double Sigma_tot[NLayer];
      double sum_Sigma_tot = 0;
      for (int l = 0; l < nhits; l++) {
        Sigma_tot[l] = getsigma(RecRadius, fHits[l].Z(), Bz, 1);
        sum_Sigma_tot += 1 / (std::pow(Sigma_tot[l], 2));
      }

      for (int l = 0; l < nhits; l++) {
        double d = distance2(fHits[l].X(), fHits[l].Y(), par, thetaR, charge) / (std::pow(Sigma_tot[l], 2));
        sum += d;
      }
      sum = sum / sum_Sigma_tot;

      return sum;
    };
  };

  se_circlefitXY fitfuncXY;

  void lineFitDZ(double* zIn, double* betaIn, double* parz, double Radius, bool vertex, std::vector<bool> hitUpdate);

  double mSigmaMeas[2][NLayer] = { { 45, 45, 45, 55, 55, 55, 55 }, { 40, 40, 40, 40, 40, 40, 40 } };    // um unit
  double mSigmaMsc[2][NLayer] = { { 30, 30, 30, 110, 110, 110, 110 }, { 25, 25, 25, 75, 75, 75, 75 } }; // um unit

  double getsigma(double R, int L, double B, int axis)
  {
    // R : cm
    // B : T
    if (L < 0 || L >= NLayer)
      return 1;
    double aL = mSigmaMeas[axis][L] * 1e-4; // um -> cm
    double bL = mSigmaMsc[axis][L] * 1e-4;  // um -> cm
    double Beff = 0.299792458 * B;
    double sigma = std::sqrt(std::pow(aL, 2) + std::pow(bL, 2) / (std::pow(Beff, 2) * std::pow(R * 1e-2, 2)));

    return sigma;
  };
};
} // namespace o2::quality_control_modules::its

#endif // QC_MODULE_ITS_ITSTRACKTASK_H
