# Frequently asked questions

<!--TOC generated with https://github.com/ekalinin/github-markdown-toc-->
<!--./gh-md-toc --insert /path/to/README.md-->
<!--ts-->
   * [Frequently asked questions](#frequently-asked-questions)
      * [Git](#git)
         * [Are there instructions how to use Git ?](#are-there-instructions-how-to-use-git-)
      * [Coding Guidelines](#coding-guidelines)
         * [Where are the Coding Guidelines ?](#where-are-the-coding-guidelines-)
         * [How do I check my code or apply the guidelines ?](#how-do-i-check-my-code-or-apply-the-guidelines-)
      * [Build](#build)
         * [How do I add a dependency to my module ?](#how-do-i-add-a-dependency-to-my-module-)
         * [How do I make my module depend on library XXX from AliceO2 ?](#how-do-i-make-my-module-depend-on-library-xxx-from-aliceo2-)
      * [Run](#run)
         * [Why are my QC processes using 100% CPU ?](#why-are-my-qc-processes-using-100-cpu-)
      * [QCDB](#ccdb-repository)
         * [How to see which objects are stored in the CCDB ?](#how-to-see-which-objects-are-stored-in-the-ccdb-)
         * [How to delete objects from the CCDB ?](#how-to-delete-objects-from-the-ccdb-)

<!-- Added by: bvonhall, at:  -->

<!--te-->

[← Go back to Advanced Topics](Advanced.md) | [↑ Go to the Table of Content ↑](../README.md) 

## Git

### Are there instructions how to use Git ? 

See [here](https://alisw.github.io/git-tutorial/).

## Coding Guidelines 

### Where are the Coding Guidelines ? 

They are [here](https://github.com/AliceO2Group/CodingGuidelines). 

### How do I check my code or apply the guidelines ? 

See [here](https://github.com/AliceO2Group/CodingGuidelines#formatting-tool). 

## Build 

### How do I add a dependency to my module ? 

We use standard CMake and you should get acquainted with this tool. In the CMakeLists.txt of your module, add the missing dependency to the command `target_link_libraries` (i.e. the name of the library without "lib" and ".so".
For AliceO2 libraries see the next question.

### How do I make my module depend on library XXX from AliceO2 ? 

Add the library name to the list `O2_LIBRARIES_NAMES` in [FindAliceO2.cmake](../cmake/FindAliceO2.cmake)

## Run 

### Why are my QC processes using 100% CPU ? 

When running `o2-qc` or other qc binaries, the system will show that the processes use 100% of the CPU. This is due to the DPL default rate for the event loop of devices with inputs. 

Simply start your binary with `--rate 10000` and it should solve the problem. The rate might have to be adapted to your workflow : 
```bash
o2-qc-run-producer | o2-qc --config json://${QUALITYCONTROL_ROOT}/etc/basic.json --rate 10000
```

## QCDB

### How to see which objects are stored in the CCDB ?

The easiest is to use the QCG (QC GUI). If you use the central test CCDB, you can use the central test QCG. Simply direct your browser to [https://qcg-test.cern.ch](https://qcg-test.cern.ch).

If for some reason you don't want or can't use the QCG, the CCDB provides a web interface accessible at [http://ccdb-test.cern.ch:8080/browse/](http://ccdb-test.cern.ch:8080/browse/).

### How to delete objects from the CCDB ?

By accessing http://ccdb-test.cern.ch:8080/truncate/path/to/folder you will delete all the objects at the given path. Careful with that please ! Don't delete data of others.<br/>In production it will of course not be possible to do so. 

### My objects are not stored due to their size. What can I do ?

You see warnings in the logs: 
```
Warning - object qc/DET/MO/xxx/xxx is bigger than the maximum allowed size (2097152B) - skipped
```
This is because the maximum allowed size for an object is 2MiB. It can be increased by updating the following item 
in the config: 
```
{
  "qc": {
    "config": {
      "database": {
        "maxObjectSize": "2097152",       "": "[Bytes, default=2MB] Maximum size allowed, larger objects are rejected."
```

[← Go back to Advanced Topics](Advanced.md) | [↑ Go to the Table of Content ↑](../README.md) 
