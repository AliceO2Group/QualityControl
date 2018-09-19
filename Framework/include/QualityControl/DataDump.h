// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
//

///
/// \author bvonhall
/// \file DataDump.h
///


#ifndef QC_CORE_DATADUMP_H
#define QC_CORE_DATADUMP_H

#include "FairMQDevice.h"
#include <boost/asio.hpp>

namespace o2
{
namespace quality_control
{
namespace core
{

struct Chunk
{
  size_t size;
  char* data;

  Chunk()
  {
    size = 0;
    data = nullptr;
  }
};

struct GUIState
{
  GUIState()
  {
    newDataAvailable = false;
  }

  bool newDataAvailable;
  std::string actionMessage;
  std::string dataAvailableMessage;
  Chunk current_payload;
  Chunk next_payload;
  Chunk current_header;
  Chunk next_header;
};

class DataDump : public FairMQDevice
{
 public:
  DataDump();
  virtual ~DataDump();

  static GUIState guiState;
  static void* window;

 protected:
  void InitTask() override;
  bool ConditionalRun() override;
  bool handleParts(FairMQParts& parts);

 private:
  void assignDataToChunk(void* data, size_t size, Chunk& chunk);
//  void test();
  int counter;
//  bool newDataAvailable;
};

}
}
}

#endif //QC_CORE_DATADUMP_H
