# Modules development

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [Modules development](#modules-development)
      * [Context](#context)
         * [QC architecture](#qc-architecture)
         * [DPL](#dpl)
         * [Data Sampling](#data-sampling)
            * [Custom Data Sampling Condition](#custom-data-sampling-condition)
            * [Bypassing the Data Sampling](#bypassing-the-data-sampling)
         * [Code Organization](#code-organization)
         * [Developing with aliBuild/alienv](#developing-with-alibuildalienv)
         * [User-defined modules](#user-defined-modules)
      * [Module creation](#module-creation)
      * [Test run](#test-run)
      * [Modification of the Task](#modification-of-the-task)
      * [Check](#check)
         * [Configuration](#configuration)
         * [Implementation](#implementation)
      * [Committing code](#committing-code)
      * [Raw data source](#raw-data-source)

<!-- Added by: barth, at: Lun 17 aoû 2020 14:57:50 CEST -->
<!--te-->

[← Go back to Quickstart](QuickStart.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Post-processing →](PostProcessing.md)

## Context

Before developing a module, one should have a bare idea of what the QualityControl is and how it is designed. The following sections explore these aspects.

### QC architecture

![alt text](images/Architecture.png)

The main data flow is represented in blue. Data samples are selected by the Data Sampling (not represented) and sent to the QC tasks, either on the same machines or on other machines. The tasks produce TObjects encapsulated in a MonitorObject, usually histograms, that are merged (if needed) and then checked. The checkers output the received MonitorObject along with a QualityObject. The MonitorObject can be modified by the Checker. Finally the MonitorObject and the QualityObject(s) are stored in the repository.

### DPL

[Data Processing Layer](https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md) is a software framework developed as a part of O2 project. It structurizes the computing into units called _Data Processors_ - processes that communicate with each other via messages. DPL takes care of generating and running the processing topology out of user declaration code, serializing and deserializing messages, providing the data processors with all the anticipated messages for a given timestamp and much more. Each piece of data is characterized by its `DataHeader`, which consists (among others) of `dataOrigin`, `dataDescription` and `SubSpecification` - for example `{"MFT", "TRACKS", 0}`.

An example of a workflow definition which describes the processing steps (_Data Processors_), their inputs and their outputs can be seen in [runBasic.cxx](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/src/runBasic.cxx). In the QC we define the workflows in files whose names are prefixed with `run`.

### Data Sampling

The Data Sampling provides the possibility to sample data in DPL workflows, based on certain conditions ( 5% randomly, when payload is greater than 4234 bytes or others, including custom conditions). The job of passing the right data is done by a data processor called `Dispatcher`. A desired data stream is specified in the form of Data Sampling Policies, defined in the QC JSON configuration file. Please refer to the main [Data Sampling readme](https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md#data-sampling) for more details.

Data Sampling is used by Quality Control to feed the tasks with data. Below we present an example of a configuration file. It instructs Data Sampling to provide a QC task with 10% randomly selected data that has the header `{"ITS", "RAWDATA", 0}`. The data will be accessible inside the QC task by the binding `"raw"`.
```json
{
  "qc": {
    ...
    "tasks": {
      "QcTask": {
        ...
        "dataSource": {
          "type": "dataSamplingPolicy",
          "name": "its-raw"
        },
        ...
      }
    }
  },
  "dataSamplingPolicies": [
    {
      "id": "its-raw",
      "active": "true",
      "machines": [],
      "query_comment" : "query is in the format of binding1:origin1/description1/subSpec1[;binding2:...]",
      "query": "raw:ITS/RAWDATA/0",
      "samplingConditions": [
        {
          "condition": "random",
          "fraction": "0.1",
          "seed": "1234"
        }
      ],
      "blocking": "false"
    }
  ]
}
```

An example of using the data sampling in a DPL workflow is visible in [runAdvanced.cxx](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/src/runAdvanced.cxx).

#### Custom Data Sampling Condition

If needed, a custom data selection can be performed by inheriting the `DataSamplingCondition` class and implementing the `configure` and `decide` methods. Then, to use it, one needs to specify the library and class names in the config file.

The class [ExampleCondition](https://github.com/AliceO2Group/QualityControl/blob/master/Modules/Example/include/Example/ExampleCondition.h) presents the how to write one's own condition, while in [example-default.json](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/example-default.json) the policy `ex1` shows how it should be configured.

#### Bypassing the Data Sampling

In case one needs to sample at a very high rate, or even monitor 100% of the data, the Data Sampling can be omitted altogether. As a result the task is connected directly to the the Device producing the data to be monitored. To do so, change the _dataSource's_ type in the config file from `dataSamplingPolicy` to `direct`. In addition, add the information about the type of data that is expected (dataOrigin, binding, etc...) and remove the dataSamplingPolicies :  

```json
{
  "qc": {
    ...
    "tasks": {
      "QcTask": {
        ...
        "dataSource": {
          "type": "direct",
          "query_comment" : "query is in the format of binding1:origin1/description1/subSpec1[;binding2:...]",
          "query" : "its-raw-data:ITS/RAWDATA/0"
        },
        ...
      }
    }
  },
  "dataSamplingPolicies": [
  ]
}
```

The file `basic-no-sampling.json` is provided as an example. To test it, you can run `o2-qc` with that configuration file instead of `basic.json`.

To use multiple direct data sources, just place them one after another in the value of `"query"`, separated with a semicolon. For example:
```
"query" : "emcal-digits:EMC/DIGITS/0;emcal-triggers:EMC/TRIGGERS/0"
```

### Code Organization

The repository QualityControl contains the _Framework_  and the _Modules_ in the respectively named directories.

The Data Sampling code is part of the AliceO2 repository.

### Developing with aliBuild/alienv

One can of course build using `aliBuild` (`aliBuild build --defaults o2 QualityControl`). However, that will take quite some time as it checks all dependencies and builds everything. 

After the initial use of `aliBuild`, which is necessary, the correct way of building is to load the environment with `alienv` and then go to the build directory and run `make` or `ninja`.

```
alienv load QualityControl/latest
cd sw/BUILD/QualityControl-latest/QualityControl
make -j8 install # or ninja -j8 install , also adapt to the number of cores available
```

### User-defined modules

The Quality Control uses _plugins_ to load the actual code to be executed by the _Tasks_ and the _Checks_. A module, or plugin, can contain one or several of these classes, both Tasks and Checks. They must subclass `TaskInterface.h` and `CheckInterface.h` respectively. We use the Template Method Design Pattern.

The same code, the same class, can be run many times in parallel. It means that you can run several qc tasks (no uppercase, i.e. processes) in parallel, each executing the same code defined in the same Task (uppercase, the class). Similarly, one Check can be executed against different Monitor Objects and Tasks.

### Repository

QC results (MonitorObjects and QualityObjects) are stored in a repository that we usually call the QCDB. Indeed, the underlying technology is the one of the CCDB (Condition and Calibration DataBase). As a matter of fact we use the test instance of the CCDB for the tests and development (ccdb-test.cern.ch:8080). In production, we will have a different instance distinct from the CCDB. 

#### Paths

We use a consistent system of path for the objects we store in the QCDB:
* MonitorObjects: ```qc/<detectorCode>/MO/<taskName>/<moName>```
* QualityObjects: ```qc/<detectorCode>/QO/<checkName>[/<moName>]```  
The last, optional, part depends on the policy (read more about that [later](#Check)).

## Module creation

Before starting to develop the code, one should create a new module if it does not exist yet. Typically each detector team should prepare a module.

The script `o2-qc-module-configurator.sh`, in the directory _Modules_, is able to prepare a new module or to add a new _Task_ or a new _Check_ to an existing module. It must be run from __within QualityControl/Modules__. See the help message below:
```
Usage: ./o2-qc-module-configurator.sh -m MODULE_NAME [OPTION]

Generate template QC module and/or tasks, checks.
If a module with specified name already exists, new tasks and checks are inserted to the existing one.
Please follow UpperCamelCase convention for modules', tasks' and checks' names.

Example:
# create new module and some task
./o2-qc-module-configurator.sh -m MyModule -t SuperTask
# add one task and two checks
./o2-qc-module-configurator.sh -m MyModule -t EvenBetterTask -c HistoUniformityCheck -c MeanTest

Options:
 -h               print this message
 -m MODULE_NAME   create a module named MODULE_NAME or add there some task/checker
 -t TASK_NAME     create a task named TASK_NAME
 -c CHECK_NAME    create a check named CHECK_NAME
 -p PP_NAME       create a postprocessing task named PP_NAME
```

For example, if your detector 3-letter code is TST you might want to do
```
# we are in ~/alice
cd QualityControl/Modules
./o2-qc-module-configurator.sh -m TST -t RawDataQcTask # create the module and a task
```

IMPORTANT: Make sure that your detector code is listed in TaskRunner::validateDetectorName. If it is not, feel free to add it. 

We will refer in the following section to the module as `Tst` and the task as `RawDataQcTask`. Make sure to use your own code and names. 

## Test run

Now that there is a module, we can build it and test it. First let's build it :
```
# We are in ~/alice, call alienv if not already done
alienv enter QualityControl/latest
# Go to the build directory of QualityControl.
cd sw/BUILD/QualityControl-latest/QualityControl
make -j8 install # or ninja, replace 8 by the number of cores on your machine
```

To test whether it works, we are going to run a basic workflow made of a producer and the qc, which corresponds to the one we saw in the [QuickStart](QuickStart.md#basic-workflow).

We are going to duplicate the config file we used previously, i.e. `basic.json`: 
```
cp ~/alice/QualityControl/Framework/basic.json ~/alice/QualityControl/Modules/TST/basic-tst.json
```

We need to modify it slightly to indicate our freshly created module and classes.
Change the lines as indicated below :

```
"tasks": {
  "MyRawDataQcTask": {
    "active": "true",
    "className": "o2::quality_control_modules::abc::RawDataQcTask",
    "moduleName": "QcTST",
    "detectorName": "TST",
```
and 
```
        "detectorName": "TST",
        "dataSource": [{
          "type": "Task",
          "name": "MyRawDataQcTask",
```

Now we can run it

```
o2-qc-run-producer | o2-qc --config json://$HOME/alice/QualityControl/Modules/TST/basic-tst.json
```

You should see an object `example` in `/qc/TST/MO/MyRawDataQcTask` at qcg-test.cern.ch.

## Modification of the Task

We are going to modify our task to make it publish a second histogram. Objects must be published only once and they will then be updated automatically every cycle (10 seconds for our example, 1 minute in general). Modify `RawDataQcTask.cxx` and its header to add a new histogram, build it and publish it with `getObjectsManager()->startPublishing(mHistogram);`.
Once done, recompile it (see section above, `make -j8 install` in the build directory) and run it (same as above). You should see the second object published in the qcg.

## Check

A Check is a function that determines the quality of the Monitor Objects produced in the previous step - Task. It can receive multiple Monitor Objects from several Tasks.

### Configuration

```json
{
  "qc" : {
    "config" : { ... },
    "tasks" : { ... },

    "checks": {
      "CheckName": {
        "active": "true",
        "className": "o2::quality_control_modules::skeleton::SkeletonCheck",
        "moduleName": "QcSkeleton",
        "policy": "OnAny",
        "dataSource": [{
          "type": "Task",
          "name": "TaskName",
          "MOs": "all"
        },
        {
          "type": "Task",
          "name": "QcTask",
          "MOs": ["example", "other"]
        }]
      },
      "QcCheck": {
         ...
      }
   }

}
```

* __active__ - Boolean value whether the checker is active or not
* __moduleName__ - The module which implements the check class (like in tasks)
* __className__ - Class inside the module with the namespace path (like in tasks)
* __policy__ - Policy for triggering the _check_ function inside the module
    * _OnAny_ (default) - if any of the declared monitor objects change, might trigger even if not all are ready
    * _OnAnyNonZero_ - if any of the declared monitor objects change with assurance that there are all MOs
    * _OnAll_ - if all of the monitor objects updated at least once
    * _OnEachSeparately_ - if MOs from any task are updated, runs the same check separately on each of them,
     obtaining one Quality per one MO. Note that if any source Task updates their MOs, all of them will be checked
      again. Prefer to assign one Check per one Task to avoid this behaviour.
    * if the MOs are not declared or _MO_: "all" in one or more dataSources, the above policy don't apply, the `check` will be triggered whenever a new MonitorObject is received from one of the inputs
* __dataSource__ - declaration of the `check` input
    * _type_ - currently only supported is _Task_
    * _name_ - name of the _Task_
    * _MOs_ - list of MonitorObjects name or "all" (not as a list!)

### Implementation
After the creation of the module described in the above section, every Check functionality requires a separate implementation. The module might implement several Check classes.
```c++
Quality check(std::map<std::string, std::shared_ptr<MonitorObject>>* moMap) {}

void beautify(std::shared_ptr<MonitorObject> mo, Quality = Quality::Null) {}

```

The `check` function is called whenever the _policy_ is satisfied. It gets a map with all declared MonitorObjects. It is expected to return Quality of the given MonitorObjects.

The `beautify` function is called after the `check` function if there is a single `dataSource` of type `Task` in the configuration of the check. If there is more than one, the `beautify()` is not called in this check. 

## Committing code

To commit your new or modified code, please follow this procedure
1. Fork the [QualityControl](github.com/AliceO2Group/QualityControl) repo using github webpage or github desktop app.
1. Clone it : `git clone https://github.com/<yourIdentifier>/QualityControl.git`
1. Before you start working on your code, create a branch in your fork : `git checkout -b feature-new-stuff`
2. Push the branch : `git push --set-upstream origin feature-new-stuff`
2. Add and commit your changes onto this branch : `git add Abc.cxx ; git commit Abc.cxx`
3. Push your commits : `git push`
4. Once you are satisfied with your changes, make a _Pull Request_ (PR). Go to your branches on the github webpage, and click "New Pull Request". Explain what you did. If you only wanted to share the progress, but your PR is not ready for a review yet, please put **[WIP]** (Work In Progress) in the beginning of its name.
5. One of the QC developers will check your code. It will also be automatically tested.
6. Once approved the changes will be merged in the main repo. You can delete your branch.

For a new feature, just create a new branch for it and use the same procedure. Do not fork again. You can work on several features at the same time by having parallel branches.

General ALICE Git guidelines can be accessed [here](https://alisw.github.io/git-tutorial/).

## Raw data source

To read a raw data file, one can use the O2's [RawFileReader](https://github.com/AliceO2Group/AliceO2/tree/dev/Detectors/Raw#rawfilereader). On the same page, there are instructions to write such file from Simulation. 

---

[← Go back to Quickstart](QuickStart.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Post-processing →](PostProcessing.md)
