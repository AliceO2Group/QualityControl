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
/// \file   DataProducerExample.cxx
/// \author Barthelemy von Haller
///
#include "QualityControl/DataProducerExample.h"

#include <random>
#include <Common/Timer.h>

using namespace o2::framework;

using SubSpec = o2::header::DataHeader::SubSpecificationType;
using namespace AliceO2::Common;

namespace o2::quality_control::core
{

DataProcessorSpec getDataProducerExampleSpec(size_t myParam)
{
  return DataProcessorSpec{
    "producer",
    Inputs{},
    Outputs{
      { { "out" }, "TST", "RAWDATA", static_cast<SubSpec>(0) } },
    getDataProducerExampleAlgorithm({ "TST", "RAWDATA", static_cast<SubSpec>(0) }, myParam)
  };
}

framework::AlgorithmSpec
  getDataProducerExampleAlgorithm(framework::ConcreteDataMatcher output, size_t myParam)
{
  return AlgorithmSpec{
    [=](InitContext&) {
      return [=](ProcessingContext& processingContext) mutable {
        // everything inside this lambda function is invoked in a loop, because this Data Processor has no inputs

        // generating data (size 1, type size_t)
        auto data = processingContext.outputs().make<size_t>({ output.origin, output.description, output.subSpec },
                                                             1);
        data[0] = myParam; // assigning the data
      };
    }
  };
}

} // namespace o2::quality_control::core
