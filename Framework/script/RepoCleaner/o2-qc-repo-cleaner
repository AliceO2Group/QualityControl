#!/usr/bin/env python3

# This script drives the clean up process of the CCDB backend of the QC. 
#
# It should ideally be ran as a cron on a machine. It uses plugins to implement 
# the actual actions defined in the config file config.yaml. Each item in the 
# config file describes which plugin (by name of the file) should be used for 
# a certain path in the CCDB. 
# 
# If several rules apply to an object, we pick the first one. Thus, mind carefully
# the order of the rules !
#
# The plugins should have a function "process()" that takes 3 arguments : 
# ccdb: Ccdb, object_path: str and delay: int
#
# We depend on requests, yaml, dryable, responses (to mock and test with requests)
#
# Usage
#         # run with debug logs and don't actually touch the database
#         ./repoCleaner --dry-run --log-level 10 

import argparse
import logging
import requests
import re
from typing import List
import tempfile

import dryable
import yaml
import time
import consul

from Ccdb import Ccdb


class Rule:
    """A class to hold information about a "rule" defined in the config file."""

    def __init__(self, object_path=None, delay=None, policy=None,  # migration=None,
                 all_params=None):
        '''
        Constructor.
        :param object_path: path to the object, or pattern, to which a rule will apply.
        :param delay: the grace period during which a new object is never deleted.
        :param policy: which policy to apply in order to clean up. It should correspond to a plugin.
        :param all_params: a map with all the parameters from the config file for this rule. We will keep only the
        extra ones.
        '''
        self.object_path = object_path
        self.delay = delay
        self.policy = policy

        self.extra_params = all_params
        if all_params is not None:
            self.extra_params.pop("object_path")
            self.extra_params.pop("delay")
            self.extra_params.pop("policy")

    def __repr__(self):
        return 'Rule(object_path={.object_path}, delay={.delay}, policy={.policy}, extra_params={.extra_params})'.format(
            self, self, self, self, self)


def parseArgs():
    """Parse the arguments passed to the script."""
    logging.info("Parsing arguments")
    parser = argparse.ArgumentParser(description='Clean the QC database.')
    parser.add_argument('--config', dest='config', action='store', default="config.yaml",
                        help='Path to the config file')
    parser.add_argument('--config-git', action='store_true',
                        help='Check out the config file from git (branch repo_cleaner), ignore --config.')
    parser.add_argument('--config-consul', action='store',
                        help='Specify the consul url and port in the form of <url>:<port>, if specified'
                             'ignore both --config and --config-git.')
    parser.add_argument('--log-level', dest='log_level', action='store', default="20",
                        help='Log level (CRITICAL->50, ERROR->40, WARNING->30, INFO->20,DEBUG->10)')
    parser.add_argument('--dry-run', action='store_true',
                        help='Dry run, no actual deletion nor modification to the CCDB.')
    parser.add_argument('--only-path', dest='only_path', action='store', default="",
                        help='Only work on given path (omit the initial slash).')
    args = parser.parse_args()
    dryable.set(args.dry_run)
    logging.info(args)
    return args


def parseConfig(config_file_path):
    """
    Read the config file and prepare a list of rules. 
    
    Return a dictionary containing the list of rules and other config elements from the file. 
    
    :param config_file_path: Path to the config file
    :raises yaml.YAMLError If the config file does not contain a valid yaml.
    """

    logging.info(f"Parsing config file {config_file_path}")
    with open(config_file_path, 'r') as stream:
        config_content = yaml.safe_load(stream)

    rules = []
    logging.debug("Rules found in the config file:")
    for rule_yaml in config_content["Rules"]:
        rule = Rule(rule_yaml["object_path"], rule_yaml["delay"], rule_yaml["policy"],  # rule_yaml["migration"],
                    rule_yaml)
        rules.append(rule)
        logging.debug(f"   * {rule}")

    ccdb_url = config_content["Ccdb"]["Url"]

    return {'rules': rules, 'ccdb_url': ccdb_url}


def downloadConfigFromGit():
    """
    Download a config file from git.
    :return: the path to the config file
    """

    logging.debug("Get it from git")
    r = requests.get(
        'https://raw.github.com/AliceO2Group/QualityControl/repo_cleaner/Framework/script/RepoCleaner/config.yaml')
    logging.debug(f"config file from git : \n{r.text}")
    path = "/tmp/config.yaml"
    with open(path, 'w') as f:
        f.write(r.text)
    logging.info(f"Config path : {path}")
    return path


def downloadConfigFromConsul(consul_url, consul_port):
    """
    Download a config file from consul.
    :return: the path to the config file
    """

    logging.debug("Get it from consul")
    consul_server = consul.Consul(host=consul_url, port=consul_port)
    index, data = consul_server.kv.get(key='folder/blabla')
    logging.debug(f"config file from consul : \n{data['Value']}")
    text = data["Value"].decode()
    logging.debug(f"config file from consul : \n{text}")
    path = "/tmp/repoCleanerConfig.yaml"
    with open(path, 'w') as f:
        f.write(text)
    logging.info(f"Config path : {path}")
    return path


def findMatchingRule(rules, object_path):
    """Return the first matching rule for the given path or None if none is found."""

    logging.debug(f"findMatchingRule for {object_path}")

    if object_path == None:
        logging.error(f"findMatchingRule: object_path is None")
        return None

    for rule in rules:
        pattern = re.compile(rule.object_path)
        result = pattern.match(object_path)
        if result != None:
            logging.debug(f"   Found! {rule}")
            return rule
    logging.debug("   No rule found, skipping.")
    return None


filepath = tempfile.gettempdir() + "/repoCleaner.txt"
currentTimeStamp = int(time.time() * 1000)


def getTimestampLastExecution():
    """
    Returns the timestamp of the last execution.
    It is stored in a file in $TMP/repoCleaner.txt.
    :return: the timestampe of the last execution or 0 if it cannot find it.
    """
    try:
        f = open(filepath, "r")
    except IOError as e:
        logging.info(f"File {filepath} not readable, we return 0 as timestamp.")
        return 0
    timestamp = f.read()
    logging.info(f"Timestamp retrieved from {filepath}: {timestamp}")
    f.close()
    return timestamp


def storeSavedTimestamp():
    """
    Store the timestamp we saved at the beginning of the execution of this script.
    """
    try:
        f = open(filepath, "w+")
    except IOError:
        logging.error(f"Could not write the saved timestamp to {filepath}")
    f.write(str(currentTimeStamp))
    logging.info(f"Stored timestamp {currentTimeStamp} in {filepath}")
    f.close()


# ****************
# We start here !
# ****************

def main():
    # Logging (you can use funcName in the template)
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                        datefmt='%d-%b-%y %H:%M:%S')

    # Parse arguments 
    args = parseArgs()
    logging.getLogger().setLevel(int(args.log_level))

    # Read configuration
    path = args.config
    if args.config_consul:
        items = args.config_consul.split(':')
        path = downloadConfigFromConsul(items[0], items[1])
        exit(0)
    elif args.config_git:
        path = downloadConfigFromGit()
    config = parseConfig(path)
    rules: List[Rule] = config['rules']
    ccdb_url = config['ccdb_url']

    # Get list of objects from CCDB
    ccdb = Ccdb(ccdb_url)
    paths = ccdb.getObjectsList(getTimestampLastExecution())
    if args.only_path != '':
        paths = [item for item in paths if item.startswith(args.only_path)]
    logging.debug(paths)
    logging.debug(len(paths))

    # For each object call the first matching rule
    logging.info("Loop through the objects and apply first matching rule.")
    for object_path in paths:
        logging.info(f"Processing {object_path}")
        # Take the first matching rule, if any
        rule = findMatchingRule(rules, object_path);
        if rule == None:
            continue

        # Apply rule on object (find the plug-in script and apply)
        module = __import__(rule.policy)
        stats = module.process(ccdb, object_path, int(rule.delay),  # rule.migration == "True",
                               rule.extra_params)
        logging.info(f"{rule.policy} applied on {object_path}: {stats}")

    logging.info(
        f" *** DONE *** (total deleted: {ccdb.counter_deleted}, total updated: {ccdb.counter_validity_updated})")
    storeSavedTimestamp()


if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
