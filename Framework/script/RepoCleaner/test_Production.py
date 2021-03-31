import logging
import time
import unittest
from datetime import timedelta, date, datetime

from Ccdb import Ccdb, ObjectVersion
from rules import production


class TestProduciton(unittest.TestCase):
    """
    This test pushes data to the CCDB and then run the Rule Production and then check.
    It does it for several use cases.
    """

    def setUp(self):
        self.ccdb = Ccdb('http://ccdb-test.cern.ch:8080')
        self.extra = {"delay_first_trimming": "30", "period_btw_versions_first": "10", "delay_final_trimming": "60",
                      "period_btw_versions_final": "15"}
        self.path = "qc/TST/MO/repo/test"
        self.run = 124321

    def test_midRun(self):
        """
        Ongoing run (1h30).
        0-30' : already trimmed
        30-90': not trimmed
        Expected output: 0-60' trimmed
        """
        logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logging.getLogger().setLevel(int(10))

        # Prepare data
        self.prepare_data(90)

        production.eor_dict[self.run] = datetime.now() - timedelta(minutes=180)
        stats = production.process(self.ccdb, self.path, 30, self.extra)
        self.assertEqual(stats["deleted"], 28)
        self.assertEqual(stats["preserved"], 35)
        self.assertEqual(stats["updated"], 3)

        objectsVersions = self.ccdb.getVersionsList(self.path)
        self.assertEqual(len(objectsVersions), 35)
        self.assertTrue("trim1" in objectsVersions[0].metadata)

    def prepare_data(self, minutes_since_sor):
        """
        Prepare a data set starting `minutes_since_sor` in the past.
        The data is layed out as follow
          1. 30 minutes already trimmed data (every 10 minutes)
          2. 1 hour untrimmed
        Depending how far in the past one starts, different outputs of the production rule are expected.
        If `minutes_since_sor` is shorter than the run, we only create data in the past, never in the future.
        :param minutes_since_sor:
        :return:
        """

        current_timestamp = int(time.time() * 1000)
        sor = current_timestamp - minutes_since_sor * 60 * 1000
        data = {'part': 'part'}
        metadata = {'Run': str(self.run), 'trim1': 'true'}

        # 1 version every 10 minutes starting at sor and finishing 30 minutes after with trim1 flag set
        for x in range(3):
            from_ts = sor + x * 10 * 60 * 1000
            if from_ts > current_timestamp:
                return
            to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
            version_info = ObjectVersion(path=self.path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
            self.ccdb.putVersion(version=version_info, data=data)

        # 1 version every 1 minutes starting at sor + 30 minutes and lasting 60 minutes
        current_timestamp = int(time.time() * 1000)
        metadata = {'Run': str(self.run)}
        for x in range(60):
            from_ts = sor + (30 + x) * 60 * 1000  # current_timestamp - (60 - x) * 60 * 1000
            if from_ts > current_timestamp:
                return
            to_ts = from_ts + 24 * 60 * 60 * 1000  # a day
            version_info = ObjectVersion(path=self.path, validFrom=from_ts, validTo=to_ts, metadata=metadata)
            self.ccdb.putVersion(version=version_info, data=data)


if __name__ == '__main__':
    unittest.main()
