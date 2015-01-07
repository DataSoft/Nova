#!/bin/bash

QUERY1="
PRAGMA page_size = 4096;
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;
PRAGMA synchronous = NORMAL;

CREATE TABLE packet_counts(
	ip TEXT,
	interface TEXT,

	count_tcp INTEGER,
	count_udp INTEGER,
	count_icmp INTEGER,
	count_other INTEGER,
	count_total INTEGER,
	count_tcpRst INTEGER,
	count_tcpAck INTEGER,
	count_tcpSyn INTEGER,
	count_tcpFin INTEGER,
	count_tcpSynAck INTEGER,
	count_bytes INTEGER,

	FOREIGN KEY (ip, interface) REFERENCES suspects(ip, interface),
	PRIMARY KEY(ip, interface)
);


CREATE TABLE suspects (
	ip TEXT,
	interface TEXT,
	mac UNSIGNED INTEGER,

	startTime INTEGER,
	endTime INTEGER,
	lastTime INTEGER,

	classification DOUBLE,
	hostileNeighbors INTEGER,
	isHostile INTEGER,

	classificationNotes TEXT,

	ip_traffic_distribution DOUBLE,
	port_traffic_distribution DOUBLE,
	packet_size_mean DOUBLE,
	packet_size_deviation DOUBLE,
	distinct_ips DOUBLE,
	distinct_tcp_ports DOUBLE,
	distinct_udp_ports DOUBLE,
	avg_tcp_ports_per_host DOUBLE,
	avg_udp_ports_per_host DOUBLE,
	tcp_percent_syn DOUBLE,
	tcp_percent_fin DOUBLE,
	tcp_percent_rst DOUBLE,
	tcp_percent_synack DOUBLE,
	haystack_percent_contacted DOUBLE,

	PRIMARY KEY(ip, interface)
);

CREATE INDEX idx ON suspects(classification);


/* Basically a copy of the suspects table with a new key added. Annoying there isn't a good way to copy the schema in sqlite */
CREATE TABLE suspect_alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
	ip TEXT,
	mac INTEGER,
	interface TEXT,

	startTime INTEGER,
	endTime INTEGER,
	lastTime INTEGER,

	classification DOUBLE,
	hostileNeighbors INTEGER,
	isHostile INTEGER,

	classificationNotes TEXT,

	ip_traffic_distribution DOUBLE,
	port_traffic_distribution DOUBLE,
	packet_size_mean DOUBLE,
	packet_size_deviation DOUBLE,
	distinct_ips DOUBLE,
	distinct_tcp_ports DOUBLE,
	distinct_udp_ports DOUBLE,
	avg_tcp_ports_per_host DOUBLE,
	avg_udp_ports_per_host DOUBLE,
	tcp_percent_syn DOUBLE,
	tcp_percent_fin DOUBLE,
	tcp_percent_rst DOUBLE,
	tcp_percent_synack DOUBLE,
	haystack_percent_contacted DOUBLE
);

CREATE TABLE packet_sizes (
	ip TEXT,
	interface,

	packetSize INTEGER,
	count INTEGER,
	
	PRIMARY KEY(ip, interface, packetSize),
	FOREIGN KEY (ip, interface) REFERENCES suspects(ip, interface)
);

CREATE TABLE ip_port_counts (
	ip TEXT,
	interface,

	type TEXT,
	dstip TEXT,
	port INTEGER,
	count INTEGER,
	
	FOREIGN KEY (ip, interface) REFERENCES suspects(ip, interface),
	PRIMARY KEY(ip, interface, type, dstip, port)
);


/* We keep track of what honeypot IPs are currently up so we can join against the ip_port_counts for haystack_percent_contacted */
CREATE TABLE honeypots (
	ip TEXT,
	interface TEXT,
	PRIMARY KEY (ip)
);

"

novadDbFilePath="$DESTDIR/usr/share/nova/userFiles/data/novadDatabase.db"
echo "Creating database schemas in $novadDbFilePath"
rm -fr "$novadDbFilePath"
sqlite3 "$novadDbFilePath" <<< $QUERY1
chgrp nova "$novadDbFilePath"
chmod g+rw "$novadDbFilePath"


SCRIPT_ALERT_SCHEMA="
PRAGMA page_size = 4096;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;

CREATE TABLE script_alerts(
	id INTEGER PRIMARY KEY,

	ip TEXT,
	interface TEXT,
	
	script TEXT,
	alert TEXT
	);
"

scriptAlertDb="$DESTDIR/usr/share/nova/userFiles/data/scriptAlerts.db"
echo "Creating database schemas in $scriptAlertDb"
rm -fr "$scriptAlertDb"
sqlite3 "$scriptAlertDb" <<< $SCRIPT_ALERT_SCHEMA
chgrp nova "$scriptAlertDb"
chmod o+rw "$scriptAlertDb"



QUERY2="
PRAGMA journal_mode = WAL;

CREATE TABLE firstrun(
	run TIMESTAMP PRIMARY KEY
);

CREATE TABLE suspectsSeen(
	ip VARCHAR(16),
	interface VARCHAR(16),
	seenSuspect INTEGER,
	seenAllData INTEGER,

	PRIMARY KEY(ip, interface)
);

CREATE TABLE novalogSeen(
	linenum INTEGER PRIMARY KEY,
	line VARCHAR(2048),
	seen INTEGER
);

CREATE TABLE honeydlogSeen(
	linenum INTEGER PRIMARY KEY,
	line VARCHAR(2048),
	seen INTEGER
);

CREATE TABLE credentials(
	user VARCHAR(100) PRIMARY KEY,
	pass VARCHAR(100),
	salt VARCHAR(100)
);

CREATE TABLE lastHoneydNodeIPs(
	mac VARCHAR(100) PRIMARY KEY,
	ip varchar(100)
);

CREATE TABLE lastTrainingDataSelection(
	uid INTEGER PRIMARY KEY,
	included INTEGER
);

"


quasarDbFilePath="$DESTDIR/usr/share/nova/userFiles/data/quasarDatabase.db"
echo "Creating database schemas in $quasarDbFilePath"
rm -fr "$quasarDbFilePath"
sqlite3 "$quasarDbFilePath" <<< $QUERY2
chgrp nova "$quasarDbFilePath"
chmod g+rw "$quasarDbFilePath"

pulsarDbFilePath="$DESTDIR/usr/share/nova/userFiles/data/pulsarDatabase.db"
echo "Creating database schemas in $pulsarDbFilePath"
rm -fr "$pulsarDbFilePath"
sqlite3 "$pulsarDbFilePath" <<< $QUERY2
chgrp nova "$pulsarDbFilePath"
chmod g+rw "$pulsarDbFilePath"

echo "SQL schema has been set up for nova."
