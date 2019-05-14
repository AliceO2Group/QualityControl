import unittest
import yaml
import repoCleaner
from repoCleaner import parseConfig, Rule, findMatchingRule
import logging

class TestRepoCleaner(unittest.TestCase):
    
    def test_parseConfig(self):
        args = parseConfig("config-test.yaml")
        self.assertEqual(args['ccdb_url'], "http://ccdb-test.cern.ch:8080")
        rules = args['rules']
        self.assertEqual(len(rules), 2)
         
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
        self.assertEqual(findMatchingRule(rules, 'task1/obj1').policy, 'policy1')
        self.assertNotEqual(findMatchingRule(rules, 'task1/obj1').policy, 'policy2')
        self.assertEqual(findMatchingRule(rules, 'task3/obj1'), None)
        self.assertEqual(findMatchingRule(rules, 'task2/obj1/obj1').policy, 'policy3')
        rules.append(Rule('.*', '0', 'policyAll'))
        self.assertEqual(findMatchingRule(rules, 'task3/obj1').policy, 'policyAll')

if __name__ == '__main__':
    unittest.main()