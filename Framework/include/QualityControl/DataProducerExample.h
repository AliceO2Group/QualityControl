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
/// \file   DataProducerExample.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_DATAPRODUCEREXAMPLE_H
#define QUALITYCONTROL_DATAPRODUCEREXAMPLE_H

#include <Framework/DataProcessorSpec.h>

namespace o2::quality_control::core
{

/// \brief Returns a random data producer specification which publishes on {"TST", "RAWDATA", <index>}
///
/// \param myParam  The value the producer should produce
/// \return         A fixed number producer specification
framework::DataProcessorSpec getDataProducerExampleSpec(size_t myParam);

/// \brief Returns an algorithm generating random messages
///
/// \param output   Origin, Description and SubSpecification of data to be produced
/// \param  myParam  The value the producer should produce.
/// \return         A fixed number producer algorithm
framework::AlgorithmSpec getDataProducerExampleAlgorithm(framework::ConcreteDataMatcher output, size_t myParam);

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_DATAPRODUCEREXAMPLE_H
