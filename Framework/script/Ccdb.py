import requests
from typing import List
import datetime


class ObjectVersion:

    def __init__(self, path, uuid, validFrom):
        self.path = path
        self.uuid = uuid
        self.validFrom = datetime.datetime.fromtimestamp(int(validFrom) / 1000)  # /1000 because we get ms
        self.validFromTimestamp = validFrom
        
    def __repr__(self):
        return f"Version of object {self.path} valid from {self.validFrom} (uuid {self.uuid}, ts {self.validFromTimestamp})"


class Ccdb:

    def __init__(self, url):
        self.url = url

    def getObjectsList(self) -> List[str]:
        url_for_all_obj = self.url + '/latest/.*'
        headers = {'Accept':'application/json'}
        r = requests.get(url_for_all_obj, headers=headers)
        json = r.json()
        paths = []
        for item in json['objects']:
            paths.append(item['path'])
        return paths

    def getVersionsList(self, item: str) -> List[ObjectVersion]:
        """Get the list of all versions for the given object"""
        url_browse_all_versions = self.url + '/browse/' + item
        headers = {'Accept':'application/json'}
        r = requests.get(url_browse_all_versions, headers=headers)
        json = r.json()
        print(f"{json}")
        versions = []
        for item in json['objects']:
            version = ObjectVersion(path=item['path'], uuid=item['id'], validFrom=item['validFrom'])
            versions.insert(0, version)
        return versions

    def deleteVersion(self, version: ObjectVersion):
        url_delete = self.url + '/' + version.uuid
        requests.delete(url_delete)

    def updateValidity(self, version: ObjectVersion, validFrom: str, validTo: str):    
        url_update_validity = self.url + '/' + version.path + '/' + validFrom + '/' + validTo
        requests.put(url_update_validity)
        
    
ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
objectsList = ccdb.getObjectsList()
print(f"{objectsList}")
