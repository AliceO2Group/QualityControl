[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/dashboard/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [QuickStart](#quickstart)
      * [Requirements](#requirements)
      * [Setup](#setup)
         * [Environment loading](#environment-loading)
      * [Execution](#execution)
         * [Basic workflow](#basic-workflow)
         * [Readout chain](#readout-chain)
   * [Modules development](#modules-development)
      * [Concepts](#concepts)
      * [Module creation](#module-creation)
         * [Manual steps to create a new module Abc](#manual-steps-to-create-a-new-module-abc)
         * [Automatic modules generation](#automatic-modules-generation)
      * [DPL workflow customization](#dpl-workflow-customization)
   * [Advanced topics](#advanced-topics)
      * [Local CCDB setup](#local-ccdb-setup)
      * [Local QCG setup](#local-qcg-setup)
         * [TObject2Json](#tobject2json)
            * [Usage](#usage)
            * [Protocol](#protocol)
            * [Architecture](#architecture)
      * [Data Inspector](#data-inspector)
         * [Prerequisite](#prerequisite)
         * [Compilation](#compilation)
         * [Execution](#execution-1)
         * [Configuration](#configuration)
      * [Information Service](#information-service)
         * [Usage](#usage-1)
      * [Use MySQL as QC backend](#use-mysql-as-qc-backend)
      * [Configuration files details](#configuration-files-details)
   * [Questions for the refactoring of the README](#questions-for-the-refactoring-of-the-readme)

<!-- Added by: bvonhall, at:  -->

<!--te-->

# QuickStart

## Requirements

A Linux machine (CC7 or Ubuntu) or a Mac. See the O2 instructions below for the exact supported versions.

## Setup

2. Install O2
     * We use alibuild, see complete instructions [here](https://alice-doc.github.io/alice-analysis-tutorial/building/).

3. Prepare the QualityControl development package
    * `aliBuild init QualityControl@master`

4. Build/install the QualityControl, its GUI (qcg) and the readout. The simplest is to use the metapackage `flpproto`.
    * `aliBuild build flpproto --default o2`

Note :  you can also use the alibuild "defaults" called `o2-dataflow` to avoid building simulation related packages.

### Environment loading

Whenever you want to work with O2 and QualityControl, do either `alienv enter qcg/latest` or `alienv load qcg/latest`.

## Execution

To make sure that your system is correctly setup, we are going to run a basic QC workflow. We will use central services for the repository and the GUI. If you want to set them up on your computer or in your lab, please have a look [here](#local-ccdb-setup) and [here](#local-qcg-setup).

### Basic workflow

We will run a basic workflow described in the following schema.

![alt text](doc/images/basic-schema.png)

The _Producer_ is a random data generator. In a more realistic setup it would be a processing device or the _Readout_. The _Data Sampling_ is the system in charge of dispatching data samples from the main data flow to the _QC tasks_. It can be configured to dispatch different proportion or different types of data. The _Checker_ is in charge of evaluating the _MonitorObjects_ produced by the _QC tasks_, for example by checking that the mean is above a certain limit. It can also modify the aspect of the histogram, e.g. by changing the background color or adding a PaveText. Finally the _Checker_ is also in charge of storing the resulting _MonitorObject_ into the repository where it will be accessible by the web GUI. It also pushes it to a _Printer_ for the sake of this tutorial.

To run it simply do

    qcRunBasic

Thanks to the Data Processing Layer (DPL, more details later) it is a single process that steers all the _devices_, i.e. processes making up the workflow. A window should appear that shows a graphical representation of the workflow. The output of any of the processes is available by double clicking a box. If a box is red it means that the process has stopped, probably abnormaly.

![alt text](doc/images/basic-dpl-gui.png)

__Repository and GUI__

The data is stored in the [ccdb-test](ccdb-test.cern.ch:8080/browse) at CERN. If everything works fine you should see the objects being published in the QC web GUI (QCG) at this address : [https://qcg-test.cern.ch](https://qcg-test.cern.ch/?page=layoutShow&layoutId=5bb34a1d18a82bb283a487bd). The link actually brings you to a "layout" that shows the object (a histo titled "example") published by the task.

TODO add a link to the user documentation of the QCG

![alt text](doc/images/basic-qcg.png)

__Configuration file__

The devices are configured in the config file named `basic.json`. It is installed in `$QUALITY_CONTROL/etc`. Each time you rebuild the code, `$QUALITY_CONTROL/etc/basic.json` is overwritten by the file in the source directory (`~/alice/QualityControl/Framework/basic.json`).

### Readout chain

In this second example, we are going to use the Readout as data source.

![alt text](doc/images/readout-schema.png)

This workflow is a bit different from the basic one. The _Readout_ is not a device and thus we have to have a _proxy_ to get data from it. This is the extra box going to the dispatcher, which then injects data to the task. This is handled in the _Readout_ if you enable the corresponding configuration flag.

TODO make the qc task use the daq code

To do so, open the readout config file located at `$READOUT_ROOT/etc/readout.cfg` and make sure that the following properties are correct :

```
# First make sure we never exit
[readout]
(...)
exitTimeout=-1
(...)
# And enable the data sampling
[consumer-data-sampling]
consumerType=DataSampling
enabled=1
(...)
```

Start Readout :
```
readout.exe file://$READOUT_ROOT/etc/readout.cfg
```

Start the QC and DS (DataSampling) workflow :
```
qcRunReadout
```

The data sampling is configured to sample 1% of the data as the readout should run by default at full speed.

__Configuration file__

The configuration file is in `$QUALITY_CONTROL/readout.json`. It is installed in `$QUALITY_CONTROL/etc`. Each time you rebuild the code, `$QUALITY_CONTROL/etc/readout.json` is overwritten by the file in the source directory (`~/alice/QualityControl/Framework/readout.json`).

To change the fraction of the data being monitored, change the option `fraction`.

```
"fraction": "0.01",
```

# Modules development

Before developing a module, one should have a bare idea of what the QualityControl is and how it is designed.

## QC architecture

![alt text](doc/images/Architecture.png)

The main data flow is represented in blue. Data samples are selected by the Data Sampling (not represented) and sent to the QC tasks, either on the same machines or on other machines. The tasks produce TObjects, usually histograms, that are merged (if needed) and then checked. The checkers output the received TObject along with a quality flag. The TObject can be modified by the Checker. Finally the TObject and its quality are stored in the repository.

## DPL

TODO -> piotr

<!--

THIS IS OLD STUFF BELOW

Quality Control has been adapted to be used as Data Processor in
[O2 framework](https://github.com/AliceO2Group/AliceO2/tree/dev/Framework/Core#data-processing-layer-in-o2-framework).
Keep in mind, that checkers are not available at this moment.
To add a QC task into workflow:

1. Create your module using SkeletonDPL as a base. Refer to the steps mentioned
in the chapter [Modules development](https://github.com/AliceO2Group/QualityControl#modules-development),
they are the same.
2. Define input data and parameters of your QC Task in .json config file. Use
[Framework/qcTaskDplConfig.json](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/qcTaskDplConfig.json)
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

WorkflowSpec defineDataProcessing(ConfigContext const&) {

  std::vector<DataProcessorSpec> specs;
  ...

  // A path to your config .json file. In this case, it is a file installed during compilation.
  const std::string qcConfigurationSource = std::string("file://") + getenv("QUALITYCONTROL_ROOT") + "/etc/qcTaskDplConfig.json";
  // An entry in config file which describes your QC task
  const std::string qcTaskName = "skeletonTask";
  o2::quality_control::core::TaskDataProcessorFactory qcFactory;
  specs.push_back(qcFactory.create(qcTaskName, qcConfigurationSource));

  o2::framework::DataSampling::GenerateInfrastructure(specs, qcConfigurationSource);

  return specs;
}
```
4. Compile & run.

In [Framework/src/runTaskDPL.cxx](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/src/runTaskDPL.cxx)
there is an exemplary DPL workflow with QC task. It is compiled to an
executable `taskDPL`.

-->

## Data Sampling

TODO -> piotr

## Code Organization

The repository QualityControl contains the _Framework_  and the _Modules_ in the respectively named directories.

The Data Sampling code is part of the AliceO2 repository.

## User-defined modules

The Quality Control uses _plugins_ to load the actual code to be executed by the _Tasks_ and the _Checkers_. A module, or plugin, can contain one or several _Tasks_ and/or one or several _Checks_. They must subclass `TaskInterface.h` and `CheckInterface.h` respectively. We use the Template Method Design Pattern.

## Module creation

Before starting to develop the code, one should create a new module if it does not exist yet. Typically each detector team should prepare a module.

The script `modulesHelper.sh`, in the directory _Modules_, is able to prepare a new module or to add a new _Task_ or a new _Check_ to an existing module. It must be run from __within QualityControl/Modules__. See the help message below:
```
Usage: ./modulesHelper.sh -m MODULE_NAME [OPTION]

Generate template QC module and/or tasks, checks.
If a module with specified name already exists, new tasks and checks are inserted to the existing one.
Please follow UpperCamelCase convention for modules', tasks' and checks' names.

Example:
# create new module and some task
./modulesHelper.sh -m MyModule -t SuperTask
# add one task and two checks
./modulesHelper.sh -m MyModule -t EvenBetterTask -c HistoUniformityCheck -c MeanTest

Options:
 -h               print this message
 -m MODULE_NAME   create module named MODULE_NAME or add there some task/checker
 -t TASK_NAME     create task named TASK_NAME
 -c CHECK_NAME    create check named CHECK_NAME
```

For example, if your detector 3-letter code is ABC you might want to do
```
# we are in ~/alice
cd QualityControl/Modules
./modulesHelper.sh -m Abc # create the module
./modulesHelper.sh -t RawDataQcTask # add a task
```

## Test run

Now that there is a module, we can build it and test it. First let's build it :
```
# We are in ~/alice
# Go to the build directory of QualityControl
cd sw/slc7_x86-64/BUILD/QualityControl-latest/QualityControl
make -j8 install
```

To test whether it works, we are going to run a basic DPL workflow defined in `runBasic.cxx`.
We need to modify slightly the config file to indicate our freshly created module and classes.
The config file is called `basic.json` and is located in `$QUALITY_CONTROL/etc/` (after installation, if you want to modify the original one it is in the source directory `Framework`).
Change the lines as indicated below :

```
"QcTaskDefinition": {
  "className": "o2::quality_control_modules::abc::RawDataQcTask",
  "moduleName": "QcAbc",
```

Now we can run it

```
qcRunBasic
```

You should see the QcTask at qcg-test.cern.ch with an object `Example` updating.

## Modification of a Task

Fill in the methods in RawDataQcTask.cxx. For example, make it send a second histogram.
Once done, recompile it (see section above) and run it. You should see the second object published in the qcg.

TODO give actual steps for dummies

## Addition of a Check

TODO

## DPL workflow customization

If you want to change the workflow, edit or copy `runBasic.cxx`. For example...

TODO

## Commit Code

TODO Github procedure

---

# Advanced topics

## Local CCDB setup

TODO

## Local QCG setup

TODO : point to QCG readme or explain again ?

TODO if we reexplain : install qcg, setup, run tobject2json, run qcg

To run (in 2 different terminals) :
```
alienv enter qcg/latest-o2
tobject2json --backend mysql://qc_user:qc_user@localhost/quality_control --zeromq-server tcp://127.0.0.1:7777
```
```
alienv enter qcg/latest-o2
qcg
```
Then open your browser at `localhost:8080`. It will show the objects as they are stored in the the database.

### TObject2Json

TODO review , do we so many details ?

Single binary multi-threaded using QC data-sources (MySQL and CCDB) to expose ROOT objects as JSON over ZeroMQ.

#### Usage

```bash
alienv enter QualityControl/latest
tobject2json --backend mysql://qc_user:qc_user@localhost/quality_control --zeromq-server tcp://127.0.0.1:7777 --workers 4
```

#### Protocol

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

#### Architecture

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

## Data Inspector

This is a GUI to inspect the data coming out of the DataSampling, in
particular the Readout.

### Prerequisite

Install GLFW for your platform. On CC7 install `glfw-devel`.

### Compilation

Build the QualityControl as usual.

### Execution

To monitor the readout, 3 processes have to be started : the Readout,
the Data Sampling and the Data Inspector. In 3 terminals, do respectively

1. Make sure that the data sampling is enabled in the readout :
```
[consumer-data-sampling]
consumerType=DataSampling
enabled=1
```
1. `readout.exe file:///absolute/path/to/config.cfg`
2. `runReadoutDataSampling --batch`
3. `dataDump --mq-config $QUALITYCONTROL_ROOT/etc/dataDump.json --id dataDump --control static`

### Configuration

__Fraction of data__
The Data Sampling tries to take 100% of the events by default.
Edit $QUALITY_CONTROL/readoutDataSampling.json
to change it. Look for the parameter `fraction` that is set to 1.

__Port__
The Data Sampling sends data to the GUI via the port `26525`.
If this port is not free, edit the config file $QUALITY_CONTROL/readoutDataSampling.json
and $QUALITY_CONTROL/dataDump.json.

## Information Service

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
### Usage
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

## Use MySQL as QC backend

TODO

- 1. Install the MySQL/MariaDB development package
       * CC7 : `sudo yum install mariadb-server`
       * Mac (or download the dmg from Oracle) : `brew install mysql`

 Post installation

  Start and populate database :

   ```
   sudo systemctl start mariadb # for CC7, check for your specific OS
   alienv enter qcg/latest
   qcDatabaseSetup.sh
   ```

## Configuration files details

TODO : task, checker, general parameters

TODO review :

The QC requires a number of configuration items. An example config file is
provided in the repo under the name _example-default.json_. Moreover, the
outgoing channel over which histograms are sent is defined in a JSON
file such as the one called _alfa.json_ in the repo.

**QC tasks** must be defined in the configuration within the element `qc/tasks_config` :

```
   "tasks_config": {
      "myTask_1": {
        "taskDefinition": "taskDefinition_1"
      },
    ...
```

We use an indirect system to allow multiple tasks to share
most of their definition (`myTask_1` uses defintion `taskDefinition_1`):

```
    ...
      "taskDefinition_1": {
        "className": "o2::quality_control_modules::example::ExampleTask",
        "moduleName": "QcExample",
        "cycleDurationSeconds": "10",
        "maxNumberCycles": "-1"
      },
```
The `moduleName` refers to which library contains the class `className`.

The data source for the task is defined in the section `qc/config/DataSampling` :

```
{
  "qc": {
    "config": {
      "DataSampling": {
        "implementation": "MockSampler"
      },
...
```

Implementation can be `FairSampler` to get data from readout or
`MockSampler` to get random data.

The JSON `alfa.json` file contains a typical FairMQ device definition. One can
 change the port or the address there:
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

**QC checkers** are defined in the config file in section `checkers_config`:

```
    "checkers_config": {
      "checker_0": {
        "broadcast": "0",
        "broadcastAddress": "tcp://*:5600",
        "id": "0"
      },
      ...
```

Here, `checker_0` is not going to broadcast its data but just store
it in the database.

And for the time, the mapping between checkers and tasks must be explicit :

```
      ...
      "numberCheckers": "1",
      "numberTasks": "1",
      "tasksAddresses": "tcp://localhost:5556,tcp://localhost:5557,tcp://localhost:5558,tcp://localhost:5559",
```
It is needed for the time being because we don't have an information service.

There are configuration items for many other aspects, for example the
database connection, the monitoring or the data sampling.

---

# Questions for the refactoring of the README
* do we really want the development package ? why not a fixed version ?
  --> because people will develop their module
* Should we split the modules from the framework then ?
* Should we install O2 and other dependencies with RPMs ?


