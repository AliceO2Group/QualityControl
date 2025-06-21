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
#include <TLegend.h>

namespace o2::quality_control_modules::common
{

static bool isBinningIdentical(TH1* h1, TH1* h2)
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

static void copyAndScaleHistograms(TH1* histogram, TH1* referenceHistogram, TH1* outputHisto, TH1* outputRefHisto, bool scaleReference)
{
  if (!histogram || !referenceHistogram || !outputHisto || !outputRefHisto) {
    ILOG(Warning, Devel) << "histogram is nullptr" << ENDM;
    return;
  }

  if (!isBinningIdentical(histogram, referenceHistogram)) {
    ILOG(Warning, Devel) << "mismatch in axis dimensions for '" << histogram->GetName() << "'" << ENDM;
    return;
  }

  outputHisto->Reset();
  outputHisto->Add(histogram);

  outputRefHisto->Reset();
  outputRefHisto->Add(referenceHistogram);

  if (scaleReference) {
    // the reference histogram is scaled to match the integral of the current histogram
    double integral = histogram->Integral();
    double integralRef = referenceHistogram->Integral();
    if (integral != 0 && integralRef != 0) {
      outputRefHisto->Scale(integral / integralRef);
    }
  }
}

template <class HIST>
static std::shared_ptr<HIST> createHisto1D(const char* name, const char* title, TH1* source)
{
  std::shared_ptr<HIST> result;
  if (source->GetXaxis()->IsVariableBinSize()) {
    result = std::make_shared<HIST>(name, title,
                                    source->GetXaxis()->GetNbins(),
                                    source->GetXaxis()->GetXbins()->GetArray());
  } else {
    result = std::make_shared<HIST>(name, title,
                                    source->GetXaxis()->GetNbins(),
                                    source->GetXaxis()->GetXmin(),
                                    source->GetXaxis()->GetXmax());
  }

  return result;
}

template <class HIST>
static std::shared_ptr<HIST> createHisto2D(const char* name, const char* title, TH1* source)
{
  std::shared_ptr<HIST> result;
  if (source->GetXaxis()->IsVariableBinSize() && source->GetYaxis()->IsVariableBinSize()) {
    result = std::make_shared<HIST>(name, title,
                                    source->GetXaxis()->GetNbins(),
                                    source->GetXaxis()->GetXbins()->GetArray(),
                                    source->GetYaxis()->GetNbins(),
                                    source->GetYaxis()->GetXbins()->GetArray());
  } else if (source->GetXaxis()->IsVariableBinSize() && !source->GetYaxis()->IsVariableBinSize()) {
    result = std::make_shared<HIST>(name, title,
                                    source->GetXaxis()->GetNbins(),
                                    source->GetXaxis()->GetXbins()->GetArray(),
                                    source->GetYaxis()->GetNbins(),
                                    source->GetYaxis()->GetXmin(),
                                    source->GetYaxis()->GetXmax());
  } else if (!source->GetXaxis()->IsVariableBinSize() && source->GetYaxis()->IsVariableBinSize()) {
    result = std::make_shared<HIST>(name, title,
                                    source->GetXaxis()->GetNbins(),
                                    source->GetXaxis()->GetXmin(),
                                    source->GetXaxis()->GetXmax(),
                                    source->GetYaxis()->GetNbins(),
                                    source->GetYaxis()->GetXbins()->GetArray());
  } else {
    result = std::make_shared<HIST>(name, title,
                                    source->GetXaxis()->GetNbins(),
                                    source->GetXaxis()->GetXmin(),
                                    source->GetXaxis()->GetXmax(),
                                    source->GetYaxis()->GetNbins(),
                                    source->GetYaxis()->GetXmin(),
                                    source->GetYaxis()->GetXmax());
  }

  return result;
}

//_________________________________________________________________________________________

class ReferenceComparatorPlotImpl
{
 public:
  ReferenceComparatorPlotImpl(bool scaleReference)
    : mScaleReference(scaleReference)
  {
  }

  virtual ~ReferenceComparatorPlotImpl() = default;

  virtual TObject* init(TH1* referenceHistogram, std::string outputPath, bool scaleReference, bool drawRatioOnly, std::string drawOption)
  {
    return nullptr;
  }

  virtual TObject* getMainCanvas()
  {
    return nullptr;
  }

  void setScaleRef(bool scaleReference)
  {
    mScaleReference = scaleReference;
  }

  bool getScaleReference() { return mScaleReference; }

  virtual void update(TH1* histogram, TH1* referenceHistogram) = 0;

 private:
  bool mScaleReference{ true };
};

template <class HIST>
class ReferenceComparatorPlotImpl1D : public ReferenceComparatorPlotImpl
{
 public:
  ReferenceComparatorPlotImpl1D(TH1* referenceHistogram,
                                int referenceRun,
                                const std::string& outputPath,
                                bool scaleReference,
                                bool drawRatioOnly,
                                double legendHeight,
                                const std::string& drawOption)
    : ReferenceComparatorPlotImpl(scaleReference), mLegendHeight(legendHeight)
  {
    float labelSize = 0.04;

    if (!referenceHistogram) {
      return;
    }

    // full name of the main canvas
    std::string canvasName = outputPath;
    mCanvas = std::make_shared<TCanvas>(canvasName.c_str(), canvasName.c_str(), 800, 600);

    // Pad where the histogram ratio is drawn. If drawRatioOnly is true the pad is placed on top
    // without any transparency, and fully covers the other pad
    mCanvas->cd();
    if (drawRatioOnly) {
      mPadHistRatio = std::make_shared<TPad>((canvasName + "_PadHistRatio").c_str(), "PadHistRatio", 0, 0, 1, 1);
    } else {
      mPadHistRatio = std::make_shared<TPad>((canvasName + "_PadHistRatio").c_str(), "PadHistRatio", 0, 1.0 / 3, 1, 1);
      mPadHistRatio->SetTopMargin(0.15);
      mPadHistRatio->SetBottomMargin(0.5);
      mPadHistRatio->SetLeftMargin(0.1);
      mPadHistRatio->SetRightMargin(0.1);
      mPadHistRatio->SetGridy();
    }

    // Pad where the histograms are drawn. If drawRatioOnly is true the pad is placed hidden
    // behind the second pad where the ratio histogram is drawn
    mCanvas->cd();
    if (drawRatioOnly) {
      mPadHist = std::make_shared<TPad>((canvasName + "_PadHist").c_str(), "PadHist", 0.2, 0.2, 0.8, 0.8);
    } else {
      mPadHist = std::make_shared<TPad>((canvasName + "_PadHist").c_str(), "PadHist", 0, 0, 1, 2.0 / 3 - 0.01);
      mPadHist->SetTopMargin(0);
      mPadHist->SetBottomMargin(0.15);
      mPadHist->SetLeftMargin(0.1);
      mPadHist->SetRightMargin(0.1);
    }

    if (drawRatioOnly) {
      // If drawRatioOnly is true the pad with the ratio plot is placed on top
      mCanvas->cd();
      mPadHist->Draw();
      mCanvas->cd();
      mPadHistRatio->Draw();
    } else {
      // otherwise the pad with the superimposed histograms goes on top
      mCanvas->cd();
      mPadHistRatio->Draw();
      mCanvas->cd();
      mPadHist->Draw();
    }

    // histogram from the current run
    mPadHist->cd();
    mPlot = createHisto1D<HIST>((canvasName + "_hist").c_str(), "", referenceHistogram);
    mPlot->GetXaxis()->SetTitle(referenceHistogram->GetXaxis()->GetTitle());
    mPlot->GetXaxis()->SetLabelSize(labelSize);
    mPlot->GetXaxis()->SetTitleSize(labelSize);
    mPlot->GetXaxis()->SetTitleOffset(1.0);
    mPlot->GetYaxis()->SetTitle(referenceHistogram->GetYaxis()->GetTitle());
    mPlot->GetYaxis()->SetLabelSize(labelSize);
    mPlot->GetYaxis()->SetTitleSize(labelSize);
    mPlot->GetYaxis()->SetTitleOffset(1.0);
    mPlot->SetLineColor(kBlack);
    mPlot->SetStats(0);
    mPlot->SetOption(drawOption.c_str());
    mPlot->Draw(drawOption.c_str());

    // histogram from the reference run
    mReferencePlot = createHisto1D<HIST>((canvasName + "_hist_ref").c_str(), "", referenceHistogram);
    mReferencePlot->SetLineColor(kBlue);
    mReferencePlot->SetOption((drawOption + "SAME").c_str());
    mReferencePlot->Draw((drawOption + "SAME").c_str());

    if (!drawRatioOnly && mLegendHeight > 0) {
      mHistogramLegend = std::make_shared<TLegend>(0.2, 0.91, 0.8, 0.98);
      mHistogramLegend->SetNColumns(2);
      mHistogramLegend->SetBorderSize(0);
      mHistogramLegend->SetFillStyle(0);
      mHistogramLegend->AddEntry(mPlot.get(), "current run", "l");
      mHistogramLegend->AddEntry(mReferencePlot.get(), TString::Format("reference run %d", referenceRun), "l");
      mHistogramLegend->Draw();
    }

    // histogram with current/reference ratio
    mPadHistRatio->cd();
    mRatioPlot = createHisto1D<HIST>((canvasName + "_hist_ratio").c_str(), "", referenceHistogram);
    if (drawRatioOnly) {
      mRatioPlot->SetTitle(TString::Format("%s (ref. %d)", referenceHistogram->GetTitle(), referenceRun));
      mRatioPlot->GetXaxis()->SetTitle(referenceHistogram->GetXaxis()->GetTitle());
      mRatioPlot->GetYaxis()->SetTitle("current / reference");
    } else {
      // hide the X axis labels
      mRatioPlot->GetXaxis()->SetLabelSize(0);
      mRatioPlot->GetXaxis()->SetTitleSize(0);
      mRatioPlot->GetYaxis()->SetTitle("ratio");
      mRatioPlot->GetYaxis()->CenterTitle(kTRUE);
      mRatioPlot->GetYaxis()->SetNdivisions(5);
      mRatioPlot->GetYaxis()->SetLabelSize(labelSize);
      mRatioPlot->GetYaxis()->SetTitleSize(labelSize);
      mRatioPlot->GetYaxis()->SetTitleOffset(1.0);
    }
    mRatioPlot->SetLineColor(kBlack);
    mRatioPlot->SetStats(0);
    mRatioPlot->SetOption("E");
    mRatioPlot->Draw("E");
    if (drawRatioOnly) {
      mRatioPlot->SetMinimum(0);
      mRatioPlot->SetMaximum(2);
    } else {
      // set the minimum and maximum sligtly above 0 and below 2.0, such that the first and last bin labels are not shown
      mRatioPlot->SetMinimum(0.001);
      mRatioPlot->SetMaximum(1.999);
    }

    mCanvas->cd();
    // We place an empty TPaveText in the good place, it will be used by the checker
    // to draw the quality labels and flags
    mQualityLabel = std::make_shared<TPaveText>(0.00, 0.9, 0.9, 0.98, "brNDC");
    mQualityLabel->SetBorderSize(0);
    mQualityLabel->SetFillStyle(0);
    mQualityLabel->SetTextAlign(12);
    mQualityLabel->SetTextFont(42);
    mQualityLabel->Draw();

    if (!drawRatioOnly) {
      // draw the histogram title
      mHistogramTitle = std::make_shared<TPaveText>(0.1, 0.94, 0.9, 1.0, "brNDC");
      mHistogramTitle->SetBorderSize(0);
      mHistogramTitle->SetFillStyle(0);
      mHistogramTitle->SetTextAlign(22);
      mHistogramTitle->SetTextFont(42);
      mHistogramTitle->AddText(TString::Format("%s (ref. %d)", referenceHistogram->GetTitle(), referenceRun));
      mHistogramTitle->Draw();
    }
  }

  TObject* getMainCanvas()
  {
    return mCanvas.get();
  }

  void update(TH1* hist, TH1* referenceHistogram)
  {
    if (!hist || !referenceHistogram) {
      return;
    }

    copyAndScaleHistograms(hist, referenceHistogram, mPlot.get(), mReferencePlot.get(), getScaleReference());

    double max = std::max(mPlot->GetMaximum(), mReferencePlot->GetMaximum());
    double histMax = (mLegendHeight > 0) ? (1.0 + mLegendHeight) * max : 1.05 * max;
    mPlot->SetMaximum(histMax);
    mReferencePlot->SetMaximum(histMax);

    mRatioPlot->Reset();
    mRatioPlot->Add(mPlot.get());
    mRatioPlot->Divide(mReferencePlot.get());
    mRatioPlot->SetMinimum(0.001);
    mRatioPlot->SetMaximum(1.999);
  }

 private:
  std::shared_ptr<TCanvas> mCanvas;
  std::shared_ptr<TPad> mPadHist;
  std::shared_ptr<TPad> mPadHistRatio;
  std::shared_ptr<HIST> mPlot;
  std::shared_ptr<HIST> mReferencePlot;
  std::shared_ptr<HIST> mRatioPlot;
  std::shared_ptr<TLine> mBorderTop;
  std::shared_ptr<TLine> mBorderMiddle;
  std::shared_ptr<TLine> mBorderRight;
  std::shared_ptr<TPaveText> mQualityLabel;
  std::shared_ptr<TPaveText> mHistogramTitle;
  std::shared_ptr<TLegend> mHistogramLegend;
  double mLegendHeight;
};

template <class HIST>
class ReferenceComparatorPlotImpl2D : public ReferenceComparatorPlotImpl
{
 public:
  ReferenceComparatorPlotImpl2D(TH1* referenceHistogram,
                                int referenceRun,
                                const std::string& outputPath,
                                bool scaleReference,
                                bool drawRatioOnly,
                                const std::string& drawOption)
    : ReferenceComparatorPlotImpl(scaleReference)
  {
    if (!referenceHistogram) {
      return;
    }

    // full name of the main canvas
    std::string canvasName = outputPath;
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
    mPlot = createHisto2D<HIST>((canvasName + "_hist").c_str(),
                                referenceHistogram->GetTitle(),
                                referenceHistogram);
    mPlot->GetXaxis()->SetTitle(referenceHistogram->GetXaxis()->GetTitle());
    mPlot->GetYaxis()->SetTitle(referenceHistogram->GetYaxis()->GetTitle());
    mPlot->SetStats(0);
    mPlot->SetOption(drawOption.c_str());
    mPlot->Draw(drawOption.c_str());

    // histogram from the reference run
    mPadHistRef->cd();
    mReferencePlot = createHisto2D<HIST>((canvasName + "_hist_ref").c_str(),
                                         TString::Format("%s (ref. %d)", referenceHistogram->GetTitle(), referenceRun),
                                         referenceHistogram);
    mReferencePlot->GetXaxis()->SetTitle(referenceHistogram->GetXaxis()->GetTitle());
    mReferencePlot->GetYaxis()->SetTitle(referenceHistogram->GetYaxis()->GetTitle());
    mReferencePlot->SetStats(0);
    mReferencePlot->SetOption(drawOption.c_str());
    mReferencePlot->Draw(drawOption.c_str());

    // histogram with current/reference ratio
    mPadHistRatio->cd();
    mRatioPlot = createHisto2D<HIST>((canvasName + "_hist_ratio").c_str(),
                                     TString::Format("%s (ratio wrt %d)", referenceHistogram->GetTitle(), referenceRun),
                                     referenceHistogram);
    mRatioPlot->GetXaxis()->SetTitle(referenceHistogram->GetXaxis()->GetTitle());
    mRatioPlot->GetYaxis()->SetTitle(referenceHistogram->GetYaxis()->GetTitle());
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

  TObject* getMainCanvas()
  {
    return mCanvas.get();
  }

  void update(TH1* histogram, TH1* referenceHistogram)
  {
    if (!histogram || !referenceHistogram) {
      return;
    }

    copyAndScaleHistograms(histogram, referenceHistogram, mPlot.get(), mReferencePlot.get(), getScaleReference());

    mRatioPlot->Reset();
    mRatioPlot->Add(mPlot.get());
    mRatioPlot->Divide(mReferencePlot.get());
    mRatioPlot->SetMinimum(0);
    mRatioPlot->SetMaximum(2);
  }

 private:
  std::shared_ptr<TCanvas> mCanvas;
  std::shared_ptr<TPad> mPadHist;
  std::shared_ptr<TPad> mPadHistRef;
  std::shared_ptr<TPad> mPadHistRatio;
  std::shared_ptr<HIST> mPlot;
  std::shared_ptr<HIST> mReferencePlot;
  std::shared_ptr<HIST> mRatioPlot;
  std::shared_ptr<TPaveText> mQualityLabel;
};

ReferenceComparatorPlot::ReferenceComparatorPlot(TH1* referenceHistogram,
                                                 int referenceRun,
                                                 const std::string& outputPath,
                                                 bool scaleReference,
                                                 bool drawRatioOnly,
                                                 double legendHeight,
                                                 const std::string& drawOption1D,
                                                 const std::string& drawOption2D)
{
  // histograms with integer values are promoted to floating point or double to allow correctly scaling the reference and computing the ratios

  // 1-D histograms
  if (referenceHistogram->InheritsFrom("TH1C") || referenceHistogram->InheritsFrom("TH1S") || referenceHistogram->InheritsFrom("TH1F")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl1D<TH1F>>(referenceHistogram, referenceRun, outputPath, scaleReference, drawRatioOnly, legendHeight, drawOption1D);
  }

  if (referenceHistogram->InheritsFrom("TH1I") || referenceHistogram->InheritsFrom("TH1D")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl1D<TH1D>>(referenceHistogram, referenceRun, outputPath, scaleReference, drawRatioOnly, legendHeight, drawOption1D);
  }

  // 2-D histograms
  if (referenceHistogram->InheritsFrom("TH2C") || referenceHistogram->InheritsFrom("TH2S") || referenceHistogram->InheritsFrom("TH2F")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl2D<TH2F>>(referenceHistogram, referenceRun, outputPath, scaleReference, drawRatioOnly, drawOption2D);
  }

  if (referenceHistogram->InheritsFrom("TH2I") || referenceHistogram->InheritsFrom("TH2D")) {
    mImplementation = std::make_shared<ReferenceComparatorPlotImpl2D<TH2D>>(referenceHistogram, referenceRun, outputPath, scaleReference, drawRatioOnly, drawOption2D);
  }
}

TObject* ReferenceComparatorPlot::getMainCanvas()
{
  return (mImplementation.get() ? mImplementation->getMainCanvas() : nullptr);
}

void ReferenceComparatorPlot::update(TH1* histogram, TH1* referenceHistogram)
{
  if (mImplementation) {
    mImplementation->update(histogram, referenceHistogram);
  }
}

} // namespace o2::quality_control_modules::common
