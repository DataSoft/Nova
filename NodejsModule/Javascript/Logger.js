
var novaconfig = require('novaconfig.node');
var NovaLogger = new novaconfig.LoggerBinding();

function getErrorObject() {
    try { throw Error('') } catch(err) { return err; }
}

exports.LOG = function(level, basicString, advancedString) {
	var err = getErrorObject();
	var caller_line = err.stack.split("\n")[4];
	var index = caller_line.indexOf("at ");
	var clean = caller_line.slice(index+2, caller_line.length);
	
	var file = clean.split(":")[0] + ")";
	var line = clean.split(":")[1];

	if (advancedString == undefined || advancedString == null) {
		advancedString = "";
	}

	NovaLogger.Log(level, basicString, advancedString, file, Number(line));
}
