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
/// \file   HistoProducer.h
/// \author Barthelemy von Haller
///

#ifndef QUALITYCONTROL_HISTOPRODUCER_H
#define QUALITYCONTROL_HISTOPRODUCER_H

#include <Framework/DataProcessorSpec.h>

namespace o2::quality_control::core
{

/// \brief Returns an histogram producer specification which publishes on {"TST", "HISTO", <index>}.
///
/// \param index    The index of this producer (i.e. the subspec).
/// \return         An histograms producer specification.
framework::DataProcessorSpec getHistoProducerSpec(size_t index, size_t nbHistograms, bool noTobjArray);

/// \brief Returns an algorithm generating histograms randomly filled.
/// The histograms have 100 bins and are named `hello<index>`.
/// The histograms are embedded in a TObjArray.
///
/// \param output   Origin, Description and SubSpecification of data to be produced
/// \param index    The value the producer should produce.
/// \return         An histogram producer algorithm
framework::AlgorithmSpec getHistoProducerAlgorithm(framework::ConcreteDataMatcher output, size_t nbHistograms, bool noTobjArray);

/// \brief Returns a printer that prints histograms coming from {"TST", "HISTO", <index>}
///
/// \param index  The index of the producer (i.e. the subspec) to which the printer must connect.
/// \return       A printer.
framework::DataProcessorSpec getHistoPrinterSpec(size_t index);

/// \brief  Returns an algorithm printing histograms.
///
/// \return An algorithm printing histograms.
framework::AlgorithmSpec getHistoPrinterAlgorithm();

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_HISTOPRODUCER_H
