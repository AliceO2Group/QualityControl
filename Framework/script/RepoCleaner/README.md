Here are the tools to clean up the CCDB of the QC.

## Entry point
It is `o2-qc-repo-cleaner`. See the long comment at the beginning.

## Usage
```
usage: o2-qc-repo-cleaner [-h] [--config CONFIG] [--config-git] [--config-consul CONFIG_CONSUL] [--log-level LOG_LEVEL] 
                          [--dry-run] [--only-path ONLY_PATH] [--workers WORKERS]
```

## Configuration
The file `config.yaml` contains the CCDB URL and the rules to be followed to clean up the database. An example is provided along this README (`config.yaml`).
A typical rule in the config file looks like:
```
  - object_path: qc/ITS/.*
    delay: 240
    policy: 1_per_hour
```
There can be any number of these rules. The order is important as we use the first matching rule for each element in the QCDB (caveat the use of the flag `continue_with_next_rule`, see below).
- `object_path`: a pattern to be matched to know if the rule applies
- `delay`: the duration in minutes of the grace period during which an object is not removed, even if it matches the above path. 
- `policy`: the name of a policy to apply on the matching objects. Here are the currently available policies (full description in the corresponding files):
   - `1_per_hour`: keep the first and extend its validity to 1 hour, remove everything in the next hour, repeat.
   - `1_per_run`: requires the "Run" or "RunNumber" metadata to be set. Keep only the most recent version of an object for a given run. 
   - `last_only`: keep only the last version, remove everything else.
   - `none_kept`: keep none, remove everything
   - `skip`: keep everything
- `from_timestamp`: the rule only applies to versions whose `valid_from` is older than this timestamp
- `to_timestamp`: the rule only applies to versions whose `valid_from` is younger than this timestamp
- `continue_with_next_rule`: if `True`, the next matching rule is also applied. 
- `xyz`: any extra argument necessary for a given policy. This is the case of the argument `delete_when_no_run` required by the policy `1_per_run`. 

The configuration for ccdb-test is described [here](../../../doc/DevelopersTips.md). 

## Unit Tests
`cd QualityControl/Framework/script/RepoCleaner ; python3 -m unittest discover`

and to test only one of them: `python3 -m unittest tests/test_NewProduction.py -k test_2_runs`

In particular there is a test for the `production` rule that is pretty extensive. It hits the ccdb though and it needs the following path to be truncated: 
`
qc/TST/MO/repo/test*
`

## Other tests
Most of the classes and Rules have a main to help test them. To run do e.g. `python3 1_per_run.py`.

## Installation
CMake will install the python scripts in bin and the config file in etc.

## Example

```
PYTHONPATH=./rules:$PYTHONPATH ./o2-qc-repo-cleaner --dry-run --config config-test.yaml --dry-run --only-path qc/DAQ --log-level 10
```

## Development

To install locally
```
cd Framework/script/RepoCleaner
python3 -m pip install . 
```

## Upload new version

Prerequisite

1. Create an account on https://pypi.org

Create new version

1. Update version number in `setup.py`
2. `python3 setup.py sdist bdist_wheel`
3. `python3 -m twine upload --repository pypi dist/*`

## Use venv

1. cd Framework/script/RepoCleaner
2. python3 -m venv env
3. source env/bin/activate
4. python -m pip install -r requirements.txt
5. python3 -m pip install . 
6. You can execute and work. Next time just do "activate" and then you are good to go