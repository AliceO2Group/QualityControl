
FLP Suite
---

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert --no-backup --hide-footer --indent 3 QualityControl/doc/Advanced.md -->
<!--ts-->
   * [Developing QC modules on a machine with FLP suite](#developing-qc-modules-on-a-machine-with-flp-suite)
   * [Switch detector in the workflow <em>readout-dataflow</em>](#switch-detector-in-the-workflow-readout-dataflow)
   * [Get all the task output to the infologger](#get-all-the-task-output-to-the-infologger)
   * [Using a different config file with the general QC](#using-a-different-config-file-with-the-general-qc)
   * [Enable the repo cleaner](#enable-the-repo-cleaner)
   * [Definition of new arguments](#definition-of-new-arguments)
   * [Reference data](#reference-data)
<!--te-->

# FLP Suite

The QC is part of the FLP Suite. The Suite is installed on FLPs through RPMs and is configured with ansible. As a consequence a few things are different in this context compared to a pure development setup.

## Developing QC modules on a machine with FLP suite

Development RPMs are available on the FLPs. Start by installing them, then compile QC and finally tell aliECS to use it.

**Installation**

As root do:

```
yum install o2-QualityControl-devel git -y
```

**Compilation**

As user `flp` do:

```
git clone https://github.com/AliceO2Group/QualityControl.git
cd QualityControl
git checkout <release> # use the release included in the installed FLP suite
mkdir build
cd build
mkdir /tmp/installdir
cmake -DCMAKE_INSTALL_PREFIX=/tmp/installdir -G Ninja -DCLANG_EXECUTABLE=/opt/o2/bin-safe/clang -DCMAKE_BUILD_TYPE=RelWithDebugInfo ..
ninja -j16 install 
```

_**Compilation on top of a local O2**_

If you want to build also O2 locally do

```
# O2
git clone https://github.com/AliceO2Group/AliceO2.git
cd AliceO2
git checkout <release> # use the release included in the installed FLP suite
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/tmp/installdir -G Ninja -DCLANG_EXECUTABLE=/opt/o2/bin-safe/clang -DCMAKE_BUILD_TYPE=RelWithDebugInfo ..
ninja -j8 install

# QC
git clone https://github.com/AliceO2Group/QualityControl.git
cd QualityControl
git checkout <release> # use the release included in the installed FLP suite
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/tmp/installdir -G Ninja -DCLANG_EXECUTABLE=/opt/o2/bin-safe/clang -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DO2_ROOT=/tmp/installdir ..
ninja -j8 install
```

_**Important step in case several nodes are involved**_

In case the workflows will span over several FLPs and/or QC machines, one should `scp` the `installdir` to the other machines in the same directory.

**Use it in aliECS**

In the aliECS gui, in the panel "Advanced Configuration", et an extra variable `extra_env_vars` and set it to

```
PATH=/tmp/installdir/bin/:$PATH; LD_LIBRARY_PATH=/tmp/installdir/lib/:/tmp/installdir/lib64/:$LD_LIBRARY_PATH; QUALITYCONTROL_ROOT=/tmp/installdir/; echo
```

Replace `/tmp/installdir` with your own path. Make sure that the directory is anyway readable and traversable by users `flp` and `qc`

## Switch detector in the workflow _readout-dataflow_

The workflow readout-dataflow works by default with the detector code TST. To run with another detector (e.g. EMC) do:

2. Replace all instances of `TST` in the QC config file in consul with the one of the detector (e.g. `EMC`).
2. Set the variable `detector` in aliECS to the detector (e.g. `EMC`).

## Get all the task output to the infologger

Set the variable log_task_output=all

## Using a different config file with the general QC

One can set the `QC URI` to a different config file that is used by the general QC when enabled. However, this is not the recommended way. One must make sure that the name of the task and the check are left untouched and that they are both enabled.

## Enable the repo cleaner

If the CCDB used in an FLP setup is the local one, the repo cleaner might be necessary as to avoid filling up the disk of the machine.

By defaults there is a _disabled_ cron job :

```shell
*/10 * * * * /opt/o2/bin/o2-qc-repo-cleaner --config /etc/flp.d/ccdb-sql/repocleaner.yaml --dry-run > /dev/null 2>> /tmp/cron-errors.txt
```

1. copy the config file /etc/flp.d/ccdb-sql/repocleaner.yaml
2. modify the config file to suit your needs
3. run by hand the repo-cleaner to check that the config file is ok
3. update the cron job to use the modified config file
4. uncomment the cron job

## Definition of new arguments

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

## Reference data

A reference object is an object from a previous run. It is usually used as a point of comparison.

### Get a reference plot in a check

To retrieve a reference plot in your Check, use

```
  std::shared_ptr<MonitorObject> CheckInterface::retrieveReference(std::string path, Activity referenceActivity);
```
* `path` : the path of the object _without the provenance (e.g. `qc`)_
* `referenceActivity` : the activity of reference (usually the current activity with a different run number)

If the reference is not found it will return a `nullptr` and the quality is `Null`.

### Compare to a reference plot

The check `ReferenceComparatorCheck` in `Common` compares objects to their reference.

The configuration looks like

```
      "QcCheck": {
        "active": "true",
        "className": "o2::quality_control_modules::common::ReferenceComparatorCheck",
        "moduleName": "QcCommon",
        "policy": "OnAny",
        "detectorName": "TST",
        "dataSource": [{
          "type": "Task",
          "name": "QcTask",
          "MOs": ["example"]
        }],
        "extendedCheckParameters": {
          "default": {
            "default": {
              "referenceRun" : "500",
              "moduleName" : "QualityControl",
              "comparatorName" : "o2::quality_control_modules::common::ObjectComparatorChi2",
              "threshold" : "0.5"
            }
          },
          "PHYSICS": {
            "pp": {
              "referenceRun" : "551890"
            }
          }
        }
      }
```

The check needs the following parameters
* `referenceRun` to specify what is the run of reference and retrieve the reference data.
* `comparatorName` to decide how to compare, see below for their descriptions.
* `threshold` to specifie the value used to discriminate between good and bad matches between the histograms.

Three comparators are provided:

1. `o2::quality_control_modules::common::ObjectComparatorDeviation`: comparison based on the average relative deviation between the bins of the current and reference histograms; the `threshold` parameter represent in this case the maximum allowed deviation
2. `o2::quality_control_modules::common::ObjectComparatorChi2`: comparison based on a standard chi2 test between the current and reference histograms; the `threshold` parameter represent in this case the minimum allowed chi2 probability
3. `o2::quality_control_modules::common::ObjectComparatorKolmogorov`: comparison based on a standard Kolmogorov test between the current and reference histograms; the `threshold` parameter represent in this case the minimum allowed Kolmogorov probability

Note that you can easily specify different reference runs for different run types and beam types.

The plot is beautified by the addition of a `TPaveText` containing the quality and the reason for the quality.

### Generate a canvas combining both the current and reference ratio histogram

The postprocessing task ReferenceComparatorTask draws a given set of plots in comparison with their corresponding references, both as superimposed histograms and as current/reference ratio histograms.
See the details [here](https://github.com/AliceO2Group/QualityControl/blob/master/doc/PostProcessing.md#the-referencecomparatortask-class).


---

[← Go back to QCDB](QCDB.md) | [↑ Go to the Table of Content ↑](../README.md) | [Continue to Miscellaneous →](Miscellaneous.md)
