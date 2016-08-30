#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging
#MACHINES=("aidrefpc00"
#          "aido2qc11"
#          "aido2qc12"
#          "aido2qc13"
#          "aido2qc40"
#          "aido2qc41"
#          "aido2qc42"
#          "aido2qc43"
#          )
MACHINES=()
for i in $(seq -f "%03g" 3 45)
do
  echo aidrefpc$i
 MACHINES+=("aidrefpc$i")
done
USER=benchmarkQC

# Notes
# One need to have root password less access to all machines
#for node in ${MACHINES[@]}; do   echo "Setting up machine $node";
#ssh-copy-id root@aidrefpc002
# done

# for node in ${MACHINES[@]}; do
#   echo "Setting up machine $node"
#   ping -c 1 $node
#   ssh root@$node "useradd ${USER}" || true
#   ssh root@$node "mkdir -p /home/$USER/.ssh"
#   ssh root@$node "chmod 700 /home/$USER/.ssh"
#   scp .ssh/authorized_keys root@$node:/home/$USER/.ssh
#   ssh root@$node "chown -R $USER:$USER /home/$USER/.ssh; chmod 644 /home/$USER/.ssh/authorized_keys"
#   ssh root@$node "yum install -y devtoolset-4 devtoolset-4-eclipse mariadb55 mariadb55-mariadb-devel nodejs010 php55 python27 python33 git19 httpd24 mariadb mariadb-devel mariadb-libs > /dev/null 2>&1" &
# #  the following lines are needed only if there is no shared file system
# #  rsync -zvhrcPl --delete -e ssh `readlink -f /opt/hackathon/*` root@$node:/opt/hackathon
# #  rsync -zvhrcPl --delete -e ssh ~/hackathon_install $USER@$node:~
#   scp ~/setup_proto.sh $USER@$node:~
#   ssh $USER@$node "echo \"source ~/setup_proto.sh\" >> .bashrc"
# done

for node in ${MACHINES[@]}; do
    echo "Setting up more, machine $node"
    # ssh root@$node " chmod a+rx /LABshare; rm -df /opt/hackathon; ln -s /LABshare/hackathon-opt /opt/hackathon;"
    # ssh $USER@$node "ln -s /LABshare/hackathon_install ~/hackathon_install;"
    #ssh $USER@$node "awk '!seen[\$0]++' .bashrc > .bashrc.2; mv .bashrc.2 .bashrc"
    #scp bashrc-for-nodes $USER@$node:.bashrc
    #ssh root@$node "rm -f /opt/hackathon;  ln -s /LABshare/hackathon-opt/ /opt/hackathon-11-08-2016; ln -s /opt/hackathon-11-08-2016 /opt/hackathon;"
#ssh root@$node "yum install -y graphviz libcurl-devel > /dev/null 2>&1" &
#scp setup_proto.sh $USER@$node:~
#ssh root@$node "yum clean all ; yum install -y ApMon_cpp* > /dev/null 2>&1" &
ssh root@$node " yum -y groupinstall \"X Window System\"> /dev/null 2>&1" &
done

# TODO add the stuff for all the rpms we need there for devtoolset
#To get all rpms you need : repotrack -a x86_64 devtoolset-4 devtoolset-4-eclipse mariadb55 mariadb55-mariadb-devel nodejs010 php55 python27 python33 git19 httpd24
#Then scp this to all nodes and install with yum