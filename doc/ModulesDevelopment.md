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
         * [User-defined modules](#user-defined-modules)
      * [Module creation](#module-creation)
      * [Test run](#test-run)
      * [Modification of a Task](#modification-of-a-task)
      * [Addition of a Check](#addition-of-a-check)
      * [Commit Code](#commit-code)
      * [DPL workflow customization](#dpl-workflow-customization)
      * [Usage of DS and QC in an existing DPL workflow](#usage-of-ds-and-qc-in-an-existing-dpl-workflow)
      * [Addition of parameters to a task](#addition-of-parameters-to-a-task)

<!-- Added by: bvonhall, at:  -->

<!--te-->

[← Go back to Quickstart](QuickStart.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Advanced Topics →](Advanced.md)

## Context

Before developing a module, one should have a bare idea of what the QualityControl is and how it is designed. The following sections explore these aspects.

### QC architecture

![alt text](images/Architecture.png)

The main data flow is represented in blue. Data samples are selected by the Data Sampling (not represented) and sent to the QC tasks, either on the same machines or on other machines. The tasks produce TObjects, usually histograms, that are merged (if needed) and then checked. The checkers output the received TObject along with a quality flag. The TObject can be modified by the Checker. Finally the TObject and its quality are stored in the repository.

### DPL

[Data Processing Layer](https://github.com/AliceO2Group/AliceO2/blob/dev/Framework/Core/README.md) is a software framework developed as a part of O2 project. It structurizes the computing into units called _Data Processors_ - processes that communicate with each other via messages. DPL takes care of generating and running the processing topology out of user declaration code, serializing and deserializing messages, providing the data processors with all the anticipated messages for a given timestamp and much more. Each piece of data is characterized by its `DataHeader`, which consists (among others) of `dataOrigin`, `dataDescription` and `SubSpecification` - for example `{"MFT", "TRACKS", 0}`.

An example of a workflow definition which describes the processing steps (_Data Processors_), their inputs and their outputs can be seen in [runBasic.cxx](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/runBasic.cxx). In the QC we define the workflows in files whose names are prefixed with `run`.

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

An example of using the data sampling in a DPL workflow is visible in [runAdvanced.cxx](https://github.com/AliceO2Group/QualityControl/blob/master/Framework/runAdvanced.cxx).

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

The file `basic-no-sampling.json` is provided as an example. To test it, you can run `o2-qc-run-qc` with that configuration file instead of `basic.json`.

### Code Organization

The repository QualityControl contains the _Framework_  and the _Modules_ in the respectively named directories.

The Data Sampling code is part of the AliceO2 repository.

### User-defined modules

The Quality Control uses _plugins_ to load the actual code to be executed by the _Tasks_ and the _Checkers_. A module, or plugin, can contain one or several _Tasks_ and/or one or several _Checks_. They must subclass `TaskInterface.h` and `CheckInterface.h` respectively. We use the Template Method Design Pattern.

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
 -m MODULE_NAME   create module named MODULE_NAME or add there some task/checker
 -t TASK_NAME     create task named TASK_NAME
 -c CHECK_NAME    create check named CHECK_NAME
```

For example, if your detector 3-letter code is ABC you might want to do
```
# we are in ~/alice
cd QualityControl/Modules
./o2-qc-module-configurator.sh -m Abc # create the module
./o2-qc-module-configurator.sh -t RawDataQcTask # add a task
```

## Test run

Now that there is a module, we can build it and test it. First let's build it :
```
# We are in ~/alice
# Go to the build directory of QualityControl
cd sw/slc7_x86-64/BUILD/QualityControl-latest/QualityControl
make -j8 install # replace 8 by the number of cores on your machine
```

To test whether it works, we are going to run a basic DPL workflow defined in `runBasic.cxx`.
We need to modify slightly the config file to indicate our freshly created module and classes.
The config file is called `basic.json` and is located in `$QUALITYCONTROL_ROOT/etc/`. After installation, if you want to modify the original one, it is in the source directory `Framework`. In case you need it updated in the installation directory, you have to `make install` the project again.
Change the lines as indicated below :

```
"MyRawDataQcTask": {
  "className": "o2::quality_control_modules::abc::RawDataQcTask",
  "moduleName": "QcAbc",
```

Now we can run it

```
o2-qc-run-basic | o2-qc-run-qc --config json://${QUALITYCONTROL_ROOT}/etc/basic.json
```

You should see the QcTask at qcg-test.cern.ch with an object `Example` updating.

## Modification of a Task

Fill in the methods in RawDataQcTask.cxx. For example, make it publish a second histogram. Objects must be published only once and they will then be updated automatically every cycle (10 seconds for our example, 1 minute in general). 
Once done, recompile it (see section above) and run it. You should see the second object published in the qcg.

TODO give actual steps

You can rename the task by simply changing its name in the config file. Change the name from 
`QcTask` to whatever you like and run it again (no need to recompile). You should see the new name
appear in the QCG.

## Addition of a Check

TODO

## Commit Code

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

## Addition of parameters to a task

A task can access custom parameters declared in the configuration file at `qc.tasks.<task_name>.taskParameters`. They are stored inside a key-value map named mCustomParameters, which is a protected member of `TaskInterface`.

One can also tell the DPL driver to accept new arguments. This is done using the `customize` method at the top of your workflow definition (usually called "runXXX" in the QC).

For example, to add two parameters of different types do : 
```
void customize(std::vector<ConfigParamSpec>& workflowOptions)
{
  workflowOptions.push_back(
    ConfigParamSpec{ "config-path", VariantType::String, "", { "Path to the config file. Overwrite the default paths. Do not use with no-data-sampling." } });
  workflowOptions.push_back(
    ConfigParamSpec{ "no-data-sampling", VariantType::Bool, false, { "Skips data sampling, connects directly the task to the producer." } });
}
```


---

[← Go back to Quickstart](QuickStart.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Advanced Topics →](Advanced.md)
