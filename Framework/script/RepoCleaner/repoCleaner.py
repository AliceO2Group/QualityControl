#!/usr/bin/env python

# This is the script driving the clean up process of the CCDB backup of the QC. 
# It should ideally be ran as a cron on a machine. It uses plugins to implement 
# the actual actions defined in the config file config.yaml. Each item in the 
# config file describes which plugin (by name of the file) should be used for 
# a certain path in the CCDB. 
# The plugins should have a function "process()" that takes two arguments : 
# ccdb: Ccdb, object_path: str and delay: int
#
# We depend on requests, yaml, dryable
#
# Usage
#         # run with debug logs and don't actually touch the database
#         ./repoCleaner --dry-run --log-level 10 

import argparse
import logging
import re
from typing import List

import dryable
import yaml

from Ccdb import Ccdb


class Rule:
    """A class to hold information about a "rule" defined in the config file."""

    def __init__(self, object_path=None, delay=None, policy=None):
        '''
        Constructor.
        :param object_path: path to the object, or pattern, to which a rule will apply.
        :param delay: the grace period during which a new object is never deleted.
        :param policy: which policy to apply in order to clean up. It should correspond to a plugin.
        '''
        self.object_path = object_path
        self.delay = delay
        self.policy = policy

    def __repr__(self):
        return 'Rule(object_path={.object_path}, delay={.delay}, policy={.policy})'.format(self, self, self)


def parseArgs():
    """Parse the arguments passed to the script."""
    logging.info("Parsing arguments")
    parser = argparse.ArgumentParser(description='Clean the QC database.')
    parser.add_argument('--config', dest='config', action='store', default="config.yaml",
                        help='Path to the config file')
    parser.add_argument('--log-level', dest='log_level', action='store', default="20",
                        help='Log level (CRITICAL->50, ERROR->40, WARNING->30, INFO->20,DEBUG->10)')
    parser.add_argument('--dry-run', action='store_true',
                        help='Dry run, no actual deletion nor modification to the CCDB.')
    args = parser.parse_args()
    dryable.set(args.dry_run)
    logging.debug(args)
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

#     if 'Rules' not in config_content:
#         raise KeyError('Element Rules is mandatory')
    rules = []
    logging.debug("Rules found in the config file:")
    for rule_yaml in config_content["Rules"]:
        rule = Rule(rule_yaml["object_path"], rule_yaml["delay"], rule_yaml["policy"])
        rules.append(rule)
        logging.debug(f"   * {rule}")

    ccdb_url = config_content["Ccdb"]["Url"]

    return {'rules': rules, 'ccdb_url': ccdb_url}


def findMatchingRule(rules, object_path):
    """Return the first matching rule for the given path or None if none is found."""
    
    logging.debug(f"findMatchingRule for {object_path}")
    for rule in rules:
        pattern = re.compile(rule.object_path)
        result = pattern.match(object_path)
        if result != None:
            logging.debug(f"   Found! {rule}")
            return rule
    logging.debug("   No rule found, skipping.")
    return None

# ****************
# We start here !
# ****************

def main():
    # Logging (you can use funcName in the template)
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s', datefmt='%d-%b-%y %H:%M:%S')
    
    # Parse arguments 
    args = parseArgs()
    logging.getLogger().setLevel(int(args.log_level))
    
    # Read configuration
    config = parseConfig(args.config)
    rules: List[Rule] = config['rules']
    ccdb_url = config['ccdb_url']
    
    # Get list of objects from CCDB
    ccdb = Ccdb(ccdb_url)
    paths = ccdb.getObjectsList()
    
    # For each object call the first matching rule
    logging.info("Loop through the objects and apply first matching rule.")
    for object_path in paths:
        # Take the first matching rule, if any
        rule = findMatchingRule(rules, object_path);
        if rule == None:
            continue
             
        # Apply rule on object (find the plug-in script and apply)
        module = __import__(rule.policy)
        module.process(ccdb, object_path, int(rule.delay))
    
    logging.info("done")

if __name__ == "__main__":  # to be able to run the test code above when not imported.
    main()
