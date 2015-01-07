var novaconfig = require('novaconfig.node');

var nova = new novaconfig.Instance();
var config = new novaconfig.NovaConfigBinding();

var honeydConfig = new novaconfig.HoneydConfigBinding();
honeydConfig.LoadAllTemplates();

var vendorToMacDb = new novaconfig.VendorMacDbBinding();
var osPersonalityDb = new novaconfig.OsPersonalityDbBinding();
var trainingDb = new novaconfig.CustomizeTrainingBinding();
var whitelistConfig = new novaconfig.WhitelistConfigurationBinding();

var assert = require("assert");

describe('honeydConfig', function(){
	describe('#GetProfileNames()', function(){
		it('should return default configuration profile names', function(){
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("default"));
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("LinuxServer"));
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("WinServer"));
			assert.notEqual(-1, honeydConfig.GetProfileNames().indexOf("BSDServer"));

			assert.equal(-1, honeydConfig.GetProfileNames().indexOf("non-existant fake profile name"));
		});
	});

	describe('#GetScript()', function(){
		it('should return the Linux httpd script', function(){
			var script = honeydConfig.GetScript("Linux httpd");
			assert.equal("Linux httpd", script.GetName());
			assert.equal("http", script.GetService());
			assert.equal("Linux | Linux", script.GetOsClass());
			assert.equal("tclsh /usr/share/honeyd/scripts/linux/httpd/httpd.tcl $ipsrc $sport $ipdst $dport", script.GetPath());
			assert(script.GetIsConfigurable());
		
			var options = script.GetOptions();
			assert(options["HTTPD_RESPONSE_FOLDER"] !== undefined);
			assert(options["HTTPD_SERVER_VERSION"] !== undefined);
			assert(options["HTTPD_SERVER_VERSION"].indexOf("Apache") !== -1);
		
		});
	});

});


var classifiersConstructor = new require('.././classifiers.js');
var classifiers = new classifiersConstructor(config);


describe('classifiers', function(){

	// Our initial configuration
	var initial = [{
		type: "KNN"
		, mode: "WEIGHTED"
		, weight: "1"
		, strings: {
			ENABLED_FEATURES: "11111111111111"
			, FEATURE_WEIGHTS: "1 1 1 1 1 1 1 1 1 1 1 1 1 1"
			, DATAFILE: "config/training/data.txt"
		  }
	}, {
		type: "THRESHOLD_TRIGGER"
		, mode: "HOSTILE_OVERRIDE"
		, weight: "0"
		, strings: {
		  	THRESHOLD_HOSTILE_TRIGGERS: "- - - - - >100 - - - - - - - -"
		  }
		}
	];

	// Resets to a known config state
	var resetClassifierConfig = function() {
		classifiers.setClassifiers(initial);
	};

	describe('#getClassifiers()', function(){
		it('should return default configuration of the classifiers', function(){
			resetClassifierConfig();

			var classifierObjects = classifiers.getClassifiers();
			assert.deepEqual(classifierObjects[0], initial[0]);
			assert.deepEqual(classifierObjects[1], initial[1]);
		});
	});
	
	describe('#getClassifier(index)', function(){
		it('should return default configuration of the classifier', function(){
			resetClassifierConfig();
		
			assert.equal(classifiers.getClassifier(9), undefined);
			assert.deepEqual(classifiers.getClassifier(0), initial[0]);
			assert.deepEqual(classifiers.getClassifier(1), initial[1]);
		});
	});


	describe('#saveClassifier()', function() {
		it('should add a classifier to the end of the list', function() {
			resetClassifierConfig();

			var c = {
				type: "KNN"
				, mode: "HOSTILE_OVERRIDE"
				, weight: "42"
				, strings: {FOO: "BAR"}
			};

			classifiers.saveClassifier(c, '-1');
			
			var classifierObjects = classifiers.getClassifiers();

			assert.deepEqual(classifierObjects[2], c);
		});
	});

	describe('#deleteClassifier()', function() {
		it('should delete the classifier at the index you specify', function() {
			resetClassifierConfig();
		
			classifiers.deleteClassifier(0);
			
			var classifierObjects = classifiers.getClassifiers();
			
			assert.deepEqual(classifierObjects[0], initial[1]);
		});
	});

	describe('Check empty behavior', function() {
		it('should return empty list if no classifiers present', function() {
			resetClassifierConfig();

			classifiers.deleteClassifier(0);
			classifiers.deleteClassifier(0);

			var classifierObjects = classifiers.getClassifiers();
			assert.equal(0, classifierObjects.length);
		});
	});


})
