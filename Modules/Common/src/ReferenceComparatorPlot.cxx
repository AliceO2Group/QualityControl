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
/// \file    ReferenceComparatorPlot.xx
/// \author  Andrea Ferrero
/// \brief   Utility class for the combined drawing of the current and reference plots, and their ratio
///

#include "Common/ReferenceComparatorPlot.h"
#include "Common/TH1Ratio.h"
#include "Common/TH2Ratio.h"
#include "QualityControl/QcInfoLogger.h"
// ROOT
#include <TClass.h>
#include <TLine.h>
#include <TH1.h>
#include <TH2.h>
#include <TCanvas.h>
#include <TPaveText.h>

namespace o2::quality_control_modules::common
{

static bool checkAxis(TH1* h1, TH1* h2)
{
  // check consistency of X-axis binning
  if (h1->GetXaxis()->GetNbins() != h2->GetXaxis()->GetNbins()) {
    return false;
  }
  if (h1->GetXaxis()->GetXmin() != h2->GetXaxis()->GetXmin()) {
    return false;
  }
  if (h1->GetXaxis()->GetXmax() != h2->GetXaxis()->GetXmax()) {
    return false;
  }

  // check consistency of Y-axis binning
  if (h1->GetYaxis()->GetNbins() != h2->GetYaxis()->GetNbins()) {
    return false;
  }
  if (h1->GetYaxis()->GetXmin() != h2->GetYaxis()->GetXmin()) {
    return false;
  }
  if (h1->GetYaxis()->GetXmax() != h2->GetYaxis()->GetXmax()) {
    return false;
  }

  // check consistency of Z-axis binning
  if (h1->GetZaxis()->GetNbins() != h2->GetZaxis()->GetNbins()) {
    return false;
  }
  if (h1->GetZaxis()->GetXmin() != h2->GetZaxis()->GetXmin()) {
    return false;
  }
  if (h1->GetZaxis()->GetXmax() != h2->GetZaxis()->GetXmax()) {
    return false;
  }

  return true;
}

//_________________________________________________________________________________________

static void copyAndScaleHistograms(TH1* histo, TH1* refHisto, TH1* outputHisto, TH1* outputRefHisto, bool scaleReference)
{
  if (!histo || !refHisto || !outputHisto || !outputRefHisto) {
    ILOG(Warning, Devel) << "histogram is nullptr" << ENDM;
    return;
  }

  if (!checkAxis(histo, refHisto)) {
    ILOG(Warning, Devel) << "mismatch in axis dimensions for '" << histo->GetName() << "'" << ENDM;
    return;
  }

  outputHisto->Reset();
  outputHisto->Add(histo);

  outputRefHisto->Reset();
  outputRefHisto->Add(refHisto);

  if (scaleReference) {
    // the reference histogram is scaled to match the integral of the current histogram
    double integral = histo->Integral();
    double integralRef = refHisto->Integral();
    if (integral != 0 && integralRef != 0) {
      outputRefHisto->Scale(integral / integralRef);
    }
  }
}

//_________________________________________________________________________________________

class ReferenceComparatorPlotImpl
{
 public:
  ReferenceComparatorPlotImpl(bool scaleRef)
  : mScaleRef(scaleRef)
  {
  }

  virtual ~ReferenceComparatorPlotImpl() = default;

  virtual TObject* init(TH1* refHist, std::string outputPath, bool scaleRef, bool drawRatioOnly, std::string drawOption)
  {
    return nullptr;
  }

  virtual TObject* getObject()
  {
    return nullptr;
  }

  void setScaleRef(bool scaleRef)
  {
    mScaleRef = scaleRef;
  }

  bool getScaleRef() { return mScaleRef; }

  virtual void update(TH1* hist, TH1* histRef) = 0;

 private:
  bool mScaleRef{ true };
};

template <class HIST>
class ReferenceComparatorPlotImpl1D : public ReferenceComparatorPlotImpl
{
 public:
  ReferenceComparatorPlotImpl1D(TH1* refHist, std::string outputPath, bool scaleRef, bool drawRatioOnly, std::string drawOption)
  : ReferenceComparatorPlotImpl(scaleRef)
  {
    if (!refHist) {
      return;
    }

    // full name of the main canvas
    std::string canvasName = outputPath + "_RefComp";
    mCanvas = std::make_shared<TCanvas>(canvasName.c_str(), canvasName.c_str(), 800, 600);

    // Pad where the histograms are drawn. If drawRatioOnly is true the pad is placed hidden
    // behind the second pad where the ratio histogram is drawn
    mCanvas->cd();
    if (drawRatioOnly) {
      mPadHist = std::make_shared<TPad>((canvasName + "_PadHist").c_str(), "PadHist", 0.2, 0.2, 0.8, 0.8);
    } else {
      mPadHist = std::make_shared<TPad>((canvasName + "_PadHist").c_str(), "PadHist", 0, 0, 1, 1);
      mPadHist->SetBottomMargin(1.0 / 3);
      mPadHist->SetFillStyle(4000);
    }
    mPadHist->Draw();

    // Pad where the histogram ratio is drawn. If drawRatioOnly is true the pad is placed on top
    // without any transparency, and fully covers the other pad
    mCanvas->cd();
    mPadHistRatio = std::make_shared<TPad>((canvasName + "_PadHistRatio").c_str(), "PadHistRatio", 0, 0, 1, 1);
    if (!drawRatioOnly) {
      mPadHistRatio->SetTopMargin(2.0 / 3);
      mPadHistRatio->SetFillStyle(4000);
    }
    mPadHistRatio->Draw();

    // histogram from the current run
    mPadHist->cd();
    mPlot = std::make_shared<HIST>((canvasName + "_hist").c_str(),
                                   refHist->GetTitle(),
                                   refHist->GetXaxis()->GetNbins(),
                                   refHist->GetXaxis()->GetXmin(),
                                   refHist->GetXaxis()->GetXmax());
    // hide the X-axis
    mPlot->GetXaxis()->SetTitle("");
    mPlot->GetXaxis()->SetLabelSize(0);
    mPlot->GetYaxis()->SetTitle(refHist->GetYaxis()->GetTitle());
    mPlot->SetLineColor(kBlack);
    mPlot->SetStats(0);
    mPlot->SetOption(drawOption.c_str());
    mPlot->Draw(drawOption.c_str());

    // histogram from the reference run
    mRefPlot = std::make_shared<HIST>((canvasName + "_hist_ref").c_str(),
                                      refHist->GetTitle(),
                                      refHist->GetXaxis()->GetNbins(),
                                      refHist->GetXaxis()->GetXmin(),
                                      refHist->GetXaxis()->GetXmax());
    mRefPlot->SetLineColor(kBlue);
    //mRefPlot->SetLineStyle(kDashed);
    //mRefPlot->SetLineWidth(2);
    mRefPlot->SetOption((drawOption + "SAME").c_str());
    mRefPlot->Draw((drawOption + "SAME").c_str());

    // histogram with current/reference ratio
    mPadHistRatio->cd();
    mRatioPlot = std::make_shared<HIST>((canvasName + "_hist_ratio").c_str(),
                                        refHist->GetTitle(),
                                        refHist->GetXaxis()->GetNbins(),
                                        refHist->GetXaxis()->GetXmin(),
                                        refHist->GetXaxis()->GetXmax());
    mRatioPlot->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
    if (drawRatioOnly) {
      mRatioPlot->GetYaxis()->SetTitle("current / reference");
    } else {
      mRatioPlot->GetYaxis()->SetTitle("ratio");
      mRatioPlot->GetYaxis()->CenterTitle(kTRUE);
      mRatioPlot->GetYaxis()->SetNdivisions(5);
      mRatioPlot->SetTitle("");
    }
    mRatioPlot->SetLineColor(kBlack);
    mRatioPlot->SetStats(0);
    mRatioPlot->SetOption("E");
    mRatioPlot->Draw("E");
    mRatioPlot->SetMinimum(0);
    if (drawRatioOnly) {
      mRatioPlot->SetMaximum(2);
    } else {
      // set the maximum sligtly below 2.0, such that the corresponding bin label is not shown
      // and does not overlap with the zero of the histogram above
      mRatioPlot->SetMaximum(1.999);
    }

    // Apparently with transparent pads the histogram border is also not draw,
    // so we need to add it by hand when drawing multiple pads
    mCanvas->cd();
    if (!drawRatioOnly) {
      mBorderTop = std::make_shared<TLine>(0.1, 0.9, 0.9, 0.9);
      mBorderTop->SetLineColor(kBlack);
      mBorderTop->Draw();

      mBorderRight = std::make_shared<TLine>(0.9, 0.1, 0.9, 0.9);
      mBorderRight->SetLineColor(kBlack);
      mBorderRight->Draw();
    }

    // We place an empty TPaveText in the good place, it will be used by the checker
    // to draw the quality labels and flags
    mQualityLabel = std::make_shared<TPaveText>(0.00, 0.9, 0.9, 0.98, "brNDC");
    mQualityLabel->SetBorderSize(0);
    mQualityLabel->SetFillStyle(0);
    mQualityLabel->SetTextAlign(12);
    mQualityLabel->SetTextFont(42);
    mQualityLabel->Draw();
  }

  TObject* getObject()
  {
    return mCanvas.get();
  }

  void update(TH1* hist, TH1* histRef)
  {
    if (!hist || !histRef) {
      return;
    }

    copyAndScaleHistograms(hist, histRef, mPlot.get(), mRefPlot.get(), getScaleRef());

    mRatioPlot->Reset();
    mRatioPlot->Add(mPlot.get());
    mRatioPlot->Divide(mRefPlot.get());
    mRatioPlot->SetMinimum(0);
    mRatioPlot->SetMaximum(1.999);
  }

 private:
  std::shared_ptr<TCanvas> mCanvas;
  std::shared_ptr<TPad> mPadHist;
  std::shared_ptr<TPad> mPadHistRatio;
  std::shared_ptr<HIST> mPlot;
  std::shared_ptr<HIST> mRefPlot;
  std::shared_ptr<HIST> mRatioPlot;
  std::shared_ptr<TLine> mBorderTop;
  std::shared_ptr<TLine> mBorderRight;
  std::shared_ptr<TPaveText> mQualityLabel;
};

template <class HIST>
class ReferenceComparatorPlotImpl2D : public ReferenceComparatorPlotImpl
{
 public:
  ReferenceComparatorPlotImpl2D(TH1* refHist, std::string outputPath, bool scaleRef, bool drawRatioOnly, std::string drawOption)
  : ReferenceComparatorPlotImpl(scaleRef)
  {
    if (!refHist) {
      return;
    }

    setScaleRef(scaleRef);

    // full name of the main canvas
    std::string canvasName = outputPath + "_RefComp";
    mCanvas = std::make_shared<TCanvas>(canvasName.c_str(), canvasName.c_str(), 800, 600);

    // Size of the pad for the ratio histogram, relative to the canvas size
    // Only used when drawRatioOnly is false, and both the ratio and the individual histograms are drawn
    const float padSizeRatio = 2.0 / 3;

    // Pad where the current histogram is drawn. If drawRatioOnly is true the pad is draw hidden
    // behind the main pad where the ratio histogram is drawn
    mCanvas->cd();
    if (!drawRatioOnly) {
      // the pad occupies a bottom-left portion of the canvas with a size equal to (1-padSizeRatio)
      mPadHist = std::make_shared<TPad>((canvasName + "_PadHist").c_str(), "PadHist", 0, 0, 0.5, 1.0 - padSizeRatio);
    } else {
      // hide the pad below the one with the ratio plot
      mPadHist = std::make_shared<TPad>((canvasName + "_PadHist").c_str(), "PadHist", 0.1, 2.0 / 3, 0.5, 0.9);
    }
    mPadHist->Draw();

    // Pad where the reference histogram is drawn. If drawRatioOnly is true the pad is draw hidden
    // behind the main pad where the ratio histogram is drawn
    mCanvas->cd();
    if (!drawRatioOnly) {
      // the pad occupies a bottom-right portion of the canvas with a size equal to (1-padSizeRatio)
      mPadHistRef = std::make_shared<TPad>((canvasName + "_PadHistRef").c_str(), "PadHistRef", 0.5, 0, 1, 1.0 - padSizeRatio);
    } else {
      // hide the pad below the one with thenratio plot
      mPadHistRef = std::make_shared<TPad>((canvasName + "_PadHistRef").c_str(), "PadHistRef", 0.5, 2.0 / 3, 0.9, 0.9);
    }
    mPadHistRef->Draw();

    // Pad where the histogram ratio is drawn. If drawRatioOnly is true the pad occupies the full canvas
    mCanvas->cd();
    if (!drawRatioOnly) {
      // the pad occupies a top portion of the canvas with a size equal to padSizeRatio
      mPadHistRatio = std::make_shared<TPad>((canvasName + "_PadHistRatio").c_str(), "PadHistRatio", 0, 1.0 - padSizeRatio, 1, 1);
      // The top margin of the pad is increased in a way inversely proportional to the pad size.
      // The resulting margin corresponds to 0.1 in the outer canvas coordinates, such that the histogram
      // title is drawn with the usual text size
      mPadHistRatio->SetTopMargin(0.1 / padSizeRatio);
    } else {
      // the pad occupies the full canvas
      mPadHistRatio = std::make_shared<TPad>((canvasName + "_PadHistRatio").c_str(), "PadHistRatio", 0, 0, 1, 1);
    }
    mPadHistRatio->Draw();

    // histogram from the current run
    mPadHist->cd();
    mPlot = std::make_shared<HIST>((canvasName + "_hist").c_str(),
                                   refHist->GetTitle(),
                                   refHist->GetXaxis()->GetNbins(),
                                   refHist->GetXaxis()->GetXmin(),
                                   refHist->GetXaxis()->GetXmax(),
                                   refHist->GetYaxis()->GetNbins(),
                                   refHist->GetYaxis()->GetXmin(),
                                   refHist->GetYaxis()->GetXmax());
    mPlot->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
    mPlot->GetYaxis()->SetTitle(refHist->GetYaxis()->GetTitle());
    mPlot->SetStats(0);
    mPlot->SetOption(drawOption.c_str());
    mPlot->Draw(drawOption.c_str());

    // histogram from the reference run
    mPadHistRef->cd();
    mRefPlot = std::make_shared<HIST>((canvasName + "_hist_ref").c_str(),
                                      TString::Format("%s (reference)", refHist->GetTitle()),
                                      refHist->GetXaxis()->GetNbins(),
                                      refHist->GetXaxis()->GetXmin(),
                                      refHist->GetXaxis()->GetXmax(),
                                      refHist->GetYaxis()->GetNbins(),
                                      refHist->GetYaxis()->GetXmin(),
                                      refHist->GetYaxis()->GetXmax());
    mRefPlot->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
    mRefPlot->GetYaxis()->SetTitle(refHist->GetYaxis()->GetTitle());
    mRefPlot->SetStats(0);
    mRefPlot->SetOption(drawOption.c_str());
    mRefPlot->Draw(drawOption.c_str());

    // histogram with current/reference ratio
    mPadHistRatio->cd();
    mRatioPlot = std::make_shared<HIST>((canvasName + "_hist_ratio").c_str(),
                                        TString::Format("%s (ratio)", refHist->GetTitle()),
                                        refHist->GetXaxis()->GetNbins(),
                                        refHist->GetXaxis()->GetXmin(),
                                        refHist->GetXaxis()->GetXmax(),
                                        refHist->GetYaxis()->GetNbins(),
                                        refHist->GetYaxis()->GetXmin(),
                                        refHist->GetYaxis()->GetXmax());
    mRatioPlot->GetXaxis()->SetTitle(refHist->GetXaxis()->GetTitle());
    mRatioPlot->GetYaxis()->SetTitle(refHist->GetYaxis()->GetTitle());
    if (!drawRatioOnly) {
      mRatioPlot->GetZaxis()->SetTitle("ratio");
    } else {
      mRatioPlot->GetZaxis()->SetTitle("current / reference");
    }
    mRatioPlot->SetStats(0);
    mRatioPlot->SetOption("COLZ");
    mRatioPlot->Draw("COLZ");
    mRatioPlot->SetMinimum(0);
    mRatioPlot->SetMaximum(2);

    // We place an empty TPaveText in the good place, it will be used by the checker
    // to draw the quality labels and flags
    mCanvas->cd();
    mQualityLabel = std::make_shared<TPaveText>(0.0, 0.9, 0.9, 0.98, "brNDC");
    mQualityLabel->SetBorderSize(0);
    mQualityLabel->SetFillStyle(0);
    mQualityLabel->SetTextAlign(12);
    mQualityLabel->SetTextFont(42);
    mQualityLabel->Draw();
  }

  TObject* getObject()
  {
    return mCanvas.get();
  }

  void update(TH1* hist, TH1* histRef)
  {
    if (!hist || !histRef) {
      return;
    }

    copyAndScaleHistograms(hist, histRef, mPlot.get(), mRefPlot.get(), getScaleRef());

    mRatioPlot->Reset();
    mRatioPlot->Add(mPlot.get());
    mRatioPlot->Divide(mRefPlot.get());
    mRatioPlot->SetMinimum(0);
    mRatioPlot->SetMaximum(2);
  }

 private:
  std::shared_ptr<TCanvas> mCanvas;
  std::shared_ptr<TPad> mPadHist;
  std::shared_ptr<TPad> mPadHistRef;
  std::shared_ptr<TPad> mPadHistRatio;
  std::shared_ptr<HIST> mPlot;
  std::shared_ptr<HIST> mRefPlot;
  std::shared_ptr<HIST> mRatioPlot;
  std::shared_ptr<TPaveText> mQualityLabel;
};

ReferenceComparatorPlot::ReferenceComparatorPlot(TH1* refHist, std::string outputPath, bool scaleRef, bool drawRatioOnly, std::string drawOption1D, std::string drawOption2D)
{
  if (refHist->IsA() == TClass::GetClass<TH1F>() || refHist->InheritsFrom("TH1F")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl1D<TH1F>>(refHist, outputPath, scaleRef, drawRatioOnly, drawOption1D);
  }

  if (refHist->IsA() == TClass::GetClass<TH1D>() || refHist->InheritsFrom("TH1D")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl1D<TH1D>>(refHist, outputPath, scaleRef, drawRatioOnly, drawOption1D);
  }

  if (refHist->IsA() == TClass::GetClass<TH2F>() || refHist->InheritsFrom("TH2F")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl2D<TH2F>>(refHist, outputPath, scaleRef, drawRatioOnly, drawOption2D);
  }

  if (refHist->IsA() == TClass::GetClass<TH2D>() || refHist->InheritsFrom("TH2D")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl2D<TH2D>>(refHist, outputPath, scaleRef, drawRatioOnly, drawOption2D);
  }
}

TObject* ReferenceComparatorPlot::getObject()
{
  return (mImplementation.get() ? mImplementation->getObject() : nullptr);
}

void ReferenceComparatorPlot::update(TH1* hist, TH1* histRef)
{
  if (mImplementation) {
    mImplementation->update(hist, histRef);
  }
}

} // namespace o2::quality_control_modules::common
