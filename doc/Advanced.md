
# Advanced topics

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [Advanced topics](#advanced-topics)
      * [Data Inspector](#data-inspector)
         * [Prerequisite](#prerequisite)
         * [Compilation](#compilation)
         * [Execution](#execution)
         * [Configuration](#configuration)
      * [Use MySQL as QC backend](#use-mysql-as-qc-backend)
      * [Local CCDB setup](#local-ccdb-setup)
      * [Local QCG (QC GUI) setup](#local-qcg-qc-gui-setup)
      * [Custom QC object metadata](#custom-qc-object-metadata)
      * [Information Service](#information-service)
         * [Usage](#usage)
      * [Configuration files details](#configuration-files-details)

<!-- Added by: bvonhall, at:  -->

<!--te-->


[← Go back to Modules Development](ModulesDevelopment.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Frequently Asked Questions →](FAQ.md)

## Data Inspector

This is a GUI to inspect the data coming out of the DataSampling, in
particular the Readout.

![alt text](images/dataDump.png)

### Prerequisite

If not already done, install GLFW for your platform. On CC7 install `glfw-devel` from epel repository : `sudo yum install glfw-devel --enablerepo=epel`

### Compilation

Build the QualityControl as usual.

### Execution

To monitor the readout, 3 processes have to be started : the Readout,
the Data Sampling and the Data Inspector.

First make sure that the Data Sampling is enabled in the readout :
```
[consumer-data-sampling]
consumerType=DataSampling
enabled=1
```

In 3 separate terminals, do respectively

1. `readout.exe file:///absolute/path/to/config.cfg`
2. `o2-qc-run-readout-for-data-dump --batch`
3. `o2-qc-data-dump --mq-config $QUALITYCONTROL_ROOT/etc/dataDump.json --id dataDump --control static`

### Configuration

__Fraction of data__
The Data Sampling tries to take 100% of the events by default.
Edit `$QUALITYCONTROL_ROOT/etc/readoutForDataDump.json`
to change it. Look for the parameter `fraction` that is set to 1.

__Port__
The Data Sampling sends data to the GUI via the port `26525`.
If this port is not free, edit the config file `$QUALITYCONTROL_ROOT/etc/readoutForDataDump.json`
and `$QUALITYCONTROL_ROOT/etc/dataDump.json`.

## Use MySQL as QC backend

1. Install the MySQL/MariaDB development package
       * CC7 : `sudo yum install mariadb-server`
       * Mac (or download the dmg from Oracle) : `brew install mysql`

2. Rebuild the QualityControl (so that the mysql backend classes are compiled)

3. Start and populate database :

   ```
   sudo systemctl start mariadb # for CC7, check for your specific OS
   alienv enter qcg/latest
   o2-qc-database-setup.sh
   ```

## Local CCDB setup

Having a central ccdb for test (ccdb-test) is handy but also means that everyone can access, modify or delete the data. If you prefer to have a local instance of the CCDB, for example in your lab or on your development machine, follow these instructions.

1. Download the local repository service from http://alimonitor.cern.ch/download/local.jar

2. The service can simply be run with
    `java -jar local.jar`

It will start listening by default on port 8080. This can be changed either with the java parameter “tomcat.port” or with the environment variable “TOMCAT_PORT”. Similarly the default listening address is 127.0.0.1 and it can be changed with the java parameter “tomcat.address” or with the environment variable “TOMCAT_ADDRESS” to something else (for example ‘*’ to listen on all interfaces).

By default the local repository is located in /tmp/QC (or java.io.tmpdir/QC to be more precise). You can change this location in a similar way by setting the java parameter “file.repository.location” or the environment variable “FILE_REPOSITORY_LOCATION”.

The address of the CCDB will have to be updated in the Tasks config file.

At the moment, the description of the REST api can be found in this document : https://docs.google.com/presentation/d/1PJ0CVW7QHgnFzi0LELc06V82LFGPgmG3vsmmuurPnUg

## Local QCG (QC GUI) setup

To install and run the QCG locally, and its fellow process tobject2json, please follow these instructions : https://github.com/AliceO2Group/WebUi/tree/dev/QualityControl#run-qcg-locally

## Custom QC object metadata

One can add custom metadata on the QC objects produced in a QC task. 
Simply call `ObjectsManager::addMetadata(...)`, like in 
```
  // add a metadata on histogram mHistogram, key is "custom" and value "34"
  getObjectsManager()->addMetadata(mHistogram->GetName(), "custom", "34");
```
This metadata will end up in the CCDB.

## Developing QC modules on a machine with FLP suite

To load a development library in a setup with FLP suite, specify its full
path in the config (e.g. `/etc/flp.d/qc/readout.json`):
```
    "tasks": {
      "QcTask": {
        "active": "true",
        "className": "o2::quality_control_modules::skeleton::SkeletonTask",
        "moduleName": "/home/myuser/alice/sw/BUILD/QualityControl-latest/QualityControl/libQcTstLibrary",
        ...
```
Make sure that:
- The name "QcTask" stays the same, as changing it might break the 
workflow specification for AliECS
- The library is compiled with the same QC version as the one which is installed
with the FLP suite. Especially, the task and check interfaces have to be identical.
- If there are checks applied to MonitorObjects, update the library path in
the addCheck() functions as well. This will not be necessary when checks are
configured inside config files.

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
o2-qc-info-service -c /absolute/path/to/InformationService.json -n information_service \
              --id information_service --mq-config /absolute/path/to/InformationService.json
```

The `o2-qc-info-service` can provide fake data from a file. This is useful
to test the clients. Use the option `--fake-data-file` and provide the
absolute path to the file. The file `infoServiceFake.json` is provided
as an example.

To check what is being output by the Information Service, one can
run the InformationServiceDump :
```
o2-qc-info-service-dump -c /absolute/path/to/InformationService.json -n information_service_dump \
                  --id information_service_dump --mq-config /absolute/path/to/InformationService.json
                  --request-task myTask1
```
The last parameter can be omitted to receive information about all tasks.

## Configuration files details

TODO : this is to be rewritten once we stabilize the configuration file format.

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

[← Go back to Modules Development](ModulesDevelopment.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Frequently Asked Questions →](FAQ.md)
