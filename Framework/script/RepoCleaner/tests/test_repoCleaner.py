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

script_dir = os.path.dirname(os.path.abspath(__file__))
script_path = os.path.join(script_dir, "../qcrepocleaner/o2-qc-repo-cleaner")
repoCleaner = import_path(script_path)
parse_config = repoCleaner.parse_config
Rule = repoCleaner.Rule
find_matching_rules = repoCleaner.find_matching_rules
get_instance_name = repoCleaner.get_instance_name
get_timestamp_filepath = repoCleaner.get_timestamp_filepath
get_ts_last_execution = repoCleaner.get_ts_last_execution
store_saved_timestamp = repoCleaner.store_saved_timestamp


class TestRepoCleaner(unittest.TestCase):
    
    def test_parseConfig(self):
        config_test_path = os.path.join(script_dir, "config-test.yaml")
        args = parse_config(config_test_path)
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

    def test_get_instance_name(self):
        class DummyArgs:
            def __init__(self, **kwargs):
                self.__dict__.update(kwargs)

        self.assertEqual(get_instance_name(None), 'default')
        self.assertEqual(get_instance_name(DummyArgs()), 'default')

        # explicit instance name
        args = DummyArgs(instance_name='custom_inst', config='config.yaml')
        self.assertEqual(get_instance_name(args), 'custom_inst')

        # config file
        args = DummyArgs(config='/path/to/my_cleaner_config.yaml')
        self.assertEqual(get_instance_name(args), 'my_cleaner_config')

        # config with only_path
        args = DummyArgs(config='config.yaml', only_path='qc/ITS/MO')
        self.assertEqual(get_instance_name(args), 'config_qc_ITS_MO')

        # consul config
        args = DummyArgs(config_consul='localhost:8500:cleaner-tpc.yaml')
        self.assertEqual(get_instance_name(args), 'cleaner-tpc')

        # git config
        args = DummyArgs(config_git=True)
        self.assertEqual(get_instance_name(args), 'git')

    def test_get_timestamp_filepath(self):
        default_path = get_timestamp_filepath()
        self.assertTrue(default_path.endswith('repoCleaner.txt'))

        inst_path = get_timestamp_filepath('custom_inst')
        self.assertTrue(inst_path.endswith('repoCleaner_custom_inst.txt'))
        self.assertNotEqual(default_path, inst_path)

    def test_instance_timestamp_isolation(self):
        inst1_file = get_timestamp_filepath('instance_1_test')
        inst2_file = get_timestamp_filepath('instance_2_test')

        # Cleanup if leftover
        for f in [inst1_file, inst2_file]:
            if os.path.exists(f):
                os.remove(f)

        try:
            # Initially no execution timestamp
            self.assertEqual(get_ts_last_execution(False, inst1_file), 0)
            self.assertEqual(get_ts_last_execution(False, inst2_file), 0)

            # Store timestamp for instance 1
            ts1 = 1600000000000
            store_saved_timestamp(inst1_file, timestamp=ts1)

            # Instance 1 has ts1, instance 2 is still 0
            self.assertEqual(int(get_ts_last_execution(False, inst1_file)), ts1)
            self.assertEqual(get_ts_last_execution(False, inst2_file), 0)

            # Store timestamp for instance 2
            ts2 = 1700000000000
            store_saved_timestamp(inst2_file, timestamp=ts2)

            # Both retain their own distinct timestamps
            self.assertEqual(int(get_ts_last_execution(False, inst1_file)), ts1)
            self.assertEqual(int(get_ts_last_execution(False, inst2_file)), ts2)

            # Test ignore_last_execution option
            self.assertEqual(get_ts_last_execution(True, inst1_file), 0)
            self.assertEqual(get_ts_last_execution(True, inst2_file), 0)
        finally:
            for f in [inst1_file, inst2_file]:
                if os.path.exists(f):
                    os.remove(f)


if __name__ == '__main__':
    unittest.main()
