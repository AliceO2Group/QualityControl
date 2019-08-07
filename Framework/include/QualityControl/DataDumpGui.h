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
/// \author Barthelemy von Haller
/// \file DataDumpGui.h
///

#ifndef QC_CORE_DATADUMP_H
#define QC_CORE_DATADUMP_H

#include <string>

#include <fairmq/FairMQDevice.h>

namespace o2::quality_control::core
{

/**
 * A chunk of data
 */
struct Chunk {
  size_t size;
  unsigned char* data;

  Chunk()
  {
    size = 0;
    data = nullptr;
  }
};

/**
 * Container for the state of the GUI.
 * As we use Imgui it is stateless and we have to keep the state ourselves.
 */
struct GUIState {
  GUIState() { newDataAvailable = false; }

  bool newDataAvailable;
  std::string actionMessage;
  std::string dataAvailableMessage;
  Chunk current_payload;
  Chunk next_payload;
  Chunk current_header;
  Chunk next_header;
};

/**
 * A GUI to display the header and the payload of events sent by the Data Sampling.
 */
class DataDumpGui : public FairMQDevice
{
 public:
  DataDumpGui() = default;
  virtual ~DataDumpGui() = default;

  static GUIState guiState;
  static void* window;

 protected:
  void InitTask() override;
  bool ConditionalRun() override;
  bool handleParts(FairMQParts& parts);

 private:
  static void assignDataToChunk(void* data, size_t size, Chunk& chunk);
};
} // namespace o2::quality_control::core

#endif // QC_CORE_DATADUMP_H
