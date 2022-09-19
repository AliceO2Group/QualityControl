import logging
import time
import unittest
from datetime import timedelta, date, datetime

from Ccdb import Ccdb, ObjectVersion
from rules import last_only




class TestLastOnly(unittest.TestCase):
    """
    This test pushes data to the CCDB and then run the Rule last_only and then check.
    It does it for several use cases.
    One should truncate /qc/TST/MO/repo/test before running it.
    """

    thirty_minutes = 1800000
    one_hour = 3600000
    in_ten_years = 1975323342000
    one_minute = 60000

    def setUp(self):
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
        self.extra = {}
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321


    def test_last_only(self):
        """
        59 versions
        grace period of 30 minutes
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_last_only"
        self.prepare_data(test_path, 60)

        stats = last_only.process(self.ccdb, test_path, 30, 1, self.in_ten_years, self.extra)
        self.assertEqual(stats["deleted"], 30)
        self.assertEqual(stats["preserved"], 29)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 29)


    def test_last_only_period(self):
        """
        59 versions
        no grace period
        only 20 minutes in the middle are in the period
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        test_path = self.path + "/test_last_only_period"
        self.prepare_data(test_path, 60)
        current_timestamp = int(time.time() * 1000)

        stats = last_only.process(self.ccdb, test_path, 0, current_timestamp-41*60*1000, current_timestamp-19*60*1000, self.extra)
        self.assertEqual(stats["deleted"], 19)
        self.assertEqual(stats["preserved"], 40)

        objects_versions = self.ccdb.getVersionsList(test_path)
        self.assertEqual(len(objects_versions), 40)


    def prepare_data(self, path, since_minutes):
        """
        Prepare a data set starting `since_minutes` in the past.
        1 version per minute
        Each data has a different run number.
        """

        current_timestamp = int(time.time() * 1000)
        data = {'part': 'part'}
        run = 1234
        counter = 0

        for x in range(since_minutes):
            counter = counter + 1
            from_ts = current_timestamp - x * 60 * 1000
            to_ts = current_timestamp
            metadata = {'RunNumber': str(run)}
            run = run + 1
            version_info = ObjectVersion(path=path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
            self.ccdb.putVersion(version=version_info, data=data)

        logging.debug(f"counter : {counter}" )


if __name__ == '__main__':
    unittest.main()
