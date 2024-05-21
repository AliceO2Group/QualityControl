#ifndef QC_MODULE_ITS_ITSHELPERS_H
#define QC_MODULE_ITS_ITSHELPERS_H

#include "QualityControl/DatabaseInterface.h"
#include "QualityControl/Quality.h"
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TText.h>
#include <algorithm> // For std::rotate

namespace o2::quality_control_modules::its
{
template <typename T>
std::vector<T> convertToArray(std::string input)
{

  std::istringstream ss{ input };

  std::vector<T> result;
  std::string token;
  while (std::getline(ss, token, ',')) {

    if constexpr (std::is_same_v<T, int>) {
      result.push_back(std::stoi(token));
    } else if constexpr (std::is_same_v<T, float>) {
      result.push_back(std::strtof(token.c_str(), NULL));
    } else if constexpr (std::is_same_v<T, std::string>) {
      result.push_back(token);
    }
  }

  return result;
}


} // namespace o2::quality_control_modules::its

class Stack {
private:
    std::vector<std::vector<int>> stack;
    int size_max;
    int current_element_id;

public:
    Stack() : size_max(0), current_element_id(0) {}

    Stack(int nSizeY, int nSizeX) : size_max(nSizeY), current_element_id(0) {
        stack.resize(nSizeY, std::vector<int>(nSizeX, 0));
    }

    void push(const std::vector<int>& element) {
        if (current_element_id < size_max) {
            stack[current_element_id] = element;
            current_element_id++;
        } else {
            std::rotate(stack.begin(), stack.begin() + 1, stack.end());
            stack.back() = element;
        }
        print();
    }

    void print() const {
        std::cout << "Printing stack:\n";
        for (const auto& vec_row : stack) {
            std::cout << "row: ";
            for (int item : vec_row) {
                std::cout << item << " ";
            }
            std::cout << "\n";
        }
    }
};

#endif
