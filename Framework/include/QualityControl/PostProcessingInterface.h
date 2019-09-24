// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file   PostProcessingInterface.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_POSTPROCESSINTERFACE_H
#define QUALITYCONTROL_POSTPROCESSINTERFACE_H

#include <string>
#include <memory>
//#include <boost/property_tree/ptree_fwd.hpp>
#include <Framework/ServiceRegistry.h>
#include <Configuration/ConfigurationInterface.h>
#include "QualityControl/Triggers.h"
#include "QualityControl/PostProcessingConfig.h"

namespace o2::quality_control::postprocessing
{

/// \brief  Skeleton of a post-processing task.
///
/// Abstract class defining the skeleton and the common interface of a post-processing task.
/// It is therefore the parent class of any post-processing task.
/// It is responsible for retrieving, processing and storing the data, mainly from and to QC repository.
///
/// \author Piotr Konopka
class PostProcessingInterface
{
 public:
  PostProcessingInterface() = default;
  virtual ~PostProcessingInterface() = default;

  // can be overridden if user wants to retrieve configuration of the task
  virtual void configure(std::string name, configuration::ConfigurationInterface& config);
  // user gets to know what triggered the init
  virtual void initialize(Trigger, framework::ServiceRegistry&) = 0;
  // user gets to know what triggered the update
  virtual void update(Trigger, framework::ServiceRegistry&) = 0;
  // user gets to know what triggered the end
  virtual void finalize(Trigger, framework::ServiceRegistry&) = 0;

  // todo: ccdb api which does not allow to delete?

  void setName(const std::string& name);
  std::string getName() const;

 private:
  std::string mName;
};

} // namespace o2::quality_control::postprocessing

#endif //QUALITYCONTROL_POSTPROCESSINTERFACE_H
