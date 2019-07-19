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
/// \file   ExampleCondition.h
/// \author Piotr Konopka
///

#ifndef QC_MODULE_EXAMPLE_EXAMPLECONDITION_H
#define QC_MODULE_EXAMPLE_EXAMPLECONDITION_H

#include "QualityControl/TaskInterface.h"
#include <Framework/DataSamplingCondition.h>
#include <TROOT.h>

namespace o2::quality_control_modules::example
{

/// \brief Example of custom Data Sampling Condition
/// \author Piotr Konopka

/// \brief A DataSamplingCondition which approves messages which have their first byte in the payload higher than
/// specified value.
class ExampleCondition : public o2::framework::DataSamplingCondition
{
 public:
  /// \brief Constructor.
  ExampleCondition() : DataSamplingCondition(){};
  /// \brief Default destructor
  ~ExampleCondition() override = default;

  /// \brief Reads 'threshold'
  void configure(const boost::property_tree::ptree& config) override;
  /// \brief Makes a positive decision if first byte is higher than 'threshold'
  bool decide(const o2::framework::DataRef& dataRef) override;

 private:
  uint8_t mThreshold;
};

} // namespace o2::quality_control_modules::example

#endif //QC_MODULE_EXAMPLE_EXAMPLECONDITION_H
