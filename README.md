## Requirements

A CC7 machine or a mac.

## Setup

We use alibuild, see instructions [here](https://alisw.github.io/alibuild/o2-daq-tutorial.html).

### Post installation

Start and populate database :

```
sudo systemctl start mariadb
qcDatabaseSetup.sh
```

## Configuration

The QC requires a number of configuration items. An example config file is
provided in the repo under the name _example-default.ini_. Moreover, the
outgoing channel over which histograms are sent is defined in a JSON
file such as the one called _alfa.json_ in the repo.

**QC tasks** must be defined in the configuration :

```
[myTask_1]                      # define parameters for task myTask_1
taskDefinition=taskDefinition_1 # indirection to the actual definition
```

We use an indirect system to allow multiple tasks to share
most of their definition.

```
[taskDefinition_1]
className=AliceO2::QualityControlModules::Example::ExampleTask
moduleName=QcExample     # which library contains the class
cycleDurationSeconds=1
```

The source of the data is defined here :

```
[DataSampling]
;implementation=FairSampler # get data from readout
implementation=MockSampler  # get random data
```

The JSON `alfa.json` file contains a typical FairMQ device definition. One can
 change the or the address there:
```
{
    "fairMQOptions": {
        "devices": [
            {
                "id": "myTask_1",
                "channels": [
                    {
                        "name": "data-out",
                        "sockets": [
                            {
                                "type": "pub",
                                "method": "bind",
                                "address": "tcp://*:5556",
                                "sndBufSize": 100,
                                "rcvBufSize": 100,
                                "rateLogging": 0
                            }
                        ]
                    }
                ]
            }
        ]
    }
}
```

**QC checkers** are defined in the config file :

```
[checker_0]
broadcast=0 # whether to broadcast the plots in addition to storing them
broadcastAddress=tcp://*:5600
id=0        # only required item
```

And for the time, the mapping between checkers and tasks must be explicit :

```
[checkers] ; needed for the time being because we don't have an information service
numberCheckers=1
; can be less than the number of items in tasksAddresses
numberTasks=1
tasksAddresses=tcp://localhost:5556,tcp://localhost:5557,tcp://localhost:5558,tcp://localhost:5559
```

There are configuration items for many other aspects, for example the
database connection, the monitoring or the data sampling.

## Usage

### Basic
With the default config files provided, we run a qctask that processes
random data and forwards it to a GUI and a checker that will check and
store the MonitorObjects.

**Note the prefix "file://" for the config file.**

```
# Launch a task named myTask_1
qcTaskLauncher -n myTask_1 \
               -c file:///absolute/path/to/example-default.ini \
               --id myTask_1 \
               --mq-config path/to/alfa.json

# Launch a gui to check what is being sent
# don't forget to click the button Start !
qcSpy

# Launch a checker named checker_0
qcCheckerLauncher -c file:///absolute/path/to/example-default.ini -n checker_0
```

### Readout chain

1. Modify the config file of readout (Readout/configDummy.cfg) :
```
[readout]
exitTimeout=-1
(...)
[sampling]
# enable/disable data sampling (1/0)
enabled=1
# which class of datasampling to use (FairInjector, MockInjector)
class=FairInjector
```
2. Start Readout
```
readout.exe file:///absolute/path/to/configDummy.cfg
```
3. Modify the config file of the QC (QualityControl/example-default.ini) :
```
[DataSampling]
implementation=FairSampler
```
4. Launch the QC task
```
qcTaskLauncher -n myTask_1 \
               -c file:///absolute/path/to/configDummy.cfg \
               --id myTask_1 \
               --mq-config path/to/alfa.json
```
5. Launch the checker
```
qcCheckerLauncher -c file:///absolute/path/to/example-default.ini -n checker_0
```
6. Launch the gui
```
qcSpy file:///absolute/path/to/example-default.ini
# then click Start !
```
In the gui, one can change the source and point to the checker by
changing the path in the field at the bottom of the window (first click
`Stop` and `Start` after filling in the details). The port of the checker
is 5600.
One can also check what is stored in the database by clicking `Stop`
and then switching to `Database` source. This will only work if a config
file was passed to the `qcSpy` utility.

### Information Service

The information service publishes information about the tasks currently
running and the objects they publish. It is needed by some GUIs, or
other clients.

By default it will publish on port 5561 the json description of a task
when it is updated. A client can also request on port 5562 the information
about a specific task or about all the tasks, by passing the name of the
task as a parameter or "all" respectively.

The JSON for a task looks like :
```
{
    "name": "myTask_1",
    "objects": [
        {
            "id": "array-0"
        },
        {
            "id": "array-1"
        },
        {
            "id": "array-2"
        },
        {
            "id": "array-3"
        },
        {
            "id": "array-4"
        }
    ]
}
```

The JSON for all tasks looks like :
```
{
    "tasks": [
        {
            "name": "myTask_1",
            "objects": [
                {
                    "id": "array-0"
                },
                {
                    "id": "array-1"
                }
            ]
        },
        {
            "name": "myTask_2",
            "objects": [
                {
                    "id": "array-0"
                },
                {
                    "id": "array-1"
                }
            ]
        }
    ]
}
```
#### Usage
```
qcInfoService -c /absolute/path/to/InformationService.json -n information_service \
              --id information_service --mq-config /absolute/path/to/InformationService.json
```

The `qcInfoService` can provide fake data from a file. This is useful
to test the clients. Use the option `--fake-data-file` and provide the
absolute path to the file. The file `infoServiceFake.json` is provided
as an example.

To check what is being output by the Information Service, one can
run the InformationServiceDump :
```
qcInfoServiceDump -c /absolute/path/to/InformationService.json -n information_service_dump \
                  --id information_service_dump --mq-config /absolute/path/to/InformationService.json
                  --request-task myTask1
```
The last parameter can be omitted to receive information about all tasks.

## Modules development

Steps to create a new module Abc

1. Duplicate the skeleton of module located in QualityControlModules/Skeleton.
2. Call it Abc
3. Use a (smart) editor to find and replace all occurrences of Skeleton by Abc
4. Make sure to rename the include/Skeleton to include/Abc as well
5. Rename all files (Skeleton->Abc)
6. Near the end of the file QualityControlModules/CMakeLists.txt add `add_subdirectory(Abc)`
7. Compile

From here, fill in the methods in AbcTask.cxx, and AbcCheck.cxx if needed.

In case special additional dependencies are needed, create a new bucket in QualityControlModules/cmake/QualityControlModulesDependencies.cmake.

## QC in Data Processing Layer framework

Quality Control has been adapted to be used as Data Processor in
[O2 framework](https://github.com/AliceO2Group/AliceO2/tree/dev/Framework/Core#data-processing-layer-in-o2-framework).
Keep in mind, that checkers are not available at this moment.
To add a QC task into workflow:

1. Create your module using SkeletonDPL as a base. Refer to the steps mentioned
in the chapter [Modules development](https://github.com/AliceO2Group/QualityControl#modules-development)
- they are the same.
2. Define input data and parameters of your QC Task in .ini config file. Use
[Framework/qcTaskDplConfig.ini](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/qcTaskDplConfig.ini)
as a reference - just update the variables in the section 'Tasks'.
3. Insert following lines in your workflow declaration code. Change the names
accordingly.
```
...

#include "QualityControl/TaskDataProcessorFactory.h"
#include "QualityControl/TaskDataProcessor.h"
#include "Framework/DataSampling.h"
#include "Framework/runDataProcessing.h"

...

void defineDataProcessing(std::vector<DataProcessorSpec>& specs) {

  ...

  // A path to your config .ini file. In this case, it is a file installed during compilation.
  const std::string qcConfigurationSource = std::string("file://") + getenv("QUALITYCONTROL_ROOT") + "/etc/qcTaskDplConfig.ini";
  // An entry in config file which describes your QC task
  const std::string qcTaskName = "skeletonTask";
  o2::quality_control::core::TaskDataProcessorFactory qcFactory;
  specs.push_back(qcFactory.create(qcTaskName, qcConfigurationSource));

  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);
}
```
4. Compile & run.

In [Framework/src/TaskDPL.cxx](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/src/TaskDPL.cxx)
there is an exemplary DPL workflow with QC task. It is compiled to an
executable `taskDPL`.

## TObject2Json

Single binary multi-threaded using QC data-sources (MySQL and CCDB) to expose ROOT objects as JSON over ZeroMQ.

### Usage

```bash
alienv enter QualityControl/latest
tobject2json --backend mysql://qc_user:qc_user@localhost/quality_control --zeromq-server tcp://127.0.0.1:7777 --workers 4
```

### Protocol

Request:

```
<agent>/<objectName>
```

Response (one of the following):

```
{"request": "<agent>/<objectName>", "payload": "<object requested in json>"}
{"request": "<agent>/<objectName>", "error": 400, "message": "Wrong request", "why": "because..."}
{"request": "<agent>/<objectName>", "error": 404, "message": "Object not found"}
{"request": "<agent>/<objectName>", "error": 500, "message": "Internal error", "why": "because..."}
```

### Architecture

```
+-----------+
|   main    |
+-----------+
      |
      |
      v
+-----------+    +-----------+
|  Server   |--->|  Factory  |
+-----------+    +-----------+
      ^ 1
      |
      v N
+-----------+    +-----------+
|  Worker   |<-->|  Backend  |
+-----------+1  N+-----------+
```
