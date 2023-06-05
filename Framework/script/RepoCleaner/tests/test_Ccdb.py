import logging
import unittest
import requests
import responses

from Ccdb import Ccdb, ObjectVersion
from rules import production

class TestCcdb(unittest.TestCase):
    
    def setUp(self):
        with open('../qcrepocleaner/objectsList.json') as f:   # will close() when we leave this block
            self.content_objectslist = f.read()
        with open('../versionsList.json') as f:   # will close() when we leave this block
            self.content_versionslist = f.read()
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')

    @responses.activate
    def test_getObjectsList(self):
        # Prepare mock response
        responses.add(responses.GET, 'http://ccdb-test.cern.ch:8080/latest/.*',
                      self.content_objectslist, status=200)
        # get list of objects
        objectsList = self.ccdb.getObjectsList()
        print(f"{objectsList}")
        self.assertEqual(len(objectsList), 3)
        self.assertEqual(objectsList[0], 'Test')
        self.assertEqual(objectsList[1], 'ITSQcTask/ChipStaveCheck')
         
    @responses.activate
    def test_getVersionsList(self):
        # Prepare mock response
        object_path='asdfasdf/example'
        responses.add(responses.GET, 'http://ccdb-test.cern.ch:8080/browse/'+object_path,
                      self.content_versionslist, status=200)
        # get versions for object
        versionsList: List[ObjectVersion] = self.ccdb.getVersionsList(object_path)
        print(f"{versionsList}")
        self.assertEqual(len(versionsList), 2)
        self.assertEqual(versionsList[0].path, object_path)
        self.assertEqual(versionsList[1].path, object_path)
        self.assertEqual(versionsList[1].metadata["custom"], "34")

if __name__ == '__main__':
    unittest.main()
