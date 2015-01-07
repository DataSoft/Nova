
#The default build should be a release build
all: release

all-the-things: release 

#Release Target
release:
	protoc -I=NovaLibrary/src/ --cpp_out=NovaLibrary/src NovaLibrary/src/protobuf/marshalled_classes.proto
	$(MAKE) novalib-release
	$(MAKE) ui_core-release
	$(MAKE) release-helper
	$(MAKE) quasar

release-helper: novad-release novacli-release hhconfig-release

#Debug target
debug:
	protoc -I=NovaLibrary/src/ --cpp_out=NovaLibrary/src NovaLibrary/src/protobuf/marshalled_classes.proto
	$(MAKE) novalib-debug
	$(MAKE) ui_core-debug
	$(MAKE) debug-helper
	$(MAKE) quasar

debug-helper: novad-debug novacli-debug hhconfig-debug novatrainer-debug

#Nova Library
novalib-release:
	$(MAKE) -C NovaLibrary/Release
	cp NovaLibrary/Release/libNovaLibrary.a NovaLibrary/

novalib-debug:
	$(MAKE) -C NovaLibrary/Debug
	cp NovaLibrary/Debug/libNovaLibrary.a NovaLibrary/

#novad
novad-release:
	$(MAKE) -C Novad/Release
	cp Novad/Release/novad Novad/

novad-debug:
	$(MAKE) -C Novad/Debug
	cp Novad/Debug/novad Novad/

#UI_Core
ui_core-release:
	$(MAKE) -C Nova_UI_Core/Release
	cp Nova_UI_Core/Release/libNova_UI_Core.so Nova_UI_Core/

ui_core-debug:
	$(MAKE) -C Nova_UI_Core/Debug
	cp Nova_UI_Core/Debug/libNova_UI_Core.so Nova_UI_Core/

#nova CLI
novacli-release:
	$(MAKE) -C NovaCLI/Release
	cp NovaCLI/Release/novacli NovaCLI/

novacli-debug:
	$(MAKE) -C NovaCLI/Debug
	cp NovaCLI/Debug/novacli NovaCLI/

# Nova trainer
novatrainer-debug:
	$(MAKE) -C NovaTrainer/Debug
	cp NovaTrainer/Debug/novatrainer NovaTrainer/novatrainer

novatrainer-release:
	$(MAKE) -C NovaTrainer/Release
	cp NovaTrainer/Release/novatrainer NovaTrainer/novatrainer

#Quasar
quasar: nodejsmodule
	cd Quasar;npm --unsafe-perm install

nodejsmodule:
	cd NodejsModule;npm --unsafe-perm install	

#Pulsar
pulsar:
	cd Pulsar;npm --unsafe-perm install

#Honeyd AutoConfig
hhconfig-release:
	$(MAKE) -C HaystackAutoConfig/Release
	cp HaystackAutoConfig/Release/haystackautoconfig HaystackAutoConfig/

hhconfig-debug:
	$(MAKE) -C HaystackAutoConfig/Debug
	cp HaystackAutoConfig/Debug/haystackautoconfig HaystackAutoConfig/

coverageTests: test-prepare
	protoc -I=NovaLibrary/src/ --cpp_out=NovaLibrary/src NovaLibrary/src/protobuf/marshalled_classes.proto
	$(MAKE) -C NovaLibrary/Coverage
	$(MAKE) -C Nova_UI_Core/Coverage
	$(MAKE) -C NovaTest/Coverage

#Unit tests
test: test-prepare
	#$(MAKE) debug
	$(MAKE) -C NovaTest/Debug

test-prepare:
	# Make the folder if it doesn't exist
	mkdir -p NovaTest/NovadSource
	# Make new links to the cpp files
	rm -fr NovaTest/NovadSource/*.cpp
	ln Novad/src/*.cpp NovaTest/NovadSource/
	# Make new links to the h files
	rm -fr NovaTest/NovadSource/*.h
	ln Novad/src/*.h NovaTest/NovadSource/
	# Delete the link to Main so we don't have multiple def of main()
	rm -f NovaTest/NovadSource/Main.cpp

clean: clean-lib clean-ui-core clean-novad clean-test clean-hhconfig clean-quasar clean-novatrainer clean-staging clean-cli clean-node

clean-dev: clean-lib clean-ui-core clean-novad clean-test clean-hhconfig clean-quasar clean-novatrainer clean-staging clean-cli

clean-node:
	rm -fr Quasar/node_modules

#remove binaries from staging area
clean-staging:
	rm -f NovaCLI/novacli
	rm -f Novad/novad
	rm -f Nova_UI_Core/libNova_UI_Core.so
	rm -f NovaLibrary/libNovaLibrary.a

#Removes created man pages
clean-man:
	rm -f Installer/miscFiles/manpages/*.gz

clean-novad: clean-novad-debug clean-novad-release

clean-novad-debug:
	$(MAKE) -C Novad/Debug clean

clean-novad-release:
	$(MAKE) -C Novad/Release clean

clean-ui-core: clean-ui-core-debug clean-ui-core-release clean-ui-core-coverage

clean-ui-core-debug:
	$(MAKE) -C Nova_UI_Core/Debug clean
	rm -f Nova_UI_Core/Debug/Nova_UI_Core

clean-ui-core-release:
	$(MAKE) -C Nova_UI_Core/Release clean
	rm -f Nova_UI_Core/Release/Nova_UI_Core

clean-ui-core-coverage:
	$(MAKE) -C Nova_UI_Core/Coverage clean
	rm -f Nova_UI_Core/Coverage/Nova_UI_Core

clean-lib: clean-lib-debug clean-lib-release clean-lib-coverage
	
clean-lib-debug:
	$(MAKE) -C NovaLibrary/Debug clean
	rm -f NovaLibrary/src/protobuf/*.cc NovaLibrary/src/protobuf/*.h

clean-lib-release:
	$(MAKE) -C NovaLibrary/Release clean
	rm -f NovaLibrary/src/protobuf/*.cc NovaLibrary/src/protobuf/*.h

clean-lib-coverage:
	$(MAKE) -C NovaLibrary/Coverage clean
	rm -f NovaLibrary/src/protobuf/*.cc NovaLibrary/src/protobuf/*.h

clean-cli: clean-cli-debug clean-cli-release

clean-cli-debug:
	$(MAKE) -C NovaCLI/Debug clean

clean-cli-release:
	$(MAKE) -C NovaCLI/Release clean

clean-test:
	rm -fr NovaTest/NovadSource/*
	rm -f NovaTest/Coverage/NovadSource/*.d
	rm -f NovaTest/Coverage/NovadSource/*.o
	rm -f NovaTest/Coverage/src/NovadSource/*.d
	rm -f NovaTest/Coverage/src/NovadSource/*.o
	$(MAKE) -C NovaTest/Coverage clean

	rm -fr NovaTest/NovadSource/*
	rm -f NovaTest/Debug/NovadSource/*.d
	rm -f NovaTest/Debug/NovadSource/*.o
	rm -f NovaTest/Debug/src/NovadSource/*.d
	rm -f NovaTest/Debug/src/NovadSource/*.o
	$(MAKE) -C NovaTest/Debug clean

clean-nodejsmodule:
	-cd NodejsModule; node-waf clean
	-cd NodejsModule; node-gyp clean

clean-quasar: clean-nodejsmodule
	-rm -rf NodejsModule/build

clean-quasar-modules:
	-rm -rf Quasar/node_modules

clean-pulsar-modules:
	-rm -rf Pulsar/node_modules

clean-hhconfig: clean-hhconfig-debug clean-hhconfig-release
	
clean-hhconfig-debug:
	$(MAKE) -C HaystackAutoConfig/Debug clean

clean-hhconfig-release:
	$(MAKE) -C HaystackAutoConfig/Release clean


clean-novatrainer: clean-novatrainer-debug clean-novatrainer-release

clean-novatrainer-debug:
	$(MAKE) -C NovaTrainer/Debug clean

clean-novatrainer-release:
	$(MAKE) -C NovaTrainer/Release clean

#Installation (requires root)
install: install-data
	$(MAKE) install-helper
	# Give read/write permissions to the nova group
	-chmod -R g+rw "$(DESTDIR)/usr/share/nova"
	-chmod -R g+rw "$(DESTDIR)/var/log/honeyd"
	-chmod g+rwx "$(DESTDIR)/var/log/nova"
	-chmod a+x ~/.config

install-helper: install-docs install-cli install-novad install-ui-core install-hhconfig install-quasar install-nodejsmodule install-novatrainer
	-sh debian/postinst
	-bash Installer/createDatabase.sh

install-data:
	#make folder in etc with path locations to nova files
	mkdir -p "$(DESTDIR)/usr/bin"
	mkdir -p "$(DESTDIR)/usr/lib"
	mkdir -p "$(DESTDIR)/usr/share/applications"
	mkdir -p "$(DESTDIR)/usr/share/nova"
	mkdir -p "$(DESTDIR)/etc/logrotate.d/"
	mkdir -p "$(DESTDIR)/usr/share/man/man1"
	mkdir -p "$(DESTDIR)/var/log/honeyd"
	mkdir -p "$(DESTDIR)/etc/rsyslog.d/"
	mkdir -p "$(DESTDIR)/etc/sysctl.d/"
	mkdir -p "$(DESTDIR)/etc/bash_completion.d/"
	mkdir -p "$(DESTDIR)/etc/sudoers.d/"
	mkdir -p "$(DESTDIR)/var/log/nova"
	
	
	cp -frup Installer/sharedFiles "$(DESTDIR)/usr/share/nova/sharedFiles"
	cp -frup Installer/userFiles "$(DESTDIR)/usr/share/nova/userFiles"
	cp -frup Installer/nova_init "$(DESTDIR)/usr/share/nova"
	cp -frup Installer/miscFiles/novalr "$(DESTDIR)/etc/logrotate.d"
	cp -frup Installer/createDatabase.sh "$(DESTDIR)/usr/share/nova"

	#Copy the scripts and logs
	install Installer/nova_init "$(DESTDIR)/usr/bin"
	install Installer/nova_rsyslog_helper "$(DESTDIR)/usr/bin"
	install Installer/miscFiles/createNovaScriptAlert.py "$(DESTDIR)/usr/bin"
	install Installer/miscFiles/novamaildaemon.pl "$(DESTDIR)/usr/bin"
	install Installer/miscFiles/cleannovasendmail "$(DESTDIR)/usr/bin"
	install Installer/miscFiles/placenovasendmail "$(DESTDIR)/usr/bin"
	install Installer/miscFiles/novasendmail "$(DESTDIR)/usr/bin"
	#Install permissions
	install Installer/miscFiles/sudoers_nova "$(DESTDIR)/etc/sudoers.d/" --mode=0440
	install Installer/miscFiles/40-nova.conf "$(DESTDIR)/etc/rsyslog.d/" --mode=664
	install Installer/miscFiles/30-novactl.conf "$(DESTDIR)/etc/sysctl.d/" --mode=0440
	# Copy the bash completion files
	install Installer/miscFiles/novacli "$(DESTDIR)/etc/bash_completion.d/" --mode=755

install-pcap-debug:
	#debug sudoers file that allows sudo gdb to pcap without password prompt
	install Installer/miscFiles/sudoers_nova_debug "$(DESTDIR)/etc/sudoers.d/" --mode=0440

install-docs:
	gzip -c Installer/miscFiles/manpages/novad.1 > Installer/miscFiles/manpages/novad.1.gz
	gzip -c Installer/miscFiles/manpages/novacli.1 > Installer/miscFiles/manpages/novacli.1.gz
	gzip -c Installer/miscFiles/manpages/quasar.1 > Installer/miscFiles/manpages/quasar.1.gz
	install Installer/miscFiles/manpages/*.1.gz "$(DESTDIR)/usr/share/man/man1"

install-quasar:
	cp -frup Quasar "$(DESTDIR)/usr/share/nova/sharedFiles"
	-install Quasar/quasar "$(DESTDIR)/usr/bin/quasar"

install-pulsar:
	cp -frup Pulsar "$(DESTDIR)/usr/share/nova/sharedFiles"
	-install Pulsar/pulsar "$(DESTDIR)/usr/bin/pulsar"	

install-hhconfig:
	-install HaystackAutoConfig/haystackautoconfig "$(DESTDIR)/usr/bin/haystackautoconfig"
	-install Installer/miscFiles/sudoers_HHConfig "$(DESTDIR)/etc/sudoers.d/" --mode=0440

install-novad:
	-install Novad/novad "$(DESTDIR)/usr/bin"

install-ui-core:
	-install Nova_UI_Core/libNova_UI_Core.so "$(DESTDIR)/usr/lib"

install-cli:
	-install NovaCLI/novacli "$(DESTDIR)/usr/bin"

install-novatrainer:
	-install NovaTrainer/novatrainer "$(DESTDIR)/usr/bin"

install-nodejsmodule:
	mkdir -p "$(DESTDIR)/usr/share/nova/sharedFiles/NodejsModule"
	-install NodejsModule/build/Release/novaconfig.node "$(DESTDIR)/usr/share/nova/sharedFiles/NodejsModule/"
	-cp -frup NodejsModule/Javascript "$(DESTDIR)/usr/share/nova/sharedFiles/NodejsModule/Javascript"


#Uninstall
uninstall: uninstall-files uninstall-permissions

uninstall-files:
	rm -rf ~/.config/nova
	rm -rf /var/log/nova
	rm -rf "$(DESTDIR)/usr/share/nova"
	rm -f "$(DESTDIR)/usr/bin/novacli"
	rm -f "$(DESTDIR)/usr/bin/novad"
	rm -f "$(DESTDIR)/usr/bin/haystackautoconfig"
	rm -f "$(DESTDIR)/usr/bin/nova_rsyslog_helper"
	rm -f "$(DESTDIR)/usr/bin/nova_init"
	rm -f "$(DESTDIR)/usr/bin/quasar"
	rm -f "$(DESTDIR)/usr/bin/novatrainer"
	rm -f "$(DESTDIR)/usr/bin/cleannovasendmail"
	rm -f "$(DESTDIR)/usr/bin/novamaildaemon.pl"
	rm -f "$(DESTDIR)/usr/bin/placenovasendmail"
	rm -f "$(DESTDIR)/usr/bin/novasendmail"
	rm -f "$(DESTDIR)/usr/lib/libNova_UI_Core.so"
	rm -f "$(DESTDIR)/etc/sudoers.d/sudoers_nova"
	rm -f "$(DESTDIR)/etc/sudoers.d/sudoers_nova_debug"
	rm -f "$(DESTDIR)/etc/sudoers.d/sudoers_HHConfig"
	rm -f "$(DESTDIR)/etc/rsyslog.d/40-nova.conf"
	rm -f "$(DESTDIR)/etc/rsyslog.d/41-nova.conf"
	rm -f "$(DESTDIR)/etc/sysctl.d/30-novactl.conf"
	rm -f "$(DESTDIR)/etc/logrotate.d/novalr"

uninstall-permissions:
	sh debian/postrm

# Reinstall nova without messing up the permissions
reinstall: uninstall-files
	$(MAKE) install

reinstall-debug: uninstall-files
	$(MAKE) install
	sudo -u $$SUDO_USER novacli writesetting SERVICE_PREFERENCES 0:0+\;1:6+\;

# Does a fresh uninstall, clean, build, and install
reset: uninstall-files
	$(MAKE) clean
	$(MAKE) release
	$(MAKE) test 
	$(MAKE) install

reset-debug: 
	$(MAKE) clean-dev
	$(MAKE) debug
	$(MAKE) test
	rm -rf ~/.config/nova
	$(MAKE) reinstall-debug

