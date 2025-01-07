import datetime
import logging
import traceback
from json import JSONDecodeError
from typing import List, Dict

import dryable
import requests

logger = logging  # default logger


class ObjectVersion:
    """
    A version of an object in the CCDB.

    In the CCDB an object can have many versions with different validity intervals.
    This class represents a single version.
    """

    def __init__(self, path: str, valid_from, valid_to, created_at, uuid=None, metadata=None):
        """
        Construct an ObjectVersion.
        :param path: path to the object
        :param uuid: unique id of the object
        :param valid_from: validity range smaller limit (in ms)
        :param valid_to: validity range bigger limit (in ms)
        :param created_at: creation timestamp of the object
        """
        self.path = path
        self.uuid = uuid
        self.valid_from = valid_from
        # precomputed Datetime ("Dt") of the timestamp `validFrom`
        self.valid_from_as_dt = datetime.datetime.fromtimestamp(int(valid_from) / 1000)  # /1000 because we get ms
        self.valid_to = valid_to
        self.metadata = metadata
        self.created_at = created_at
        # precomputed Datetime ("Dt") of the timestamp `createdAt`
        self.created_at_as_dt = datetime.datetime.fromtimestamp(int(created_at) / 1000)  # /1000 because we get ms

    def __repr__(self):
        if "Run" in self.metadata or "RunNumber" in self.metadata:
            run_number = self.metadata["Run"] if "Run" in self.metadata else self.metadata["RunNumber"]
            return f"Version of object {self.path} created at {self.created_at_as_dt.strftime('%Y-%m-%d %H:%M:%S')}, valid from {self.valid_from_as_dt.strftime('%Y-%m-%d %H:%M:%S')}, run {run_number} (uuid {self.uuid})"
        else:
            return f"Version of object {self.path} created at {self.created_at_as_dt.strftime('%Y-%m-%d %H:%M:%S')}, valid from {self.valid_from_as_dt.strftime('%Y-%m-%d %H:%M:%S')} (uuid {self.uuid}, " \
                   f"ts {self.valid_from})"


class Ccdb:
    """
    Class to interact with the CCDB.
    """
    
    counter_deleted: int = 0
    counter_validity_updated: int = 0
    counter_preserved: int = 0
    set_adjustable_eov: bool = False  # if True, set the metadata adjustableEOV before change validity

    def __init__(self, url):
        logger.info(f"Instantiate CCDB at {url}")
        self.url = url

    def get_objects_list(self, added_since: int = 0, path: str = "", no_wildcard: bool = False) -> List[str]:
        """
        Get the full list of objects in the CCDB that have been created since added_since.

        :param no_wildcard: if true, the path for which we get the list is not modified to add `/.*`.
                            Set it to true if you need to get the versions of an object and not a folder.
        :param path: the path
        :param added_since: if specified, only return objects added since this timestamp in epoch milliseconds.
        :return A list of strings, each containing a path to an object in the CCDB.
        """
        url_for_all_obj = self.url + '/latest/' + path
        url_for_all_obj += '/' if path else ''
        url_for_all_obj += '' if no_wildcard else '.*'
        logger.debug(f"Ccdb::getObjectsList -> {url_for_all_obj}")
        headers = {'Accept': 'application/json', 'If-Not-Before':str(added_since)}
        r = requests.get(url_for_all_obj, headers=headers)
        r.raise_for_status()
        try:
            json = r.json()
        except JSONDecodeError as err:
            logger.error(f"JSON decode error: {err}")
            raise
        paths = []
        for item in json['objects']:
            paths.append(item['path'])

        return paths

    def get_full_objects_details(self, path: str = "") -> List[Dict]:
        """
        Return the full json of all the objects found in the path.
        :param path:
        :return:
        """
        url_for_all_obj = self.url + '/latest/' + path + '.*'
        logger.debug(f"Ccdb::getFullObjectsDetails -> {url_for_all_obj}")
        headers = {'Accept': 'application/json'}
        r = requests.get(url_for_all_obj, headers=headers)
        r.raise_for_status()
        try:
            json = r.json()
        except JSONDecodeError as err:
            logger.error(f"JSON decode error: {err}")
            raise
        return json['objects']

    def get_versions_list(self, object_path: str, from_ts: str = "", to_ts: str = "", run: int = -1, metadata: str = "") \
            -> List[ObjectVersion]:
        """
        Get the list of all versions for a given object sorted by CreatedAt.
        :param metadata: only objects matching these metadata. Format: "[/key=value]*"
        :param run: only objects for this run (based on metadata)
        :param object_path: Path to the object for which we want the list of versions.
        :param from_ts: only objects created at or after this timestamp
        :param to_ts: only objects created before or at this timestamp
        :return A list of ObjectVersion.
        """
        url_browse_all_versions = self.url + '/browse/' + object_path
        headers = {'Accept': 'application/json', 'Connection': 'close'}
        if from_ts != "":
            headers["If-Not-Before"] = from_ts
        if to_ts != "":
            headers["If-Not-After"] = to_ts
        if run != -1:
            url_browse_all_versions += '/RunNumber=' + str(run)
        if metadata != "":
            url_browse_all_versions += metadata
        logger.debug(f"Ccdb::getVersionsList -> {url_browse_all_versions}")
        logger.debug(f"{headers}")
        r = requests.get(url_browse_all_versions, headers=headers)
        r.raise_for_status()
        try:
            json_result = r.json(strict=False)  # to survive bad characters in the strings of the json
        except ValueError as e:
            print(f"Error while reading json for object {object_path} from CCDB: {e}")
            exit(1)
        versions = []
        for object_path in json_result['objects']:
            version = ObjectVersion(path=object_path['path'], uuid=object_path['id'],
                                    valid_from=object_path['validFrom'], valid_to=object_path['validUntil'],
                                    metadata=object_path, created_at=object_path['Created'])
            versions.insert(0, version)
        versions.sort(key=lambda v: v.created_at, reverse=False)
        return versions

    @dryable.Dryable()
    def delete_version(self, version: ObjectVersion):
        """
        Delete the specified version of an object.
        :param version: The version of the object to delete, as an instance of ObjectVersion.
        """
        url_delete = self.url + '/' + version.path + '/' + str(version.valid_from) + '/' + version.uuid
        logger.debug(f"Delete version at url {url_delete}")
        headers = {'Connection': 'close'}
        try:
            r = requests.delete(url_delete, headers=headers)
            r.raise_for_status()
            self.counter_deleted += 1
        except requests.exceptions.RequestException:
            logging.error(f"Exception in deleteVersion: {traceback.format_exc()}")

    @dryable.Dryable()
    def move_version(self, version: ObjectVersion, to_path: str):
        """
        Move the version to a different path.
        :param version: The version of the object to move, as an instance of ObjectVersion.
        :param to_path: The destination path
        """
        url_move = self.url + '/' + version.path + '/' + str(version.valid_from) + '/' + version.uuid
        logger.debug(f"Move version at url {url_move} to {to_path}")
        headers = {'Connection': 'close', 'Destination': to_path}
        try:
            r = requests.request("MOVE", url_move, headers=headers)
            r.raise_for_status()
            self.counter_deleted += 1
        except requests.exceptions.RequestException:
            logging.error(f"Exception in moveVersion: {traceback.format_exc()}")

    @dryable.Dryable()
    def update_validity(self, version: ObjectVersion, valid_from: int, valid_to: int, metadata=None):
        """
        Update the validity range of the specified version of an object.
        :param version: The ObjectVersion to update.
        :param valid_from: The new "from" validity.
        :param valid_to: The new "to" validity.
        :param metadata: Add or modify metadata
        """
        full_path = self.url + '/' + version.path + '/' + str(valid_from) + '/' + str(valid_to) + '/' + str(version.uuid) + '?'
        logger.debug(f"Update end limit validity of {version.path} ({version.uuid}) from {version.valid_to} to {valid_to}")
        if metadata is not None:
            logger.debug(f"{metadata}")
            for key in metadata:
                full_path += key + "=" + metadata[key] + "&"
        if self.set_adjustable_eov:
            logger.debug(f"As the parameter force is set, we add metadata adjustableEOV")
            full_path += "adjustableEOV=1&"
        try:
            headers = {'Connection': 'close'}
            r = requests.put(full_path, headers=headers)
            r.raise_for_status()
            self.counter_validity_updated += 1
        except requests.exceptions.RequestException:
            logging.error(f"Exception in updateValidity: {traceback.format_exc()}")

    @dryable.Dryable()
    def update_metadata(self, version: ObjectVersion, metadata):
        logger.debug(f"update metadata : {metadata}")
        full_path = self.url + '/' + version.path + '/' + str(version.valid_from) + '/' + str(version.uuid) + '?'
        if metadata is not None:
            for key in metadata:
                full_path += key + "=" + metadata[key] + "&"
        if self.set_adjustable_eov:
            logger.debug(f"As the parameter force is set, we add metadata adjustableEOV")
            full_path += "adjustableEOV=1&"
        try:
            headers = {'Connection': 'close'}
            r = requests.put(full_path, headers=headers)
            r.raise_for_status()
        except requests.exceptions.RequestException:
            logging.error(f"Exception in updateMetadata: {traceback.format_exc()}")

    @dryable.Dryable()
    def put_version(self, version: ObjectVersion, data):
        """
        :param version: An ObjectVersion that describes the data to be uploaded.
        :param data: the actual data to send. E.g.:{'somekey': 'somevalue'}
        :return A list of ObjectVersion.
        """
        full_path= self.url + "/" + version.path + "/" + str(version.valid_from) + "/" + str(version.valid_to) + "/"
        if version.metadata is not None:
            for key in version.metadata:
                full_path += key + "=" + version.metadata[key] + "/"
        logger.debug(f"fullpath: {full_path}")
        headers = {'Connection': 'close'}
        r = requests.post(full_path, files=data, headers=headers)
        if r.ok:
            logger.debug(f"Version pushed to {version.path}")
        else:
            logger.error(f"Could not post a new version of {version.path}: {r.text}")

