///
/// \file   PhysicsTask.h
/// \author Barthelemy von Haller
/// \author Piotr Konopka
/// \author Andrea Ferrero
///

#ifndef QC_MODULE_MUONCHAMBERS_GLOBALHISTOGRAM_H
#define QC_MODULE_MUONCHAMBERS_GLOBALHISTOGRAM_H

#include <map>
#include <TH2.h>

namespace o2
{
namespace quality_control_modules
{
namespace muonchambers
{

class GlobalHistogram : public TH2F
{
  int getLR(int de);
  void getDeCenter(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST3(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST4(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);
  void getDeCenterST5(int de, float& xB0, float& yB0, float& xNB0, float& yNB0);

 public:
  GlobalHistogram(std::string name, std::string title);

  void init();

  // add the histograms of the individual detection elements
  void add(std::map<int, TH2F*>& histB, std::map<int, TH2F*>& histNB);

  // replace the contents with the histograms of the individual detection elements
  void set(std::map<int, TH2F*>& histB, std::map<int, TH2F*>& histNB, bool doAverage = true);
};

} // namespace muonchambers
} // namespace quality_control_modules
} // namespace o2

#endif // QC_MODULE_MUONCHAMBERS_GLOBALHISTOGRAM_H
