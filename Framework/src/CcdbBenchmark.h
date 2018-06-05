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
/// \file   CcdbBenchmark.h
/// \author Barthelemy von Haller
///

#ifndef PROJECT_CCDBBENCHMARK_H
#define PROJECT_CCDBBENCHMARK_H

#include "FairMQDevice.h"
#include "QualityControl/CcdbDatabase.h"
#include <TH1F.h>

namespace o2 {
namespace quality_control {
namespace core {

class CcdbBenchmark : public FairMQDevice
{
  public:
    CcdbBenchmark();
    virtual ~CcdbBenchmark();

  protected:
    virtual void InitTask();
    virtual bool ConditionalRun();
    void emptyDatabase();

  private:
    uint64_t mMaxIterations;
    uint64_t mNumIterations;
    uint64_t mNumberObjects;
    uint64_t mSizeObjects;
    std::string mTaskName;
    std::string mObjectName;
    bool mDeletionMode;
    o2::quality_control::repository::CcdbDatabase* mDatabase;
    MonitorObject *mMyObject;
    TH1 *mMyHisto;
};

}
}
}

#endif //PROJECT_CCDBBENCHMARK_H
