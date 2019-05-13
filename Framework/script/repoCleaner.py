#!/usr/bin/env python

import argparse
import yaml
import requests
import re


def getListCcdbObjects():
    url = 'http://ccdb-test.cern.ch:8080/latest/.*'
    headers = {'Accept':'application/json'}
    r = requests.get(url, headers=headers)
    json = r.json()
    paths = []
    for item in json['objects']:
        paths.append(item['path'])
    return paths

class Rule:
    def __init__(self, object_path=None, delay=None, policy=None):
        self.object_path = object_path
        self.delay = delay
        self.policy = policy

    def __repr__(self):
        return 'Rule(object_path={.object_path}, delay={.delay}, policy={.policy})'.format(self, self, self)
    
def parseArgs():
    print("parseArgs")
    parser = argparse.ArgumentParser(description='Clean the QC database.')
    parser.add_argument('--config', dest='config', action='store', default="config.yaml",
                        help='Path to the config file')
    
    args = parser.parse_args()
    return args

def parseConfig(args):
    with open(args.config, 'r') as stream:
        try:
            config_content = yaml.safe_load(stream)
        except yaml.YAMLError as exc:
            print(exc)
            pass
        
    rules=[]
    for rule_yaml in config_content["Rules"]:
        rule = Rule(rule_yaml["object_path"], rule_yaml["delay"], rule_yaml["policy"])
        rules.append(rule)
    return rules

# return the first matching rule for the given path
def findMatchingRule(rules, object_path):
#     print(f"findMatchingRule for {object_path}")
    for rule in rules:
#         print(rule)
        pattern = re.compile(rule.object_path)
        result = pattern.match(object_path)
        if result != None:
            return rule
    return None
    
# Parse arguments 
args = parseArgs()
print(args.config)

# Read configuration
rules = parseConfig(args)
        
# Get list of objects from CCDB
paths = getListCcdbObjects()

# For each object
for object_path in paths:
    # Take the first matching rule, if any
    rule = findMatchingRule(rules, object_path);
    if rule == None:
        print(f"Did not find a rule matching {object_path}")
        continue
    else: 
        print(f"Rule matching {object_path} : {rule}")
        
    # Apply rule on object (find the plug-in script and apply)
    module = __import__(rule.policy)
    module.process(object_path)

print("done")