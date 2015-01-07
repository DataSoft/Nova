//============================================================================
// Name        : NowjsMethods.js
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
// Description : Nowjs remote callabale functions
//============================================================================

var dns = require('dns');
var fs = require('fs');
var exec = require('child_process').exec;
var child_process = require('child_process');
var sanitizeCheck = require('validator').sanitize;
var NovaCommon = require('./NovaCommon.js');
var LOG = NovaCommon.LOG;

var NovaHomePath = NovaCommon.config.GetPathHome();
var NovaSharedPath = NovaCommon.config.GetPathShared();

var NowjsMethods = function(everyone) {

function objCopy(src, dst) 
{
    for(var member in src) 
    {
        if(typeof src[member] == 'function') 
        {
            dst[member] = src[member]();
        }
        // Need to think about this
        //        else if ( typeof src[member] == 'object' )
        //        {
        //            copyOver(src[member], dst[member]);
        //        }
        else 
        {
            dst[member] = src[member];
        }
    }
}

everyone.now.shutdownQuasar = function() {
    LOG("ALERT", "Quasar is exiting due to user issued shutdown command on the web interface");
    process.exit(1);
};

everyone.now.changeGroup = function(group, cb)
{
    var res = NovaCommon.config.SetGroup(group);
    cb && cb(res);
};

everyone.now.ChangeNodeInterfaces = function(nodes, newIntf, cb)
{
    NovaCommon.honeydConfig.ChangeNodeInterfaces(nodes, newIntf);
    cb && cb();
};

everyone.now.GetProfileNames = function(cb)
{
    var res = NovaCommon.honeydConfig.GetProfileNames();
    cb && cb(res);
};

everyone.now.GetLeafProfileNames = function(cb)
{
  var res = NovaCommon.honeydConfig.GetLeafProfileNames();
  cb && cb(res);
};

everyone.now.addScriptOptionValue = function (script, key, value, cb) {
    NovaCommon.honeydConfig.AddScriptOptionValue(script, key, value);
    NovaCommon.honeydConfig.SaveAll();
    cb && cb();
};

everyone.now.deleteScriptOptionValue = function (script, key, value, cb) {
    NovaCommon.honeydConfig.DeleteScriptOptionValue(script, key, value);
    NovaCommon.honeydConfig.SaveAll();
    cb && cb();
};

everyone.now.createHoneydNodes = function(ipType, ip1, ip2, ip3, ip4, profile, portSet, vendor, ethinterface, count, cb)
{
    var ipAddress;
    if(ipType == "DHCP")
    {
        ipAddress = "DHCP";
    }
    else
    {
        ipAddress = ip1 + "." + ip2 + "." + ip3 + "." + ip4;
    }

    var result = null;
    if(!NovaCommon.honeydConfig.AddNodes(profile, portSet, vendor, ipAddress, ethinterface, Number(count)))
    {
        result = "Unable to create new nodes";  
    }

    if(!NovaCommon.honeydConfig.SaveAll())
    {
        result = "Unable to save honeyd configuration";
    }

    cb && cb(result);
};

everyone.now.checkVendor = function(vendorName, cb) {
  var vendors = NovaCommon.vendorToMacDb.GetVendorNames();
  if(vendorName == '')
  {
    cb && cb(false);
    return;
  }
  var ok = false;
  for(var i in vendors)
  {
    if(vendors[i] == vendorName)
    {
      ok = true;
    }
  }
  cb && cb(ok);
}

everyone.now.SaveDoppelganger = function(node, cb)
{
    var ipAddress = node.ip;
    if(node.ipType == "DHCP")
    {
        ipAddress = "DHCP";
    }

    if(!NovaCommon.honeydConfig.SaveDoppelganger(node.profile, node.portSet, ipAddress, node.mac, node.intface))
    {
        cb && cb("SaveDoppelganger Failed");
        return;
    }
    else
    {
        if(!NovaCommon.honeydConfig.SaveAll())
        {
            cb && cb("Unable to save honeyd configuration");
        }
        else
        {
            cb && cb(null);
        }
    }
};

everyone.now.SaveHoneydNode = function(node, cb)
{
    var ipAddress = node.ip;
    if(node.ipType == "DHCP")
    {
        ipAddress = "DHCP";
    }

    // Delete the old node and then add the new one 
    NovaCommon.honeydConfig.DeleteNode(node.oldName);

    if(node.oldName == "doppelganger")
    {
        if(!NovaCommon.honeydConfig.SetDoppelganger(node.profile, parseInt(node.portSet), ipAddress, node.mac, node.intface))
        {
            cb && cb("doppelganger Failed");
            return;
        }
        else
        {
            NovaCommon.config.WriteSetting('DOPPELGANGER_IP', ipAddress);
            NovaCommon.config.WriteSetting('DOPPELGANGER_INTERFACE', node.intface);
            
            if(!NovaCommon.honeydConfig.SaveAll())
            {
                cb && cb("Unable to save honeyd configuration");
            }
            else
            {
                cb && cb(null);
            }
        }

    }
    else
    {
        if(!NovaCommon.honeydConfig.AddNode(node.profile, parseInt(node.portSet), ipAddress, node.mac, node.intface))
        {
            cb && cb("AddNode Failed");
            return;
        }
        else
        {
            if(!NovaCommon.honeydConfig.SaveAll())
            {
                cb && cb("Unable to save honeyd configuration");
            }
            else
            {
               cb && cb(null);
            }
        }
    }

};

everyone.now.ClearAllSuspects = function (cb)
{
    NovaCommon.nova.CheckConnection();
    NovaCommon.nova.ClearAllSuspects();
};

everyone.now.ClearSuspect = function (suspectIp, ethinterface, cb)
{
    NovaCommon.nova.CheckConnection();
    var result = NovaCommon.nova.ClearSuspect(suspectIp, ethinterface);

    if (cb != undefined)
    {
        cb(result);
    }
};

everyone.now.GetInheritedEthernetList = function (parent, cb)
{
    var prof = NovaCommon.honeydConfig.GetProfile(parent);

    if (prof == null)
    {
        console.log("ERROR Getting profile " + parent);
        cb(null);
    }
    else
    {
        cb(prof.GetVendors(), prof.GetVendorCounts());
    }
};

everyone.now.RestartHaystack = function(cb)
{
    NovaCommon.StopHaystack(function() {
        // Note: the other honeyd may be shutting down still,
        // but the slight overlap doesn't cause problems
        NovaCommon.StartHaystack(function() {
            cb && cb();
        });
    });
};

everyone.now.StartHaystack = function()
{
  if(!NovaCommon.nova.IsHaystackUp())
  {
      NovaCommon.StartHaystack(false);
  }

  setTimeout(function(){
    if(!NovaCommon.nova.IsHaystackUp())
    {
      everyone.now.HaystackStartFailed();
    }
    else
    {
      try 
      {
          everyone.now.updateHaystackStatus(NovaCommon.nova.IsHaystackUp())
      } 
      catch(err)
      {};
    }
  }, 1000);
};

everyone.now.StopHaystack = function()
{
    NovaCommon.StopHaystack(function() {
        try 
        {
            everyone.now.updateHaystackStatus(NovaCommon.nova.IsHaystackUp());
        } 
        catch(err)
        {};
    });
};

everyone.now.IsHaystackUp = function(cb)
{
    cb(NovaCommon.nova.IsHaystackUp());
};

everyone.now.IsNovadConnected = function(cb)
{
    cb(NovaCommon.nova.IsNovadConnected());
};

everyone.now.StartNovad = function()
{
    var result = NovaCommon.StartNovad(false);
    setTimeout(function(){
    result = NovaCommon.nova.CheckConnection();
      try 
      {
          everyone.now.updateNovadStatus(NovaCommon.nova.IsNovadConnected());
      }
      catch(err){};
    }, 1000);
};

everyone.now.StopNovad = function(cb)
{
  if(NovaCommon.StopNovad() == false)
  {
    cb && cb('false');
    return;
  }
  NovaCommon.nova.CloseNovadConnection();
  try 
  {
    everyone.now.updateNovadStatus(NovaCommon.nova.IsNovadConnected());
  }
  catch(err){};
};

everyone.now.HardStopNovad = function(passwd)
{
  NovaCommon.nova.HardStopNovad(passwd);
  NovaCommon.nova.CloseNovadConnection();
  try 
  {
    everyone.now.updateNovadStatus(NovaCommon.nova.IsNovadConnected());
  }
  catch(err){};
};

everyone.now.sendSuspect = function (ethinterface, ip, cb)
{
    var suspect = NovaCommon.nova.sendSuspect(ethinterface, ip);
    if(suspect.GetIdString === undefined)
    {
        console.log("Failed to get suspect");
        return;
    }
    var s = new Object();
    objCopy(suspect, s);
    cb(s);
};

// Deletes a honeyd node
everyone.now.deleteNodes = function (nodeNames, cb)
{
  var nodeName;
  for(var i = 0; i < nodeNames.length; i++)
  {
    nodeName = nodeNames[i];
    if(nodeName != null && !NovaCommon.honeydConfig.DeleteNode(nodeName))
    {
      cb(false, "Failed to delete node " + nodeName);
      return;
    }

  }

  if(!NovaCommon.honeydConfig.SaveAll())
  {
    cb(false, "Failed to save XML templates");
    return;
  }

  cb(true, "");
};

everyone.now.deleteProfiles = function (profileNames, cb)
{
  var profileName;
  for(var i = 0; i < profileNames.length; i++)
  {
    profileName = profileNames[i];

    if(!NovaCommon.honeydConfig.DeleteProfile(profileName))
    {
      cb(false, "Failed to delete profile " + profileName);
      return;
    }

    if(!NovaCommon.honeydConfig.SaveAll())
    {
      cb(false, "Failed to save XML templates");
      return;
    }
  }

  cb(true, "");
};

everyone.now.addWhitelistEntry = function(ethinterface, entry, cb)
{
    entry = entry.replace(/\s+/g, '');
    var individualIp = new RegExp('^((((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\\.){3,3})(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2}))$');
    individualIp.global = false;
    var subnetRangeIp = new RegExp('^((((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\\.){3,3})(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\\/[1-3]?[0-9])$');
    subnetRangeIp.global = false;
    var slashSubnetMaskIp = new RegExp('^((((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\\.){3,3})(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2}))\\/((((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2})\\.){3,3})(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]{1,2}))$');
    slashSubnetMaskIp.global = false;

    if((individualIp.test(entry) || subnetRangeIp.test(entry) || slashSubnetMaskIp.test(entry)) && NovaCommon.whitelistConfig.AddEntry(ethinterface + "," + entry))
    {
        cb(true, "");
    }
    else
    {
        cb(true, "Attempt to add whitelist entry failed");
    }
};

everyone.now.deleteWhitelistEntry = function (whitelistEntryNames, cb)
{
    var whitelistEntryName;
    for(var i = 0; i < whitelistEntryNames.length; i++)
    {
        whitelistEntryName = whitelistEntryNames[i];

        if(!NovaCommon.whitelistConfig.DeleteEntry(whitelistEntryName))
        {
            cb(false, "Failed to delete whitelistEntry " + whitelistEntryName);
            return;
        }
    }

    cb(true, "");
};

everyone.now.GetScript = function (scriptName, cb)
{
    var script = NovaCommon.honeydConfig.GetScript(scriptName);
    var methodlessScript = {};

    objCopy(script, methodlessScript);

    cb(methodlessScript);

};

everyone.now.GetVendors = function (profileName, cb)
{
    var profile = NovaCommon.honeydConfig.GetProfile(profileName);

    if(profile == null)
    {
        console.log("ERROR Getting profile " + profileName);
        cb(null);
        return;
    }


    var ethVendorList = [];

    var profVendors = profile.GetVendors();
    var profDists = profile.GetVendorCounts();

    for(var i = 0; i < profVendors.length; i++)
    {
        var element = {
            vendor: "",
            count: ""
        };
        element.vendor = profVendors[i];
        element.count = parseFloat(profDists[i]);
        ethVendorList.push(element);
    }

    cb(profVendors, profDists);
};

function jsProfileToHoneydProfile(profile)
{
    var honeydProfile = new NovaCommon.novaconfig.HoneydProfileBinding(profile.parentProfile, profile.name);
    
        //Set Ethernet vendors
    var ethVendors = [];
    var ethDists = [];

    for(var i in profile.ethernet)
    {
        ethVendors.push(profile.ethernet[i].vendor);
        ethDists.push(parseFloat(Number(profile.ethernet[i].count)));
    }
    honeydProfile.SetVendors(ethVendors, ethDists);
    

    // Move the Javascript object values to the C++ object
    honeydProfile.SetUptimeMin(Number(profile.uptimeMin));
    honeydProfile.SetUptimeMax(Number(profile.uptimeMax));
    honeydProfile.SetDropRate(Number(profile.dropRate));
    honeydProfile.SetPersonality(profile.personality);
    honeydProfile.SetCount(profile.count);

    honeydProfile.SetIsPersonalityInherited(Boolean(profile.isPersonalityInherited));
    honeydProfile.SetIsDropRateInherited(Boolean(profile.isDropRateInherited));
    honeydProfile.SetIsUptimeInherited(Boolean(profile.isUptimeInherited));


    // Add new ports
    honeydProfile.ClearPorts();
    var portName;
    for(var i = 0; i < profile.portSets.length; i++) 
    {
        //Make a new port set
        honeydProfile.AddPortSet();

        honeydProfile.SetPortSetBehavior(i, "tcp", profile.portSets[i].TCPBehavior);
        honeydProfile.SetPortSetBehavior(i, "udp", profile.portSets[i].UDPBehavior);
        honeydProfile.SetPortSetBehavior(i, "icmp", profile.portSets[i].ICMPBehavior);

        for(var j = 0; j < profile.portSets[i].PortExceptions.length; j++)
        {
            var scriptConfigKeys = new Array();
            var scriptConfigValues = new Array();

            for(var key in profile.portSets[i].PortExceptions[j].scriptConfiguration)
            {
                scriptConfigKeys.push(key);
                scriptConfigValues.push(profile.portSets[i].PortExceptions[j].scriptConfiguration[key]);
            }

            honeydProfile.AddPort(i,
                    profile.portSets[i].PortExceptions[j].behavior, 
                    profile.portSets[i].PortExceptions[j].protocol, 
                    Number(profile.portSets[i].PortExceptions[j].portNum), 
                    profile.portSets[i].PortExceptions[j].scriptName,
                    scriptConfigKeys,
                    scriptConfigValues);
        }
    }

    honeydProfile.ClearBroadcasts();
    for (var i = 0; i < profile.broadcasts.length; i++)
    {
        honeydProfile.AddBroadcast(profile.broadcasts[i].script, Number(profile.broadcasts[i].srcPort), Number(profile.broadcasts[i].dstPort), Number(profile.broadcasts[i].time));
    }

    honeydProfile.ClearProxies();
    for (var i = 0; i < profile.proxies.length; i++) {
        honeydProfile.AddProxy(profile.proxies[i].protocol, Number(profile.proxies[i].honeypotPort), profile.proxies[i].proxyIP, Number(profile.proxies[i].proxyPort));
    }
        
    return honeydProfile;
}

everyone.now.DeletePortSet = function(profile, portSetIndex, cb)
{
    var error = NovaCommon.honeydConfig.DeletePortSet(profile, portSetIndex);
    NovaCommon.honeydConfig.SaveAll();
    cb && cb(!error);
}

everyone.now.AddPortSet = function(profile, cb)
{
    var error = NovaCommon.honeydConfig.AddPortSet(profile);
    NovaCommon.honeydConfig.SaveAll();
    cb && cb(!error);
}

//portSets = A 2D array. (array of portSets, which are arrays of Ports)
everyone.now.SaveProfile = function (profile, newProfile, cb)
{
    // Check input
    var profileNameRegexp = new RegExp("[a-zA-Z]+[a-zA-Z0-9 ]*");
    var match = profileNameRegexp.exec(profile.name);
   
    if(match != profile.name) 
    {
        var err = "ERROR: Attempt to save a profile with an invalid name. Must be alphanumeric and not begin with a number.";
        cb(err);
        return;
    }

    // Check for duplicate profile
    if(newProfile) 
    {
        var existingProfile = NovaCommon.honeydConfig.GetProfile(profile.name);
        if(existingProfile != null)
        {
            cb && cb("ERROR: Profile with name already exists");
            return;
        }
    }


    // Check we have ethernet vendors
    if(profile.ethernet.length == 0)
    {
        var err = "ERROR: Must have at least one ethernet vendor!";
        cb && cb(err);
        return;
    }


    // Check for valid drop percentage
    if(isNaN(parseInt(profile.dropRate)))
    {
        cb && cb("ERROR: Can't convert drop rate to integer");
        return;
    }

    profile.dropRate = parseInt(profile.dropRate);

    if(profile.dropRate < 0 || profile.dropRate > 100)
    {
        cb && cb("ERROR: Droprate must be between 0 and 100");
        return;
    }

    // Check uptimes
    if(profile.uptimeValueMax < 0 || profile.uptimeValueMin < 0)
    {
        cb && cb("ERROR: Uptime must be a positive integer");
        return;
    }


    // Check that we have the scriptnames set for profiles that need scripts
    for(var i = 0; i < profile.portSets.length; i++) 
    {
        for(var j = 0; j < profile.portSets[i].PortExceptions.length; j++)
        {
            var port = profile.portSets[i].PortExceptions[j];

            if(isNaN(parseInt(port.portNum)))
            {
                cb && cb("ERROR: unable to parse port into an integer!");
                return;
            }

            if(parseInt(port.portNum) <= 0 || parseInt(port.portNum) > 65535)
            {
                cb && cb("ERROR: Unable to save profile with invalid port number!");
                return;
            }

            if(port.behavior == "script" || port.behavior == "tarpit script")
            {
                if(port.scriptName == "" || port.scriptName == "NA")
                {
                    var err = "ERROR: Attempt to save a profile with an invalid port script value.";
                    cb && cb(err);
                    return;
                }
            }

        }
    }

    var honeydProfile = jsProfileToHoneydProfile(profile);
    honeydProfile.Save();


    // Save the profile
    if(!NovaCommon.honeydConfig.SaveAll())
    {
        result = "Unable to save honeyd configuration";
    }

    cb();
};

everyone.now.ShowAutoConfig = function (nodeInterface, numNodesType, numNodes, subnets, groupName, append, cb, route)
{
    if(!(new RegExp('^[a-zA-Z0-9 \\-_]+$')).test(groupName))
    {
        cb && cb("Haystack name must not be blank and must contain only letters, numbers, and hyphens. Invalid haystack name given.");
        return;
    }

    var executionString = 'haystackautoconfig';

    var hhconfigArgs = new Array();

    hhconfigArgs.push('--nodeinterface');
    hhconfigArgs.push(nodeInterface);

    if(numNodesType == "fixed") 
    {
        if(numNodes !== undefined) 
        {
            hhconfigArgs.push('-n');
            hhconfigArgs.push(numNodes);
        }
    } 
    else if(numNodesType == "ratio") 
    {
        if(numNodes !== undefined) 
        {
            hhconfigArgs.push('-r');
            hhconfigArgs.push(numNodes);
        }
    }
    else if(numNodesType == 'range')
    {
      if(numNodes !== undefined)
      {
        hhconfigArgs.push('e');
        hhconfigArgs.push(numNodes);
      }
    }
    
    if(subnets !== undefined && subnets.length > 0)
    {
        hhconfigArgs.push('-a');
        hhconfigArgs.push(subnets);
    }

    if(!append) 
    {
        hhconfigArgs.push('-t');
        hhconfigArgs.push(groupName);
        NovaCommon.honeydConfig.AddConfiguration(groupName, 'false', '');
        NovaCommon.config.SetCurrentConfig(groupName);
    }
    else
    {
        hhconfigArgs.push('-t');
        hhconfigArgs.push(groupName);
    }

    var util = require('util');
    var spawn = require('child_process').spawn;
    
    console.log("Running: " + executionString.toString());
    console.log("Args: " + hhconfigArgs);

    autoconfig = spawn(executionString.toString(), hhconfigArgs);

    autoconfig.stdout.on('data', function (data)
    {
      if(typeof cb == 'function')
      {
        cb(null, '' + data);
      }
    });

    autoconfig.stderr.on('data', function (data)
    {
        if(/^execvp\(\)/.test(data))
        {
          console.log("haystackautoconfig failed to start.");
          var response = "haystackautoconfig failed to start.";
          everyone.now.SwitchConfigurationTo('default');
          if(typeof route == 'function')
          {
            route("/autoConfig", response);
          }
        }
    });

    autoconfig.on('exit', function (code, signal)
    {
      console.log("autoconfig exited with code " + code);
      var response = "autoconfig exited with code " + code;
      try
      {
        if(fs.statSync(NovaHomePath + '/data/hhconfig.lock').isFile())
        {
          fs.unlinkSync(NovaHomePath + '/data/hhconfig.lock');
        }
      }
      catch(err)
      {
      }
      if(typeof route == 'function' && signal != 'SIGTERM')
      {
        route("/honeydConfigManage", response);
      }
      else if(signal == 'SIGTERM')
      {
        response = "autoconfig scan terminated early";
        route("/autoConfig", response);
      }
      else if(signal == 'SIGSEGV')
      {
        response = "autoconfig experienced a segmentation fault";
        route("/honeydConfigManage", response);
      }
    });
};

everyone.now.CancelAutoScan = function(groupName)
{
  try
  {
    autoconfig.kill();
    autoconfig = undefined;
    everyone.now.RemoveConfiguration(groupName);
    
    everyone.now.SwitchConfigurationTo('default');
  }
  catch(e)
  {
    LOG("ERROR", "CancelAutoScan threw an error: " + e);
  }
};

everyone.now.WriteHoneydConfig = function(cb)
{
   NovaCommon.honeydConfig.WriteHoneydConfiguration(NovaCommon.config.GetPathConfigHoneydHS());
   
   if(typeof cb == 'function')
   {
     cb(); 
   }
};

everyone.now.GetConfigSummary = function(configName, cb)
{
  NovaCommon.honeydConfig.LoadAllTemplates();
  
  var scriptProfileBindings = NovaCommon.GetPorts();
  var profiles = NovaCommon.honeydConfig.GetProfileNames();
  var profileObj = {};
  
  for(var i = 0; i < profiles.length; i++) 
  {
    if(profiles[i] != undefined && profiles[i] != '')
    {
      var prof = NovaCommon.honeydConfig.GetProfile(profiles[i]);
      var obj = {};
      var vendorNames = prof.GetVendors();
      var vendorDist = prof.GetVendorCounts();
      
      obj.name = prof.GetName();
      obj.parent = prof.GetParentProfile();
      obj.os = prof.GetPersonality();
      obj.packetDrop = prof.GetDropRate();
      obj.vendors = [];
      
      for(var j = 0; j < vendorNames.length; j++)
      {
        var push = {};
        
        push.name = vendorNames[j];
        push.count = vendorDist[j];
        obj.vendors.push(push);
      }
      
      if(prof.GetUptimeMin() == prof.GetUptimeMax())
      {
        obj.fixedOrRange = 'fixed';
        obj.uptimeValue = prof.GetUptimeMin();
      }
      else
      {
        obj.fixedOrRange = 'range';
        obj.uptimeValueMin = prof.GetUptimeMin();
        obj.uptimeValueMax = prof.GetUptimeMax();        
      }
      
      //obj.defaultTCP = prof.GetTcpAction();
      //obj.defaultUDP = prof.GetUdpAction();
      //obj.defaultICMP = prof.GetIcmpAction();
      profileObj[profiles[i]] = obj;
    }
  }
  
  var nodeNames = NovaCommon.honeydConfig.GetNodeMACs();
  var nodeList = [];
  
  for(var i = 0; i < nodeNames.length; i++)
  {
    var node = NovaCommon.honeydConfig.GetNode(nodeNames[i]);
    var push = NovaCommon.cNodeToJs(node);
    nodeList.push(push);
  }
  
  nodeNames = null;
  
  if(typeof cb == 'function')
  {
    cb(scriptProfileBindings, profileObj, profiles, nodeList);
  }
};

everyone.now.SwitchConfigurationTo = function(configName, cb)
{
    NovaCommon.honeydConfig.SwitchConfiguration(configName); 
    NovaCommon.config.WriteSetting('CURRENT_CONFIG', configName);
    cb && cb();
};

everyone.now.RemoveConfiguration = function(configName, cb)
{
  if(configName == 'default')
  {
    console.log('Cannot delete default haystack');
  }
  
  NovaCommon.honeydConfig.RemoveConfiguration(configName);
  
  if(typeof cb == 'function')
  {
    cb(configName);
  }
};

everyone.now.RemoveScript = function(scriptName, cb)
{
  NovaCommon.honeydConfig.RemoveScript(scriptName);
  
  NovaCommon.honeydConfig.SaveAllTemplates();
  
  if(typeof cb == 'function')
  {
    cb();
  }
};

everyone.now.GetLocalIP = function (iface, cb)
{
    cb(NovaCommon.nova.GetLocalIP(iface));
};

everyone.now.GetSubnetFromInterface = function (iface, index, cb)
{
  cb(iface, NovaCommon.nova.GetSubnetFromInterface(iface), index);
};

everyone.now.RemoveScriptFromProfiles = function(script, cb)
{
    NovaCommon.honeydConfig.DeleteScriptFromPorts(script);

    NovaCommon.honeydConfig.SaveAllTemplates();

    if(typeof cb == 'function')
    {
        cb();
    }
};

everyone.now.GenerateMACForVendor = function(vendor, cb)
{
    cb(NovaCommon.honeydConfig.GenerateRandomUnusedMAC(vendor));
};

everyone.now.restoreDefaultSettings = function(cb)
{
    var source = NovaSharedPath + "/../userFiles/config/NOVAConfig.txt";
    var destination = NovaHomePath + "/config/NOVAConfig.txt";
    exec('cp -f ' + source + ' ' + destination, function(err)
    {
        cb();
    }); 
};

everyone.now.reverseDNS = function(ip, cb)
{
    dns.reverse(ip, cb);
};

everyone.now.addTrainingPoint = function(ip, ethinterface, features, hostility, cb)
{
    if(hostility != '0' && hostility != '1')
    {
        cb("Error: Invalid hostility. Should be 0 or 1");
        return;
    }
    
    if(features.toString().split(" ").length != NovaCommon.nova.GetDIM()) {
        cb("Error: Invalid number of features!")
        return;
    }

    var point = features.toString() + " " + hostility + "\n";
    fs.appendFile(NovaHomePath + "/config/training/data.txt", point, function(err)
    {
        if(err)
        {
            console.log("Error: " + err);
            cb(err);
            return;
        }

        var d = new Date();
        var trainingDbString = "";
        trainingDbString += hostility + ' "User customized training point for suspect ' + ip + " added on " + d.toString() + '"\n';
        trainingDbString += "\t" + features.toString();
        trainingDbString += "\n\n";

        fs.appendFile(NovaHomePath + "/config/training/training.db", trainingDbString, function(err)
        {
            if(!NovaCommon.nova.ReclassifyAllSuspects())
            {
                cb("Error: Unable to reclassify suspects with new training data");
                return;
            }
            cb();
        });

    }); 
};

everyone.now.GetHaystackDHCPStatus = function(cb)
{
    fs.readFile("/var/log/honeyd/ipList", 'utf8', function (err, data)
    {
        var DHCPIps = new Array();
        if(err)
        {
            RenderError(res, "Unable to open Honeyd status file for reading due to error: " + err);
            NovaCommon.dbqClearLastHoneydNodeIPs.run();
            return;
        }
        else
        {
            data = data.toString().replace(/ /g, '').split("\n");
            var tmp = [];
            for (var i = 0; i < data.length; i++) {
                if (data[i] == "") {
                    continue
                } else {
                    tmp.push(data[i]);
                }
            }

            data = tmp;

            if (data.length > 0) {
                NovaCommon.dbqClearLastHoneydNodeIPs.run(function(err) {
                    if (err) {LOG("ERROR", "Database error:" + err);}
                
                    for(var i = 0; i < data.length; i++)
                    {
                        if (data[i] == "") {continue};
                        var entry = {
                            ip: data[i].toString().split(",")[0],
                            mac: data[i].toString().split(",")[1],
                            current: 1
                        };


                        NovaCommon.dbqAddLastHoneydNodeIP.run(entry.mac, entry.ip, function(err) {
                            if (err) {LOG("ERROR", "Database error:" + err);}
                        });
                        DHCPIps.push(entry);
                    }
                    cb(DHCPIps);
                });
            } else {
                // If iplist file is empty, resort to pulling a version out of our db cache
                NovaCommon.dbqGetLastHoneydNodeIPs.all(function(err, results) {
                    if (err) {LOG("ERROR", "Database error:" + err);}

                    for (var i = 0; i < results.length; i++) {
                        results[i].current = 0;
                        DHCPIps.push(results[i]);
                    }
                    cb(DHCPIps);
                });
            }
        }
    });
};

everyone.now.deleteClassifier = function(index, cb)
{
    NovaCommon.classifiers.deleteClassifier(index);
    if(cb) cb();
};

everyone.now.saveClassifier = function(classifier, index, cb)
{
    // Convert the model instance settings to strings for config file
    var enabledFeaturesString = "";
    var weightString = "";
    var thresholdString = "";

    for(var i = 0; i < classifier.features.length; i++)
    {
        if(classifier.features[i].enabled)
        {
            enabledFeaturesString += "1";
        }
        else
        {
            enabledFeaturesString += "0";
        }

        if(classifier.type == "KNN")
        {
            weightString += String(classifier.features[i].weight) + " ";
        }
        else if(classifier.type == "THRESHOLD_TRIGGER")
        {
            thresholdString += classifier.features[i].threshold + " ";
        }
    }
    
    classifier.strings = {};
    classifier.strings["ENABLED_FEATURES"] = enabledFeaturesString;
    if(classifier.type == "KNN")
    {
        classifier.strings["FEATURE_WEIGHTS"] = weightString;
    }
    else if(classifier.type == "THRESHOLD_TRIGGER")
    {
        classifier.strings["THRESHOLD_HOSTILE_TRIGGERS"] = thresholdString;
    }
    else if (classifier.type == "SCRIPT_ALERT")
    {
        classifier.strings = {};
    }
    else if (classifier.type == "UNAUTHORIZED_SUSPECTS" || classifier.type == "UNAUTHORIZED_MACS")
    {
        classifier.strings = {};
    }


    NovaCommon.classifiers.saveClassifier(classifier, index);
    if(cb) cb();
};

everyone.now.GetProfile = function (profileName, cb)
{
    var profile = NovaCommon.honeydConfig.GetProfile(profileName);

    
    if(profile == null)
    {
        cb(null);
        return;
    }

    // Nowjs can't pass the object with methods, they need to be member vars
    profile.name = profile.GetName();
    profile.personality = profile.GetPersonality();

    profile.uptimeMin = profile.GetUptimeMin();
    profile.uptimeMax = profile.GetUptimeMax();
    profile.dropRate = profile.GetDropRate();
    profile.parentProfile = profile.GetParentProfile();

    profile.isPersonalityInherited = profile.IsPersonalityInherited();
    profile.isUptimeInherited = profile.IsUptimeInherited();
    profile.isDropRateInherited = profile.IsDropRateInherited();

    profile.count = profile.GetCount();
    profile.portSets = GetPortSets(profileName);


    var ethVendorList = [];

    var profVendors = profile.GetVendors();
    var profCounts = profile.GetVendorCounts();

    for(var i = 0; i < profVendors.length; i++)
    {
        var element = {};
        element.vendor = profVendors[i];
        element.count = profCounts[i];
        ethVendorList.push(element);
    }

    profile.ethernet = ethVendorList;


    cb(profile);
};


everyone.now.GetHostileEvents = function (cb)
{
    NovaCommon.dbqSuspectAlertsGet.all(function(err, results){
        if (err)
        {
            console.log("Database error: " + err);
            cb(err);
            return;
        }

        cb(null, results);
    });
};

everyone.now.ClearHostileEvents = function (cb)
{
    NovaCommon.dbqSuspectAlertsDeleteAll.run(function(err){
        if (err)
        {
            console.log("Database error: " + err);
            cb(err);
            return;
        }

        cb(null);
    });
};

everyone.now.WizardHasRun = function (cb)
{
    NovaCommon.dbqFirstrunInsert.run(cb);
};

everyone.now.deleteUserEntry = function (usernamesToDelete, cb)
{
    var username;
    for (var i = 0; i < usernamesToDelete.length; i++)
    {
        username = String(usernamesToDelete[i]);
        NovaCommon.dbqCredentialsDeleteUser.run(username, function (err)
        {
            if (err)
            {
                console.log("Database error: " + err);
                cb(false);
                return;
            }
            else
            {
                cb(true);
            }
        });
    }
};

everyone.now.updateUserPassword = function (username, newPassword, cb)
{
  var salt = '';
  var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
  for(var i = 0; i < 8; i++)
  {
    salt += possible[Math.floor(Math.random() * possible.length)];
  }
  
  //update credentials set pass=? and salt=? where user=?
  NovaCommon.dbqCredentialsChangePassword.run(NovaCommon.HashPassword(newPassword, salt), salt, username, function(err){
    console.log('err ' + err);
    if(err)
    {
      cb(false);
    }
    else
    {
      cb(true);
    }
  });
};


var GetPortSets = function (profileName, cb)
{
    var portSetNames = NovaCommon.honeydConfig.GetPortSetNames(profileName);
    
    var portSets = [];  

    for(var i = 0; i < portSetNames.length; i++)
    {
        var portSet = NovaCommon.honeydConfig.GetPortSet( profileName, portSetNames[i] );
        portSet.setName = i;
        portSet.TCPBehavior = portSet.GetTCPBehavior();
        portSet.UDPBehavior = portSet.GetUDPBehavior();
        portSet.ICMPBehavior = portSet.GetICMPBehavior();

        portSet.PortExceptions = portSet.GetPorts();
        for(var j = 0; j < portSet.PortExceptions.length; j++)
        {
            portSet.PortExceptions[j].portNum = portSet.PortExceptions[j].GetPortNum();
            portSet.PortExceptions[j].protocol = portSet.PortExceptions[j].GetProtocol();
            portSet.PortExceptions[j].behavior = portSet.PortExceptions[j].GetBehavior();
            portSet.PortExceptions[j].scriptName = portSet.PortExceptions[j].GetScriptName();
            portSet.PortExceptions[j].scriptConfiguration = portSet.PortExceptions[j].GetScriptConfiguration();
        }
        portSets.push(portSet);
    }

    if(typeof cb == 'function')
    {
    cb(portSets, profileName);
  }
  return portSets;
};
everyone.now.GetPortSets = GetPortSets;


function databaseError(err, cb) {
        if (err)
        {
            LOG("ERROR", "Database error: " + err);
            cb && cb(err);
            return true;
        }
        return false;
}

everyone.now.GetUnseenSuspects = function(cb) {
    NovaCommon.dbqGetUnseenSuspects.all(function(err, results){
        if (databaseError(err, cb)) {return;}   
        cb && cb(null, results);
    });
};

everyone.now.MarkSuspectSeen = function(ip, ethinterface, cb) {
    NovaCommon.dbqMarkSuspectSeen.run(ip, ethinterface, function(err){
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};

everyone.now.MarkAllSuspectSeen = function(cb) {
    NovaCommon.dbqMarkAllSuspectSeen.run(function(err) {
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};

everyone.now.GetUnseenDataSuspects = function(cb) {
    NovaCommon.dbqGetUnseenDataSuspects.all(function(err, results){
        if (databaseError(err, cb)) {return;}   
        cb && cb(null, results);
    });
};

everyone.now.MarkSuspectDataSeen = function(ip, ethinterface, cb) {
    NovaCommon.dbqMarkSuspectDataSeen.run(ip, ethinterface, function(err){
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};

everyone.now.MarkAllSuspectDataSeen = function(cb) {
    NovaCommon.dbqMarkAllSuspectDataSeen.run(function(err) {
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};


// Functions related to the nova log entry seen table in the DB
everyone.now.GetUnseenNovaLogs = function(cb) {
    NovaCommon.dbqGetUnseenNovaLogs.all(function(err, results) {
        if (databaseError(err, cb)) {return;}
        cb && cb(null, results);
    });
};

everyone.now.MarkNovaLogEntrySeen = function(linenum, cb) {
    NovaCommon.dbqMarkNovaLogEntrySeen.run(linenum, function(err) {
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};

everyone.now.MarkAllNovaLogEntriesSeen = function(cb) {
    NovaCommon.dbqMarkAllNovaLogEntriesSeen.run(function(err) {
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};

// Functions related  to the honeyd log entry seen table in the DB
everyone.now.GetUnseenHoneydLogs = function(cb) {
    NovaCommon.dbqGetUnseenHoneydLogs.all(function(err, results) {
        if (databaseError(err, cb)) {return;}
        cb && cb(null, results);
    });
};

everyone.now.MarkHoneydLogEntrySeen = function(linenum, cb) {
    NovaCommon.dbqMarkHoneydLogEntrySeen.run(linenum, function(err) {
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};

everyone.now.MarkAllHoneydLogEntriesSeen = function(cb) {
    NovaCommon.dbqMarkAllHoneydLogEntriesSeen.run(function(err) {
        if (databaseError(err,cb)) {return;}    
        cb && cb(null);
    });
};


// Hostname related database calls
everyone.now.GetHostnames = function(cb) {
    if (!NovaCommon.dbqGetHostnames) {
        cb("Unable to access hostnames database");
        return;
    }

    NovaCommon.dbqGetHostnames.all(function(err, results) {
        if (databaseError(err, cb)) {return;}
        cb && cb(null, results);
    }); 
};

everyone.now.InsertHostname = function(hostname, cb) {
    // Convert hostname to lower case and check if it is valid
    if (typeof(hostname) != "string") {
        cb("Hostname must be a string! Invalid type.");
        return; 
    }

    hostname = hostname.toLowerCase();

    if (!(new RegExp("^[a-zA-Z0-9\-]+$")).test(hostname)) {
        cb("Hostname must not be blank and must contain only letters, numbers, and hyphens. Invalid hostname given.");
        return;
    }

    if (!NovaCommon.dbqGetHostnames) {
        cb("Unable to access hostnames database");
        return;
    }
    
    // Check if it exists already
    
    NovaCommon.dbqGetHostname.all(hostname, function(err, results) {
        if (databaseError(err, cb)) {return;}

        if (results.length != 0) {
            cb && cb("Hostname already exists! Can not insert it.");
            return;
        } else {
            NovaCommon.dbqInsertHostname.run(hostname, function(err) {
                if (databaseError(err, cb)) {return;}
                cb && cb(null);
            });
        }
    }); 

};

everyone.now.ClearHostnameAllocations = function(cb) {
    if (!NovaCommon.dbqGetHostnames) {
        cb("Unable to access hostnames database");
        return;
    }

    NovaCommon.dbqClearHostnameAllocations.run(function(err) {
        if (databaseError(err, cb)) {return;}
        cb && cb(null);
    });
};
everyone.now.DeleteHostname = function(hostname, cb) {
    if (!NovaCommon.dbqGetHostnames) {
        cb("Unable to access hostnames database");
        return;
    }

    NovaCommon.dbqDeleteHostname.run(hostname, function(err) {
        if (databaseError(err, cb)) {return;}
        cb && cb(null);
    });
};


everyone.now.GetSuspects = function(limit, offset, orderBy, direction, showUnclassified, cb) {
    NovaCommon.GetSuspects(limit, offset, orderBy, direction, showUnclassified, cb);
};

everyone.now.GetNumberOfSuspects = function(showUnclassified, cb) {
    if (showUnclassified) {
        var queryString = "SELECT COUNT() as count FROM suspects";
    } else {
        var queryString = "SELECT COUNT() as count FROM suspects WHERE classification != -2";
    }
    NovaCommon.novaDb.all(queryString, function(err, results) {
        if (databaseError(err, cb)) {return;}
        cb && cb(null, results);
    });
};

everyone.now.GetSuspect = function(ip, iface, cb) {
    NovaCommon.dbqGetSuspect.all(ip, iface, function(err, results) {
        if (databaseError(err, cb)) {return;}

        if (results.length == 0) {
            cb && cb("No such suspect", null);
        } else {
                cb && cb(null, results[0]);
        }
    });
};

everyone.now.GetIpPortsContacted = function(ip, iface, cb) {
    NovaCommon.dbqGetIpPorts.all(ip, iface, function(err, results) {
        if (databaseError(err, cb)) {return;}
            cb && cb(null, results);
    });
};

everyone.now.GetPacketSizes = function(ip, iface, cb) {
    NovaCommon.dbqGetSuspectPacketSizes.all(ip, iface, function(err, results) {
        if (databaseError(err, cb)) {return;}
            cb && cb(null, results);
    });
};


everyone.now.GetBroadcasts = function(profile, cb) {
    var bcasts = NovaCommon.honeydConfig.GetBroadcasts(profile);
    
    for (var i = 0; i < bcasts.length; i++) {
        bcasts[i].srcPort = bcasts[i].GetSrcPort();
        bcasts[i].dstPort = bcasts[i].GetDstPort();
        bcasts[i].time = bcasts[i].GetTime();
        bcasts[i].script = bcasts[i].GetScript();
    }
    cb && cb(bcasts);
};

everyone.now.GetProxies = function(profile, cb) {
    var proxies = NovaCommon.honeydConfig.GetProxies(profile);

    for (var i = 0; i < proxies.length; i++) {
        proxies[i].protocol = proxies[i].GetProtocol();
        proxies[i].honeypotPort = proxies[i].GetHoneypotPort();
        proxies[i].proxyIP = proxies[i].GetProxyIP();
        proxies[i].proxyPort = proxies[i].GetProxyPort();
    }

    cb && cb(proxies);  
};

everyone.now.testSMTPSettings = function(settings, cb) {
    var args = ['/usr/share/nova/userFiles/config/novatestmail', '-s', settings.SERVER, '-p', settings.PORT, '-f', settings.FROM, '-t', settings.TO, '-x', settings.PASS, '-a', settings.AUTH];
    child_process.execFile("python", args, {}, function(err, stdout, stderr) {
        if (err) {
            var res = "ERROR: " + err;
            cb && cb(res);
            return;
        }

        var res = stdout + stderr;

        cb && cb(res);
    });
};

everyone.now.generateNewCerts = function(certInfo, cb) {
    var res = ""; 
    var subjString = "/";

    var keyPath = NovaCommon.config.ReadSetting("QUASAR_WEBUI_TLS_KEY");
    var certPath = NovaCommon.config.ReadSetting("QUASAR_WEBUI_TLS_CERT");

    if(certInfo.country != "") {subjString += "C=" + certInfo.country + "/";}
    if(certInfo.state != "") {subjString += "ST=" + certInfo.state + "/";}
    if(certInfo.city != "") {subjString += "L=" + certInfo.city + "/";}
    if(certInfo.organization != "") {subjString += "O=" + certInfo.organization + "/";}
    if(certInfo.unit != "") {subjString += "OU=" + certInfo.unit + "/";}
    if(certInfo.name != "") {subjString += "CN=" + certInfo.name + "/";}
    if(certInfo.email != "") {subjString += "emailAddress=" + certInfo.email + "/";}

    // TODO this should really be an atomic operation that restores the old files if it fails
    var args = ['genrsa', '-out', NovaHomePath + keyPath, '1024'];
    child_process.execFile("openssl", args, {}, function(err, stdout, stderr) {
        if (err) {
            res += "ERROR: " + err;
            cb && cb(res);
            return;
        }

        res += stderr + stdout;
  
        args = ['req', '-new', '-key', NovaHomePath + keyPath, '-subj', subjString, '-out', NovaHomePath + '/config/keys' + 'ui.csr'];
        child_process.execFile("openssl", args, {}, function (err, stdout, stderr) {
            if (err) {
                res += "ERROR: " + err;
                cb && cb(res);
                return;
            }

            res += stderr + stdout;

            args = ['x509', '-req', '-days', '365', '-in', NovaHomePath + '/config/keys' + 'ui.csr', '-signkey', NovaHomePath + keyPath, '-out', NovaHomePath + certPath];
            child_process.execFile("openssl", args, {}, function(err, stdout, stderr) {
                if (err) {
                    res += "ERROR: " + err;
                    cb && cb(res);
                    return;
                }

                res += stderr + stdout;
                cb && cb(null, res);
            });
        });
    });
};

}



module.exports = NowjsMethods;

