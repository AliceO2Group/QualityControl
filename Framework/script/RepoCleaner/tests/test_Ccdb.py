import logging
import unittest
import responses

from qcrepocleaner.Ccdb import Ccdb, ObjectVersion, logger
from typing import List

class TestCcdb(unittest.TestCase):
    
    def setUp(self):
        with open('objectsList.json') as f:   # will close() when we leave this block
            self.content_objectslist = f.read()
        with open('versionsList.json') as f:   # will close() when we leave this block
            self.content_versionslist = f.read()
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
        logging.getLogger().setLevel(logging.DEBUG)

    @responses.activate
    def test_getObjectsList(self):
        # Prepare mock response
        responses.add(responses.GET, 'http://ccdb-test.cern.ch:8080/latest/.*',
                      self.content_objectslist, status=200)
        # get list of objects
        objects_list = self.ccdb.getObjectsList()
        print(f"{objects_list}")
        self.assertEqual(len(objects_list), 3)
        self.assertEqual(objects_list[0], 'Test')
        self.assertEqual(objects_list[1], 'ITSQcTask/ChipStaveCheck')
         
    @responses.activate
    def test_getVersionsList(self):
        # Prepare mock response
        object_path='asdfasdf/example'
        responses.add(responses.GET, 'http://ccdb-test.cern.ch:8080/browse/'+object_path,
                      self.content_versionslist, status=200)
        # get versions for object
        versions_list: List[ObjectVersion] = self.ccdb.getVersionsList(object_path)
        print(f"{versions_list}")
        self.assertEqual(len(versions_list), 2)
        self.assertEqual(versions_list[0].path, object_path)
        self.assertEqual(versions_list[1].path, object_path)
        self.assertEqual(versions_list[1].metadata["custom"], "34")

if __name__ == '__main__':
    unittest.main()
