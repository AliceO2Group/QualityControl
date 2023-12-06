#ifndef QC_MODULE_ITS_ITSHELPERS_H
#define QC_MODULE_ITS_ITSHELPERS_H

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Quality.h"
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TText.h>

namespace o2::quality_control_modules::its
{
template <typename T>
std::vector<T> convertToArray(std::string input)
{

  std::istringstream ss{ input };

  std::vector<T> result;
  std::string token;
  while (std::getline(ss, token, ',')) {

    if constexpr (std::is_same_v<T, int>) {
      result.push_back(std::stoi(token));
    } else if constexpr (std::is_same_v<T, std::string>) {
      result.push_back(token);
    }
  }

  return result;
}

} // namespace o2::quality_control_modules::its
#endif
