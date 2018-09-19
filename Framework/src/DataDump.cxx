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
/// \file DataDump.cxx
///

#include "QualityControl/DataDump.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "FairMQLogger.h"
#include <options/FairMQProgOptions.h>
#include <QualityControl/DataDump.h>
#include "imgui/imgui.h"
#include <Framework/DebugGUI.h>
#include "imgui/BaseGui.h"
#include <Common/DataBlock.h>
#include <Headers/DataHeader.h>

using namespace std;
namespace pt = boost::property_tree;
using namespace o2::framework;

namespace o2 {
namespace quality_control {
namespace core {

GUIState DataDump::guiState;
void* DataDump::window = nullptr;

DataDump::DataDump() : counter(0)
{
}

DataDump::~DataDump()
{
}

vector<string> getBinRepresentation(unsigned char* data, size_t size)
{
  stringstream ss;
  vector<string> result;
  result.reserve(size);

  for (int i = 0; i < size; i++) {
    std::bitset<16> x(data[i]);
    ss << x << " ";
    result.push_back(ss.str());
    ss.str(std::string());
  }
  return result;
}

vector<string> getHexRepresentation(unsigned char* data, size_t size)
{
  stringstream ss;
  vector<string> result;
  result.reserve(size);
  ss << std::hex << std::setfill('0');

  for (int i = 0; i < size; i++) {
    ss << std::setw(2) << static_cast<unsigned>(data[i]) << " ";
    result.push_back(ss.str());
    ss.str(std::string());
  }
  return result;
}

void DataDump::InitTask()
{
  window = initGUI("O2 Data Inspector");
}

void updateGuiState()
{
  if (ImGui::Button("Next")) {
    // update view with latest data if any available
    if (DataDump::guiState.newDataAvailable) {
      // do stuff : delete old data if any, move pointer next -> current
      if (DataDump::guiState.current_payload.data != nullptr) {
        delete DataDump::guiState.current_payload.data;
        DataDump::guiState.current_payload.data = nullptr;
        DataDump::guiState.current_payload.size = 0;
        DataDump::guiState.current_header.data = nullptr;
        DataDump::guiState.current_header.size = 0;
      }
      DataDump::guiState.current_payload.data = DataDump::guiState.next_payload.data;
      DataDump::guiState.current_payload.size = DataDump::guiState.next_payload.size;
      DataDump::guiState.current_header.data = DataDump::guiState.next_header.data;
      DataDump::guiState.current_header.size = DataDump::guiState.next_header.size;
      DataDump::guiState.actionMessage = "";
    } else {
      DataDump::guiState.actionMessage = "";
    }
  } else {
    if (DataDump::guiState.newDataAvailable) {
      DataDump::guiState.dataAvailableMessage = "";
    } else {
      DataDump::guiState.dataAvailableMessage = "No data available.";
    }
  }
  if (DataDump::guiState.dataAvailableMessage.length() > 0) {
    ImGui::TextUnformatted(DataDump::guiState.dataAvailableMessage.c_str());
  }
  if (DataDump::guiState.actionMessage.length() > 0) {
    ImGui::TextUnformatted(DataDump::guiState.actionMessage.c_str());
  }
}

void updatePayloadGui()
{
  if (DataDump::guiState.current_payload.data == nullptr) {
    ImGui::Text("No data loaded yet, click Next.");
  } else { // all the stuff below should go to a method

    static int representation = 0, old_representation = 0;
    old_representation = representation;
    ImGui::RadioButton("hexadecimal", &representation, 0);
    ImGui::SameLine();
    ImGui::RadioButton("binary", &representation, 1);

    // scrollable area
    ImGui::SetNextWindowContentSize(ImVec2(ImGui::GetWindowContentRegionWidth(), 0.0f));
    ImGui::BeginChild("##ScrollingRegion", ImVec2(0, ImGui::GetFontSize() * 25), false);
    // table
    ImGui::Columns(5, "payload_display", true);

    // header row
    ImGui::Separator();
    static bool once = false;
    if(!once || representation != old_representation) {
      if(representation == 1) { // binary
        ImGui::SetColumnWidth(0, 40.0f);
        ImGui::SetColumnWidth(1, 243.0f);
        ImGui::SetColumnWidth(2, 243.0f);
        ImGui::SetColumnWidth(3, 243.0f);
        ImGui::SetColumnWidth(4, 243.0f);
      } else if (representation == 0) {
        ImGui::SetColumnWidth(0, 40.0f);
        ImGui::SetColumnWidth(1, 50.0f);
        ImGui::SetColumnWidth(2, 50.0f);
        ImGui::SetColumnWidth(3, 50.0f);
        ImGui::SetColumnWidth(4, 50.0f);
      }
      once = true;
    }
    ImGui::Text("");
    ImGui::NextColumn();
    ImGui::Text("#1");
    ImGui::NextColumn();
    ImGui::Text("#2");
    ImGui::NextColumn();
    ImGui::Text("#3");
    ImGui::NextColumn();
    ImGui::Text("#4");
    ImGui::NextColumn();
    ImGui::Separator();

    // print the hex values in the columns and raws of the table
    vector<string> formattedData =
      (representation == 0)
        ? getHexRepresentation(DataDump::guiState.current_payload.data, DataDump::guiState.current_payload.size)
        : getBinRepresentation(DataDump::guiState.current_payload.data, DataDump::guiState.current_payload.size);
    int line = 0;
    static int selected = -1;
    for (unsigned long pos = 0; pos < formattedData.size();) {
      char label[32];
      sprintf(label, "%04d", line * 4);
      if (ImGui::Selectable(label, selected == line, ImGuiSelectableFlags_SpanAllColumns)) {
        selected = line;
      }
      for (int i = 0; i < 4; i++) { // four columns
        ImGui::NextColumn();
        string temp;
        for (int j = 0; j < 2; j++) { // 2 x 16 bits words (char)
          if (pos < formattedData.size()) {
            temp += formattedData[pos];
            pos++;
          }
        }
        ImGui::TextUnformatted(temp.c_str());
      }
      ImGui::NextColumn();
      line++;
    }

    // footer
    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::Separator();
  }
}

void updateHeaderGui()
{
  if (DataDump::guiState.current_header.data == nullptr) {
    ImGui::Text("No data loaded yet, click Next.");
  } else { // all the stuff below should go to a method
    auto* header = header::get<header::DataHeader*>(DataDump::guiState.current_header.data);
    ImGui::Text("sMagicString : %d", o2::header::DataHeader::sMagicString);
    ImGui::Text("sVersion : %d", o2::header::DataHeader::sVersion);
    ImGui::Text("sHeaderType : %s", o2::header::DataHeader::sHeaderType.as<string>().c_str());
    ImGui::Text("sSerializationMethod : %s", o2::header::DataHeader::sSerializationMethod.as<string>().c_str());

    ImGui::Text("Header size : %d", header->headerSize);
    ImGui::Text("Payload size : %ld", header->payloadSize);
    ImGui::Text("Header version : %d", header->headerVersion);
    ImGui::Text("flagsNextHeader : %d", header->flagsNextHeader);
    ImGui::Text("description : %s", header->description.as<string>().c_str());
    ImGui::Text("serialization : %s", header->serialization.as<string>().c_str());

    // TODO add the next headers (how to "discover" what headers is there ? )
  }
}

void redrawGui()
{
  ImGui::Begin("DataDump");
  if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen)) {
    updateGuiState();
  }

  if (ImGui::CollapsingHeader("Header", ImGuiTreeNodeFlags_DefaultOpen)) {
    updateHeaderGui();
  }

  if (ImGui::CollapsingHeader("Payload", ImGuiTreeNodeFlags_DefaultOpen)) {
    updatePayloadGui();
  }
  ImGui::End();

  ImGui::ShowTestWindow();
}

bool DataDump::ConditionalRun()
{
  unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

  FairMQParts parts;
  auto result = fChannels.at("data-in").at(0).ReceiveAsync(parts);
  if (result > 0) {
    this->handleParts(parts);
  }

  return pollGUI(window, redrawGui);
}

bool DataDump::handleParts(FairMQParts& parts)
{
  if (parts.Size() % 2 != 0) {
    cout << "number of parts must be a multiple of 2" << endl;
    return false;
  }

  DataDump::guiState.newDataAvailable = true;
  assignDataToChunk(parts.At(0)->GetData(), parts.At(0)->GetSize(), DataDump::guiState.next_header);
  assignDataToChunk(parts.At(1)->GetData(), parts.At(1)->GetSize(), DataDump::guiState.next_payload);
  return true;
}

void DataDump::assignDataToChunk(void* data, size_t size, Chunk& chunk)
{
  auto* copy = new unsigned char[size];
  memcpy(copy, data, size);
  chunk.data = copy;
  chunk.size = size;
}
}
}
}