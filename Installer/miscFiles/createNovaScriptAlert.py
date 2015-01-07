#!/usr/bin/python

import sqlite3
import sys
import os

databasepath = ""

if ("HONEYD_HOME" in os.environ):
	databasepath = os.getenv('HONEYD_HOME')  + "/../nova/data/scriptAlerts.db"
else:
	databasepath = os.path.expanduser("~/.config/nova/data/scriptAlerts.db")

databasepath = os.path.abspath(databasepath)


if len(sys.argv) != 5:
	sys.stderr.write("Arguements should be: suspectIp suspectInterface scriptName alertInfo\n")
	sys.exit(1)

ip = sys.argv[1]
interface = sys.argv[2]
script = sys.argv[3]
alert = sys.argv[4].encode("utf8")


db = sqlite3.connect(databasepath)
c = db.cursor()

params = [ip, interface, script, alert]
c.execute("INSERT INTO script_alerts (ip, interface, script, alert) VALUES(?, ?, ?, ?)", params)

db.commit()
db.close()

