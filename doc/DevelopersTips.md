## Developers' tips

This is a resource meant for the developers of the QC. Whenever we learn something useful we put it
here. It is not sanitized or organized. Just a brain dump.

### Release procedure / check list


One can use the script `release.sh` : 
```shell
release.sh
```
It is able to work out what is the release number and will drive the user through all the steps. 

Alternatively do it manually:
1. Update the version number in [CMakeLists.txt](../CMakeLists.txt), commit and push
2. Release in JIRA
2. Prepare the release notes using the commits since the last release in github (see [this template](ReleaseNotesTemplate.md)).
3. Release in github, paste the release notes
4. A PR is automatically created in alidist
5. Once merged, send an email to alice-o2-wp7@cern.ch, alice-o2-qc-contact@cern.ch and alice-dpg-qa-tools@cern.ch to announce the new release. Use the email for the previous release as a template.

### Create a fix version

One can use the script `createPatch.sh` : 
```shell
patch.sh v1.9.1 # current version
```

Alternatively do it manually:
1. checkout last tagged version, e.g. `git checkout v0.26.1`
2. branch, e.g. `git checkout -b branch_v0.26.2`
2. cherry-pick the commit from master, e.g. `git cherry-pick b187ddbe52058d53a9bbf3cbdd53121c6b936cd8`
3. change version in CMakeLists and commit
2. push the branch upstream, e.g. `git push upstream -u branch_v0.26.2`
5. tag, e.g. `git tag -a v0.26.2 -m "v0.26.2"`
4. push the tag upstream, e.g. `git push upstream v0.26.2`
6. Release in github using this tag 
4. A PR is automatically created in alidist

### Where and how to configure the repo_cleaner of the ccdb-test

The config file is stored in git in the branch `repo_cleaner` (careful not to update in master instead !). Check out the branch, update the file Framework/script/RepoCleaner/config.yaml and commit it. A PR is necessary but in case of emergency, force-merge it. As soon as it is merged, it will be used by the script.

The config file used to be in `aldaqci@aidrefflp01:~/alice-ccdb/config.yaml` but it is not the case any more.

The different cleaning policies available at the moment are: 1_per_hour, 1_per_run, last_only, none_kept, skip

The repo_cleaner is launched every 5 minutes by [Jenkins](https://alijenkins.cern.ch/job/FLP/job/CCDB%20Clean%20up/).

Documentation of the repo_cleaner can be found [here](../Framework/script/RepoCleaner/README.md).

### How to backup the local data of the QCDB

Four things to backup: 

1. database dump
2. postgresql.conf
3. QC directory (this is big)
4. repocleaner.yaml (if not stored in consul)

```
ssh root@ali-qcdb

# you could let them run but the data might not be 100% consistent
systemctl stop ccdb-sql
systemctl stop postgresql

su - postgres
cd /data/pgsql/backups/
pg_dumpall -l ccdb -p 5432 -v | pbzip2 > ccdb-dd-mm-yy.sql.bz2
cp ../data/postgresql.conf postgresql-dd.mm.yy.conf
cp /etc/flp.d/ccdb-sql/repocleaner.yaml repocleaner-dd.mm.yy.yaml
exit

cd /data/pgsql
cp -r QC QC-dd-mm-yy
```

### Automatic QCDB backups

The script `qcdb-backup.sh` on the qcdb machine is called by a cron every night to create a tarball. 
It is then taken by the backup system to the central backup machine before being moved to EOS. 

The script and the crontab installation are stored in ansible (in gitlab). 

### Trick used to load old data
Until version 3 of the class MonitorObject, objects were stored in the repository directly. They are now stored within TFiles. The issue with the former way is that the StreamerInfo are lost. To be able to load old data, the StreamerInfos have been saved in a root file "streamerinfos.root". The CcdbDatabase access class loads this file and the StreamerInfos upon creation which allows for a smooth reading of the old objects. The day we are certain nobody will add objects in the old format and that the old objects have been removed from the database, we can delete this file and remove the loading from CcdbDatabase. Moreover, the following lines can be removed : 
```
// We could not open a TFile we should now try to open an object directly serialized
object = ccdbApi.retrieve(path, metadata, getCurrentTimestamp());
cout << "We could retrieve the object " << path << " as a streamed object." << endl;
if(object == nullptr) {
  return nullptr;
}
```

### Doxygen generation

To generate locally the doxygen doc, do `cd sw/BUILD/QualityControl-latest/QualityControl; make doc`.
It will be available in doc/html, thus to open it quickly do `[xdg-]open doc/html/index.html`.

### Monitoring debug

When we don't see the monitoring data in grafana, here is what to do to pinpoint the source of the problem.

1. `ssh root@alio2-cr1-mvs01.cern.ch`
2. See if the data reach the servers :
    1. `systemctl stop influxdb`
    2. `nc -u -l 8087`  <-- make sure to use the proper port (cf your monitoring url)
    3. The metrics should appear in clear in the terminal as they arrive. If not, check the monitoring url in your code/config.
    4. systemctl start influxdb
3. If they arrive, we want to check whether the metrics are stored
    1. emacs /etc/influxdb/influxdb.conf
        1. Search for the port you are using
        2. Note the name of the database (for 8087 it is "qc")
    2. `influx`
    2. `show databases`
    3. `use qc`  <-- use the proper database as found in influxdb.conf
    4. `show series`
    5. `select count(*) from cpuUsedPercentage` <-- use the correct metrics name
    6. Repeat the last command and see if the number increases. If it increases it denotes that the metrics is stored correctly in the database. If it is the case, the problem lies in your grafana.
    
### Monitoring setup for building the grafana dashboard

1. Go to `root@flptest1`
2. Edit telegraf config file, add following lines:
```
echo "[[inputs.socket_listener]]    
   service_address = \"udp://:8089\"" >> /etc/telegraf/telegraf.conf
```
3. Restart telegraf: `systemctl restart telegraf`
4. Open port
```
firewall-cmd --add-port=8089/udp --zone=public --permanent
firewall-cmd --reload
```
5. Go to Grafana (flptest1.cern.ch:3000) and login as `admin`, the password is in here: https://gitlab.cern.ch/AliceO2Group/system-configuration/-/blob/dev/ansible/roles/grafana/vars/main.yml#L2
6. Go to "Explore" tab (4th icon from top), and select `qc` as data source
7. Run your workflow with `--monitoring-backend influxdb-udp://flptest1.cern.ch:8089 --resources-monitoring 2`
8. The metrics should be there 
9. Make a copy of the QC dashboard that you can edit.
10. Set the monitoring url to `"url": "stdout://?qc,influxdb-udp://flptest1.cern.ch:8089"`
11. Once the dashboard is ready, tell Adam.

### Monitoring setup for building the grafana dashboard with prod data

1. Go to http://pcald24.cern.ch:3000/?orgId=1


### Avoid writing QC objects to a repository

In case of a need to avoid writing QC objects to a repository, one can choose the "Dummy" database implementation in the config file. This is might be useful when one expects very large amounts of data that would be stored, but not actually needed (e.g. benchmarks).

### QCG 

#### Generalities

The QCG server for qcg-test.cern.ch is hosted on qcg@barth-qcg. The config file is `/home/qcg/QualityControlGui/config.js`.

#### Access rights

Any one in alice-member has access. We use the egroup alice-o2-qcg-access to grant access or not and this egroup contains alice-member plus a few extra. This allows for non ALICE members to access the QCG. 

#### Start and stop

`systemctl restart qcg`

### Update host certificate for qcg-test

1. Create a new host certificate and download it
2. scp the p12 file to qcg-test
3. `openssl pkcs12 -in /tmp/qcg-test-new.p12 -out qcg-test.pem -clcerts -nokeys`
4. `openssl pkcs12 -in /tmp/qcg-test-new.p12 -out qcg-test.key -nocerts -nodes`
5. `cd /etc/pki/tls/certs/ ; mv qcg-test.pem qcg-test.pem-old ; mv path/to/qcg-test.pem .`
6. `cd /etc/pki/tls/private/ ; mv qcg-test.key qcg-test.key-old ; mv path/to/qcg-test.key .`
7. Check that it works by connecting to the website and clicking the little lock in the bar to display the certificate.

### Logging

We use the infologger. There is a utility class, `QcInfoLogger`, that can be used. It is a singleton. See [the header](../Framework/include/QualityControl/QcInfoLogger.h) for its usage.

Related issues : https://alice.its.cern.ch/jira/browse/QC-224

To have the full details of what is sent to the logs, do `export O2_INFOLOGGER_MODE=raw`.

### Service Discovery (Online mode)

Service discovery (Online mode) is used to list currently published objects by running QC tasks and checkers. It uses Consul to store:
 - List of running QC tasks that respond to health check, known as "services" in Consul
 - List of published object by each QC task ("service"), knows as "tags" of a "service" in Consul
 - List of published Quality Objects by each CheckRunner. 

Both lists are updated from within QC task using [Service Discovery C++ API](#Service-Discovery-C++-API-and-Consul-HTTP-API):
- `register` - when a tasks starts
- `deregister` - when tasks ends

If the "health check" is failing, make sure the ports 7777 (Tasks) and 7778 (CheckRuners) are open.

#### Register (and health check)
When a QC task starts, it register its presence in Consul by calling [register endpoit of Consul HTTP API](https://www.consul.io/api/agent/service.html#register-service). The request needs the following fields:
- `Id` - Task ID (must be unique)
- `Name` - Task name, tasks can have same name when they run on mutiple machines
- `Tags` - List of published objects
- `Checks` - Array of health check details for Consul, each should contain `Name`, `Interval`, type of check with endpoint to be check by Consul (eg. `"TCP": "localhost:1234"`) and `DeregisterCriticalServiceAfter` that defines timeout to automatically deregister service when fails health checks (minimum value `1m`).

#### Deregister
In order to deregister a service [`deregister/:Id` endpoint of Consul HTTP API](https://www.consul.io/api/agent/service.html#deregister-service) needs to be called. It does not need any additional parameters.

### QCG and QC integration tests 

What are the QC integration tests in the FLP Pipeline doing?

- They pretend to be one of us when doing a test for an FLP release.
- Start a QC environment
- Opening QCG and check existence of certain objects in offline & online mode and that when you click on them they open a plot
- Stop/Destroy that env

Those object names are configurable from Ansible so that we do not have to release a new QCG rpm if we need to update the objects we check. So, if you know something will change 
modify the following file: https://gitlab.cern.ch/AliceO2Group/system-configuration/-/blob/dev/ansible/roles/flp-deployment-checks/templates/qcg-test-config.js.j2

If this test fail and one wants to investigate, they should first resume the VM in openstack. Then the normal web interfaces are available. 

### Check the logs of the QCG

```
journalctl -u o2-qcg
```

### Deploy a modified version of the ansible recipes

When working on the ansible recipes and deploying with o2-flp-setup, the recipes to modify are in 
`.local/share/o2-flp-setup/system-configuration/`. 

### Test with STFBuilder
https://alice.its.cern.ch/jira/browse/O2-169
```
o2-o2-readout-exe file:///afs/cern.ch/user/b/bvonhall/dev/alice/sw/slc7_x86-64/DataDistribution/latest/config/readout_emu.cfg
 
StfBuilder \
	--id stf_builder-0 \
	--transport shmem \
	--detector TPC \
	--dpl-channel-name=dpl-chan \
	--channel-config "name=dpl-chan,type=push,method=bind,address=ipc:///tmp/stf-builder-dpl-pipe-0,transport=shmem,rateLogging=1" \
	--channel-config "name=readout,type=pull,method=connect,address=ipc:///tmp/readout-pipe-0,transport=shmem,rateLogging=1"
        --detector-rdh=4
 
o2-dpl-raw-proxy \
      -b \
      --session default \
      --dataspec "B:TPC/RAWDATA" \
      --channel-config "name=readout-proxy,type=pull,method=connect,address=ipc:///tmp/stf-builder-dpl-pipe-0,transport=shmem,rateLogging=1" \
 | o2-qc \
      --config json://$PWD/datadistribution.json \
      -b \
      --session default
```


### How to connect to one of the builder nodes used in GH PRs 

Install aurora. 
https://alisw.github.io/infrastructure-aurora
The only one that worked for me was the precompiled macos binary. 

Don't forget to install the CERN CA (see the troubleshooting at the bottom).
* `scp lxplus.cern.ch:/etc/ssl/certs/ca-bundle.crt ca-bundle.crt`
* `export REQUESTS_CA_BUNDLE=$PWD/ca-bundle.crt`
* `aurora task ssh -l root build/mesosci/prod/ci_alisw_slc8-gpu-builder_latest/1`

You might also have to ask Costin to install your public key if it a node prepared by him. 

Then 
* `docker ps` to see what is running
* `docker exec -it id /bin/bash` to enter the environment "id"
* `cd /mnt/mesos/sandbox/sandbox/`
* `cd [check-name]`
* `WORK_DIR=$PWD/sw source sw/slc7_x86-64/QualityControl/latest/etc/profile.d/init.sh `
* `cd sw/BUILD/QualityControl-latest/QualityControl/tests/`
* run the test


The problem is that builds continuously happen in the machine. So you cannot just do `cd sw/BUILD/O2-latest/O2/` and `make`

### How to find from which IP an object was sent to the QCDB ? 

Use `curl 'http://ccdb-test.cern.ch:8080/browse/qc/path/to/object?Accept=text/json' | jq '.["objects"][] | .UploadedFrom'`.

Example:
```
~ $ curl 'http://ccdb-test.cern.ch:8080/browse/qc/ITS/MO/ITSTrackTask/AngularDistribution?Accept=text/json' | jq '.["objects"][] | .UploadedFrom'
% Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
Dload  Upload   Total   Spent    Left  Speed
100  974k    0  974k    0     0  6015k      0 --:--:-- --:--:-- --:--:-- 6015k
"128.141.19.252"
"165.132.27.119"
"128.141.19.252"
"165.132.27.119"
"128.141.19.252"
"165.132.27.119"
"128.141.19.252"
```

### Remove empty folders in QCDB

```bash
psql -h localhost ccdb ccdb_user -c "delete from ccdb_paths where pathid in (select pathid from ccdb_stats where object_count=0);"
```

### Count number of objects (not versions) in a path

`curl -s 'http://ali-qcdb-gpn.cern.ch:8083/latest/qc/EMC.*' | grep -c ^Path:`

### Update the certificate of the QCDB

1. go to ca.cern.ch -> New Grid Host Certificate. Subject: alio2-cr1-hv-qcdb-gpn.cern.ch (alternative: alio2-cr1-hv-qcdb.cern.ch)
2. download
3. scp the p12 file to qcdb
4. `openssl pkcs12 -in /tmp/new-certif.p12 -out hostcert.pem -clcerts -nokeys`
5. `openssl pkcs12 -in /tmp/new-certif.p12 -out hostkey.pem -nocerts -nodes`
6. `cd /var/lib/pgsql/.globus `
7. backup the old files
8. copy hostcert and hostkey
9 chmod 600 them

### ControlWorkflows

#### Parameter `qcConfiguration` in tasks

This parameter is used to point to the config file in consul that should be loaded and passed to the task, check
and aggregator runners upon the Start transition. It looks like:
```
qcConfiguration: "{{ ToPtree(GetConfig('qc/ANY/any/stfb_to_daqtask-cru-ctp2'), 'json') }}"
```
Typically a task gets a config file via the command line options which is used to prepare the workflow and then the 
consul one is loaded, if available, when starting. 

The reconfiguration is done only on devices that have the label `qc-reconfigurable` as defined in `Framework/Core/src/O2ControlLabels.cxx`, 
i.e. `o2::framework::ecs::qcReconfigurable`. This is the case for Tasks, Checks and Aggregators runners. 

A more complex example with dumping the payload for debugging:
```
qcConfiguration: {{ ToPtree(Dump(GetConfigLegacy('qc/ANY/any/stfb_to_daqtask-alio2-cr1-mvs03'), '/tmp/QcTaskPayload-' + environment_id + '.dump'), 'json') }}
```

Related issue: QC-310

### ccdb-test connection

Ask Costin to put your key on the server. 

Address: root@alicdb1
So the file repo is in the default location, `/root/QC`, but the database is also there. Careful.


### Config file on EPNs

The config files on EPNs are merged to build a humongous config file used for the whole workflow. 
The common part is stored here: https://github.com/AliceO2Group/O2DPG/blob/master/DATA/production/qc-sync/qc-global-epn.json
The config file to use for each detector are defined [here](https://github.com/AliceO2Group/O2DPG/blob/master/DATA/production/qc-workflow.sh)

To use a different common part file, one can copy the `qc-global-epn.json` to some path on the EPNs (from the gateway, in your home as it is shared) and change in that json what we need for the tests. Then you can add to `pdp_extra_env_vars` the variable (they are space separated) : `QC_JSON_GLOBAL=/path/to/your/qc-global-epn.json` and that will be used

## run locally multi-node
Terminal 1
```bash
cd sw/BUILD/QualityControl-latest/QualityControl
export JSON_DIR=${PWD}/tests
export UNIQUE_PORT_1=12345
export UNIQUE_PORT_2=12346
o2-qc-run-producer --producers 2 --message-amount 1500  --message-rate 10 -b | o2-qc --config json://${JSON_DIR}/multinode-test.json -b --local --host localhost --run 
```
Terminal 2
```bash
cd sw/BUILD/QualityControl-latest/QualityControl
export JSON_DIR=${PWD}/tests
export UNIQUE_PORT_1=12345
export UNIQUE_PORT_2=12346
o2-qc --config json://${JSON_DIR}/multinode-test.json -b --remote --run

## Notes on the host certificate for qcdb

- It must be a "grid host certificate", not a "cern host certificate". 
- subject must be `alio2-cr1-hv-qcdb-gpn.cern.ch`
- no password
- SAN: don't forget to add the alias: `ali-qcdb-gpn.cern.ch`
- Convert the p12 to the pem:
   ```
   openssl pkcs12 -in alio2-cr1-hv-qcdb-gpn.p12 -out alio2-cr1-hv-qcdb-gpn.crt.pem -clcerts -nokeys
   openssl pkcs12 -in alio2-cr1-hv-qcdb-gpn.p12 -out alio2-cr1-hv-qcdb-gpn.key.pem -nocerts -nodes
   ```
- The name and path of the files must be : $HOME/.globus/host{cert,key}.pem
  
=======
```

## Collect statistics about versions published by detectors

On the QCDB, become `postgres` and launch `psql`. 

To get the number of objects in a given run :
```
select count(distinct pathid) from ccdb where ccdb.metadata -> '1048595860' = '529439';
```

(1048595860 is the metadata id for RunNumber obtained with `select metadataid from ccdb_metadata where metadatakey = 'RunNumber';`)

Metadata:

- RunNumber=1048595860
- qc_detector_name=1337188343

query to see for a given run the number of objects per detector:
```
select ccdb.metadata -> '1337188343' as detector, count(distinct path), SUM(COUNT(distinct path)) from ccdb, ccdb_paths where ccdb_paths.pathid = ccdb.pathid AND ccdb.metadata -> '1048595860' = '529439' group by detector;
```

### Merge and upload QC results for all subjobs of a grid job

Please keep in mind that the file pattern in Grid could have changed since this was written.
```
#!/usr/bin/env bash
set -e
set -x
set -u

# we get the list of all QC.root files for o2_ctf and o2_rawtf directories
alien_find /alice/data/2022/LHC22m/523821/apass2_cpu 'o2_*/QC.root' > qc.list
# we add alien:// prefix, so ROOT knows to look for them in alien
sed -i -e 's/^/alien:\/\//' qc.list
# we split the big list into smallers ones of -l lines, so we can parallelize the processing
# one can play with the -l parameter
split -d -l 25 qc.list qc_list_

# for each split file run the merger executable
for QC_LIST in qc_list_*
do
  o2-qc-file-merger --enable-alien --input-files-list "${QC_LIST}" --output-file "merged_${QC_LIST}.root" &
done

# wait for the jobs started in the loop
wait $(jobs -p)

# we merge the files of the first "stage"
o2-qc-file-merger --input-files merged_* --output-file QC_fullrun.root

# we take the first QC config file we find and use it to perform the remote-batch QC
CONFIG=$(alien_find /alice/data/2022/LHC22m/523821/apass2_cpu QC_production.json | head -n 1)
if [ -n "$CONFIG" ]
then
  alien_cp "$CONFIG" file://QC_production.json
  # we override activity values, as QC_production.json might have only placeholders
  o2-qc --remote-batch QC_fullrun.root --config "json://QC_production.json" -b --override-values "qc.config.Activity.number=523897;qc.config.Activity.passName=apass2;qc.config.Activity.periodName=LHC22m"
fi
```
