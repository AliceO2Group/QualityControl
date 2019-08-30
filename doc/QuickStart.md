# QuickStart

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
            * [Getting real data from readout](#getting-real-data-from-readout)
            * [Readout data format as received by the Task](#readout-data-format-as-received-by-the-task)

<!-- Added by: bvonhall, at:  -->

<!--te-->

[↑ Go to the Table of Content ↑](../README.md) | [Continue to Modules Development →](ModulesDevelopment.md)

## Requirements

A Linux machine (CC7 or Ubuntu) or a Mac. See the O2 instructions below for the exact supported versions.

## Setup

2. Install O2
     * We use alibuild, see complete instructions [here](https://alice-doc.github.io/alice-analysis-tutorial/building/). We strongly encourage users to follow the instructions for a __manual__ installation.

3. Prepare the QualityControl development package
    * `aliBuild init QualityControl@master --defaults o2`

4. Install GLFW to have GUIs in the DPL (optional, DPL GUIs do not work in containers).
    * On CC7 : `sudo yum install glfw-devel --enablerepo=epel`
    * On Mac : `brew install glfw`

5. Build/install the QualityControl, its GUI (qcg) and the readout. The simplest is to use the metapackage `O2Suite`.
    * `aliBuild build O2Suite --defaults o2`

   At this point you might encounter a message about missing system requirements. Run `aliDoctor O2Suite` to get a full information about what is missing and how to install it.

Note : on non-CC7 systems, you can also use the alibuild "defaults" called `o2-dataflow` to avoid building simulation related packages.

### Environment loading

Whenever you want to work with O2 and QualityControl, do either `alienv enter O2Suite/latest` or `alienv load O2Suite/latest`.

## Execution

To make sure that your system is correctly setup, we are going to run a basic QC workflow attached to a simple data producer. We will use central services for the repository and the GUI. If you want to set them up on your computer or in your lab, please have a look [here](#local-ccdb-setup) and [here](#local-qcg-setup).

### Basic workflow

We will run a basic workflow with attached QC described in the following schema.

![alt text](images/basic-schema.png)

The _Producer_ is a random data generator. In a more realistic setup it would be a processing device or the _Readout_. The _Data Sampling_ is the system in charge of dispatching data samples from the main data flow to the _QC tasks_. It can be configured to dispatch different proportion or different types of data. The _Checker_ is in charge of evaluating the _MonitorObjects_ produced by the _QC tasks_, for example by checking that the mean is above a certain limit. It can also modify the aspect of the histogram, e.g. by changing the background color or adding a PaveText. Finally the _Checker_ is also in charge of storing the resulting _MonitorObject_ into the repository where it will be accessible by the web GUI. It also pushes it to a _Printer_ for the sake of this tutorial.

To run it simply do:

    o2-qc-run-basic

Thanks to the Data Processing Layer (DPL, more details later) it is a single process that steers all the _devices_, i.e. processes making up the workflow. A window should appear that shows a graphical representation of the workflow. The output of any of the processes is available by double clicking a box. If a box is red it means that the process has stopped, probably abnormally.

![alt text](images/basic-dpl-gui.png)

The presented example consists of one DPL workflow which has both the main processing and QC infrastructure declared inside. In the real case, we would usually prefer to attach the QC without modifying the original topology. It can be done by merging two (or more) workflows, as below:

    o2-qc-run-producer | o2-qc-run-qc --config json://${QUALITYCONTROL_ROOT}/etc/basic.json
 
This command uses two executables. The first one contains the _Producer, which represents a main data flow. The second executable generates the QC infrastructure based on given configuration file. These two workflows are joined together using the pipe | character. This example illustrates how to add QC to any DPL workflow by using `o2-qc-run-qc` and passing it a configuration file. 

__Repository and GUI__

The data is stored in the [ccdb-test](ccdb-test.cern.ch:8080/browse) at CERN. If everything works fine you should see the objects being published in the QC web GUI (QCG) at this address : [https://qcg-test.cern.ch/?page=objectTree](https://qcg-test.cern.ch/?page=objectTree). The link brings you to the hierarchy of objects (see screenshot below). Open "QcTask" (the task you are running) and click on "example" which is the name of your histogram. The plot should be displayed on the right. If you wait a bit and hit "REFRESH NOW" in the far left menu you should see it changing from time to time (see second screenshot below). 
Please note that anyone running o2-qc-run-basic publishes the same object and you might see the one published by someone else. 

![alt text](images/basic-qcg1.png)
![alt text](images/basic-qcg2.png)

TODO add a link to the user documentation of the QCG

__Configuration file__

The devices are configured in the config file named `basic.json`. It is installed in `$QUALITYCONTROL_ROOT/etc`. Each time you rebuild the code, `$QUALITYCONTROL_ROOT/etc/basic.json` is overwritten by the file in the source directory (`~/alice/QualityControl/Framework/basic.json`).

### Readout chain

In this second example, we are going to use the Readout as data source.

![alt text](images/readout-schema.png)

This workflow is a bit different from the basic one. The _Readout_ is not a device and thus we have to have a _proxy_ to get data from it. This is the extra box going to the _Dispatcher_, which then injects data to the task. This is handled in the _Readout_ if you enable the corresponding configuration flag.

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

Start the proxy, DS (DataSampling) and QC workflows :
```
o2-qc-run-readout | o2-qc-run-qc --config json://${QUALITYCONTROL_ROOT}/etc/readout.json
```

The data sampling is configured to sample 1% of the data as the readout should run by default at full speed.

#### Getting real data from readout

The first option is to configure readout.exe to connect to a cru. Please refer to the [Readout documentation](https://github.com/AliceO2Group/Readout/blob/master/doc/README.md). 

A more practical approach is to record a data file with Readout and then replay it on your development setup to develop and test your QC. The configuration options are described [here](https://github.com/AliceO2Group/Readout/blob/master/doc/configurationParameters.md), in particular : 

```
equipment-player-* 	filePath 	string 	
	Path of file containing data to be injected in readout.
equipment-player-* 	preLoad 	int 	1 	If 1, data pages preloaded with file content on startup. If 0, data is copied at runtime.
equipment-player-* 	fillPage 	int 	1 	If 1, content of data file is copied multiple time in each data page until page is full (or almost full: on the last iteration, there is no partial copy if remaining space is smaller than full file size). If 0, data file is copied exactly once in each data page.
```

#### Readout data format as received by the Task

The header is a O2 header populated with data from the header built by the Readout. 
The payload received is a 2MB (configurable) data page made of CRU pages (8kB).

__Configuration file__

The configuration file is installed in `$QUALITYCONTROL_ROOT/etc`. Each time you rebuild the code, `$QUALITYCONTROL_ROOT/etc/readout.json` is overwritten by the file in the source directory (`~/alice/QualityControl/Framework/readout.json`).
To avoid this behaviour and preserve the changes you do to the configuration, you can copy the file and specify the path to it with the parameter `--config` when launch `o2-qc-run-qc`.

To change the fraction of the data being monitored, change the option `fraction`.

```
"fraction": "0.01",
```

---

[↑ Go to the Table of Content ↑](../README.md) | [Continue to Modules Development →](ModulesDevelopment.md)
