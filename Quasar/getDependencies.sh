#!/bin/bash
# This is a quick install script for nodejs and cvv8 headers

# Get nodejs

# Old version
#wget https://github.com/joyent/node/zipball/v0.6.12
#unzip v0.6.12
#cd joyent-node-346a3a8


version=$(node --version)
perm=$SUDO_USER:$SUDO_USER
if [[ $version != "v0.8.5" ]];
then

	wget http://nodejs.org/dist/v0.8.5/node-v0.8.5.tar.gz
	tar -xf node-v0.8.5.tar.gz
	chown -R -f $perm node-v0.8.5/
	cd node-v0.8.5

	./configure
	make
	make install

else
	echo "Already found correct nodejs version. Skipping."
fi

npm config set ca=""

# Install the forever daemon
npm install -g forever

# Get cvv8
wget http://v8-juice.googlecode.com/files/libv8-convert-20120219.tar.gz
tar -xf libv8-convert-20120219.tar.gz
sudo cp -fr libv8-convert-20120219/include/cvv8 /usr/include
