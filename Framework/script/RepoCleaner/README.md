Here are the tools to clean up the CCDB of the QC.

## Entry point
It is `repoCleaner.py`. See the long comment at the beginning.

## Usage
```
./repoCleaner [--dry-run] [--log-level 10] [--config config.yaml]
```

## Configuration
The file `config.yaml` contains the rules to be followed to clean up the database.
It also contains the CCDB url.

The configuration for ccdb-test is described [here](../../../doc/DevelopersTips.md). 

## Test
`cd QualityControl/Framework/script/RepoCleaner ; python3 -m unittest discover`

To run just one of the rules, do `python3 1_per_run.py`.

## Installation
CMake will install the python scripts in bin and the config file in etc.