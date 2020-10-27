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
/// \file DataDumpGui.cxx
///

#include "QualityControl/DataDumpGui.h"
#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/stringUtils.h"
#include "imgui/BaseGui.h"
#include "imgui/imgui.h"
#include <Headers/DataHeader.h>
#include <iomanip>

using namespace std;
using namespace o2::framework;

namespace o2::quality_control::core
{

GUIState DataDumpGui::guiState;
void* DataDumpGui::window = nullptr;

void DataDumpGui::InitTask() { window = initGUI("O2 Data Inspector"); }

void updateGuiState()
{
  if (ImGui::Button("Next")) {
    // update view with latest data if any available
    if (DataDumpGui::guiState.newDataAvailable) {
      // do stuff : delete old data if any, move pointer next -> current
      if (DataDumpGui::guiState.current_payload.data != nullptr) {
        delete DataDumpGui::guiState.current_payload.data;
        DataDumpGui::guiState.current_payload.data = nullptr;
        DataDumpGui::guiState.current_payload.size = 0;
        DataDumpGui::guiState.current_header.data = nullptr;
        DataDumpGui::guiState.current_header.size = 0;
      }
      DataDumpGui::guiState.current_payload.data = DataDumpGui::guiState.next_payload.data;
      DataDumpGui::guiState.current_payload.size = DataDumpGui::guiState.next_payload.size;
      DataDumpGui::guiState.current_header.data = DataDumpGui::guiState.next_header.data;
      DataDumpGui::guiState.current_header.size = DataDumpGui::guiState.next_header.size;
      DataDumpGui::guiState.actionMessage = "";
    } else {
      DataDumpGui::guiState.actionMessage = "";
    }
  } else {
    if (DataDumpGui::guiState.newDataAvailable) {
      DataDumpGui::guiState.dataAvailableMessage = "";
    } else {
      DataDumpGui::guiState.dataAvailableMessage = "No data available.";
    }
  }
  if (DataDumpGui::guiState.dataAvailableMessage.length() > 0) {
    ImGui::TextUnformatted(DataDumpGui::guiState.dataAvailableMessage.c_str());
  }
  if (DataDumpGui::guiState.actionMessage.length() > 0) {
    ImGui::TextUnformatted(DataDumpGui::guiState.actionMessage.c_str());
  }
}

void resizeColumns(int representation /*, int old_representation*/)
{
  //  static bool firstDrawColumns = true;
  //  if(firstDrawColumns || representation != old_representation) {
  if (representation == 0) {
    ImGui::SetColumnWidth(0, 40.0f);
    ImGui::SetColumnWidth(1, 50.0f);
    ImGui::SetColumnWidth(2, 50.0f);
    ImGui::SetColumnWidth(3, 50.0f);
    ImGui::SetColumnWidth(4, 50.0f);
  } else if (representation == 1) { // binary
    ImGui::SetColumnWidth(0, 40.0f);
    ImGui::SetColumnWidth(1, 243.0f);
    ImGui::SetColumnWidth(2, 243.0f);
    ImGui::SetColumnWidth(3, 243.0f);
    ImGui::SetColumnWidth(4, 243.0f);
  }
  //  }
  //  firstDrawColumns = false;
}

void updatePayloadGui()
{
  if (DataDumpGui::guiState.current_payload.data == nullptr) {
    ImGui::Text("No data loaded yet, click Next.");
  } else { // all the stuff below should go to a method

    static int representation = 0 /*, old_representation = 1*/;
    //    old_representation = representation;
    ImGui::RadioButton("hexadecimal", &representation, 0);
    ImGui::SameLine();
    ImGui::RadioButton("binary", &representation, 1);

    // scrollable area
    ImGui::BeginChild("##ScrollingRegion", ImVec2(0, 430), false, ImGuiWindowFlags_HorizontalScrollbar);
    // table
    ImGui::Columns(5, "payload_display", true);

    // header row
    ImGui::Separator();
    resizeColumns(representation /*, old_representation*/);
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

    // print the hex/bin values in the columns and rows of the table
    vector<string> formattedData =
      (representation == 0)
        ? getHexRepresentation(DataDumpGui::guiState.current_payload.data, DataDumpGui::guiState.current_payload.size)
        : getBinRepresentation(DataDumpGui::guiState.current_payload.data, DataDumpGui::guiState.current_payload.size);
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
  if (DataDumpGui::guiState.current_header.data == nullptr) {
    ImGui::Text("No data loaded yet, click Next.");
  } else {
    auto* header = header::get<header::DataHeader*>(DataDumpGui::guiState.current_header.data);
    if (header == nullptr) {
      ImGui::Text("No header available in this data.");
      return;
    }

    ImGui::BeginChild(
      "Static", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, ImGui::GetTextLineHeightWithSpacing() * 4), false);
    ImGui::Text("sMagicString : %d", o2::header::DataHeader::sMagicString);
    ImGui::Text("sVersion : %d", o2::header::DataHeader::sVersion);
    ImGui::Text("sHeaderType : %s", o2::header::DataHeader::sHeaderType.as<string>().c_str());
    ImGui::Text("sSerializationMethod : %s", o2::header::DataHeader::sSerializationMethod.as<string>().c_str());
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("Non-static",
                      ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, ImGui::GetTextLineHeightWithSpacing() * 7),
                      false);
    ImGui::Text("Header size : %d", header->headerSize);
    ImGui::Text("Payload size : %llu", static_cast<unsigned long long int>(header->payloadSize));
    ImGui::Text("Header version : %d", header->headerVersion);
    ImGui::Text("flagsNextHeader : %d", header->flagsNextHeader);
    ImGui::Text("dataDescription : %s", header->dataDescription.str);
    ImGui::Text("dataOrigin : %s", header->dataOrigin.str);
    ImGui::Text("serialization : %s", header->serialization.as<string>().c_str());
    ImGui::EndChild();
    // TODO add the next headers (how to "discover" what headers is there ? )
  }
}

void redrawGui()
{
  static bool firstDraw = true;

  if (firstDraw) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(1100, 700));
  }

  ImGui::Begin("DataDumpGui", nullptr, ImGuiWindowFlags_NoTitleBar);
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

  //  ImGui::ShowTestWindow();

  firstDraw = false;
}

bool DataDumpGui::ConditionalRun()
{
  unique_ptr<FairMQMessage> msg(fTransportFactory->CreateMessage());

  FairMQParts parts;
  auto result = fChannels.at("data-in").at(0).Receive(parts, 0);
  if (result > 0) {
    this->handleParts(parts);
  }

  return pollGUI(window, redrawGui);
}

bool DataDumpGui::handleParts(FairMQParts& parts)
{
  if (parts.Size() != 2) {
    ILOG(Warning, Support) << "number of parts must be 2" << ENDM;
    return false;
  }

  DataDumpGui::guiState.newDataAvailable = true;
  assignDataToChunk(parts.At(0)->GetData(), parts.At(0)->GetSize(), DataDumpGui::guiState.next_header);
  assignDataToChunk(parts.At(1)->GetData(), parts.At(1)->GetSize(), DataDumpGui::guiState.next_payload);
  return true;
}

void DataDumpGui::assignDataToChunk(void* data, size_t size, Chunk& chunk)
{
  auto* copy = new unsigned char[size];
  memcpy(copy, data, size);
  chunk.data = copy;
  chunk.size = size;
}
} // namespace o2::quality_control::core