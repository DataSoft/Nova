//============================================================================
// Name        : main.js
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
// Description : Main node.js file for the Quasar web interface of Nova
//============================================================================

// Used for debugging. Download the node-segfault-handler to use
//var segvhandler = require('./node_modules/segvcatcher/lib/segvhandler')
//segvhandler.registerHandler();

//var agent = require('webkit-devtools-agent');

/*var memwatch = require('memwatch');

memwatch.on('leak', function(info)
{
    console.log(info);
});

var hd = new memwatch.HeapDiff();
memwatch.on('stats', function(stats)
{
    console.log(stats);
    var diff = hd.end();

    console.log(diff.change.details);
    //console.log(diff);
    console.log("\n\n");

    hd = new memwatch.HeapDiff();
});
*/

// Modules that provide bindings to C++ code in NovaLibrary and Nova_UI_Core

require('tls').SLAB_BUFFER_SIZE = 100 * 1024;

var NovaCommon = require('./NovaCommon.js');
NovaCommon.nova.CheckConnection();

var maildaemon = ''

// Modules from NodejsModule/Javascript
var LOG = NovaCommon.LOG;

if(!NovaCommon.honeydConfig.LoadAllTemplates())
{
    LOG("ERROR", "Call to initial LoadAllTemplates failed!");
}

var os = require('os');
var fs = require('fs');
var jade = require('jade');
var express = require('express');
var https = require('https');
var passport = require('passport');
var BasicStrategy = require('passport-http').BasicStrategy;
var exec = require('child_process').exec;
var nowjs = require("now");
var Validator = require('validator').Validator;
var sanitizeCheck = require('validator').sanitize;

var NovaHomePath = NovaCommon.config.GetPathHome();
var NovaSharedPath = NovaCommon.config.GetPathShared();

var novadLogPath = "/var/log/nova/Nova.log";
var honeydLogPath = "/var/log/nova/Honeyd.log";

var benignRequest = false;
var pulsar;

var autoconfig;

var interfaceAliases = new Object();
ReloadInterfaceAliasFile();

var RenderError = function (res, err, link)
{
    // Redirect them to the main page if no link was set
    link = typeof link !== 'undefined' ? link : "/";

    console.log("Reported Client Error: " + err);
    res.render('error.jade', {
            redirectLink: link
            , errorDetails: err
    });
}

LOG("ALERT", "Starting QUASAR version " + NovaCommon.config.GetVersionString());


process.chdir(NovaHomePath);

var DATABASE_HOST = NovaCommon.config.ReadSetting("DATABASE_HOST");
var DATABASE_USER = NovaCommon.config.ReadSetting("DATABASE_USER");
var DATABASE_PASS = NovaCommon.config.ReadSetting("DATABASE_PASS");

passport.serializeUser(function (user, done)
{
    done(null, user);
});

passport.deserializeUser(function (user, done)
{
    done(null, user);
});

passport.use(new BasicStrategy(

function (username, password, done)
{
  var user = username;
  process.nextTick(function (){
  NovaCommon.dbqCredentialsRowCount.all(function (err, rowcount)
  {
      if(err)
      {
        console.log("Database error: " + err);
      }

      // If there are no users, add default nova and log in
      if(rowcount[0].rows === 0)
      {
        console.log("No users in user database. Creating default user.");
        NovaCommon.dbqCredentialsInsertUser.run('nova', NovaCommon.HashPassword('toor', 'root'), 'root', function (err)
        {
          if(err)
          {
            throw err;
          }

          switcher(err, user, true, done);
        });
      }
      else
      {
        NovaCommon.dbqCredentialsGetSalt.all(user, function cb(err, salt)
        {
          if(err || (salt[0] == undefined))
          {
            switcher(err, user, false, done);
          }
          else
          {
            NovaCommon.dbqCredentialsCheckLogin.all(user, NovaCommon.HashPassword(password, salt[0].salt),
            function selectCb(err, results)
            {
              if(err)
              {
                console.log("Database error: " + err);
              }
              if(results[0] === undefined)
              {
                switcher(err, user, false, done);
              } 
              else if(user === results[0].user)
              {
                switcher(err, user, true, done);
              } 
              else
              {
                switcher(err, user, false, done);
              }
            });
         }});
        }
      });
    });
}));

// Setup TLS
var app = express();
if(NovaCommon.config.ReadSetting("QUASAR_WEBUI_TLS_ENABLED") == "1")
{
    var keyPath = NovaCommon.config.ReadSetting("QUASAR_WEBUI_TLS_KEY");
    var certPath = NovaCommon.config.ReadSetting("QUASAR_WEBUI_TLS_CERT");
    var passPhrase = NovaCommon.config.ReadSetting("QUASAR_WEBUI_TLS_PASSPHRASE");
    var express_options = {
        key: fs.readFileSync(NovaHomePath + keyPath),
        cert: fs.readFileSync(NovaHomePath + certPath),
        passphrase: passPhrase
    };
 
    var httpsServer = https.createServer(express_options, app);
}
else
{
    var httpsServer = https.createServer(app);
}

app.configure(function()
{
    app.use(passport.initialize());
    app.use(express.bodyParser());
    app.use(passport.authenticate('basic', {session: false}));
    app.use(app.router);
    app.use(express.static(NovaSharedPath + '/Quasar/www'));
});

app.set('views', __dirname + '/views');

app.set('view options', {
    layout: false
});

// Do the following for logging
console.info("Logging to ./serverLog.log");
var logFile = fs.createWriteStream('./serverLog.log', {flags: 'a'});
app.use(express.logger({stream: logFile}));

var WEB_UI_PORT = NovaCommon.config.ReadSetting("WEB_UI_PORT");
var WEB_UI_IFACE = '';
var WEB_UI_ADDRESS = '';
if(NovaCommon.config.ReadSetting("MANAGE_IFACE_ENABLE") == '1')
{
  WEB_UI_IFACE = NovaCommon.config.ReadSetting("WEB_UI_IFACE");
  WEB_UI_ADDRESS = '';
  var interfaces = os.networkInterfaces();
  var length = 0;
  
  for(var i in interfaces)
  {
    length++;
    if(i == WEB_UI_IFACE)
    {
      for(var j in interfaces[i])
      {
        var address = interfaces[i][j];
        if(address.family == 'IPv4' && !address.internal)
        {
          WEB_UI_ADDRESS = address.address;
        }
      }
    }
  }
  
  if(WEB_UI_ADDRESS == '')
  {
    console.log('Could not procure a value for WEB_UI_ADDRESS, using none.');
    console.info("Listening on port " + WEB_UI_PORT);
    httpsServer.listen(WEB_UI_PORT); 
  }
  else if(length == 2)
  {
    console.log('Only one interface available, defaulting');
    console.info("Listening on port " + WEB_UI_PORT);
    httpsServer.listen(WEB_UI_PORT); 
  }
  else
  {
    console.info("Listening on address " + WEB_UI_ADDRESS + ":" + WEB_UI_PORT + " (" + WEB_UI_IFACE + ")");
    httpsServer.listen(WEB_UI_PORT, WEB_UI_ADDRESS);
  }
}
else
{
  httpsServer.listen(WEB_UI_PORT);
  console.info("Listening on port " + WEB_UI_PORT);
}

var everyone = nowjs.initialize(httpsServer);
var NowjsMethods = require('./NowjsMethods.js');
var initEveryone = new NowjsMethods(everyone);

// Testing some log watching stuff
var logLines = new Array();

function LiveFileReader(filePath, cb) {
    this.processedLines = new Array();
    this.initialLength = 0;
    this.filePath = filePath;
    this.cb = cb;
    
    this.file = null;
    this.chunkLength = 2048;
    this.readBytes = 0;

    var self = this;
   
    fs.readFile(self.filePath, function(err, data)
    {
        if (err)
        {
            LOG("ERROR", "ERROR reading file: " + err);
            console.log("ERROR reading file: " + err);
            self.cb(err);
            return;
        }

        self.initialLength = String(data).length;
        self.processedLines = String(data).split("\n");
        self.reading = false;
        
        if (self.processedLines[self.processedLines.length - 1] == "") {
            self.processedLines.pop();
        }

        for (var i = 0; i < self.processedLines.length; i++) {
            cb(null, self.processedLines[i], i);
        }

        self.readBytes = self.initialLength;
    
        fs.open(self.filePath, 'r', function(err, fd)
        {
            if (err)
            {
                LOG("ERROR", "Unable to open log file for reading due to error: " + err);
                console.log("Unable to open log file for reading due to error: " + err);
                self.cb(err);
                return;
            }
            self.file = fd; 
            self.readSomeData();
        });

        self.processData = function(err, bytecount, buff)
        {
            self.reading = false;
            if (err)
            {
                LOG("ERROR", "Error reading log file: " + err);
                console.log("ERROR reading file: " + err);
                self(err);
                return;
            }
        
            var lastLineFeed = buff.toString('ascii', 0, bytecount).lastIndexOf('\n');
            if (lastLineFeed != -1)
            {
                var lineArray = buff.toString('ascii', 0, bytecount).slice(0, lastLineFeed).split("\n");
            
                for (var i = 0; i < lineArray.length; i++)
                {
                    if (lineArray[i] != "") {
                        self.cb(null, lineArray[i], self.processedLines.length);
                        self.processedLines.push(lineArray[i]);
                    }
                }

                self.readBytes += lastLineFeed + 1;
            } else {
                //self.readBytes += bytecount;
            }
    
        }

        self.readSomeData = function()
        {
            var fb = fs.read(self.file, new Buffer(self.chunkLength), 0, self.chunkLength, self.readBytes, self.processData);
        }
    
        fs.watch(self.filePath, {persistent: true}, function(event, filename)
        //fs.watchFile(self.filePath, function(curr, prev)
        {
            if (!self.reading)
            {
                self.reading = true;
                self.readSomeData();
            }
        });
    });
}

var initLogWatch = function ()
{
    var novadLogFileReader = new LiveFileReader(novadLogPath, function(err, line, lineNum) {
        if (err)
        {
            console.log("Callback got error" + err);
            return;
        }

        NovaCommon.dbqIsNewNovaLogEntry.all(lineNum, function(err, results) {
            if (err)
            {
                LOG("ERROR", err);
                console.log("ERROR: " + err);
                return;
            }
        
            if (results[0].rows === 0)
            {
                NovaCommon.dbqAddNovaLogEntry.run(lineNum, line);
            }
        });

        try {
            everyone.now.newLogLine(lineNum, line);
        } catch (err) {};
    });
    var novadLogFileReader = new LiveFileReader(honeydLogPath, function(err, line, lineNum) {
        if (err)
        {
            console.log("Callback got error" + err);
            return;
        }
        
        NovaCommon.dbqIsNewHoneydLogEntry.all(lineNum, function(err, results) {
            if (err)
            {
                LOG("ERROR", err);
                console.log("ERROR: " + err);
                return;
            }
        
            if (results[0].rows === 0)
            {
                NovaCommon.dbqAddHoneydLogEntry.run(lineNum, line);
            }
        });


        try {
            everyone.now.newHoneydLogLine(lineNum, line);
        } catch (err) {};
    });
}

initLogWatch();

console.log('Watching Nova.log and Honeyd.log');

if(NovaCommon.config.ReadSetting('MASTER_UI_ENABLED') === '1')
{
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // SOCKET STUFF FOR PULSAR 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Thoughts: Probably going to need a global variable for the websocket connection to
  // Pulsar; an emit needs to be made to Pulsar in certain circumstances,
  // such as recieving a hostile suspect, so having that be accessible to the nowjs methods
  // is ideal. Also need to find a way to maintain IP address of Pulsar and any needed
  // credentials for reboots, etc. 
  
  // The reason for the + 1 at the end is because that's the port in use for the 
  // server-to-server component of websockets on Pulsar. Have to do something in case 
  // the port gets changed on the Pulsar machine
  var connected = NovaCommon.config.ReadSetting('MASTER_UI_IP') + ':' + (parseInt(NovaCommon.config.ReadSetting('MASTER_UI_PORT')) + 1);
  
  var WebSocketClient = require('websocket').client;
  var client;
  
  if(NovaCommon.config.ReadSetting("QUASAR_TETHER_TLS_ENABLED"))
  {
    client = new WebSocketClient({
      tlsOptions: {
        cert: fs.readFileSync(NovaHomePath + NovaCommon.config.ReadSetting("QUASAR_TETHER_TLS_CERT")),
        key: fs.readFileSync(NovaHomePath + NovaCommon.config.ReadSetting("QUASAR_TETHER_TLS_KEY")),
        passphrase: NovaCommon.config.ReadSetting("QUASAR_TETHER_TLS_PASSPHRASE")
      }
    });
  }
  else
  {
    client = new WebSocketClient();
  }

  var clientId = NovaCommon.config.ReadSetting('MASTER_UI_CLIENT_ID');
  var reconnecting = false;
  var clearReconnect;
  var reconnectTimer = parseInt(NovaCommon.config.ReadSetting('MASTER_UI_RECONNECT_TIME')) * 1000;
  
  // If the connection fails, print an error message
  client.on('connectFailed', function(error)
  {
    console.log('Connect to Pulsar error: ' + error.toString());
    if(!reconnecting)
    {
      console.log('No current attempts to reconnect, starting reconnect attempts every ', (reconnectTimer / 1000) ,' seconds.');
      clearReconnect = setInterval(function(){console.log('attempting reconnect to wss://' + connected); client.connect('wss://' + connected, null);}, reconnectTimer);
      reconnecting = true;
    }
  });
  
  // If we successfully connect to pulsar, perform event-based actions here.
  // The interface for websockets is a little less nice than socket.io, simply because
  // you cannot name your own events, everything has to be done within the message
  // directive; it's only one more level of indirection, however, so no big deal
  client.on('connect', function(connection){
    // When the connection is established, we save the connection variable
    // to p global so that actions can be taken on the connection
    // outside of this callback if needed (hostile suspect, etc.)
    reconnecting = false;
    clearInterval(clearReconnect);
    pulsar = connection;
    var quick = {};
    quick.type = 'addId';
    quick.id = clientId;
    quick.nova = NovaCommon.nova.IsNovadConnected().toString();
    quick.haystack = NovaCommon.nova.IsHaystackUp(false).toString();
    quick.benignRequest = (benignRequest == true ? 'true' : 'false');
    quick.port = NovaCommon.config.ReadSetting("WEB_UI_PORT");
    // I don't know that we HAVE to use UTF8 here, there's a send() method as 
    // well as a 'data' member inside the message objects instead of utf8Data.
    // But, as it was in the Websockets tutorial Pherric found, we'll use it for now
    pulsar.sendUTF(JSON.stringify(quick));
    console.log('Registering client Id with pulsar');
    
    var configSend = {};
    configSend.type = 'registerConfig';
    configSend.id = clientId;
    configSend.filename = 'NOVAConfig@' + clientId + '.txt';
    configSend.file = fs.readFileSync(NovaHomePath + '/config/NOVAConfig.txt', 'utf8');
    pulsar.sendUTF(JSON.stringify(configSend));
    console.log('Registering NOVAConfig with pulsar');
  
    var interfaceSend = {};
    interfaceSend.type = 'registerClientInterfaces';
    interfaceSend.id = clientId;
    interfaceSend.filename = 'iflist@' + clientId + '.txt';
    interfaceSend.file = NovaCommon.config.ListInterfaces().sort().join();
    pulsar.sendUTF(JSON.stringify(interfaceSend));
    console.log('Registering interfaces with pulsar');
  
    console.log('successfully connected to pulsar');
  
    connection.on('message', function(message)
    {
      if(message.type === 'utf8')
      {
        // We send the messages as JSON, so we have to parse it out
        var json_args = JSON.parse(message.utf8Data);
        if(json_args != undefined)
        {
          // the various actions to take when a message is received
          switch(json_args.type)
          {
            case 'startNovad':
              everyone.now.StartNovad();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Novad is being started on ' + clientId;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'stopNovad':
              NovaCommon.StopNovad();
              NovaCommon.nova.CloseNovadConnection();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Novad is being stopped on ' + clientId;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'startHaystack':
              if(!NovaCommon.nova.IsHaystackUp())
              {
                NovaCommon.StartHaystack(false);
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Haystack is being started on ' + clientId;
                pulsar.sendUTF(JSON.stringify(response));
              }
              else
              {
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Haystack is already up, doing nothing';
                pulsar.sendUTF(JSON.stringify(response));
              }
              break;
            case 'stopHaystack':
              NovaCommon.StopHaystack();
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Haystack is being stopped on ' + clientId;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'writeSetting':
              // The nice thing about using JSON for the message passing is we
              // can name the object literal members whatever we want, allowing for
              // implicit specificity when creating and parsing the object
              var setting = json_args.setting;
              var value = json_args.value;
              NovaCommon.config.WriteSetting(setting, value);
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = setting + ' is now ' + value;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'getHostileSuspects':
              // TODO Broken during the suspecttable -> sqlite refactor
              //NovaCommon.nova.sendSuspectList(distributeSuspect);
              break;
            case 'requestBenign':
              benignRequest = true;
              // TODO broken during the suspecttable -> sqlite refactor
              //NovaCommon.nova.sendSuspectList(distributeSuspect);
              break;
            case 'cancelRequestBenign':
              benignRequest = false;
              break;
            case 'updateConfiguration':
              for(var i in json_args)
              {
                if(i !== 'type' && i !== 'id')
                {
                  NovaCommon.config.WriteSetting(i, json_args[i]);
                }
              }
              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Configuration for ' + clientId + ' has been updated. Registering new config...';
              pulsar.sendUTF(JSON.stringify(response));
              configSend.file = fs.readFileSync(NovaHomePath + '/config/NOVAConfig.txt', 'utf8');
              pulsar.sendUTF(JSON.stringify(configSend));
              break;
            case 'haystackConfig':
              var executionString = 'haystackautoconfig';
              var nFlag = '-n';
              var rFlag = '-r';
              var iFlag = '-i';
              var eFlag = '-e';
            
              var hhconfigArgs = new Array();
            
              if(json_args.numNodesType == "number") 
              {
                if(json_args.numNodes !== undefined) 
                {
                  hhconfigArgs.push(nFlag);
                  hhconfigArgs.push(json_args.numNodes);
                }
              } 
              else if(json_args.numNodesType == "ratio") 
              {
                if(json_args.numNodes !== undefined) 
                {
                  hhconfigArgs.push(rFlag);
                  hhconfigArgs.push(json_args.numNodes);
                }
              }
              else if(json_args.numNodesType == "range")
              {
                if(json_args.numNodes !== undefined)
                {
                  hhconfigArgs.push(eFlag);
                  hhconfigArgs.push(json_args.numNodes);
                }
              }
              if(json_args.ethinterface !== undefined && json_args.ethinterface.length > 0)
              {
                hhconfigArgs.push(iFlag);
                hhconfigArgs.push(json_args.ethinterface);
              }
            
              var spawn = require('child_process').spawn;
           
              console.log("Running: " + executionString.toString());
              console.log("Args: " + hhconfigArgs);
              autoconfig = spawn(executionString.toString(), hhconfigArgs);
            
              autoconfig.stdout.on('data', function (data){
                console.log('' + data);
              });
            
              autoconfig.stderr.on('data', function (data){
                if (/^execvp\(\)/.test(data))
                {
                  console.log("haystackautoconfig failed to start.");
                  var message = "haystackautoconfig failed to start.";
                  var response = {};
                  response.id = clientId;
                  response.type = 'response';
                  response.response_message = message;
                  pulsar.sendUTF(JSON.stringify(response));
                }
              });
            
              autoconfig.on('exit', function (code){
                console.log("autoconfig exited with code " + code);
                var message = "autoconfig exited with code " + code;
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = message;
                pulsar.sendUTF(JSON.stringify(response));
              });

              var response = {};
              response.id = clientId;
              response.type = 'response';
              response.response_message = 'Haystack Autoconfiguration commencing';
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'suspectDetails':
              var suspectString = NovaCommon.nova.GetSuspectDetailsString(json_args.ip, json_args.iface);
              suspectString = suspectString.replace(/(\r\n|\r|\n)/gm, "<br>");
              var response = {};
              response.id = clientId;
              response.type = 'detailsReceived';
              response.data = suspectString;
              pulsar.sendUTF(JSON.stringify(response));
              break;
            case 'clearSuspects':
              everyone.now.ClearAllSuspects(function(){
                var response = {};
                response.id = clientId;
                response.type = 'response';
                response.response_message = 'Suspects cleared on ' + json_args.id;
                pulsar.sendUTF(JSON.stringify(response));
              });
              break;
            default:
              console.log('Unexpected/unknown message type ' + json_args.type + ' received, doing nothing');
              break;
          }
        }
      }
    });
    connection.on('close', function(){
      // If the connection gets closed, we want to try to reconnect; we will use
      // the stored IP of Pulsar to make the reconnect attempts
      pulsar = undefined;
      if(!reconnecting)
      {
        console.log('closed, beginning reconnect attempts every ', (reconnectTimer / 1000) ,' seconds');
        clearReconnect = setInterval(function(){console.log('attempting reconnect to wss://' + connected); client.connect('wss://' + connected, null);}, reconnectTimer);
        reconnecting = true;
      }
    });
    connection.on('error', function(error){
      console.log('Error: ' + error.toString());
    });
  });
  
  client.connect('wss://' + connected, null);
  
  setInterval(function()
  {
    if(pulsar != undefined && pulsar != null)
    {
      var message1 = {};
      message1.id = clientId;
      message1.type = 'statusChange';
      message1.component = 'nova';
      message1.status = NovaCommon.nova.IsNovadConnected(false).toString();
      var message2 = {};
      message2.id = clientId;
      message2.type = 'statusChange';
      message2.component = 'haystack';
      message2.status = NovaCommon.nova.IsHaystackUp().toString();
      pulsar.sendUTF(JSON.stringify(message1));
      pulsar.sendUTF(JSON.stringify(message2));
    }
  }, 5000);
  //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

app.get('/honeydConfigManage', function(req, res){
  var tab;
  if(req.query["tab"] === undefined)
  {
    tab = "settingsGroups";
  }
  else
  {
    tab = req.query["tab"];
  }

  var nodeNames = NovaCommon.honeydConfig.GetNodeMACs();
  var nodeList = [];
  
  for(var i = 0; i < nodeNames.length; i++)
  {
    var node = NovaCommon.honeydConfig.GetNode(nodeNames[i]);
    var push = NovaCommon.cNodeToJs(node);
    nodeList.push(push);
  }

  var interfaces = NovaCommon.config.ListInterfaces().sort();

  res.render('honeydConfigManage.jade', {
      configurations: NovaCommon.honeydConfig.GetConfigurationsList(),
      current: NovaCommon.config.GetCurrentConfig(),
      nodes: nodeList,
      INTERFACES: interfaces,
      interfaceAliases: ConvertInterfacesToAliases(interfaces),
      tab: tab
  });
});

app.get('/downloadNovadLog.log', function (req, res)
{
    res.download(novadLogPath, 'novadLog.log');
});

app.get('/downloadHoneydLog.log', function (req, res)
{
    res.download(honeydLogPath, 'honeydLog.log');
});

app.get('/nodeState.csv', function(req, res)
{
  var nodeNames = NovaCommon.honeydConfig.GetNodeMACs();
  var nodeList = [];

  var csvString = "ENABLED,IP,INTERFACE,MAC,PROFILE\n";
  
  for (var i = 0; i < nodeNames.length; i++)
  {
      var node = NovaCommon.honeydConfig.GetNode(nodeNames[i]);
      csvString += node.IsEnabled() + ",";
      csvString += node.GetIP() + ",";
      csvString += node.GetInterface() + ",";
      csvString += node.GetMAC() + ",";
      csvString += node.GetProfile();
      csvString += "\n";
  }

  res.header('Content-Type', 'text/csv');
  res.send(csvString);
});

app.get('/novaState.csv', function (req, res)
{
    exec('novacli get all csv > ' + NovaHomePath + "/state.csv",
    function(error, stdout, stderr)
    {
        if (error != null)
        {
            // Don't really care. Probably failed because novad was down.
            //console.log("exec error: " + error);
        }
        
        fs.readFile(NovaHomePath + "/state.csv", 'utf8', function(err, data)
        {
            res.header('Content-Type', 'text/csv');
            var reply = data.toString();
            res.send(reply);
        });
    });
});

app.get('/viewNovadLog', function (req, res)
{
    fs.readFile(novadLogPath, 'utf8', function (err, data)
    {
        if (err)
        {
            RenderError(res, "Unable to open NOVA log file for reading due to error: " + err);
            return;
        } else {
            fs.readFile(honeydLogPath, 'utf8', function (err, honeydData)
            {
                if (err)
                {
                    RenderError(res, "Unable to open HONEYD log file for reading due to error: " + err);
                    return;
                } else {
                    res.render('viewNovadLog.jade', {
                        log: data
                        , honeydLog: honeydData
                    });
                }
            });
        }
    });
});

app.get('/advancedOptions', function (req, res)
{
    var all = NovaCommon.config.ListInterfaces().sort();
    var used = NovaCommon.config.GetInterfaces().sort();

    var pass = [];

    for (var i in all)
    {
        var checked = false;

        for (var j in used)
        {
            if (all[i] === used[j])
            {
                checked = true;
                break;
            }
        }

        if (checked)
        {
            pass.push({
                iface: all[i],
                checked: 1
            });
        } else {
            pass.push({
                iface: all[i],
                checked: 0
            });
        }
    }

    res.render('advancedOptions.jade', {
        INTERFACES: NovaCommon.config.ListInterfaces().sort()
        , DEFAULT: NovaCommon.config.GetUseAllInterfacesBinding()
        , HS_HONEYD_CONFIG: NovaCommon.config.ReadSetting("HS_HONEYD_CONFIG")
        , READ_PCAP: NovaCommon.config.ReadSetting("READ_PCAP")
        , PCAP_FILE: NovaCommon.config.ReadSetting("PCAP_FILE")
        , CLASSIFICATION_TIMEOUT: NovaCommon.config.ReadSetting("CLASSIFICATION_TIMEOUT")
        , K: NovaCommon.config.ReadSetting("K")
        , EPS: NovaCommon.config.ReadSetting("EPS")
        , CLASSIFICATION_THRESHOLD: NovaCommon.config.ReadSetting("CLASSIFICATION_THRESHOLD")
        , DOPPELGANGER_IP: NovaCommon.config.ReadSetting("DOPPELGANGER_IP")
        , DOPPELGANGER_INTERFACE: NovaCommon.config.ReadSetting("DOPPELGANGER_INTERFACE")
        , DM_ENABLED: NovaCommon.config.ReadSetting("DM_ENABLED")
        , ENABLED_FEATURES: NovaCommon.config.ReadSetting("ENABLED_FEATURES")
        , FEATURE_NAMES: NovaCommon.nova.GetFeatureNames()
        , THINNING_DISTANCE: NovaCommon.config.ReadSetting("THINNING_DISTANCE")
        , SAVE_FREQUENCY: NovaCommon.config.ReadSetting("SAVE_FREQUENCY")
        , DATA_TTL: NovaCommon.config.ReadSetting("DATA_TTL")
        , CE_SAVE_FILE: NovaCommon.config.ReadSetting("CE_SAVE_FILE")
        , SMTP_ADDR: NovaCommon.config.ReadSetting("SMTP_ADDR")
        , SMTP_PORT: NovaCommon.config.ReadSetting("SMTP_PORT")
        , SMTP_DOMAIN: NovaCommon.config.ReadSetting("SMTP_DOMAIN")
        , SMTP_PASS: NovaCommon.config.ReadSetting("SMTP_PASS")
        , SERVICE_PREFERENCES: NovaCommon.config.ReadSetting("SERVICE_PREFERENCES")
        , CAPTURE_BUFFER_SIZE: NovaCommon.config.ReadSetting("CAPTURE_BUFFER_SIZE")
        , MIN_PACKET_THRESHOLD: NovaCommon.config.ReadSetting("MIN_PACKET_THRESHOLD")
        , CUSTOM_PCAP_FILTER: NovaCommon.config.ReadSetting("CUSTOM_PCAP_FILTER")
        , CUSTOM_PCAP_MODE: NovaCommon.config.ReadSetting("CUSTOM_PCAP_MODE")
        , MANAGE_IFACE_ENABLE: NovaCommon.config.ReadSetting("MANAGE_IFACE_ENABLE")
        , WEB_UI_PORT: NovaCommon.config.ReadSetting("WEB_UI_PORT")
        , WEB_UI_IFACE: NovaCommon.config.ReadSetting("WEB_UI_IFACE")
        , CLEAR_AFTER_HOSTILE_EVENT: NovaCommon.config.ReadSetting("CLEAR_AFTER_HOSTILE_EVENT")
        , MASTER_UI_IP: NovaCommon.config.ReadSetting("MASTER_UI_IP")
        , MASTER_UI_RECONNECT_TIME: NovaCommon.config.ReadSetting("MASTER_UI_RECONNECT_TIME")
        , MASTER_UI_CLIENT_ID: NovaCommon.config.ReadSetting("MASTER_UI_CLIENT_ID")
        , MASTER_UI_ENABLED: NovaCommon.config.ReadSetting("MASTER_UI_ENABLED") 
        , FEATURE_WEIGHTS: NovaCommon.config.ReadSetting("FEATURE_WEIGHTS")
        , CLASSIFICATION_ENGINE: NovaCommon.config.ReadSetting("CLASSIFICATION_ENGINE")
        , THRESHOLD_HOSTILE_TRIGGERS: NovaCommon.config.ReadSetting("THRESHOLD_HOSTILE_TRIGGERS")
        , ONLY_CLASSIFY_HONEYPOT_TRAFFIC: NovaCommon.config.ReadSetting("ONLY_CLASSIFY_HONEYPOT_TRAFFIC")
        , TRAINING_DATA_PATH: NovaCommon.config.ReadSetting("TRAINING_DATA_PATH")
        , MESSAGE_WORKER_THREADS: NovaCommon.config.ReadSetting("MESSAGE_WORKER_THREADS")
        , ADDITIONAL_HONEYD_ARGS: NovaCommon.config.ReadSetting("ADDITIONAL_HONEYD_ARGS")
    });
});

function renderBasicOptions(jadefile, res, req)
{
    var all = NovaCommon.config.ListInterfaces().sort();
    var used = NovaCommon.config.GetInterfaces().sort();

    var pass = [];

    for (var i in all)
    {
        var checked = false;

        for (var j in used)
        {
            if (all[i] === used[j])
            {
                checked = true;
                break;
            }
        }

        if (checked)
        {
            pass.push({
                iface: all[i],
                checked: 1
            });
        } else {
            pass.push({
                iface: all[i],
                checked: 0
            });
        }
    }

    var doppelPass = [];

    all = NovaCommon.config.ListLoopbacks().sort();
    used = NovaCommon.config.GetDoppelInterface();

    for (var i in all)
    {
        var checked = false;

        for (var j in used)
        {
            if (all[i] === used[j])
            {
                checked = true;
                break;
            } else if (used[j].length == 1 && used.length > 1)
            {
                if (all[i] === used)
                {
                    checked = true;
                    break;
                }
            }
        }

        if (checked)
        {
            doppelPass.push({
                iface: all[i],
                checked: 1
            });
        } else {
            doppelPass.push({
                iface: all[i],
                checked: 0
            });
        }
    }

    var ifaceForConversion = new Array();
    for (var i = 0; i < pass.length; i++)
    {
        ifaceForConversion.push(pass[i].iface);
    }
    
    res.render(jadefile, {
        INTERFACES: pass,
        INTERFACE_ALIASES: ConvertInterfacesToAliases(ifaceForConversion),
        DEFAULT: NovaCommon.config.GetUseAllInterfacesBinding(),
        DOPPELGANGER_IP: NovaCommon.config.ReadSetting("DOPPELGANGER_IP"),
        DOPPELGANGER_INTERFACE: doppelPass,
        DM_ENABLED: NovaCommon.config.ReadSetting("DM_ENABLED"),
        SMTP_ADDR: NovaCommon.config.ReadSetting("SMTP_ADDR"),
        SMTP_PORT: NovaCommon.config.ReadSetting("SMTP_PORT"),
        SMTP_DOMAIN: NovaCommon.config.ReadSetting("SMTP_DOMAIN"),
        SMTP_PASS: NovaCommon.config.ReadSetting("SMTP_PASS"),
        SMTP_INTERVAL: NovaCommon.config.ReadSetting("SMTP_INTERVAL"),
        SMTP_USEAUTH: NovaCommon.config.GetSMTPUseAuth().toString(),
        RSYSLOG_USE: NovaCommon.config.ReadSetting("RSYSLOG_USE"),
        RSYSLOG_IP: NovaCommon.config.ReadSetting("RSYSLOG_IP"),
        RSYSLOG_PORT: NovaCommon.config.ReadSetting("RSYSLOG_PORT"),
        RSYSLOG_CONNTYPE: NovaCommon.config.ReadSetting("RSYSLOG_CONNTYPE"),
        EMAIL_ALERTS_ENABLED: NovaCommon.config.ReadSetting("EMAIL_ALERTS_ENABLED"),
        SERVICE_PREFERENCES: NovaCommon.config.ReadSetting("SERVICE_PREFERENCES"),
        RECIPIENTS: NovaCommon.config.ReadSetting("RECIPIENTS")
    });
}

app.get('/error', function (req, res)
{
    RenderError(res, req.query["errorDetails"], req.query["redirectLink"]);
    return;
});

app.get('/basicOptions', function (req, res)
{
    renderBasicOptions('basicOptions.jade', res, req);
});

app.get('/configHoneydNodes', function (req, res)
{
  if (!NovaCommon.honeydConfig.LoadAllTemplates())
  {
    RenderError(res, "Unable to load honeyd configuration XML files");
    return;
  }

  var profiles = NovaCommon.honeydConfig.GetLeafProfileNames();
  var interfaces = NovaCommon.config.ListInterfaces().sort();
    
  res.render('configHoneydNodes.jade', {
      INTERFACES: interfaces,
      interfaceAliases: ConvertInterfacesToAliases(interfaces),
      profiles: profiles,
      currentGroup: NovaCommon.config.GetGroup()
  });
});

app.get('/help', function (req, res)
{
    res.render('help.jade');   
});

app.get('/getSuspectDetails', function (req, res)
{
  if(req.query['ip'] === undefined)
  {
    RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", '/');
    return;
  }
  
  if(req.query['interface'] === undefined)
  {
    RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", '/');
    return;
  }
  
  var suspectIp = req.query['ip'];
  var suspectInterface = req.query['interface'];
  
    res.render('suspectDetails.jade', {
        suspectIp: suspectIp
        , interface: suspectInterface
        , suspectInterface: suspectInterface
        , featureNames: NovaCommon.nova.GetFeatureNames()
        ,  suspect: suspectIp
    });
});

app.get('/editHoneydNode', function (req, res)
{
  if(req.query["node"] === undefined)
  {
    RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
    return;
  }
  
  var nodeName = req.query["node"];
  var node = NovaCommon.honeydConfig.GetNode(nodeName);

  if(node == null)
  {
    RenderError(res, "Unable to fetch node: " + nodeName, "/configHoneydNodes");
    return;
  }

  var interfaces;
  if(nodeName == "doppelganger")
  {
    interfaces = NovaCommon.config.ListLoopbacks().sort();
  }
  else
  {
    interfaces = NovaCommon.config.ListInterfaces().sort();
  }

  res.render('editHoneydNode.jade', {
    oldName: nodeName,
    INTERFACES: interfaces,
    interfaceAliases: ConvertInterfacesToAliases(interfaces),
    profiles: NovaCommon.honeydConfig.GetProfileNames(),
    profile: node.GetProfile(),
    interface: node.GetInterface(),
    ip: node.GetIP(),
    mac: node.GetMAC(),
    portSet: node.GetPortSet()
  })
});

app.get('/editHoneydProfile', function (req, res)
{
    if(req.query["profile"] === undefined)
    {
        RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
        return;
    }
    var profileName = req.query["profile"];

    res.render('editHoneydProfile.jade', {
      oldName: profileName,
      parentName: "",
      newProfile: false,
      vendors: NovaCommon.vendorToMacDb.GetVendorNames(),
      scripts: NovaCommon.honeydConfig.GetScriptNames(),
      bcastScripts: NovaCommon.honeydConfig.GetBroadcastScriptNames(),
      personalities: NovaCommon.osPersonalityDb.GetPersonalityOptions()
    })
});

app.get('/addHoneydProfile', function (req, res)
{
    if(req.query["parent"] === undefined)
    {
        RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.", "/configHoneydNodes");
        return;
    }
    parentName = req.query["parent"];

    res.render('editHoneydProfile.jade', {
      oldName: parentName,
      parentName: parentName,
      newProfile: true,
      vendors: NovaCommon.vendorToMacDb.GetVendorNames(),
      scripts: NovaCommon.honeydConfig.GetScriptNames(),
      bcastScripts: NovaCommon.honeydConfig.GetBroadcastScriptNames(),
      personalities: NovaCommon.osPersonalityDb.GetPersonalityOptions()
    })
});

app.get('/customizeTraining', function (req, res)
{
    NovaCommon.trainingDb = new NovaCommon.novaconfig.CustomizeTrainingBinding();
    
    NovaCommon.dbqGetLastTrainingDataSelection.all(function(err, results) {
        if(err) {LOG("ERROR", "Database error: " + err)};
        var includedLastTime = {};

        for (var i = 0; i < results.length; i++) {
            includedLastTime[results[i].uid] = results[i].included;
        }

        res.render('customizeTraining.jade', {
          includedLastTime: includedLastTime,
          desc: NovaCommon.trainingDb.GetDescriptions(),
          uids: NovaCommon.trainingDb.GetUIDs(),
          hostiles: NovaCommon.trainingDb.GetHostile()
        });
    });
});

app.get('/importCapture', function (req, res)
{
    if(req.query["trainingSession"] === undefined)
    {
        RenderError(res, "Invalid GET arguements. You most likely tried to refresh a page that you shouldn't.");
        return;
    }

    var trainingSession = req.query["trainingSession"];
    trainingSession = NovaHomePath + "/data/" + trainingSession + "/nova.dump";
    var ips = NovaCommon.trainingDb.GetCaptureIPs(trainingSession);

    if(ips === undefined)
    {
        RenderError(res, "Unable to read capture dump file");
        return;
    } else {
        res.render('importCapture.jade', {
          ips: NovaCommon.trainingDb.GetCaptureIPs(trainingSession),
          trainingSession: req.query["trainingSession"]
        })
    }
});

app.post('/importCaptureSave', function (req, res)
{
    var hostileSuspects = new Array();
    var includedSuspects = new Array();
    var descriptions = new Object();

    var trainingSession = req.query["trainingSession"];
    trainingSession = NovaHomePath + "/data/" + trainingSession + "/nova.dump";

    var trainingDump = new NovaCommon.novaconfig.TrainingDumpBinding();
    if(!trainingDump.LoadCaptureFile(trainingSession))
    {
        ReadSetting(res, "Unable to parse dump file: " + trainingSession);
    }

    trainingDump.SetAllIsIncluded(false);
    trainingDump.SetAllIsHostile(false);

    for (var id in req.body)
    {
        id = id.toString();
        var type = id.split('_')[0];
        var ip = id.split('_')[1];

        if(type == 'include')
        {
            includedSuspects.push(ip);
            trainingDump.SetIsIncluded(ip, true);
        } else if(type == 'hostile')
        {
            hostileSuspects.push(ip);
            trainingDump.SetIsHostile(ip, true);
        } else if(type == 'description')
        {
            descriptions[ip] = req.body[id];
            trainingDump.SetDescription(ip, req.body[id]);
        } else {
            console.log("ERROR: Got invalid POST values for importCaptureSave");
        }
    }

    if(!trainingDump.SaveToDb(NovaHomePath + "/config/training/training.db"))
    {
        RenderError(res, "Unable to save to training db");
        return;
    }

    res.render('saveRedirect.jade', {
      redirectLink: "/customizeTraining"
    })

});

app.get('/configWhitelist', function (req, res)
{
    var interfaces = NovaCommon.config.ListInterfaces().sort();
    res.render('configWhitelist.jade', {
      whitelistedIps: NovaCommon.whitelistConfig.GetIps(),
      whitelistedRanges: NovaCommon.whitelistConfig.GetIpRanges(),
      INTERFACES: interfaces,
      interfaceAliases: ConvertInterfacesToAliases(interfaces)
    })
});

app.get('/editUsers', function (req, res)
{
    var usernames = new Array();
    NovaCommon.dbqCredentialsGetUsers.all(

    function (err, results)
    {
        if(err)
        {
            RenderError(res, "Database Error: " + err);
            return;
        }

        var usernames = new Array();
        for (var i in results)
        {
            usernames.push(results[i].user);
        }
        res.render('editUsers.jade', {
          usernames: usernames
        });
    });
});

app.get('/configWhitelist', function (req, res)
{
    res.render('configWhitelist.jade', {
        whitelistedIps: NovaCommon.whitelistConfig.GetIps(),
        whitelistedRanges: NovaCommon.whitelistConfig.GetIpRanges()
    })
});

app.get('/suspects', function (req, res)
{
    var interfaces = NovaCommon.config.ListInterfaces().sort();
    res.render('main.jade', {
        user: req.user,
        featureNames: NovaCommon.nova.GetFeatureNames(),
        interfaces: interfaces,
        interfaceAliases: ConvertInterfacesToAliases(interfaces)
    });
});

app.get('/events', function (req, res)
{
    res.render('events.jade', {
        featureNames: NovaCommon.nova.GetFeatureNames()
    });
});

app.get('/', function (req, res)
{
    NovaCommon.dbqFirstrunCount.all(

    function (err, results)
    {
        if(err)
        {
            RenderError(res, "Database Error: " + err);
            return;
        }

        if(results[0].rows == 0)
        {
            res.redirect('/welcome');
        } else {
            res.redirect('/suspects');
        }

    });
});

app.get('/createNewUser', function (req, res)
{
    res.render('createNewUser.jade');
});

app.get('/welcome', function (req, res)
{
    res.render('welcome.jade');
});

app.get('/setup1', function (req, res)
{
    res.render('setup1.jade');
});

app.get('/setup2', function (req, res)
{
    renderBasicOptions('setup2.jade', res, req)
});

app.get('/setup3', function (req, res)
{
    res.render('hhautoconfig.jade', {
        user: req.user,
        INTERFACES: NovaCommon.config.ListInterfaces().sort(),
        interfaceAliases: ConvertInterfacesToAliases(interfaces),
        SCANERROR: ""
    });
});

app.get('/shutdown', function (req, res)
{
    res.render('shutdown.jade');
});

app.get('/about', function (req, res)
{
    res.render('about.jade', {version: NovaCommon.config.GetVersionString()});
});

app.get('/editAuthorizedIPs', function(req, res) {

    fs.readFile(NovaHomePath + "/config/authorizedIPs.config", function(err, data) {
        if (err) {
            RenderError(res, "Unable to open authorizedIPs.config", "/advancedOptions");
            return;
        }

        var array = data.toString().split("\n");
        var IPs = [];

        for (var i in array) {
            if (array[i].length < 7) {continue;}
            if (array[i][0] == '#') {continue;}
            IPs.push(array[i]);
        }

        res.render('authorizedIPs.jade', {authorizedIPs: JSON.stringify(IPs)});

    });

});

app.get('/editAuthorizedMACs', function(req, res) {

    fs.readFile(NovaHomePath + "/config/authorizedMACs.config", function(err, data) {
        if (err) {
            RenderError(res, "Unable to open authorizedMACs.config", "/advancedOptions");
            return;
        }

        var array = data.toString().split("\n");
        var MACs = [];

        for (var i in array) {
            if (array[i].length < 7) {continue;}
            if (array[i][0] == '#') {continue;}
            MACs.push(array[i]);
        }

        res.render('authorizedMACs.jade', {authorizedMACs: JSON.stringify(MACs)});
    });
});

app.get('/newInformation', function (req, res)
{
    res.render('newInformation.jade');
});

app.post('/createNewUser', function (req, res)
{
    var password = req.body["password"];
    var userName = req.body["username"];
    
    if(password.length == 0 || userName.length == 0) {
      RenderError(res, "Can not have blank username or password!", "/setup1");
      return;
    }

    NovaCommon.dbqCredentialsGetUser.all(userName,

    function selectCb(err, results, fields)
    {
        if(err)
        {
            RenderError(res, "Database Error: " + err, "/createNewUser");
            return;
        }

        if(results[0] == undefined)
        {
          var salt = '';
          var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
          for(var i = 0; i < 8; i++)
          {
            salt += possible[Math.floor(Math.random() * possible.length)];
          }
            NovaCommon.dbqCredentialsInsertUser.run(userName, NovaCommon.HashPassword(password, salt), salt, function ()
            {
                res.render('saveRedirect.jade', {
                        redirectLink: "/"
                });
            });
            return;
        } else {
            RenderError(res, "Username you entered already exists. Please choose another.", "/createNewUser");
            return;
        }
    });
});

app.post('/createInitialUser', function (req, res)
{
    var password = req.body["password"];
    var userName = req.body["username"];

    if(password.length == 0 || userName.length == 0) {
      RenderError(res, "Can not have blank username or password!", "/setup1");
      return;
    }

    NovaCommon.dbqCredentialsGetUser.all(userName,

    function selectCb(err, results)
    {
        if(err)
        {
            RenderError(res, "Database Error: " + err);
            return;
        }

        if(results[0] == undefined)
        {
          var salt = '';
      var possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
      for(var i = 0; i < 8; i++)
      {
        salt += possible[Math.floor(Math.random() * possible.length)];
      }
            NovaCommon.dbqCredentialsInsertUser.run(userName, NovaCommon.HashPassword(password, salt), salt);
            NovaCommon.dbqCredentialsDeleteUser.run('nova');
            res.render('saveRedirect.jade', {
                    redirectLink: "/setup2"
            });
            return;
        } else {
            RenderError(res, "Username already exists. Please choose another", "/setup1");
            return;
        }
    });
});

app.get('/autoConfig', function (req, res)
{
    var interfaces = NovaCommon.config.ListInterfaces().sort();
    res.render('hhautoconfig.jade', {
        user: req.user,
        INTERFACES: interfaces,
        interfaceAliases: ConvertInterfacesToAliases(interfaces),
        SCANERROR: "",
        GROUPS: NovaCommon.honeydConfig.GetConfigurationsList()
    });
});

app.get("/editTLSCerts", function (req, res)
{
    res.render('editTLSCerts.jade');    
});

app.get("/editClassifiers", function (req, res)
{
    res.render('editClassifiers.jade', {
            classifiers: NovaCommon.classifiers.getClassifiers()
    }); 
});

app.get("/generateNewCerts", function (req, res) {
    res.render("generateNewCerts.jade");
});

app.get("/editClassifier", function (req, res)
{
    var featureNames = NovaCommon.nova.GetFeatureNames();
    if(req.query['classifierIndex'] == undefined)
    {
        var classifier = {
            type: "THRESHOLD_TRIGGER"
            , mode: "HOSTILE_OVERRIDE"
            , config: "/config/newClassifier.config"
            , weight: "0"
            , strings: {}
            , index: '-1'
        };
        
        var features = [];
        for (var i = 0; i < featureNames.length; i++) {
          var feature = {
            name: featureNames[i]
            , enabled: true
            , weight: 1
            , threshold: "-"
          };
  
          features.push(feature);
        }

        classifier.features = features;
    } else {
        var index = req.query['classifierIndex'];
        var classifier = NovaCommon.classifiers.getClassifier(index);
     
        var enabledFeatures = String(classifier.strings["ENABLED_FEATURES"]);
        var weightString = classifier.strings["FEATURE_WEIGHTS"];
        var thresholdString = classifier.strings["THRESHOLD_HOSTILE_TRIGGERS"];
        var features = [];
        for (var i = 0; i < featureNames.length; i++)
        {
            var feature = {
                name: featureNames[i]
                , enabled: enabledFeatures.charAt(i) == '1'
                , weight: 1
                , threshold: "-"
            }
           
            if(classifier.type == "KNN")
            {
                feature.weight = weightString.split(" ")[i];     
            } 
            else if(classifier.type == "THRESHOLD_TRIGGER")
            {
                feature.threshold = thresholdString.split(" ")[i];
            }
          
            features.push(feature);
        }
        
        classifier.index = index;
    }
  
    classifier.features = features;


    res.render('editClassifier.jade', {
            classifier: classifier
            , featureNames: NovaCommon.nova.GetFeatureNames()
    }); 
});

app.get("/hostnames", function (req, res) {
    if (!NovaCommon.dbqGetHostnames) {
        RenderError(res, "Unable to access honeyd hostnames database. Something probably went wrong during the honeyd install.");
        return;
    }
    res.render('hostnames.jade', {});
});

app.get("/interfaceAliases", function (req, res)
{
    ReloadInterfaceAliasFile();
    res.render('interfaceAliases.jade', {
      interfaceAliases: interfaceAliases
      , INTERFACES: NovaCommon.config.ListInterfaces().sort(),
    });
});

app.post("/editAuthorizedIPs", function (req, res) {
    if (req.files["IPs"] == undefined || req.files["IPs"].size == 0) {
        RenderError(res, "Invalid form submission. This was likely caused by refreshing a page you shouldn't.", "/editAuthorizedIPs");
        return;
    }
    
    fs.rename(req.files["IPs"].path, NovaHomePath + "/config/authorizedIPs.config", function (err) {
        if (err) {
            RenderError(res, "Something went wrong when reading the uploaded file: " + err.toString(), "/editAuthorizedIPs");
            return;
        }

        res.redirect('/editAuthorizedIPs');
    });
});

app.post("/editAuthorizedMACs", function (req, res) {
    if (req.files["MACs"] == undefined || req.files["MACs"].size == 0) {
        RenderError(res, "Invalid form submission. This was likely caused by refreshing a page you shouldn't.", "/editAuthorizedMACs");
        return;
    }
    
    fs.rename(req.files["MACs"].path, NovaHomePath + "/config/authorizedMACs.config", function (err) {
        if (err) {
            RenderError(res, "Something went wrong when reading the uploaded file: " + err.toString(), "/editAuthorizedMACs");
            return;
        }

        res.redirect('/editAuthorizedMACs');
    });
});

app.post("/editTLSCerts", function (req, res)
{
    if(req.files["cert"] == undefined || req.files["key"] == undefined)
    {
        RenderError(res, "Invalid form submission. This was likely caused by refreshing a page you shouldn't.");
        return;
    }

    if(req.files["cert"].size == 0 || req.files["key"].size == 0)
    {
        RenderError(res, "You must choose both a key and certificate to upload");
        return;
    }

    fs.readFile(req.files["key"].path, function (readErrKey, data)
    {
        fs.writeFile(NovaHomePath + "/config/keys/quasarKey.pem", data, function(writeErrKey)
        {
            
            fs.readFile(req.files["cert"].path, function (readErrCert, certData)
            {
                fs.writeFile(NovaHomePath + "/config/keys/quasarCert.pem", certData, function(writeErrCert)
                {
                    if(readErrKey != null) {RenderError(res, "Error when reading key file"); return;}
                    if(readErrCert != null) {RenderError(res, "Error when reading cert file"); return;}
                    if(writeErrKey != null) {RenderError(res, "Error when writing key file"); return;}
                    if(writeErrCert != null) {RenderError(res, "Error when writing cert file"); return;}
                    
                    res.render('saveRedirect.jade', {
                        redirectLink: "/"
                    })
                });
            });
        });
    });
});

app.post('/honeydConfigManage', function (req, res){
  var newName = (req.body['newName'] != undefined ? req.body['newName'] : req.body['newNameClone']);
  var configToClone = (req.body['cloneSelect'] != undefined ? req.body['cloneSelect'] : '');
  var cloneBool = false;
  if(configToClone != '')
  {
    cloneBool = true;
  }
  
  if((new RegExp('^[a-zA-Z0-9 \\-_]+$')).test(newName))
  {
    NovaCommon.honeydConfig.AddConfiguration(newName, cloneBool, configToClone);
    NovaCommon.honeydConfig.SwitchConfiguration(newName);
    NovaCommon.config.SetCurrentConfig(newName);
    NovaCommon.honeydConfig.LoadAllTemplates();
  
    res.render('saveRedirect.jade', {
       redirectLink: '/honeydConfigManage'
    });
  } 
  else
  {
    RenderError(res, 'Unacceptable characters in new haystack name', 'honeydConfigManage');
  }
});

app.post('/customizeTrainingSave', function (req, res)
{
    var uids = NovaCommon.trainingDb.GetUIDs();

    NovaCommon.dbqClearLastTrainingDataSelection.run(function(err) {
        if(err) {LOG("ERROR", 'Database error: ' + err);}
     });

    for (var uid in uids) {
        if(req.body[uid] == undefined) {
            NovaCommon.dbqAddLastTrainingDataSelection.run(uid, 0, function(err) {
                if(err) {LOG("ERROR", 'Database error: ' + err);}
            });
        } else {
            NovaCommon.dbqAddLastTrainingDataSelection.run(uid, 1, function(err) {
                if(err) {LOG("ERROR", 'Database error: ' + err);}
            });
        }
    }

    for (var uid in req.body)
    {
        NovaCommon.trainingDb.SetIncluded(uid, true);
    }

    NovaCommon.trainingDb.Save();

    res.render('saveRedirect.jade', {
            redirectLink: "/customizeTraining"
    })
});



app.post('/configureNovaSave', function (req, res)
{

    // If we get two of the same form element, just use the last one
    for (var key in req.body) {
        if (typeof(req.body[key]) == "object") {
            req.body[key] = req.body[key][req.body[key].length - 1];
        }
    }

    // These are accepted configuration inputs and validation functions for them
    var configItems = [
    {
        key:  "ADVANCED"
        ,validator: function(val) {
            //TODO what is this? Rename it, I have no idea what "ADVANCED" is.
        }
    },
    {
        key:  "DEFAULT"
        ,validator: function(val) {
            //TODO what is this?
        }
    },
    {
        key:  "INTERFACES"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key:  "RSYSLOG_IP"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a valid IP address').isIP();
        }
    },
    {
        key:  "HS_HONEYD_CONFIG"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key:  "READ_PCAP"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "PCAP_FILE"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key:  "CLASSIFICATION_TIMEOUT"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer').isInt();
        }
    },
    {
        key:  "K"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer').isInt();
        }
    },
    {
        key:  "EPS"
        ,validator: function(val) {
            validator.check(val, 'EPS must be a positive floating point number').isFloat();
        }
    },
    {
        key:  "CLASSIFICATION_THRESHOLD"
        ,validator: function(val) {
            validator.check(val, 'Classification threshold must be a floating point value').isFloat();
            validator.check(val, 'Classification threshold must be a value between 0 and 1').max(1);
            validator.check(val, 'Classification threshold must be a value between 0 and 1').min(0);
        }
    },
    {
        key:  "DOPPELGANGER_IP"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a valid IP address').isIP();
        }
    },
    {
        key:  "DOPPELGANGER_INTERFACE"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key:  "DM_ENABLED"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "ENABLED_FEATURES"
        ,validator: function(val) {
            validator.check(val, 'Enabled Features mask must be ' + NovaCommon.nova.GetDIM() + 'characters long').len(NovaCommon.nova.GetDIM(), NovaCommon.nova.GetDIM());
            validator.check(val, 'Enabled Features mask must contain only 1s and 0s').regex('[0-1]{9}');
        }
    },
    {
        key:  "THINNING_DISTANCE"
        ,validator: function(val) {
            validator.check(val, 'Thinning Distance must be a positive number').isFloat();
        }
    },
    {
        key:  "SMTP_ADDR"
        ,validator: function(val) {
            validator.check(val, this.key + ' is the wrong format').regex('^(([A-z]|[0-9])*\\.)*(([A-z]|[0-9])*)\\@((([A-z]|[0-9])*)\\.)*(([A-z]|[0-9])*)\\.(([A-z]|[0-9])*)$');
        }
    },
    {
        key:  "SMTP_PORT"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer between 1 and 65535').isInt();
            validator.check(val, this.key + ' must be an integer between 1 and 65535').min(1);
            validator.check(val, this.key + ' must be an integer between 1 and 65535').max(65535);
        }
    },
    {
        key:  "SMTP_DOMAIN"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key : "SMTP_PASS",
        validator : function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key:  "SMTP_USEAUTH"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "RECIPIENTS"
        ,validator: function(val) {
            validator.check(val, "Must have at least one email address").notEmpty();
            if (val.length != 0) {
                var emails = val;
                emails = emails.split(",");

                for (var i = 0; i < emails.length; i++) {
                    validator.check(emails[i], "Invalid email address: " + emails[i]).isEmail();
                }
                
            }
        }
    },
    {
        key:  "SERVICE_PREFERENCES"
        ,validator: function(val) {
            validator.check(val, "Service Preferences string is formatted incorrectly").is('^0:[0-7](\\+|\\-)?;1:[0-7](\\+|\\-)?;$');
        }
    },
    {
        key:  "MIN_PACKET_THRESHOLD"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer').isInt();
        }
    },
    {
        key:  "CUSTOM_PCAP_FILTER"
        ,validator: function(val) {
        }
    },
    {
        key:  "CUSTOM_PCAP_MODE"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer').isInt();
        }
    },
    {
        key:  "WEB_UI_PORT"
        ,validator: function(val) {
            validator.check(val, 'Must be a nonnegative integer between 1 and 65535').isInt();
            validator.check(val, 'Must be a nonnegative integer between 1 and 65535').max(65535);
            validator.check(val, 'Must be a nonnegative integer between 1 and 65535').min(0);
        }
    },
    {
        key:  "CLEAR_AFTER_HOSTILE_EVENT"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "MASTER_UI_IP"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a valid IP address').isIP();
        }
    },
    {
        key:  "MASTER_UI_RECONNECT_TIME"
        ,validator: function(val) {
            validator.check(val, 'Pulsar server reconnect time must be a positive integer').isInt();
            validator.check(val, 'Pulsar server reconnect time must be a positive integer').min(0);
        }
    },
    {
        key:  "MASTER_UI_CLIENT_ID"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
            validator.check(val, this.key + ' must be an alphanumeric string').isAlphanumeric();
        }
    },
    {
        key:  "MASTER_UI_ENABLED"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "MANAGE_IFACE_ENABLE"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "CAPTURE_BUFFER_SIZE"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer').isInt();
            validator.check(val, this.key + ' must be a positive integer greater than 88').min(88);
        }
    },
    {
        key:  "ONLY_CLASSIFY_HONEYPOT_TRAFFIC"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "EMAIL_ALERTS_ENABLED"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be a boolean').isInt();
        }
    },
    {
        key:  "TRAINING_DATA_PATH"
        ,validator: function(val) {
            validator.check(val, this.key + ' must not be empty').notEmpty();
        }
    },
    {
        key:  "MESSAGE_WORKER_THREADS"
        ,validator: function(val) {
            validator.check(val, this.key + ' must be an integer').isInt();
            validator.check(val, this.key + ' must be a positive integer greater than 1').min(1);
        }
    },
    {
        key: "ADDITIONAL_HONEYD_ARGS"
        ,validator: function(val) {
        }
    }];

    Validator.prototype.error = function (msg)
    {
        this._errors.push(msg);
    }

    Validator.prototype.getErrors = function ()
    {
        return this._errors.join("<br>");
    }

    var validator = new Validator();

    NovaCommon.config.ClearInterfaces();

    if(req.body["SMTP_USEAUTH"] == '0')
    {
      NovaCommon.config.SetSMTPUseAuth("false");
    }
    else if(req.body["SMTP_USEAUTH"] == '1')
    {
      NovaCommon.config.SetSMTPUseAuth("true");
    }
    
    if(req.body["MASTER_UI_ENABLED"] != undefined && req.body["MASTER_UI_ENABLED"] == "1")
    {
      if(clientId != undefined && req.body["MASTER_UI_CLIENT_ID"] != clientId)
      {
        if(pulsar != undefined)
        {
          var renameMessage = {};
          renameMessage.type = 'renameRequest';
          renameMessage.id = clientId;
          renameMessage.newId = req.body["MASTER_UI_CLIENT_ID"];
          pulsar.sendUTF(JSON.stringify(renameMessage));
          clientId = req.body["MASTER_UI_CLIENT_ID"];
        }
      }
    }

    var interfaces = "";

    if(req.body["DOPPELGANGER_INTERFACE"] !== undefined) 
    {
        NovaCommon.config.SetDoppelInterface(req.body["DOPPELGANGER_INTERFACE"]);
    }

    if(req.body["INTERFACES"] !== undefined) 
    {
        spl = req.body["INTERFACES"].split(',');
        for(item in spl) 
        {
          interfaces += spl[item] + " ";
          NovaCommon.config.AddIface(spl[item]);
        }

        req.body["INTERFACE"] = interfaces;
    }
  
    var portChanged = false;
  
    for(var item = 0; item < configItems.length; item++) 
    {
        if(req.body[configItems[item].key] == undefined) 
        {
          continue;
        }

        configItems[item].validator(req.body[configItems[item].key]);
    }

    var useRsyslog = req.body["RSYSLOG_USE"];
  
    if(useRsyslog != undefined && useRsyslog == "1")
    {
      var spawn = require('sudo');
      var options = {
        cachePassword: true
        , prompt: 'You have requested to change the target server for Rsyslog. This requires superuser permissions.'
      };
    
      var writeIP = req.body['RSYSLOG_IP'];
      var writePort = req.body['RSYSLOG_PORT'];
      var writeConnType = req.body['RSYSLOG_CONNTYPE'];
    
      var execution = ['nova_rsyslog_helper', writeIP, writePort, writeConnType];
    
      var rsyslogHelper = spawn(execution, options);
  
      rsyslogHelper.on('exit', function(code){
        if(code.toString() != '0')
        {
          console.log('nova_rsyslog_helper could not complete update of rsyslog configuration, exited with code ' + code);
          RenderError(res, 'Could not update rsyslog configuration, update script returned ' + code, '/suspects');
        }
        else
        {
          console.log('nova_rsyslog_helper updated rsyslog configuration');
          NovaCommon.config.WriteSetting('RSYSLOG_USE', 'true');
          NovaCommon.config.WriteSetting('RSYSLOG_IP', writeIP);
          NovaCommon.config.WriteSetting('RSYSLOG_PORT', writePort);
          NovaCommon.config.WriteSetting('RSYSLOG_CONNTYPE', writeConnType);
        }
      });
    }
    else
    {
      if(req.body['ADVANCED'] == '0')
      {
        var spawn = require('sudo');
        var options = {
          cachePassword: true
          , prompt: 'You have requested to change the target server for Rsyslog. This requires superuser permissions.'
        };
        
        var execution = ['nova_rsyslog_helper','remove'];
        var rm = spawn(execution, options); 
        rm.on('exit', function(code){
          if(code.toString() !='0')
          {
            console.log('Could not remove 41-node.conf from /etc/rsyslog.d/!');
          }
          else
          {
            console.log('41-nova.conf has been removed from /etc/rsyslog.d/');
            NovaCommon.config.WriteSetting('RSYSLOG_USE', 'false');
          }
        });
      }
    }

    var errors = validator.getErrors();

    if(errors.length > 0)
    {
      var errorRedirect = "/suspects";
      if (req.body["ERROR_REDIRECT"] != undefined) {
        errorRedirect = req.body["ERROR_REDIRECT"];
      }
    
      RenderError(res, errors, errorRedirect);
    } 
    else 
    {
      NovaCommon.config.UseAllInterfaces(req.body["DEFAULT"]);
      if(req.body["DEFAULT"] == 'true')
      {
        NovaCommon.config.WriteSetting("INTERFACE", "default");
      }
      else
      {
        NovaCommon.config.WriteSetting("INTERFACE", req.body["INTERFACE"]);
      }

      if(req.body["SMTP_PASS"] !== undefined)
      {
        NovaCommon.config.WriteSetting("SMTP_PASS", req.body["SMTP_PASS"]);
      }

      //if no errors, send the validated form data to the WriteSetting method
      for(var item = 5; item < configItems.length; item++)
      {
        if(req.body[configItems[item].key] != undefined)
        {
          NovaCommon.config.WriteSetting(configItems[item].key, req.body[configItems[item].key]);
        }
      }

      NovaCommon.config.ReloadConfiguration();
      
      if(req.body["EMAIL_ALERTS_ENABLED"] == "0")
      {
        var spawn = require('child_process').spawn;
        var execution = 'cleannovasendmail';
        var rm = spawn(execution, []); 
        rm.on('exit', function(code){
          //console.log('code == ' + code);
        });
        if(maildaemon != '')
        {
          maildaemon.kill('SIGINT');
        }
        NovaCommon.config.WriteSetting("EMAIL_ALERTS_ENABLED", "0");
      }
      else
      {
        var interval = req.body['SMTP_INTERVAL'];

        if(interval != 'hourly' && interval != 'daily' && interval != 'weekly' && interval != 'monthly' && interval != undefined)
        {
          RenderError(res, 'SMTP_INTERVAL value was incorrect!', "/suspects");
        }
        else if(interval != undefined)
        {
          if(maildaemon == '')
          {
            var spawn = require('child_process').spawn;
            var args = [interval];
            cpspawn = spawn('placenovasendmail', args);
            cpspawn.on('close', function(code){
              if(code === 0)
              {
                var spawn = require('child_process').spawn;
                var execstring = 'novamaildaemon.pl';
                maildaemon = spawn(execstring.toString());
                maildaemon.on('close', function(code){
                  if(code !== 0)
                  {
                    console.log('novamaildaemon.pl died an unnatural death with code ' + code);
                  }
                }); 
              }
              else
              {
                RenderError(res, 'Could not start mail daemon, check /etc/cron.' + interval + ' for orphaned script novasendmail', '/suspects');
              }
            });
          }
          else
          {
              var spawn = require('child_process').spawn;
              var execution = 'cleannovasendmail';
              var rm = spawn(execution, []); 
              rm.on('exit', function(code){
                  //console.log('code == ' + code);
                  if(maildaemon != '')
                  {
                    maildaemon.kill('SIGINT');
                  }
                  var spawn = require('child_process').spawn;
                  var args = [interval];
                  cpspawn = spawn('placenovasendmail', args);
                  cpspawn.on('close', function(code){
                    if(code === 0)
                    {
                      var spawn = require('child_process').spawn;
                      var execstring = 'novamaildaemon.pl';
                      maildaemon = spawn(execstring.toString());
                      maildaemon.on('close', function(code){
                        if(code !== 0)
                        {
                          console.log('novamaildaemon.pl died an unnatural death with code ' + code);
                        }
                      }); 
                    }
                    else
                    {
                      RenderError(res, 'Could not start mail daemon, check /etc/cron.' + interval + ' for orphaned script novasendmail', '/suspects');
                    }
                  });
              });
          }
          NovaCommon.config.WriteSetting("EMAIL_ALERTS_ENABLED", "1");
        }
      }
    }

    NovaCommon.config.ReloadConfiguration();

    var route = "/suspects";
    if(req.body['route'] != undefined)
    {
      route = req.body['route'];
      if(route == 'manconfig')
      {
        route = 'honeydConfigManage';
      }
      else
      {
        route = 'autoConfig';
      }
    }

    res.render('saveRedirect.jade', {
        redirectLink: route
    });
});

app.get('/scripts', function(req, res){
  var namesAndPaths = [];
  
  var scriptNames = NovaCommon.honeydConfig.GetScriptNames();

  for (var i = 0; i < scriptNames.length; i++) {
    var script = NovaCommon.honeydConfig.GetScript(scriptNames[i]);
    namesAndPaths.push({script: script.GetName(), path: script.GetPath(), configurable: script.GetIsConfigurable()});
  }
  

  function cmp(a, b)
  {
    return a.script.localeCompare(b.script);
  }
  
  namesAndPaths.sort(cmp);
  var scriptBindings = NovaCommon.GetPorts(); 
  
  res.render('scripts.jade', {
      scripts: namesAndPaths,
      bindings: scriptBindings
  });
});

var SendHostileEventToPulsar  = function(suspect)
{
  suspect.client = clientId;
  suspect.type = 'hostileSuspect';
  if(pulsar != undefined)
  {
    pulsar.sendUTF(JSON.stringify(suspect));
  }
};
everyone.now.SendHostileEventToPulsar = SendHostileEventToPulsar;

var SendBenignSuspectToPulsar = function(suspect)
{
  suspect.client = clientId;
  suspect.type = 'benignSuspect';
  if(pulsar != undefined)
  {
    pulsar.sendUTF(JSON.stringify(suspect));
  }
};
everyone.now.SendBenignSuspectToPulsar = SendBenignSuspectToPulsar;

process.on('SIGINT', function ()
{
    NovaCommon.nova.Shutdown();
    process.exit();
});

process.on('exit', function ()
{
    if(maildaemon != '')
    {
      maildaemon.kill('SIGINT');
    }
    var spawn = require('sudo');
    var options = {
      cachePassword: true
      , prompt: 'You need permission to remove the Nova mail script from the cron directories.'
    };
    
    var execution = ['cleannovasendmail.sh'];
    var rm = spawn(execution, options); 
    rm.on('exit', function(code){
      //console.log('code == ' + code);
      LOG("ALERT", "Quasar is exiting cleanly.");
    });
});

function objCopy(src, dst) 
{
    for (var member in src) 
    {
        if(typeof src[member] == 'function') 
        {
            dst[member] = src[member]();
        }
        // Need to think about this
        //        else if( typeof src[member] == 'object' )
        //        {
        //            copyOver(src[member], dst[member]);
        //        }
        else 
        {
            dst[member] = src[member];
        }
    }
}

function switcher(err, user, success, done) 
{
    if(!success) 
    {
        return done(null, false, {
            message: 'Username/password combination is incorrect'
        });
    }
    return done(null, user);
}

function pad(num) 
{
  return ("0" + num.toString()).slice(-2);
}

function getRsyslogIp()
{
  try
  {
    var read = fs.readFileSync('/etc/rsyslog.d/41-nova.conf', 'utf8');
    read = read.replace(/\n/,' ');
  }
  catch(err)
  {
    return 'NULL'; 
  }
  
  var idx = parseInt(read.indexOf('@@')) + 2;
  var ret = '';
   
  if(idx != -1)
  {
    ret = read.substring(parseInt(idx), parseInt(read.indexOf(':programname', idx)));
  }  
  
  return ret;
}

function ReloadInterfaceAliasFile() 
{
    var aliasFileData = fs.readFileSync(NovaHomePath + "/config/interface_aliases.txt");
    interfaceAliases = JSON.parse(aliasFileData);
}

function ConvertInterfacesToAliases(interfaces) 
{
    var aliases = new Array();
    for (var i in interfaces) 
    {
        aliases.push(ConvertInterfaceToAlias(interfaces[i]));
    }
    return aliases;
}

function ConvertInterfaceToAlias(iface) 
{
    if(interfaceAliases[iface] !== undefined) 
    {
        return interfaceAliases[iface] + ' (' + iface + ')';
    } 
    else 
    {
        return iface;
    }
}

setInterval(function(){
    try 
    {
        NovaCommon.nova.CheckConnection();
        everyone.now.updateHaystackStatus(NovaCommon.nova.IsHaystackUp(true));
        everyone.now.updateNovadStatus(NovaCommon.nova.IsNovadConnected());
    } 
    catch(err) 
    {

    }
}, 1000);


everyone.now.AddInterfaceAlias = function(iface, alias, callback)
{
    if(alias != "") 
    {
        if(!(new RegExp('^[a-zA-Z0-9 \\-_]+$')).test(alias)) {
            callback("Error: invalid interface alias. Must contain only letters, numbers, spaces, and hyphens.");
            return;
        }
        interfaceAliases[iface] = sanitizeCheck(alias).entityEncode();
    } 
    else 
    {
        delete interfaceAliases[iface];
    }

    var fileString = JSON.stringify(interfaceAliases);
    fs.writeFile(NovaHomePath + "/config/interface_aliases.txt", fileString, callback);
};

