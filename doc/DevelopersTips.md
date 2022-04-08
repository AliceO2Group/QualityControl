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

## ControlWorkflows

### Parameter `qcConfiguration` in tasks

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

## ccdb-test connection

Ask Costin to put your key on the server. 

Address: root@alicdb1
So the file repo is in the default location, `/root/QC`, but the database is also there. Careful.



