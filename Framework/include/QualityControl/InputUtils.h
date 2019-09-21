#ifndef QC_INPUT_UTILS_H
#define QC_INPUT_UTILS_H

// std
#include <vector>
#include <string>
//o2
#include <Framework/DataProcessorSpec.h>
#include <Framework/DataSpecUtils.h>

inline std::vector<std::string> stringifyInput(o2::framework::Inputs &inputs){
  std::vector<std::string> vec;
  for (auto& input: inputs){
    vec.push_back(o2::framework::DataSpecUtils::describe(input));
  }
  return vec;
} 

#endif
