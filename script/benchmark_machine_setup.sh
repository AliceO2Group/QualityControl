#!/usr/bin/env bash
set -e ;# exit on error
set -u ;# exit when using undeclared variable
#set -x ;# debugging
MACHINES=("aido2qc10"
          "aido2qc11"
          "aido2qc12"
          "aido2qc13"
#          "aido2qc40"
#          "aido2qc41"
#          "aido2qc42"
#          "aido2qc43"
          )
USER=benchmarkQC

# Notes
# One need to have root password less access to all machines
# for node in ${MACHINES[@]}; do   echo "Setting up machine $node"; ssh root@$node "mkdir .ssh; chmod 700 .ssh"; scp .ssh/authorized_keys root@$node:~/.ssh; done

for node in ${MACHINES[@]}; do
  echo "Setting up machine $node"
  ping -c 1 $node
  ssh root@$node "useradd ${USER}" || true
  ssh root@$node "mkdir -p /home/$USER/.ssh"
  ssh root@$node "chmod 700 /home/$USER/.ssh"
  scp .ssh/authorized_keys root@$node:/home/$USER/.ssh
  ssh root@$node "chown -R $USER:$USER /home/$USER/.ssh; chmod 644 /home/$USER/.ssh/authorized_keys;\
                  yum -y install rsync; mkdir -p /opt/hackathon"
#  the following lines are needed only if there is no shared file system
#  rsync -zvhrcP --delete -e ssh `readlink -f /opt/hackathon/*` root@$node:/opt/hackathon
#  rsync -zvhrcP --delete -e ssh ~/hackathon_install $USER@$node:~
  scp ~/setup_proto.sh $USER@$node:~
  ssh $USER@$node "echo \"source ~/setup_proto.sh\" >> .bashrc"
done

# TODO add the stuff for all the rpms we need there for devtoolset
#To get all rpms you need : repotrack -a x86_64 devtoolset-4 devtoolset-4-eclipse mariadb55 mariadb55-mariadb-devel nodejs010 php55 python27 python33 git19 httpd24
#Then scp this to all nodes and install with yum