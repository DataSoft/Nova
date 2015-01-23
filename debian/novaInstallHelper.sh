#!/bin/bash

PWD=`pwd`

SAVE=$PWD'/'

NEW_PERM=$SUDO_USER:$SUDO_USER

if [ -z $BUILDDIR ]; then
	BUILDDIR=~/nova-build
fi

if [ -z $1 ]; then
	BRANCH="master"
else
	BRANCH="$1"
fi

echo "Build Dir is $BUILDDIR"

check_err() {
	ERR=$?
	if [ "$ERR" -ne "0" ] ; then
		echo "Error occurred during build process; terminating script!"
		exit $ERR
	fi
}

mkdir -p ${BUILDDIR}

echo "##############################################################################"
echo "#                          NOVA DEPENDENCY CHECK                             #"
echo "##############################################################################"
apt-get -y install git build-essential libann-dev libpcap0.8-dev libboost-program-options-dev libboost-serialization-dev sqlite3 libsqlite3-dev libcurl3 libcurl4-gnutls-dev iptables libevent-dev libprotoc-dev protobuf-compiler libdumbnet-dev libpcap-dev libpcre3-dev libedit-dev bison flex libtool automake libcap2-bin libboost-system-dev libboost-filesystem-dev python perl tcl liblinux-inotify2-perl libfile-readbackwards-perl
check_err

echo "##############################################################################"
echo "#                         DOWNLOADING NOVA FROM GIT                          #"
echo "##############################################################################"
cd ${BUILDDIR}
rm -fr Honeyd
rm -fr Nova

git clone git://github.com/DataSoft/Honeyd.git
check_err
git clone git://github.com/DataSoft/Nova.git
check_err

echo "##############################################################################"
echo "#                              BUILDING HONEYD                               #"
echo "##############################################################################"
cd ${BUILDDIR}/Honeyd
git checkout -f $BRANCH
./autogen.sh
check_err
automake
check_err
./configure
check_err
make -j2
check_err
make install
check_err

echo "##############################################################################"
echo "#                             BUILDING NOVA                                  #"
echo "##############################################################################"
cd ${BUILDDIR}/Nova/Quasar
bash getDependencies.sh
check_err
chown -R -f $NEW_PERM node-v0.8.5/
chown -f $NEW_PERM node-v0.8.5.tar.gz
cd ${HOME}
chown -R $NEW_PERM .npm/
check_err
cd ${BUILDDIR}/Nova/Quasar
npm install -g forever
check_err

cd ${BUILDDIR}/Nova
make -j2 debug
check_err
make uninstall-files
make install
check_err

bash ${BUILDDIR}/Nova/Installer/nova_init

echo "##############################################################################"
echo "#                             FETCHING NMAP 6                                #"
echo "##############################################################################"
version=$(nmap --version | sed -n '2p')
if [ "$version" != "Nmap version 6.01 ( http://nmap.org )" ]; then
	cd ${BUILDDIR}
	wget http://nmap.org/dist/nmap-6.01.tar.bz2
	check_err
	tar -xf nmap-6.01.tar.bz2
	check_err
	chown -R nova:nova nmap-6.01
	cd nmap-6.01
	./configure
	check_err
	make -j2
	check_err
	make install
	check_err
else
  echo "Nmap version already matches required version. Skipping step."
fi

cd $SAVE
chown -R -f $NEW_PERM nova-build/
cd $HOME
chown -R -f $NEW_PERM .node-gyp/
chown -R -f $NEW_PERM .config/

cd /usr/share/honeyd/scripts/
chown -R -f $NEW_PERM misc/

echo "##############################################################################"
echo "#                                    DONE                                    #"
echo "##############################################################################"
