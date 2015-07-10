//
// Created by flpprotodev on 7/10/15.
//

#include "BasicTask.h"

#include <iostream>

using namespace std;

namespace AliceO2 {
namespace QualityControl {
namespace Core {

BasicTask::BasicTask()
{
}

BasicTask::~BasicTask()
{
}

void BasicTask::initialize()
{
  cout << "initialize" << endl;
}

void BasicTask::startOfActivity(Activity &activity)
{
  cout << "startOfActivity" << endl;
}

void BasicTask::startOfCycle()
{
  cout << "startOfCycle" << endl;
}

void BasicTask::monitorDataBlock(DataBlock &block)
{
  cout << "monitorDataBlock" << endl;
}

void BasicTask::endOfCycle()
{
  cout << "endOfCycle" << endl;
}

void BasicTask::endOfActivity(Activity &activity)
{
  cout << "endOfActivity" << endl;
}

void BasicTask::Reset()
{
  cout << "Reset" << endl;
}

}
}
}