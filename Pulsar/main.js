//============================================================================
//// Name        : main.js
//// Copyright   : DataSoft Corporation 2011-2013
////  Nova is free software: you can redistribute it and/or modify
////   it under the terms of the GNU General Public License as published by
////   the Free Software Foundation, either version 3 of the License, or
////   (at your option) any later version.
////
////   Nova is distributed in the hope that it will be useful,
////   but WITHOUT ANY WARRANTY; without even the implied warranty of
////   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
////   GNU General Public License for more details.
////
////   You should have received a copy of the GNU General Public License
////   along with Nova.  If not, see <http://www.gnu.org/licenses/>.
//// Description : Main node.js file for the Pulsar mothership UI
////============================================================================

var novaconfig = require('novaconfig.node');

var express = require('express');
var nowjs = require('now');
var fs = require('fs');
var crypto = require('crypto');
var sql = require('sqlite3');
var schedule = require('node-schedule');
var ncp = require('ncp').ncp;
var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();

var passport = require('passport');
var BasicStrategy = require('passport-http').BasicStrategy;

var NovaHomePath = config.GetPathHome();
var NovaSharedPath = config.GetPathShared();

ncp.limit = 16;

if(!fs.existsSync(NovaHomePath + '/config/Pulsar/'))
{
  fs.mkdirSync(NovaHomePath + '/config/Pulsar/');
  ncp(NovaSharedPath + '/Pulsar/client_groups.txt', NovaHomePath + '/config/Pulsar/client_groups.txt', {clobber:false}, function(err){
    if(err)
    {
      console.log('copy file failed: ' + err);
      return;
    }
    ncp(NovaSharedPath + '/Pulsar/Instructions.txt', NovaHomePath + '/config/Pulsar/Instructions.txt', {clobber:false}, function(err){
      if(err)
      {
        console.log('copy file failed: ' + err);
        return;
      }
      ncp(NovaSharedPath + '/Pulsar/notifications.txt', NovaHomePath + '/config/Pulsar/notifications.txt', {clobber:false}, function(err){
        if(err)
        {
          console.log('copy file failed: ' + err);
          return;
        }
        ncp(NovaSharedPath + '/Pulsar/ClientConfigs/', NovaHomePath + '/config/Pulsar/ClientConfigs/', {clobber:false}, function(err){
          if(err)
          {
            console.log('copy file failed: ' + err);
            return;
          }
        });
      });
    });
  });
}

var matchHostToConnection = {};

// Setup TLS
var app;
if(config.ReadSetting("PULSAR_WEBUI_TLS_ENABLED") == "1") 
{
	var keyPath = config.ReadSetting("PULSAR_WEBUI_TLS_KEY");
	var certPath = config.ReadSetting("PULSAR_WEBUI_TLS_CERT");
	var passPhrase = config.ReadSetting("PULSAR_WEBUI_TLS_PASSPHRASE");
	express_options = {
		key: fs.readFileSync(NovaHomePath + keyPath),
		cert: fs.readFileSync(NovaHomePath + certPath),
		passphrase: passPhrase
	};
 
	app = express.createServer(express_options);
} 
else 
{
	app = express.createServer();
}

// Configure the express server to use the 
// bodyParser so that we can view and use the 
// contents of the req variable in the get/post
// directives
app.configure(function()
{
  app.use(passport.initialize());
	app.use(express.bodyParser());
	app.use(app.router);
});

var LOG = require('../NodejsModule/Javascript/Logger').LOG;

LOG('ALERT', 'Starting PULSAR version ' + config.GetVersionString());

process.chdir(NovaHomePath);

var DATABASE_HOST = config.ReadSetting('DATABASE_HOST');
var DATABASE_USER = config.ReadSetting('DATABASE_USER');
var DATABASE_PASS = config.ReadSetting('DATABASE_PASS');

var databaseOpenResult = function(err){
  if (err == null) 
  {
    console.log('Opened sqlite3 database file.');
  } 
  else 
  {
    LOG(ERROR, 'Error opening sqlite3 database file: ' + err);
  }
}

var db = new sql.Database(NovaHomePath + '/data/pulsarDatabase.db', sql.OPEN_READWRITE, databaseOpenResult);

var dbqCredentialsRowCount = db.prepare('SELECT COUNT(*) AS rows from credentials');
var dbqCredentialsCheckLogin = db.prepare('SELECT user, pass FROM credentials WHERE user = ? AND pass = ?');
var dbqCredentialsGetUsers = db.prepare('SELECT user FROM credentials');
var dbqCredentialsGetUser = db.prepare('SELECT user FROM credentials WHERE user = ?');
var dbqCredentialsGetSalt = db.prepare('SELECT salt FROM credentials WHERE user = ?');
var dbqCredentialsChangePassword = db.prepare('UPDATE credentials SET pass = ?, salt = ? WHERE user = ?');
var dbqCredentialsInsertUser = db.prepare('INSERT INTO credentials VALUES(?, ?, ?)');
var dbqCredentialsDeleteUser = db.prepare('DELETE FROM credentials WHERE user = ?');

var HashPassword = function(password, salt){
  var shasum = crypto.createHash('sha1');
  shasum.update(password + salt);
  return shasum.digest('hex');
}

passport.serializeUser(function(user, done){
  done(null, user);
});

passport.deserializeUser(function(user, done){
  done(null, user);
});

passport.use(new BasicStrategy(function(username, password, done){
  var user = username;
  
  process.nextTick(function(){
    dbqCredentialsRowCount.all(function(err, rowcount){
      if(err) 
      {
        console.log('Database error: ' + err);
      }

      // If there are no users, add default nova and log in
      if(rowcount[0].rows == 0)
      {
        console.log('No users in user database. Creating default user.');
        dbqCredentialsInsertUser.run('nova', HashPassword('toor', 'root'), 'root', function(err) {
          if (err) 
          {
            throw err;
          }

          switcher(err, user, true, done);
        });
      } 
      else
      {
        dbqCredentialsGetSalt.all(user, function cb(err, salt)
        {
          if(err || (salt[0] == undefined))
          {
            switcher(err, user, false, done);
          }
          else
          {
            dbqCredentialsCheckLogin.all(user, HashPassword(password, salt[0].salt),
    
            function selectCb(err, results){
              if (err) 
              {
                console.log('Database error: ' + err);
              }
    
              if (results[0] === undefined) 
              {
                switcher(err, user, false, done);
              } 
              else if (user === results[0].user) 
              {
                switcher(err, user, true, done);
              } 
              else 
              {
                switcher(err, user, false, done);
              }
            });
          }
        });
      }
    });
  });
}));

// Initailize the object that will store the connections from the various
// Quasar clients
var novaClients = new Object();
var fileAssociations = new Array();
var suspectIPs = new Array();
var eventCounter = new Array();
var scheduledMessages = new Array();
var details = '';
var clientsBenignRequests = new Array();
var notifications = 0;
var hostileEvents = 0;

// Set the various view options, as well as the listening port
// and the directory for the server and the jade files to look for 
// src files, etc.
app.set('view options', { layout: false });
app.set('views', __dirname + '/views');
var MASTER_UI_PORT = parseInt(config.ReadSetting("MASTER_UI_PORT"));
app.listen(MASTER_UI_PORT);
app.use(express.static(NovaSharedPath + '/Pulsar/www'));

// Initialize nowjs to listen to our express server
var everyone = nowjs.initialize(app);

process.on('SIGTERM', function(){
  console.log('SIGTERM recieved');
  cleanUI(function(){
    SaveClientIds();
  });
  saveScheduledEvents(function(){
    process.exit(1);
  });
  process.exit(1);
});

process.on('SIGKILL', function(){
  console.log('SIGKILL received');
  cleanUI(function(){
    SaveClientIds();
  });
  saveScheduledEvents(function(){
    process.exit(1);
  });
  process.exit(1);
});

process.on('SIGINT', function(){
  console.log('SIGINT received');
  cleanUI(function(){
    SaveClientIds();
  });
  saveScheduledEvents(function(){
    process.exit(1);
  });
  process.exit(1);
});

function cleanUI(callback)
{
  if(typeof(everyone.now.ClearGroupsList) == 'function')
  {
    everyone.now.ClearGroupsList();
  }
  if(typeof(everyone.now.ClearClientList) == 'function')
  {
    everyone.now.ClearClientList();
  } 
  if(typeof(everyone.now.UpdateConnectionsList) == 'function')
  {
    everyone.now.UpdateConnectionsList('', 'clear'); 
  }
  if(typeof(callback) == 'function')
  {
    callback; 
  }
}

readScheduledEvents();

function saveScheduledEvents(callback)
{
  // Convert the objects within the scheduledMessages array
  // into JSON (or some other format easily parsed into and 
  // from XML) and write them to a file;
  // Upon restart, read from this file and convert the object
  // literal information back into the format required by the 
  // array, and the schedule module for Nodejs
  try
  {
    fs.unlinkSync(NovaHomePath + '/config/Pulsar/scheduledEvents.txt');
  }
  catch(err)
  {
    console.log('could not save scheduled events: ' + err);
    return;
  }
  
  var closeMe = fs.open(NovaHomePath + '/config/Pulsar/scheduledEvents.txt', 'w');
  closeMe.close(closeMe);
  
  var writer = fs.createWriteStream(NovaHomePath + '/config/Pulsar/scheduledEvents.txt', {'flags':'a'});
  
  for(var i in scheduledMessages)
  {
    writeScheduledEventStructure(scheduledMessages[i], function(data){
      writer.write(data);
    });
  } 
  writer.end();
  
  writer.on('close', function(){
    callback();
  });
}

function readScheduledEvents()
{
  try
  {
    var fsread = fs.readFileSync(NovaHomePath + '/config/Pulsar/scheduledEvents.txt', 'utf8');
    var splitUp = fsread.split(/\r\n|\r|\n/);
    
    if(splitUp != '')
    {
      for(var i in splitUp)
      {
        if(splitUp[i] != '')
        {
          addScheduledEventToArray(JSON.parse(splitUp[i])); 
        }      
      }
    }
    else
    {
      console.log('No scheduled events saved');
      fs.unlinkSync(NovaHomePath + '/config/Pulsar/scheduledEvents.txt'); 
    }
  }
  catch(err)
  {
    console.log('No scheduled events saved.');
  } 
}

function addScheduledEventToArray(json)
{
  if((json.recurring == '' || json.recurring == undefined) && (json.date == undefined || json.date == ''))
  {
    return; 
  }
  
  var newSchedule = {};
  newSchedule.id = json.id;
  newSchedule.clientId = json.clientId;
  newSchedule.messageType = json.messageType;
  
  var message = {};
  message.id = json.clientId + ':';
  message.type = json.messageType;
  
  if((json.date == undefined || json.date == '') && (json.recurring != '' && json.recurring != undefined))
  {
    newSchedule.eventType = 'recurring';
    var passObj = new schedule.RecurrenceRule();
    passObj.dayOfWeek = json.dayOfWeek;
    passObj.hour = json.hour;
    passObj.minute = json.minute;
    newSchedule.job = schedule.scheduleJob(newSchedule.id, passObj, function(){
      everyone.now.MessageSend(message);
    });
    newSchedule.recurring = json.recurring;
    newSchedule.date = '';
  }
  else if(json.date != undefined && json.date != '')
  {
    newSchedule.eventType = 'date';
    var passDate = new Date(json.date);
    newSchedule.job = schedule.scheduleJob(newSchedule.id, passDate, function(){
      everyone.now.MessageSend(message);
    });
    newSchedule.date = passDate.toString();
    newSchedule.recurring = '';
  }
  else
  {
    console.log('No date or cron string specified in ' + newSchedule.id + ', doing nothing.');
    return; 
  }
  
  for(var i in scheduledMessages)
  {
    if(scheduledMessages[i].id == newSchedule.id)
    {
      console.log('There is already an event with that name, please choose another name.');
      return;
    } 
  }
  
  scheduledMessages.push(newSchedule);
}

function writeScheduledEventStructure(sEvent, callback)
{
  var stringifyMe = {};
  stringifyMe.id = sEvent.id;
  stringifyMe.clientId = sEvent.clientId;
  stringifyMe.messageType = sEvent.messageType;
  stringifyMe.eventType = sEvent.eventType;
  stringifyMe.recurring = sEvent.recurring;
  stringifyMe.date = sEvent.date;
  
  if(stringifyMe.recurring != undefined && stringifyMe.recurring != '')
  {
    stringifyMe.dayOfWeek = sEvent.dayOfWeek;
    stringifyMe.hour = sEvent.hour;
    stringifyMe.minute = sEvent.minute;
  }
  
  callback(JSON.stringify(stringifyMe) + '\n');
}

// A note about the everyone.now.* functions, especially those
// that are rooted in the jade files:
// If, for some reason, a call is made to a nowjs call that isn't
// yet defined (i.e. something like getting a message that forces a 
// jade-side nowjs call for updating elements, but that jade file isn't
// loaded), the Pulsar connections will break and some undefined
// behavior begins to occur. To be safe, structure the message cases
// to update some server side element, and then call any jade-side nowjs
// magic inside of a conditional checking that the typeof the function
// to call is the string 'function'

// Generic message sending method; can be called from the jade files
// as well as on the server side. Parsing for messages sent from here
// is handled on the Quasar side.
MessageSend = function(message) 
{
    var targets = message.id.split(':');
    var seen = new Array();
    var sendMessage = true;
    
    for(var i in targets)
    {
      sendMessage = true;
      for(var j in seen)
      {
        if(targets[i] == seen[j])
        {
          sendMessage = false;
        }  
      }
      if(targets[i] != '' && targets[i] != undefined && sendMessage)
      {
        if(message.type == 'requestBenign')
        {
          AddClientBenignRequest(targets[i]);
        }
        else if(message.type == 'cancelRequestBenign')
        {
          RemoveClientBenignRequest(targets[i]);
        }
	      novaClients[targets[i]].connection.sendUTF(JSON.stringify(message));
	      seen.push(targets[i]);
      }
    }
    seen.length = 0;
    targets.length = 0;
};
everyone.now.MessageSend = MessageSend;

GetSuspectDetails = function(suspect)
{
  var message = {};
  message.type = 'suspectDetails';
  message.id = suspect.clientId;
  message.ip = suspect.ip;
  message.iface = suspect.interface;
  novaClients[suspect.clientId].connection.sendUTF(JSON.stringify(message));
}
everyone.now.GetSuspectDetails = GetSuspectDetails

SetScheduledMessage = function(clientId, name, message, eventObj, cb)
{
  if(eventObj == undefined || eventObj == '')
  {
    cb(clientId, 'failed');
    return; 
  }
  
  var newSchedule = {};
  newSchedule.id = (clientId + '_' + name);
  newSchedule.clientId = clientId;
  newSchedule.messageType = message.type;
  
  if(typeof eventObj == 'object')
  {
    newSchedule.eventType = 'recurring';
    var passObj = new schedule.RecurrenceRule();
    passObj.dayOfWeek = eventObj.dayOfWeek;
    passObj.hour = parseInt(eventObj.hour);
    passObj.minute = parseInt(eventObj.minute);
    newSchedule.dayOfWeek = passObj.dayOfWeek;
    newSchedule.hour = passObj.hour;
    newSchedule.minute = (passObj.minute.toString().length == 2 ? passObj.minute : '0' + passObj.minute);
    newSchedule.job = schedule.scheduleJob(newSchedule.id, passObj, function(){
      MessageSend(message);
    });
    var recurringString = '';
    for(var i in eventObj.dayOfWeek)
    {
      switch(eventObj.dayOfWeek[i])
      {
        case 0: recurringString += 'Su ';
                  break;
        case 1: recurringString += 'M ';
                  break;
        case 2: recurringString += 'Tu ';
                  break;
        case 3: recurringString += 'W ';
                  break;
        case 4: recurringString += 'Th ';
                  break;
        case 5: recurringString += 'F ';
                  break;
        case 6: recurringString += 'Sa ';
                  break;
      }
    }
    newSchedule.recurring = recurringString + newSchedule.hour + ':' + newSchedule.minute;
    newSchedule.date = '';
  }
  else if(typeof eventObj == 'string')
  {
    newSchedule.eventType = 'date';
    var passDate = new Date(eventObj);
    newSchedule.job = schedule.scheduleJob(newSchedule.id, passDate, function(){
      MessageSend(message);
      UnsetScheduledMessage(newSchedule.id, function(result, message){
        console.log(message);
      });
      if(typeof everyone.now.RemoveEventFromList == 'function')
      {
        everyone.now.RemoveEventFromList(newSchedule.id);
      }
    });
    newSchedule.date = passDate.toString();
    newSchedule.recurring = '';
  }
  else
  {
    console.log('No date or cron string specified in ' + newSchedule.id + ', doing nothing.');
    return; 
  }
  
  for(var i in scheduledMessages)
  {
    if(scheduledMessages[i].id == newSchedule.id)
    {
      console.log('There is already an event with that name, please choose another name.');
      cb(clientId, 'failed', 'Name conflict for ' + newSchedule.id + ', choose another name.');
      return;
    } 
  }
  
  cb(clientId, 'succeeded');
  scheduledMessages.push(newSchedule);
}
everyone.now.SetScheduledMessage = SetScheduledMessage;

UnsetScheduledMessage = function(name, cb)
{
  for(var i in scheduledMessages)
  {
    if(scheduledMessages[i].id == name)
    { 
       scheduledMessages[i].job.cancel();
       cb('true', 'Removed event ' + scheduledMessages[i].id);
       delete scheduledMessages[i];
       return;
    }
  }
  cb('false', 'No event by id ' + name + ', doing nothing');
}
everyone.now.UnsetScheduledMessage = UnsetScheduledMessage;

GetScheduledEvents = function(callback)
{
  for(var i in scheduledMessages)
  {
    var json = {};
    json.clientId = scheduledMessages[i].clientId;
    json.messageType = scheduledMessages[i].messageType;
    json.eventName = scheduledMessages[i].id;
    json.eventType = scheduledMessages[i].eventType;
    json.recurring = scheduledMessages[i].recurring; 
    json.date = scheduledMessages[i].date;
    callback(json);
  }
}
everyone.now.GetScheduledEvents = GetScheduledEvents;

WriteNotification = function(notify)
{
  var append = fs.readFileSync(NovaHomePath + '/config/Pulsar/notifications.txt', 'utf8');
  var newNotify = '';
  if(append != undefined)
  {
    newNotify = notify.toString() + '\n' + append;
  }
  else
  {
    newNotify = notify.toString() + '\n';
  }
  fs.writeFileSync(NovaHomePath + '/config/Pulsar/notifications.txt', newNotify);
}
everyone.now.WriteNotification = WriteNotification;

GetNotifications = function(callback)
{
  var notificationData = fs.readFileSync(NovaHomePath + '/config/Pulsar/notifications.txt', 'utf8');
  if(typeof callback == 'function')
  {
    callback(notificationData);
  }
}
everyone.now.GetNotifications = GetNotifications;

GetNotifyCount = function(callback)
{
  if(typeof callback == 'function')
  {
    callback(notifications);
  }
}
everyone.now.GetNotifyCount = GetNotifyCount;

GetHostileEventsCount = function(callback)
{
  if(typeof callback == 'function')
  {
    callback(hostileEvents);
  }
}
everyone.now.GetHostileEventsCount = GetHostileEventsCount;

UpdateNotificationsCount = function(count)
{
  notifications = count;
}
everyone.now.UpdateNotificationsCount = UpdateNotificationsCount;

UpdateHostileEventsCount = function(count)
{
  hostileEvents = parseInt(count);
}
everyone.now.UpdateHostileEventsCount = UpdateHostileEventsCount;

GetClients = function(callback)
{
  var ret = new Array();
  for(var i in novaClients)
  {
    ret.push(i);
  }
  if(typeof callback == 'function')
  {
    callback(ret);
  }
}
everyone.now.GetClients = GetClients;

UpdateEventCounter = function(client, newNum)
{
  for(var i in eventCounter)
  {
    if(eventCounter[i].client == client)
    {
      eventCounter[i].events = newNum;
      return;
    }
  }
}
everyone.now.UpdateEventCounter = UpdateEventCounter;

ClearEventCounter = function(callback)
{
  for(var i in eventCounter)
  {
    eventCounter[i].events = 0;
  }  
}
everyone.now.ClearEventCounter = ClearEventCounter;

GetEventCount = function(client, callback)
{
  var count = 0;
  for(var i in eventCounter)
  {
    if(eventCounter[i].client == client)
    {
      count = parseInt(eventCounter[i].events);
      if(typeof callback == 'function')
      {
        callback(count);
      }
      break;
    }
  }
}
everyone.now.GetEventCount = GetEventCount;

UpdateStatus = function(clients, component, running)
{
  var clientsToUpdate = clients.split(':');
  for(var i in clientsToUpdate)
  {
    if(clientsToUpdate[i] != '' && clientsToUpdate[i] != undefined)
    {
      if(component == 'nova')
      {
        novaClients[clientsToUpdate[i]].statusNova = running;
      }
      else if(component == 'haystack')
      {
        novaClients[clientsToUpdate[i]].statusHaystack = running;
      }
      if(typeof everyone.now.UpdateConnectionsList == 'function')
      {
        everyone.now.UpdateConnectionsList(clientsToUpdate[i], 'updateStatus');
      }
    }
  }
}
everyone.now.UpdateStatus = UpdateStatus;

IsNovadUp = function(clientId, callback)
{
  if(typeof callback == 'function')
  {
    callback(clientId, novaClients[clientId].statusNova);
  } 
}
everyone.now.IsNovadUp = IsNovadUp;

IsHaystackUp = function(clientId, callback)
{
  if(typeof callback == 'function')
  {
    callback(clientId, novaClients[clientId].statusHaystack);
  }
}
everyone.now.IsHaystackUp = IsHaystackUp;

GetHostileSuspects = function()
{
  var message = {};
  message.type = 'getHostileSuspects';
  
  for(var i in novaClients)
  {
    if(novaClients[i].connection != null)
    {
      novaClients[i].connection.sendUTF(JSON.stringify(message));
    }
  }
}
everyone.now.GetHostileSuspects = GetHostileSuspects;

ClearSuspectIPs = function(callback)
{
  suspectIPs.length = 0; 
  callback(suspectIPs.length);
}
everyone.now.ClearSuspectIPs = ClearSuspectIPs;

// Remove a user-defined group from the client_groups.txt file
RemoveGroup = function(group, cb)
{
  console.log('Removing group ' + group + ' from the client groups file');
  try
  {
    var groupFile = fs.readFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', 'utf8');
    var regex = group + '.*?;';
    var replaceWithNull = new RegExp(regex, 'g');
    groupFile = groupFile.replace(replaceWithNull, '');
    groupFile = trimNewlines(groupFile);
    fs.writeFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', groupFile);
  }
  catch(err)
  {
    console.log('err during RemoveGroup: ' + err);
    if(cb != undefined)
    {
      cb('Could not Remove Group: ' + err);
    }
  }
};
everyone.now.RemoveGroup = RemoveGroup;

// Update a group inside the client_groups.txt file to have
// a new list of member clientIds.
UpdateGroup = function(group, newMembers, callback)
{
  console.log('Updating group ' + group + ' to have members ' + newMembers);
  try
  {
    var groupFile = fs.readFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', 'utf8');
    var regex = group + '.*?;';
    var replaceWithNull = new RegExp(regex, 'g');
    var sanitizeMemberString = '';
    for(var i in newMembers)
    {
      if(newMembers[i] != ',' && newMembers[i] != ';')
      {
        sanitizeMemberString += newMembers[i];
      }
      else
      {
        if(newMembers[i] == ',')
        {
          var nextIndex = (parseInt(i) + 1);
          if(newMembers[nextIndex] != undefined && (newMembers[nextIndex] == ';' || newMembers[nextIndex] == ','))
          {
           
          }
          else if(newMembers[nextIndex] == undefined)
          {
            
          }
          else
          {
            sanitizeMemberString += newMembers[i]; 
          }
        }
        if(newMembers == ';')
        {
          
        }
      }
    }
    groupFile = groupFile.replace(replaceWithNull, group + ':' + sanitizeMemberString + ';');
    fs.writeFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', groupFile);
    if(typeof callback == 'function')
    {
      callback();
    }
  }
  catch(err)
  {
    if(callback != undefined)
    {
      callback('Could not update group: ' + err);
    }
  }
}
everyone.now.UpdateGroup = UpdateGroup;

// Add a new user-defined group to the client_groups.txt file
AddGroup = function(group, members, cb)
{
  try
  {
    console.log('Adding group ' + group + ' with members ' + members);
    var groupFile = fs.readFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', 'utf8');
    groupFile += '\n' + group + ':' + members + ';';
    fs.writeFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', groupFile);
  }
  catch(err)
  {
    if(cb != undefined)
    {
      cb('Could not add group: ' + err);
    }
  }
}
everyone.now.AddGroup = AddGroup;

AddClientBenignRequest = function(clientId)
{
  for(var i in clientsBenignRequests)
  {
    if(clientsBenignRequests[i] == clientId)
    {
      return; 
    }
  }
  clientsBenignRequests.push(clientId);
}
everyone.now.AddClientBenignRequest = AddClientBenignRequest;

RemoveClientBenignRequest = function(clientId)
{
  for(var i in clientsBenignRequests)
  {
    if(clientsBenignRequests[i] == clientId)
    { 
      delete clientsBenignRequests[i]; 
    }
  }
}
everyone.now.RemoveClientBenignRequest = RemoveClientBenignRequest;

GetClientBenignRequest = function(callback)
{
  var ret = [];
  for(var i in clientsBenignRequests)
  {
    ret.push(clientsBenignRequests[i]);
  }
  
  if(typeof(callback) == 'function')
  {
    callback(ret);
  } 
}
everyone.now.GetClientBenignRequest = GetClientBenignRequest;

GetGroupMembers = function(group, callback)
{
  var groupFile = fs.readFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', 'utf8');
  var start = groupFile.indexOf(group);
  var end = groupFile.indexOf(';', start) - 2;
  if(typeof callback == 'function')
  {
    callback(trimNewlines(groupFile.substr((start + group.length + 1), end))); 
  }
}
everyone.now.GetGroupMembers = GetGroupMembers;

// Given the client id and a cb function (which will process the results)
// grab the list of interfaces for that client.
GetInterfacesOfClient = function(clientId, cb)
{
  var interfaceFile = fs.readFileSync(NovaHomePath + '/config/Pulsar/ClientConfigs/iflist@' + clientId + '.txt', 'utf8');
  var pass = interfaceFile.split(',');
  if(typeof cb == 'function')
  {
    cb(pass);
  }
}
everyone.now.GetInterfacesOfClient = GetInterfacesOfClient;

function UpdateBaseConfig(configValues)
{
  for(var i in configValues)
  {
    if(i != 'id' && i != 'type')
    {
      config.WriteSetting(i, configValues[i]);
    }
  }
}
everyone.now.UpdateBaseConfig = UpdateBaseConfig;

// Convenience method for trimming file new-lines, in the case
// that we eliminate the first group in the client_groups.txt file
// for example. 
function trimNewlines(string)
{
  var ret = string;
  if(string[0] == '\n')
  {
    ret = string.substr(1);
  }
  if(string[string.length - 1] == '\n')
  {
    ret = ret.substr(0, ret.length - 1); 
  }
  return ret;
}

function getEventList()
{
  return JSON.stringify(eventCounter);
}

function getClientIds()
{
  var ret = '';
  var seen = new Array();
  var push = true;
  for(var i in novaClients)
  {
    for(var i in seen)
    {
      if(i == seen[i])
      {
        push = false;
      } 
    }
    if(push)
    {
     ret += i + '\n';
     seen.push(i);
    }
  }
  seen = null;
  return ret;
}

function SaveClientIds(callback)
{
  fs.writeFileSync(NovaHomePath + '/config/Pulsar/clientIds.txt', getClientIds());
  if(typeof callback == 'function')
  {
    callback; 
  }
}

function populateNovaClients()
{
  try
  {
    var seen = new Array();
    var clientFileList = fs.readFileSync(NovaHomePath + '/config/Pulsar/clientIds.txt', 'utf8').split(/\r\n|\r|\n/); 
    
    if(clientFileList == '' || clientFileList == undefined)
    {
      console.log('clientIds.txt empty, doing nothing');
      return;
    }
    for(var i in clientFileList)
    {
      var push = true;
      if(clientFileList[i].toString().trim() != '')
      {
        for(var i in seen)
        {
          if(clientFileList[i].toString().trim() == seen[i])
          {
            push = false;
          }
        }  
        if(push)
        {
          console.log('Adding clientId ' + clientFileList[i].toString().trim() + ' to novaClients');
          novaClients[clientFileList[i].toString().trim()] = {statusNova: '', statusHaystack: '', connection: null};
          seen.push(clientFileList[i].toString().trim());
        }
      } 
    }
    seen = null;
  }
  catch(err)
  {
    console.log('clientIds.txt does not exist, it will be created when Pulsar next goes down'); 
  } 
}

// Begin setup of websocket server. Going to have an 
// https base server so that we can use wss:// connections
// instead of basic ws://
var WebSocketServer = require('websocket').server;
var https = require('https');

// Eventually we will want these options to include the
// commented stanzas; the ca option will contain a list of 
// trusted certs, and the other options do what they say
// or something Pulsar specific

var options;
if (config.ReadSetting("PULSAR_TETHER_TLS_ENABLED") == "1") {
	options = {
	  key: fs.readFileSync(NovaHomePath + config.ReadSetting("PULSAR_TETHER_TLS_KEY")),
	  cert: fs.readFileSync(NovaHomePath + config.ReadSetting("PULSAR_TETHER_TLS_CERT")),
	  passphrase: config.ReadSetting("PULSAR_TETHER_TLS_PASSPHRASE"),
	  ca: fs.readFileSync(NovaHomePath + config.ReadSetting("PULSAR_TETHER_TLS_CA")),
	  requestCert:		true,
	  rejectUnauthorized:	true
	};
} else {
	options = {
	  key: fs.readFileSync(NovaHomePath + config.ReadSetting("PULSAR_TETHER_TLS_KEY")),
	  cert: fs.readFileSync(NovaHomePath + config.ReadSetting("PULSAR_TETHER_TLS_CERT")),
	  passphrase: config.ReadSetting("PULSAR_TETHER_TLS_PASSPHRASE"),
	 };
}

// Create the httpsServer, and make it so that any requests for urls over http
// to the server are met with 404s; after the initial handshake, we shouldn't be 
// dealing directly with http, we should let websockets do the work. We may want to 
// look into https as a fallback, however, in case websockets is unsupported/firewalled
// etc.
var httpsServer = https.createServer(options, function(request, response)
{
	console.log('Recieved request for url ' + request.url);
	response.writeHead(404);
	response.end();
});

// Have to have the websockets listen on a different port than the express
// server, or else it'll catch all the messages that express is meant to get
// and lead to some undesirable behavior
httpsServer.listen((MASTER_UI_PORT + 1), function()
{
	console.log('Pulsar Server is listening on ' + (MASTER_UI_PORT + 1));
});

// Initialize the WebSocketServer to use the httpsServer as the 
// base server
var wsServer = new WebSocketServer({
	httpServer: httpsServer
});

populateNovaClients();

// On request, accept the connection. I'm a little wary of the way this is 
// structured (in terms of connection acceptance) but I've made Quasar attempt to 
// connect over ws://, which didn't work (a good thing) as well as trying to connect
// with wss:// but bad certs, which didn't work either (double good). I think there's 
// a lot of back-end warlockery that happens to moderate authentication, but we may want
// to find an explicit way to manage it purely for code readability's sake.
wsServer.on('request', function(request)
{
	console.log('connection accepted');
	var connection = request.accept(null, request.origin);
  matchHostToConnection[connection] = request.remoteAddress;
  // The most important directive, if we have a message, we need to parse it out
  // and determine what to do from there
	connection.on('message', function(message){
    // Probably not needed, but here for kicks
		if(message.type === 'utf8')
		{
      // Parse the message data into a JSON object
			var json_args = JSON.parse(message.utf8Data);
      // If the parsing went well...
			if(json_args != undefined)
			{
        // ...then look at the message type and switch from there
        // A note on the message types. The string that is put into the 
        // json_args.type JSON member on the client side MUST MATCH CASE 
        // EXACTLY with the case strings
				switch(json_args.type)
				{
          // addId is the first message a client sends to the Pulsar,
          // essentially binds their client id to the connection that has
          // has been created for future reference and connection management
					case 'addId':
						for(var i in novaClients)
						{
						  if(i == json_args.id.toString() && novaClients[i].connection != null)
						  {
						    console.log('There is already a client connected with id ' + json_args.id);
						    connection.close();
						    return;
						  }
						}
						if(json_args.benignRequest == 'true')
						{
						   AddClientBenignRequest(json_args.id);
						   console.log('benignRequest was true on connection, adding to benign request list');
						   if(typeof(everyone.now.RenderBenignRequests) == 'function')
						   {
						     everyone.now.RenderBenignRequests(); 
						   }
						}
						novaClients[json_args.id.toString()] = {statusNova: json_args.nova, 
						                                        statusHaystack: json_args.haystack, 
						                                        connection: connection, 
						                                        url: matchHostToConnection[connection] + ':' + json_args.port};
            delete matchHostToConnection[connection]
						var date = new Date();
						WriteNotification(json_args.id + ' connected at ' + date);
						if(typeof everyone.now.UpdateNotificationsButton == 'function')
						{
						  everyone.now.UpdateNotificationsButton('new');
						}
            
            var getHostile = {};
            getHostile.type = 'getHostileSuspects';
            getHostile.id = json_args.id + ':';
            MessageSend(getHostile);
            if(typeof everyone.now.UpdateClientsList == 'function')
            {
              everyone.now.UpdateClientsList(json_args.id, 'add');
            }
            if(typeof everyone.now.UpdateConnectionsList == 'function')
            {
              everyone.now.UpdateConnectionsList(json_args.id, 'add');
            }
            eventCounter.push({client: json_args.id, events: '0'});
            GetGroupMembers('all', function(members){
              if(members.indexOf(json_args.id) == -1)
              {
                var newList = (members.split(';')[0] == '' ? json_args.id : members.split(';').join() + json_args.id);
                UpdateGroup('all', newList);
                if(typeof everyone.now.UpdateGroupList == 'function')
                {
                  everyone.now.UpdateGroupList('all', 'update');
                }
              }
            });
            SaveClientIds();
						break;
          // This case is reserved for response from the clients;
          // we should figure out a standard format for the responses 
          // that includes the client id and the message, at the very least.
          // Messages are formatted and constructed on the Quasar side.
					case 'response':
						console.log(json_args.response_message);
						WriteNotification(json_args.id + ' says ' + '"' + json_args.response_message + '"');
						if(typeof everyone.now.UpdateNotificationsButton == 'function')
            {
              everyone.now.UpdateNotificationsButton('new');
            }
						break;
          // This case is reserved for receiving and properly addressing
          // hostile suspect events from a client. Parses the message out into
          // an object literal which is then sent to the OnNewSuspect event. 
          // Might be able to get away with just sending the JSON object, my intent
          // was to validate the message here with conditionals, just isn't done yet.
					case 'hostileSuspect':
            console.log('Hostile Suspect ' + json_args.ip + ' received from ' + json_args.client + ' at ' + json_args.lastpacket);
						var suspect = {};
						suspect.ip = json_args.ip;
						suspect.classification = json_args.classification;		
						suspect.lastpacket = json_args.lastpacket;
						suspect.ishostile = json_args.ishostile;
						suspect.client = json_args.client;
						suspect.interface = json_args.interface;
						
						try
						{
						  var append = fs.readFileSync(NovaHomePath + '/config/Pulsar/ClientConfigs/suspects@' + json_args.client + '.txt', 'utf8');
						  var start = append.indexOf(suspect.ip + '@' + suspect.client + '_' + suspect.interface);
						  if(start == -1)
						  {
						    append += suspect.ip + '@' + suspect.client + '_' + suspect.interface + ':' + suspect.classification + ' ' + suspect.lastpacket + ';\n';
						  }
						  else
						  {
						    var end = append.indexOf(';', start);
						    var newAppend = suspect.ip + '@' + suspect.client + '_' + suspect.interface + ':' + suspect.classification + ' ' + suspect.lastpacket + ';';
						    append = append.replace(append.substr(start, end), newAppend);
						  }
						  fs.writeFileSync(NovaHomePath + '/config/Pulsar/ClientConfigs/suspects@' + json_args.client + '.txt', append);
						}
						catch(err)
						{
						  var write = suspect.ip + '@' + suspect.client + '_' + suspect.interface + ':' + suspect.classification + ' ' + suspect.lastpacket + ';\n';
						  fs.writeFileSync(NovaHomePath + '/config/Pulsar/ClientConfigs/suspects@' + json_args.client + '.txt', write); 
						}
						
						if(typeof everyone.now.OnNewSuspect == 'function')
						{
						  everyone.now.OnNewSuspect(suspect);
						}
						if(contains(suspectIPs, (suspect.ip + '@' + suspect.client)))
            {
              console.log('already found suspect ' + suspect.ip + '@' + suspect.client + ', not updating hostile events button');
            }
            else
            {
              console.log('adding suspect ' + suspect.ip + '@' + suspect.client + ' to suspectIPs');
						  suspectIPs.push({ip: suspect.ip + '@' + suspect.client});
						  for(var i in eventCounter)
						  {
						    if(eventCounter[i].client == json_args.client)
						    {
						      eventCounter[i].events++;
						    }
						  }
						  if(typeof everyone.now.UpdateHostileEventsButton == 'function')
						  {
						    everyone.now.UpdateHostileEventsButton('new',  everyone.now.UpdateConnectionsList(json_args.client, 'updateEvents'));
						  }
						}
						break;
				  // This case is reserved for properly receiving and addressing benign requests from 
				  // a client. Similar to hostile suspect (in fact, exactly the same), but in the future
				  // I anticipate Pulsar-side logging differences, so I figured i'd make another
				  // case.
				  case 'benignSuspect':
				    console.log('Benign Suspect ' + json_args.ip + ' received from ' + json_args.client + ' at ' + json_args.lastpacket);
				    var suspect = {};
            suspect.ip = json_args.ip;
            suspect.classification = json_args.classification;    
            suspect.lastpacket = json_args.lastpacket;
            suspect.ishostile = json_args.ishostile;
            suspect.client = json_args.client;
            suspect.interface = json_args.interface;
            if(typeof everyone.now.OnNewSuspect == 'function')
            {
              everyone.now.OnNewSuspect(suspect);
            }
				    break;
				  // This case is for receiving client configuration files. It places
				  // of the client configuration files inside one folder, and differentiates them
				  // by their clientId. In the future, I'm planning to use these files both to show
				  // a given Quasar instances' configuration options, as well as maintaining synced
				  // configuration files. 
					case 'registerConfig':
						console.log('Nova Configuration received from ' + json_args.id);
						var push = {};
						push.client = json_args.id;
						push.file = (NovaHomePath + '/config/Pulsar/ClientConfigs/' + json_args.filename);
						if(novaClients[json_args.id] == undefined)
						{
						  fileAssociations.push(push);
              fs.writeFileSync(push.file, json_args.file);
              console.log('Configuration for ' + json_args.id + ' can be found at ' + json_args.filename);   
						}
            else
            {
              for(var i in fileAssociations)
              {
                if(fileAssociations[i].client == push.client)
                {
                  fs.unlinkSync(fileAssociations[i].file);
                  fileAssociations[i].file = push.file;
                  fs.writeFileSync(push.file, json_args.file);
                  break;
                }
              }
            }
						break;
				  // Rather than just query the client whenever a list of interfaces is needed (like for the haystack
				  // autoconfig tool) I opted to have the client register a file containing a list of their interfaces.
				  // Lightens the load on messaging, and once it's more fleshed out, will be polled to ensure
				  // that it remains up to date. 
				  case 'registerClientInterfaces':
				    fs.writeFileSync(NovaHomePath + '/config/Pulsar/ClientConfigs/' + json_args.filename, json_args.file);
				    console.log('Interfaces files for ' + json_args.id + ' can be found at ' + json_args.filename);
				    break;
				  // Case reserved for status updates from a given client about the classification and 
				  // haystack components.
				  case 'statusChange':
				    if(json_args.component == 'haystack')
				    {
				      if(novaClients[json_args.id].statusHaystack != json_args.status)
				      {
				        WriteNotification(json_args.id + ' has a component status change: IsHaystackUp returned ' + json_args.status);
                if(typeof everyone.now.UpdateNotificationsButton == 'function')
                {
                  everyone.now.UpdateNotificationsButton('new');
                }
				        novaClients[json_args.id].statusHaystack = json_args.status; 
				      }
				    }
				    else if(json_args.component == 'nova')
				    {
				      if(novaClients[json_args.id].statusNova != json_args.status)
				      {
  				      WriteNotification(json_args.id + ' has a component status change: IsNovadUp returned ' + json_args.status);
                if(typeof everyone.now.UpdateNotificationsButton == 'function')
                {
                  everyone.now.UpdateNotificationsButton('new');
                }
  				      novaClients[json_args.id].statusNova = json_args.status;
				      }
				    }
				    if(typeof everyone.now.UpdateConnectionsList == 'function')
				    {
				      everyone.now.UpdateConnectionsList(json_args.id, 'updateStatus');
				    }
				    break;
				  case 'renameRequest':
				    novaClients[json_args.newId] = novaClients[json_args.id];
				    delete novaClients[json_args.id];
				    WriteNotification(json_args.id + ' has changed its clientId to ' + json_args.newId);
				    if(typeof everyone.now.UpdateNotificationsButton == 'function')
            {
              everyone.now.UpdateNotificationsButton('new');
            }
				    if(everyone.now.UpdateConnectionsList == 'function')
				    {
				      everyone.now.UpdateConnectionsList(json_args.id, 'remove');
				      everyone.now.UpdateConnectionsList(json_args.newId, 'add'); 
				    }
				    if(typeof everyone.now.UpdateClientsList == 'function')
            {
              everyone.now.UpdateClientsList(json_args.id, 'remove');
              everyone.now.UpdateClientsList(json_args.newId, 'add');
            }
            var change = fs.readFileSync(NovaHomePath + '/config/Pulsar/clientIds.txt', 'utf8');
            change = change.replace(new RegExp(json_args.id), json_args.newId);
            fs.writeFileSync(NovaHomePath + '/config/Pulsar/clientIds.txt', change);
            if(typeof everyone.now.RefreshPageAfterRename == 'function')
            {
              everyone.now.RefreshPageAfterRename();
            }
				    break;
				  case 'detailsReceived':
				    console.log('Receiving suspect details from ' + json_args.id);
				    details = json_args.data;
				    if(typeof everyone.now.RenderSuspectDetails == 'function')
				    {
				      everyone.now.RenderSuspectDetails(details);
				    }
				    break;
					// If we've found a message type that we weren't expecting, or don't have a case
          // for, log this message to the console and do nothing.
					default:
						console.log('Unexpected/Unknown message type ' + json_args.type + ' received, doing nothing');
						break;
				}
			}
      else
      {
      }
		}	
	});
});


// If the connection to the server is dropped for some reason, just print a description.
// The client side will handle reconnects. Still need to find a way to remove connections 
// from novaClients when this action is hit; since we don't have the client id to use,
// however, it may be tough. Have to look and see if the connection passed in can be matched
// to elements in the object and bound that way
wsServer.on('close', function(connection, reason, description)
{
	console.log('Closed connection: ' + description);
  for(var i in novaClients)
  {
    if(novaClients[i].connection === connection)
    {
      novaClients[i] = {statusNova: '', statusHaystack: '', connection: null};
      if(typeof everyone.now.UpdateClientsList == 'function')
      {
        everyone.now.UpdateClientsList(i, 'remove');
      }
      if(typeof everyone.now.UpdateConnectionsList == 'function')
      {
        everyone.now.UpdateConnectionsList(i, 'updateStatus');
      }
      if(typeof everyone.now.ClearFromClientSide == 'function')
      {
        everyone.now.ClearFromClientSide(i);
      }
      GetGroupMembers('all', function(members){
        var newList = members.replace(new RegExp(i), '');
        newList = newList.substr(0, newList.length - 1);
        UpdateGroup('all', newList);
        if(typeof everyone.now.UpdateGroupList == 'function')
        {
          everyone.now.UpdateGroupList('all', 'update');
        }
      });
      var date = new Date();
      WriteNotification(i + ' disconnected at ' + date);
      if(typeof everyone.now.UpdateNotificationsButton == 'function')
      {
        everyone.now.UpdateNotificationsButton('new');
      }
    }
  }
});

function ClearFromNovaClients(client, cb)
{
  novaClients[client].connection.close();
  delete novaClients[client];
  
  if(typeof cb == 'function')
  {
    cb();
  }
}
everyone.now.ClearFromNovaClients = ClearFromNovaClients;

// A function to get a string representation of the clients list 
// from the novaClients object. Probably could do it with just 
// an array, but since the intent is to pass it to jade which will
// just make a string out of it anyways, I figured I'd cut out the 
// middle man.
function getClients()
{
	var ret = '';
	for(var i in novaClients)
	{
		if(i != undefined && novaClients[i].connection != null)
		{
			ret += (i + ':');
		}	
	}
	return ret;
}

everyone.now.GetClientHost = function(client, cb)
{
  if(typeof(cb) == 'function')
  {
    cb(novaClients[client].url);
  }
}

// A function that reads the file client_groups.txt that contains user-created groups names
// that have an associated list of last-known clientIds. 
function getGroups()
{
  try
  {
    var group = fs.readFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', 'ascii');
  }
  catch(err)
  {
    console.log('client_groups.txt was not found: ' + err);
    var data = '';
    var group = fs.writeFileSync(NovaHomePath + '/config/Pulsar/client_groups.txt', data, 'w');
    return JSON.stringify('');
  }
  if(group == '')
  {
    console.log('client groups file is empty');
  }
  else
  {
    var clientGroups = group.split(';');
    var ret = {};
    ret.groups = '';
    ret.members = '';
    
    for(var i = 0; i < clientGroups.length; i++)
    {
      if(clientGroups[i] != '' && clientGroups[i].length > 1)
      {
        var members = clientGroups[i].split(':');
        ret.groups += members[0].replace(/(\r\n|\n|\r)/gm,'') + ':';
        ret.members += members[1] + '|';
      }
    }
  }
  
  return JSON.stringify(ret);
}

function contains(a, obj) 
{
    var i = a.length;
    while (i--) 
    {
       if (a[i].ip === obj) 
       {
           return true;
       }
    }
    return false;
}

function getMessageTypes()
{
  // For now
  var ret = new Array();
  ret.push('startNovad');
  ret.push('startHaystack');
  ret.push('stopNovad');
  ret.push('stopHaystack');
  ret.push('requestBenign');
  ret.push('cancelRequestBenign');
  return ret.join();
}

// Going to need to do passport for these soon, I think.
app.get('/', passport.authenticate('basic', {session: false}), function(req, res) 
{
	res.render('main.jade', {locals:{
		CLIENTS: getClients()
		, GROUPS: getGroups()
		, EVENTS: getEventList()
	}});
});

app.get('/hostile', passport.authenticate('basic', {session: false}), function(req, res)
{
  res.render('main.jade', {locals:{
    CLIENTS: getClients()
    , GROUPS: getGroups()
    , EVENTS: getEventList()
  }});
});

app.get('/about', passport.authenticate('basic', {session: false}), function(req, res) 
{
    res.render('about.jade', {locals:{
      EVENTS: getEventList()
    }});
});

app.get('/config', passport.authenticate('basic', {session: false}), function(req, res) 
{
	res.render('config.jade', {locals:{
		CLIENTS: getClients()
		, GROUPS: getGroups()
		, EVENTS: getEventList()
		, TCP_TIMEOUT: config.ReadSetting('TCP_TIMEOUT')
		, TCP_CHECK_FREQ: config.ReadSetting('TCP_CHECK_FREQ')
		, CLASSIFICATION_TIMEOUT: config.ReadSetting('CLASSIFICATION_TIMEOUT')
		, K: config.ReadSetting('K')
		, EPS: config.ReadSetting('EPS')
		, CLASSIFICATION_THRESHOLD: config.ReadSetting('CLASSIFICATION_THRESHOLD')
		, DM_ENABLED: config.ReadSetting('DM_ENABLED')
		, ENABLED_FEATURES: config.ReadSetting('ENABLED_FEATURES')
		, FEATURE_NAMES: nova.GetFeatureNames()
		, SAVE_FREQUENCY: config.ReadSetting('SAVE_FREQUENCY')
		, DATA_TTL: config.ReadSetting('DATA_TTL')
		, SERVICE_PREFERENCES: config.ReadSetting('SERVICE_PREFERENCES')
		, CAPTURE_BUFFER_SIZE: config.ReadSetting('CAPTURE_BUFFER_SIZE')
		, MIN_PACKET_THRESHOLD: config.ReadSetting('MIN_PACKET_THRESHOLD')
		, CUSTOM_PCAP_FILTER: config.ReadSetting('CUSTOM_PCAP_FILTER')
		, CUSTOM_PCAP_MODE: config.ReadSetting('CUSTOM_PCAP_MODE')
		, CLEAR_AFTER_HOSTILE_EVENT: config.ReadSetting('CLEAR_AFTER_HOSTILE_EVENT')
	}});
});

app.get('/haystack', passport.authenticate('basic', {session: false}), function(req, res){
  res.render('haystack.jade', {locals:{
    CLIENTS: getClients()
    , GROUPS: getGroups()
    , EVENTS: getEventList()
  }});
});

app.get('/groups', passport.authenticate('basic', {session: false}), function(req, res){
  res.render('groups.jade', {locals:{
    CLIENTS: getClients()
    , GROUPS: getGroups()
    , EVENTS: getEventList()
  }});
});

app.get('/notifications', passport.authenticate('basic', {session: false}), function(req, res){
  res.render('notifications.jade', {locals:{
    EVENTS: getEventList()
  }});
});

app.get('/schedule', passport.authenticate('basic', {session: false}), function(req, res){
  res.render('schedule.jade', {locals:{
    CLIENTS: getClients()
    , GROUPS: getGroups()
    , EVENTS: getEventList()
    , TYPES: getMessageTypes()
  }});
});

app.get('/listschedule', passport.authenticate('basic', {session: false}), function(req, res){
  res.render('listschedule.jade', {locals:{
    CLIENTS: getClients()
    , GROUPS: getGroups()
    , EVENTS: getEventList()
    , TYPES: getMessageTypes()
  }});
});

function switcher(err, user, success, done) {
  if (!success) {
    return done(null, false, {
      message: 'Username/password combination is incorrect'
    });
  }
  return done(null, user);
}
