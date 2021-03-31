Here are the tools to clean up the CCDB of the QC.

## Entry point
It is `repoCleaner.py`. See the long comment at the beginning.

## Usage
```
./repoCleaner [--dry-run] [--log-level 10] [--config config.yaml]
```

## Configuration
The file `config.yaml` contains the CCDB URL and the rules to be followed to clean up the database. An example is provided along this README (`config.yaml`).
A typical rule in the config file looks like:
```
  - object_path: qc/ITS/.*
    delay: 240
    policy: 1_per_hour
```
There can be any number of these rules. The order is important as we use the first matching rule for each element in the QCDB. 
- `object_path`: a pattern to be matched to know if the rule applies
- `delay`: the duration in minutes of the grace period during which an object is not removed, even if it matches the above path. 
- `policy`: the name of a policy to apply on the matching objects. Here are the currently available policies (full description in the corresponding files):
   - `1_per_hour`: keep the first and extend its validity to 1 hour, remove everything in the next hour, repeat.
   - `1_per_run`: requires the "Run" or "RunNumber" metadata to be set. Keep only the most recent version of an object for a given run. 
   - `last_only`: keep only the last version, remove everything else.
   - `none_kept`: keep none, remove everything
   - `skip`: keep everything
- `xyz`: any extra argument necessary for a given policy. This is the case of the argument `delete_when_no_run` required by the policy `1_per_run`. 

The configuration for ccdb-test is described [here](../../../doc/DevelopersTips.md). 

## Unit Tests
`cd QualityControl/Framework/script/RepoCleaner ; python3 -m unittest discover`

In particular there is a test for the `production` rule that is pretty extensive. It hits the ccdb though.

## Other tests
Most of the classes and Rules have a main to help test them. To run do `python3 1_per_run.py`.

## Installation
CMake will install the python scripts in bin and the config file in etc.
