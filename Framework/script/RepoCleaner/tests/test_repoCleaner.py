import importlib
import os
import sys
import unittest
from importlib.machinery import SourceFileLoader
from importlib.util import spec_from_loader

import yaml

from tests.test_utils import CCDB_TEST_URL


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
parse_config = repoCleaner.parse_config
Rule = repoCleaner.Rule
find_matching_rules = repoCleaner.find_matching_rules


class TestRepoCleaner(unittest.TestCase):
    
    def test_parseConfig(self):
        args = parse_config("config-test.yaml")
        self.assertEqual(args['ccdb_url'], CCDB_TEST_URL)
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
            parse_config(destination)
         
        # now remove the faulty 2 lines at the beginning
        with open(destination, 'w+') as out:
            out.writelines(document.splitlines(True)[3:])
        with self.assertRaises(KeyError):
            parse_config(destination)
            
    def test_findMatchingRule(self):
        rules = [Rule('task1/obj1', '120', 'policy1'), Rule('task1/obj1', '120', 'policy2'), Rule('task2/.*', '120', 'policy3')]
        self.assertEqual(find_matching_rules(rules, 'task1/obj1')[0].policy, 'policy1')
        self.assertNotEqual(find_matching_rules(rules, 'task1/obj1')[0].policy, 'policy2')
        self.assertEqual(find_matching_rules(rules, 'task3/obj1'), [])
        self.assertEqual(find_matching_rules(rules, 'task2/obj1/obj1')[0].policy, 'policy3')
        rules.append(Rule('.*', '0', 'policyAll'))
        self.assertEqual(find_matching_rules(rules, 'task3/obj1')[0].policy, 'policyAll')


if __name__ == '__main__':
    unittest.main()
