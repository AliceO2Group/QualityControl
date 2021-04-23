<!--  \cond EXCLUDE_FOR_DOXYGEN -->
[![aliBuild](https://img.shields.io/badge/aliBuild-dashboard-lightgrey.svg)](https://alisw.cern.ch/cockpit-legacy/d/000000001/main-dashboard?orgId=1&var-storagename=All&var-reponame=All&var-checkname=build%2FQualityControl%2Fo2-dataflow%2F0&var-upthreshold=30m&var-minuptime=30)
[![JIRA](https://img.shields.io/badge/JIRA-Report%20issue-blue.svg)](https://alice.its.cern.ch/jira/secure/CreateIssue.jspa?pid=11201&issuetype=1)
[![doxygen](https://img.shields.io/badge/doxygen-documentation-blue.svg)](https://aliceo2group.github.io/QualityControl/)
[![Discourse](https://img.shields.io/badge/discourse-Get%20help-blue.svg)](https://alice-talk.web.cern.ch/)

<!--  \endcond  --> 

This is the repository of the data quality control (QC) software for the ALICE O<sup>2</sup> system. 
 
For a general overview of our (O2) software, organization and processes, please see this [page](https://aliceo2group.github.io/).

* [QuickStart](#quickstart)
    * [Read this first!](#read-this-first)
    * [Requirements](#requirements)
    * [Setup](#setup)
        * [Environment loading](#environment-loading)
    * [Execution](#execution)
        * [Basic workflow](#basic-workflow)
        * [Readout chain](#readout-chain)
        * [Post-processing example](#post-processing-example)

* [Modules development](#modules-development)
    * [Context](#context)
        * [QC architecture](#qc-architecture)
        * [DPL](#dpl)
        * [Data Sampling](#data-sampling)
        * [Code Organization](#code-organization)
        * [Developing with aliBuild/alienv](#developing-with-alibuildalienv)
        * [User-defined modules](#user-defined-modules)
        * [Repository](#repository)
    * [Module creation](#module-creation)
    * [Test run](#test-run)
        * [Saving the QC objects in a local file](#saving-the-qc-objects-in-a-local-file)
    * [Modification of the Task](#modification-of-the-task)
    * [Check](#check)
        * [Configuration](#configuration)
        * [Implementation](#implementation)
    * [Quality Aggregation](#quality-aggregation)
        * [Quick try](#quick-try)
        * [Configuration](#configuration-1)
        * [Implementation](#implementation-1)
    * [Committing code](#committing-code)
    * [Data sources](#data-sources)
        * [Readout](#readout)
        * [DPL workflow](#dpl-workflow)
    * [Run number](#run-number)
    * [A more advanced example](#a-more-advanced-example)
    * [Monitoring](#monitoring)
    
* [The post-processing framework](#the-post-processing-framework)
    * [Post-processing interface](#post-processing-interface)
    * [Post-processing Task configuration](#configuration)
        * [Triggers configuration](#triggers-configuration)
    * [Running it](#running-it)
* [Convenience classes](#convenience-classes)
    * [The TrendingTask class](#the-trendingtask-class)
        * [TrendingTask Configuration](#configuration-1)
    * [The TRFCollectionTask class](#the-trfcollectiontask-class)

* [Advanced topics](#advanced-topics)
    * [Plugging the QC to an existing DPL workflow](#plugging-the-qc-to-an-existing-dpl-workflow)
    * [Production of QC objects outside this framework](#production-of-qc-objects-outside-this-framework)
    * [Multi-node setups](#multi-node-setups)
    * [Writing a DPL data producer](#writing-a-dpl-data-producer)
    * [Access run conditions and calibrations from the CCDB](#access-run-conditions-and-calibrations-from-the-ccdb)
    * [Definition and access of task-specific configuration](#definition-and-access-of-task-specific-configuration)
    * [Custom QC object metadata](#custom-qc-object-metadata)
    * [Canvas options](#canvas-options)
    * [QC with DPL Analysis](#qc-with-dpl-analysis)
    * [Data Inspector](#data-inspector)
    * [Details on the data storage format in the CCDB](#details-on-the-data-storage-format-in-the-ccdb)
    * [Local CCDB setup](#local-ccdb-setup)
    * [Local QCG (QC GUI) setup](#local-qcg-qc-gui-setup)
    * [FLP Suite](#flp-suite)
    * [Configuration files details](#configuration-files-details)

### Where to get help

* CERN Mailing lists: alice-o2-wp7 and alice-o2-qc-contact
* Discourse: https://alice-talk.web.cern.ch/
* JIRA: https://alice.its.cern.ch
* O2 development newcomers' guide: https://aliceo2group.github.io/quickstart
* Doxygen: https://aliceo2group.github.io/QualityControl/
