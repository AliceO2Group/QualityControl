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
/// \file    runFileMerger.cxx
/// \author  Piotr Konopka
///
/// \brief This is an executable which reads MonitorObjectCollections from files and creates a file with the merged result.

#include "QualityControl/QcInfoLogger.h"
#include "QualityControl/MonitorObject.h"
#include "QualityControl/MonitorObjectCollection.h"

#include <string>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <TFile.h>
#include <TKey.h>
#include <TGrid.h>

namespace bpo = boost::program_options;
using namespace o2::quality_control::core;

int main(int argc, const char* argv[])
{
  size_t filesRead = 0;
  try {
    bpo::options_description desc{ "Options" };
    desc.add_options()                                                                                                                                                         //
      ("help,h", "Help message")                                                                                                                                               //
      ("enable-alien", bpo::bool_switch()->default_value(false), "Connect to alien before accessing input files.")                                                             //
      ("exit-on-error", bpo::bool_switch()->default_value(false), "Makes the executable exit if any of the input files could not be read.")                                    //
      ("output-file", bpo::value<std::string>()->default_value("merged.root"), "File path to store the merged results, if the file exists, it will be merged with new files.") //
      ("input-files-list", bpo::value<std::string>()->default_value(""), "Path to a file containing a list of input files (row by row)")                                       //
      ("input-files", bpo::value<std::vector<std::string>>()->composing(),
       "Space-separated file paths which should be merged.");

    bpo::positional_options_description positionalArgs;
    positionalArgs.add("input-files", -1);

    bpo::variables_map vm;
    store(bpo::command_line_parser(argc, argv).options(desc).positional(positionalArgs).run(), vm);
    notify(vm);

    QcInfoLogger::setFacility("runFileMerger");

    if (vm.count("help")) {
      ILOG(Info, Support) << desc << ENDM;
      return 0;
    } else if (vm.count("input-files") > 0 && !vm["input-files-list"].as<std::string>().empty()) {
      ILOG(Error, Support) << "One should use either --input-files-list or --input-files, but not both." << ENDM;
      return 1;
    } else if (vm.count("input-files") == 0 && vm["input-files-list"].as<std::string>().empty()) {
      ILOG(Error, Support) << "No input files were provided. Use either --input-files-list or --input-files." << ENDM;
      return 1;
    }

    if (vm.count("help")) {
      // no infologger here, because the message is too long.
      std::cout << desc << std::endl;
      return 0;
    }

    std::vector<std::string> inputFilePaths;
    if (vm.count("input-files") > 0) {
      inputFilePaths = vm["input-files"].as<std::vector<std::string>>();
    } else if (auto inputFileList = vm["input-files-list"].as<std::string>(); !inputFileList.empty()) {
      std::ifstream file(inputFileList);
      if (!file.is_open()) {
        ILOG(Error, Support) << "Could not open the file with input list: " << inputFileList << ENDM;
        return 1;
      }
      for (std::string line; std::getline(file, line);) {
        inputFilePaths.push_back(line);
      }
    }

    if (vm["enable-alien"].as<bool>()) {
      ILOG(Info, Support) << "Connecting to alien" << ENDM;
      TGrid::Connect("alien:");
    }

    auto handleError = vm["exit-on-error"].as<bool>()
                         ? [](const std::string& message) { throw std::runtime_error(message); }
                         : [](const std::string& message) { ILOG(Error, Support) << message << ENDM; };

    auto outputFilePath = vm["output-file"].as<std::string>();
    auto outputFile = new TFile(outputFilePath.c_str(), "UPDATE");
    if (outputFile->IsZombie()) {
      throw std::runtime_error("File '" + outputFilePath + "' is zombie.");
    }
    if (!outputFile->IsOpen()) {
      throw std::runtime_error("Failed to open the file: " + outputFilePath);
    }
    if (!outputFile->IsWritable()) {
      throw std::runtime_error("File '" + outputFilePath + "' is not writable.");
    }
    ILOG(Debug) << "Output file '" << outputFilePath << "' successfully open." << ENDM;

    std::unordered_map<std::string, MonitorObjectCollection*> mergedMocMap;
    for (const auto& inputFilePath : inputFilePaths) {
      auto* file = TFile::Open(inputFilePath.c_str(), "READ");
      if (file == nullptr) {
        handleError("File handler for '" + inputFilePath + "' is nullptr.");
        continue;
      }
      if (file->IsZombie()) {
        handleError("File '" + inputFilePath + "' is zombie.");
        continue;
      }
      if (!file->IsOpen()) {
        handleError("Failed to open the file: " + inputFilePath);
        continue;
      }
      ILOG(Debug) << "Input file '" << inputFilePath << "' successfully open." << ENDM;

      TIter next(file->GetListOfKeys());
      TKey* key;
      while ((key = (TKey*)next())) {
        auto inputTObj = file->Get(key->GetName());
        if (inputTObj != nullptr) {
          auto inputMOC = dynamic_cast<MonitorObjectCollection*>(inputTObj);
          if (inputMOC == nullptr) {
            handleError("Could not cast the input object to MonitorObjectCollection.");
            delete inputTObj;
            continue;
          }

          inputMOC->postDeserialization();

          if (mergedMocMap.count(inputMOC->GetName()) == 0) {
            auto mergedTObj = outputFile->Get(key->GetName());
            if (mergedTObj != nullptr) {
              auto mergedMOC = dynamic_cast<MonitorObjectCollection*>(mergedTObj);
              if (mergedMOC == nullptr) {
                handleError("Could not cast the merged object to MonitorObjectCollection, skipping.");
                delete mergedMOC;
                continue;
              }
              mergedMOC->postDeserialization();
              mergedMocMap[mergedMOC->GetName()] = mergedMOC;
              ILOG(Info) << "Read merged object '" << mergedMOC->GetName() << "'" << ENDM;
            }
          }

          if (mergedMocMap.count(inputMOC->GetName())) {
            mergedMocMap[inputMOC->GetName()]->merge(inputMOC);
            delete inputMOC;
          } else {
            mergedMocMap[inputMOC->GetName()] = inputMOC;
          }
        }
      }
      file->Close();
      delete file;

      filesRead++;
    }

    for (auto& [mocName, moc] : mergedMocMap) {
      outputFile->WriteObject(moc, mocName.c_str(), "Overwrite");
      delete moc;
    }
    outputFile->Close();

  } catch (const bpo::error& ex) {
    ILOG(Error, Ops) << "Exception caught: " << ex.what() << ENDM;
    return 1;
  } catch (const boost::exception& ex) {
    ILOG(Error, Ops) << "Exception caught: " << boost::current_exception_diagnostic_information(true) << ENDM;
    return 1;
  }

  if (filesRead > 0) {
    ILOG(Info, Support) << "Successfully merged " << filesRead << " files into one." << ENDM;
  } else {
    ILOG(Info, Support) << "No files were merged." << ENDM;
  }
  return 0;
}