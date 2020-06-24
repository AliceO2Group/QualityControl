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
/// \file   MonitorObjectCollection.h
/// \author Piotr Konopka
///

#ifndef QUALITYCONTROL_MONITOROBJECTCOLLECTION_H
#define QUALITYCONTROL_MONITOROBJECTCOLLECTION_H

#include <TObjArray.h>
#include <Mergers/MergeInterface.h>

namespace o2::quality_control::core
{

class MonitorObjectCollection : public TObjArray, public mergers::MergeInterface
{
 public:
  MonitorObjectCollection() = default;
  ~MonitorObjectCollection() = default;

  void merge(mergers::MergeInterface* const other) override;
  ClassDefOverride(MonitorObjectCollection, 0);
};

} // namespace o2::quality_control::core

#endif //QUALITYCONTROL_MONITOROBJECTCOLLECTION_H
