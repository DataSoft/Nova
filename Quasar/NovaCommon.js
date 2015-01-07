//============================================================================
// Name        : NovaCommon.js
// Copyright   : DataSoft Corporation 2011-2013
//  Nova is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   Nova is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
// Description : Common code for Nova modules
//============================================================================

var sys = require('sys');
var exec = require('child_process').exec;
var sql = require('sqlite3').verbose();
var crypto = require('crypto');

var NovaCommon = new function() {
    console.log("Initializing nova C++ code");
    this.novaconfig = require('novaconfig.node');
    this.nova = new this.novaconfig.Instance();
    this.config = new this.novaconfig.NovaConfigBinding();
    this.honeydConfig = new this.novaconfig.HoneydConfigBinding();
    this.vendorToMacDb = new this.novaconfig.VendorMacDbBinding();
    this.osPersonalityDb = new this.novaconfig.OsPersonalityDbBinding();
    this.trainingDb = new this.novaconfig.CustomizeTrainingBinding();
    this.whitelistConfig = new this.novaconfig.WhitelistConfigurationBinding();
    this.LOG = require("../NodejsModule/Javascript/Logger").LOG;

    var classifiersConstructor = new require('./classifiers.js');
    this.classifiers = new classifiersConstructor(this.config);

    this.cNodeToJs = function(node){
        var ret = {};
        ret.enabled = node.IsEnabled();
        ret.pfile = node.GetProfile();
        ret.ip = node.GetIP();
        ret.mac = node.GetMAC();
        ret.interface = node.GetInterface();
        return ret;
    }

    this.StartNovad = function(){
        var command = NovaCommon.config.ReadSetting("COMMAND_START_NOVAD");
        exec(command, function(error, stdout, stderr){
            if(error != null){console.log("Error running command '" + command + "' :" + error);}
        });
    }
    
    this.StopNovad = function(){
        var command = NovaCommon.config.ReadSetting("COMMAND_STOP_NOVAD");
        exec(command, function(error, stdout, stderr){
            if(error != null){console.log("Error running command '" + command + "' :" + error);}
        });
    }
    
    this.StartHaystack = function(cb){
        var command = NovaCommon.config.ReadSetting("COMMAND_START_HAYSTACK");
        exec(command, function(error, stdout, stderr){
            if(error != null){console.log("Error running command '" + command + "' :" + error);}
            cb && cb();
        });
    }
    
    this.StopHaystack = function(cb){
        var command = NovaCommon.config.ReadSetting("COMMAND_STOP_HAYSTACK");
        exec(command, function(error, stdout, stderr){
            if(error != null){console.log("Error running command '" + command + "' :" + error);}
            cb && cb();
        });
    }

    this.GetPorts = function()
    {
      var scriptBindings = {};
      
      var profiles = this.honeydConfig.GetProfileNames();
      
      for(var i in profiles)
      {
        var profileName = profiles[i];
    
        var portSets = this.honeydConfig.GetPortSetNames(profiles[i]);
        for(var portSetName in portSets)
        {
            var portSet = this.honeydConfig.GetPortSet(profiles[i], portSets[portSetName]);
            var ports = [];

                        var tmp = portSet.GetPorts();
            for (var p in tmp)
            {
                ports.push(tmp[p]);
            }
            for(var p in ports)
            {
                if(ports[p].GetScriptName() != undefined && ports[p].GetScriptName() != '')
                {
                    if(scriptBindings[ports[p].GetScriptName()] == undefined)
                    {
                        scriptBindings[ports[p].GetScriptName()] = profileName + "(" + portSetName + ")" + ':' + ports[p].GetPortNum();
                    } else {
                        scriptBindings[ports[p].GetScriptName()] += '<br>' + profileName + "(" + portSetName + ")" + ':' + ports[p].GetPortNum();
                    }
                }
            }
        }
      }
    
      return scriptBindings;
    }

    var self = this;
    this.novaDb = new sql.Database(this.config.GetPathHome() + "/data/novadDatabase.db", sql.OPEN_READWRITE, function(err) {
        if (err) {
            self.LOG("ERROR", "Unable to open novadDatabase.db due to error: " + err)
            process.exit(1);
            return;
        }
    });

    this.db = new sql.Database(this.config.GetPathHome() + "/data/quasarDatabase.db", sql.OPEN_READWRITE, function(err) {
        if (err) {
            self.LOG("ERROR", "Unable to open quasarDatabase.db due to error: " + err);
            process.exit(1);
            return;
        }
    });

    this.hostNameDb = new sql.Database(this.config.GetPathHome() + "/../honeyd/names", sql.OPEN_READWRITE, function(err) {
        if (err) {
            self.LOG("ERROR", "Unable to open honeyd names database due to error: " + err);
            return;
        }
        
        self.dbqGetHostnames = hostNameDb.prepare('SELECT * from allocs');
        self.dbqGetHostname = hostNameDb.prepare('SELECT * from allocs WHERE name = ?');
        self.dbqInsertHostname = hostNameDb.prepare('INSERT into allocs(name) VALUES (?)');
        self.dbqClearHostnameAllocations = hostNameDb.prepare('UPDATE allocs SET IP = NULL');
        self.dbqDeleteHostname = hostNameDb.prepare('DELETE from allocs WHERE name = ?');
    });

    var db = this.db;
    var novaDb = this.novaDb;
    var hostNameDb = this.hostNameDb;


    // We set a few pragmas to speed up access to novadb
    novaDb.run("PRAGMA cache_size = 100000");
    novaDb.run("PRAGMA temp_store = MEMORY");

    // Prepare query statements
    this.dbqCredentialsRowCount = db.prepare('SELECT COUNT(*) AS rows from credentials');
    this.dbqCredentialsCheckLogin = db.prepare('SELECT user, pass FROM credentials WHERE user = ? AND pass = ?');
    this.dbqCredentialsGetUsers = db.prepare('SELECT user FROM credentials');
    this.dbqCredentialsGetUser = db.prepare('SELECT user FROM credentials WHERE user = ?');
    this.dbqCredentialsGetSalt = db.prepare('SELECT salt FROM credentials WHERE user = ?');
    this.dbqCredentialsChangePassword = db.prepare('UPDATE credentials SET pass = ?, salt = ? WHERE user = ?');
    this.dbqCredentialsInsertUser = db.prepare('INSERT INTO credentials VALUES(?, ?, ?)');
    this.dbqCredentialsDeleteUser = db.prepare('DELETE FROM credentials WHERE user = ?');

    this.dbqFirstrunCount = db.prepare("SELECT COUNT(*) AS rows from firstrun");
    this.dbqFirstrunInsert = db.prepare("INSERT INTO firstrun values(datetime('now'))");

    this.dbqSuspectAlertsGet = novaDb.prepare('SELECT * FROM suspect_alerts');
    this.dbqSuspectAlertsDeleteAll = novaDb.prepare('DELETE FROM suspect_alerts');
    this.dbqSuspectAlertsDeleteAlert = novaDb.prepare('DELETE FROM suspect_alerts where id = ?');

     // Queries regarding the seen suspects table
    this.dbqAddNewSuspect = db.prepare('INSERT INTO suspectsSeen values(?, ?, 0, 0)');
    this.dbqIsNewSuspect = db.prepare('SELECT COUNT(*) AS rows from suspectsSeen WHERE ip = ? AND interface = ?');
    this.dbqSeenAllData = db.prepare('SELECT seenAllData FROM suspectsSeen WHERE ip = ? AND interface = ?');
    
    this.dbqMarkAllSuspectSeen = db.prepare('UPDATE suspectsSeen SET seenSuspect = 1');
    this.dbqMarkAllSuspectDataSeen = db.prepare('UPDATE suspectsSeen SET seenAllData = 1');
    
    this.dbqMarkSuspectSeen = db.prepare('UPDATE suspectsSeen SET seenSuspect = 1 WHERE ip = ? AND interface = ?');
    this.dbqMarkSuspectDataSeen = db.prepare('UPDATE suspectsSeen SET seenAllData = 1 WHERE ip = ? and interface = ?');
    
    this.dbqMarkSuspectDataUnseen = db.prepare('UPDATE suspectsSeen SET seenAllData = 0 WHERE ip = ? and interface = ?');
    
    this.dbqGetUnseenSuspects = db.prepare('SELECT ip, interface FROM suspectsSeen WHERE seenSuspect = 0');
    this.dbqGetUnseenDataSuspects = db.prepare('SELECT ip, interface FROM suspectsSeen WHERE seenAllData = 0');


    this.dbqIsNewNovaLogEntry = db.prepare('SELECT COUNT(*) AS rows from novalogSeen WHERE linenum = ?');
    this.dbqAddNovaLogEntry = db.prepare('INSERT INTO novalogSeen VALUES(?, ?, 0)');
    this.dbqMarkNovaLogEntrySeen = db.prepare('UPDATE novalogSeen SET seen = 1 WHERE linenum = ?');
    this.dbqMarkAllNovaLogEntriesSeen = db.prepare('UPDATE novalogSeen SET seen = 1');
    this.dbqGetUnseenNovaLogs = db.prepare('SELECT * from novalogSeen WHERE seen = 0');

    this.dbqIsNewHoneydLogEntry = db.prepare('SELECT COUNT(*) AS rows from honeydlogSeen WHERE linenum = ?');
    this.dbqAddHoneydLogEntry = db.prepare('INSERT INTO honeydlogSeen VALUES(?, ?, 0)');
    this.dbqMarkHoneydLogEntrySeen = db.prepare('UPDATE honeydlogSeen SET seen = 1 WHERE linenum = ?');
    this.dbqMarkAllHoneydLogEntriesSeen = db.prepare('UPDATE honeydlogSeen SET seen = 1');
    this.dbqGetUnseenHoneydLogs = db.prepare('SELECT * from honeydlogSeen WHERE seen = 0');

    this.dbqGetLastHoneydNodeIPs = db.prepare('SELECT * FROM lastHoneydNodeIPs');
    this.dbqAddLastHoneydNodeIP = db.prepare('INSERT INTO lastHoneydNodeIPs VALUES(?, ?)');
    this.dbqClearLastHoneydNodeIPs = db.prepare('DELETE FROM lastHoneydNodeIPs');

    this.dbqGetLastTrainingDataSelection = db.prepare('SELECT * FROM lastTrainingDataSelection');
    this.dbqAddLastTrainingDataSelection = db.prepare('INSERT INTO lastTrainingDataSelection VALUES(?, ?)');
    this.dbqClearLastTrainingDataSelection = db.prepare('DELETE FROM lastTrainingDataSelection');


    this.dbqGetSuspect = novaDb.prepare("SELECT * from suspects JOIN packet_counts ON suspects.ip = packet_counts.ip AND suspects.interface = packet_counts.interface WHERE suspects.ip = ? AND suspects.interface = ?");
    this.dbqGetIpPorts = novaDb.prepare("SELECT * from ip_port_counts where ip = ? AND interface = ?");
    this.dbqGetSuspectPacketCounts = novaDb.prepare("SELECT * from packet_counts WHERE ip = ? AND interface = ?");
    this.dbqGetSuspectPacketSizes = novaDb.prepare("SELECT * from packet_sizes WHERE ip = ? AND interface = ?");



    this.GetSuspects = function(limit, offset, orderBy, direction, showUnclassified, cb) {
        var classifiedFilter = "";
        if (!showUnclassified) {
            classifiedFilter = " WHERE classification != -2 ";
        }


        // We only allow classifiedFilter to be one of the following
        var allowedOrderBy = new Array("classification", 
            "ip",
            "interface",
            "lastTime",
            "ip_traffic_distribution", 
            "port_traffic_distribution",
            "packet_size_mean",
            "packet_size_deviation",
            "distinct_ips",
            "distinct_tcp_ports",
            "distinct_udp_ports",
            "avg_tcp_ports_per_host",
            "avg_udp_ports_per_host",
            "tcp_percent_syn",
            "tcp_percent_fin",
            "tcp_percent_rst",
            "tcp_percent_synack",
            "haystack_percent_contacted"
        );

        if (allowedOrderBy.indexOf(orderBy) == -1) {
            cb("ERROR: Invalid arg orderBy. Must be one of the sqlite columns for the suspect table");
            return;
        }

        if (direction != "ASC" && direction != "DESC") {
            cb("ERROR: Invalid arg direction. Must be ASC or DESC.");
            return;
        }

        if (isNaN(parseInt(limit))) {
            cb("ERROR: Invalid arg limit. Must be an integer.");
            return;
        }
        
        if (isNaN(parseInt(offset))) {
            cb("ERROR: Invalid arg offset. Must be an integer.");
            return;
        }


        var queryString = "SELECT * FROM suspects " + classifiedFilter + "ORDER BY " + orderBy + " " + direction + " LIMIT " + limit + " OFFSET " + offset;
        NovaCommon.novaDb.all(queryString, function(err, results) {
            if (err) {
                LOG("ERROR", "Database error: " + err);
                cb && cb(err);
                return;
            }
            
            cb && cb(null, results);
        });

    };
    
    this.HashPassword = function (password, salt)
    {
        var shasum = crypto.createHash('sha1');
        shasum.update(password + salt);
        return shasum.digest('hex');
    };

}();

module.exports = NovaCommon;
