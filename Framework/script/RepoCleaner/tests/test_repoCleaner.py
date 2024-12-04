import importlib
import os
import sys
import unittest
from importlib.machinery import SourceFileLoader
from importlib.util import spec_from_loader

import yaml


def import_path(path):  # needed because o2-qc-repo-cleaner has no suffix
    module_name = os.path.basename(path).replace('-', '_')
    spec = importlib.util.spec_from_loader(
        module_name,
        importlib.machinery.SourceFileLoader(module_name, path)
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    sys.modules[module_name] = module
    return module

repoCleaner = import_path("../qcrepocleaner/o2-qc-repo-cleaner")
parseConfig = repoCleaner.parseConfig
Rule = repoCleaner.Rule
findMatchingRules = repoCleaner.findMatchingRules


class TestRepoCleaner(unittest.TestCase):
    
    def test_parseConfig(self):
        args = parseConfig("config-test.yaml")
        self.assertEqual(args['ccdb_url'], "http://ccdb-test.cern.ch:8080")
        rules = args['rules']
        self.assertEqual(len(rules), 3)
         
    def test_parseConfigFault(self):
        document = """
        asdf
        asdf asdf
            Rulesss:
              - object_path: asdfasdf/.* 
                delay: 1440              
                policy: 1_per_hour       
              - object_path: QcTask/example
                delay: 120
                policy: 1_per_hour
                 
        """
        destination = "/tmp/faulty-config.yaml"
        f = open(destination, "w+")
        f.write(document)
        f.close()
        with self.assertRaises(yaml.YAMLError):
            parseConfig(destination)
         
        # now remove the faulty 2 lines at the beginning
        with open(destination, 'w+') as fout:
            fout.writelines(document.splitlines(True)[3:])
        with self.assertRaises(KeyError):
            parseConfig(destination)
            
    def test_findMatchingRule(self):
        rules = []
        rules.append(Rule('task1/obj1', '120', 'policy1'))
        rules.append(Rule('task1/obj1', '120', 'policy2'))
        rules.append(Rule('task2/.*', '120', 'policy3'))
        self.assertEqual(findMatchingRules(rules, 'task1/obj1')[0].policy, 'policy1')
        self.assertNotEqual(findMatchingRules(rules, 'task1/obj1')[0].policy, 'policy2')
        self.assertEqual(findMatchingRules(rules, 'task3/obj1'), [])
        self.assertEqual(findMatchingRules(rules, 'task2/obj1/obj1')[0].policy, 'policy3')
        rules.append(Rule('.*', '0', 'policyAll'))
        self.assertEqual(findMatchingRules(rules, 'task3/obj1')[0].policy, 'policyAll')


if __name__ == '__main__':
    unittest.main()
