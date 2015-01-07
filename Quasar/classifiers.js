var fs = require('fs');

// Constructor should be passed the honeyd config binding object
var Classifiers = function(config) {
    this.config = config
}

Classifiers.prototype = {
  saveClassifier: function(c, index) {
  	var classifiers = this.getClassifiers();
    if (index < classifiers.length && index >= 0) {
        // If this is editin an existing index
        classifiers[index] = c;
    } else {
        // Otherwise it's a new classifier
        classifiers.push(c);
    }
	this.setClassifiers(classifiers);
  },

  deleteClassifier: function(index) {
  	var classifiers = this.getClassifiers();
	classifiers.splice(index, 1);
	this.setClassifiers(classifiers);
  },

  setClassifiers: function(classifierObjects) {
  	var engines = "";
	var engineConfigs = "";
	var engineModes = "";
	var engineWeights = "";

	for (var i = 0; i < classifierObjects.length; i++) {
		var config = '/config/CE_' + i + '.config';
		if (i != 0) {
			engines += ';';
			engineConfigs += ';';
			engineModes += ';';
			engineWeights += ';';
		}

		engines += classifierObjects[i].type;
		engineConfigs += config;
		engineModes += classifierObjects[i].mode;
		engineWeights += classifierObjects[i].weight;
	
		var stringsData = "";

		for (var key in classifierObjects[i].strings) {
			stringsData += key + " " + classifierObjects[i].strings[key] + "\n";
		}

		try {
			fs.writeFileSync(this.config.GetPathHome() + '/' + config, stringsData, 'utf8');
		} catch (err) {
			console.log("ERROR WRITING: " + err);
		}

	}
	
	this.config.WriteSetting("CLASSIFICATION_ENGINES", engines);
	this.config.WriteSetting("CLASSIFICATION_CONFIGS", engineConfigs);
	this.config.WriteSetting("CLASSIFICATION_MODES", engineModes);
	this.config.WriteSetting("CLASSIFICATION_WEIGHTS", engineWeights);
  	
  },

  getClassifiers: function() {
  	// Split out the classifiers into objects
  	var engines = this.config.ReadSetting("CLASSIFICATION_ENGINES").split(';');
  	var engineConfigs = this.config.ReadSetting("CLASSIFICATION_CONFIGS").split(';');
  	var engineModes = this.config.ReadSetting("CLASSIFICATION_MODES").split(';');
  	var engineWeights = this.config.ReadSetting("CLASSIFICATION_WEIGHTS").split(';');

  
  	var engineObjects = [];

	// Split returns an array with an empty string if empty string is passed in (wtf Javascript)
	if (engines.length === 1 && engines[0] === "") {
		return engineObjects;
	}
  
  	for (var i = 0; i < engines.length; i++) {
  		var obj = {};

		obj.type = engines[i];
		var configInstance = engineConfigs[i];
		obj.mode = engineModes[i];
		obj.weight = engineWeights[i];
		obj.strings = {};

		try {
			var stringsData = fs.readFileSync(this.config.GetPathHome() + '/' + configInstance, 'utf8').split("\n");
		} catch (err) {
			console.log("ERROR: " + err);
		}

		for (var stringLine in stringsData) {
			var line = stringsData[stringLine];
			if (line == "") continue;
			if (line[0] == "#") continue;

			obj.strings[line.substring(0, line.indexOf(" "))] = line.substring(line.indexOf(" ") + 1);
		}


  
  		engineObjects.push(obj);
  	}
	
	return engineObjects;
  },


	getClassifier: function(index) {
		var classifiers = this.getClassifiers();
		if (classifiers.length > index) {
			return classifiers[index];
		} else {
			return;
		}
	}
};

module.exports = Classifiers;


