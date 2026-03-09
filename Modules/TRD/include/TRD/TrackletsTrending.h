#ifndef QUALITYCONTROL_TRACKLETSTRENDING_H
#define QUALITYCONTROL_TRACKLETSTRENDING_H

#include "QualityControl/PostProcessingInterface.h"
#include "QualityControl/Reductor.h"
#include "TRD/TrendingTaskConfigTRD.h"

#include <memory>
#include <unordered_map>
#include <TTree.h>
#include <TCanvas.h>
#include <map>
#include <vector>
#include <string>

namespace o2::quality_control::repository
{
class DatabaseInterface;
}

namespace o2::quality_control::postprocessing
{

// class TrackletsTrending : public PostProcessingInterface
class TrackletsTrending final : public PostProcessingInterface
{
 public:
  TrackletsTrending() = default;
  ~TrackletsTrending() override = default;

  void configure(const boost::property_tree::ptree& config) override;
  void initialize(Trigger, framework::ServiceRegistryRef) override;
  void update(Trigger, framework::ServiceRegistryRef) override;
  void finalize(Trigger, framework::ServiceRegistryRef) override;

 private:
  struct {
    Long64_t runNumber = 0;
  } mMetaData;

  void trendValues(const Trigger& t, repository::DatabaseInterface& qcdb);
  void generatePlots(repository::DatabaseInterface& qcdb);

  TrendingTaskConfigTRD mConfig;
  Int_t ntreeentries = 0;
  UInt_t mTime;
  std::vector<std::string> runlist;
  std::unique_ptr<TTree> mTrend;
  std::unordered_map<std::string, std::unique_ptr<Reductor>> mReductors;
  std::map<std::string, TObject*> mPlots;
  ClassDef(TrackletsTrending, 1);

};

} // namespace o2::quality_control::postprocessing

#endif // QUALITYCONTROL_TRACKLETSTRENDING_H
