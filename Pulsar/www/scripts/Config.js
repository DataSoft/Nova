function updateConfiguration()
{
  if(message.id == '')
  {
    alert('Attemping to submit to null target string, doing nothing');
    return;
  }
  
  now.UpdateConfiguration();
}
 
function manualConfigure()
{
  var clientContainer = document.getElementById('selectClient');
  var client = clientContainer.options[clientContainer.selectedIndex].value;
  var clientHost = now.GetClientHost(client, function(host){
    window.open('https://' + host, client);
  });
}
 
function checkInt(source)
{
  var check = parseFloat(document.getElementById(source).value);
  if(check < 0 || isNaN(check))
  {
    document.getElementById(source).value = '0';
  }
  var cond = check - parseInt(check);
  if(cond != 0)
  {
    document.getElementById(source).value = (parseInt(check) != 0 ? parseInt(check) : '0');
  }
}
 
now.UpdateConfiguration = function()
{
  //construct json here
  //message type: updateConfiguration
  message.type = 'updateConfiguration';
  
  message.CLASSIFICATION_TIMEOUT = parseInt(document.getElementsByName('CLASSIFICATION_TIMEOUT')[0].value);
  message.K = parseInt(document.getElementsByName('K')[0].value);
  message.EPS = parseFloat(document.getElementsByName('EPS')[0].value);
  message.CLASSIFICATION_THRESHOLD = parseFloat(document.getElementsByName('CLASSIFICATION_THRESHOLD')[0].value);
  if(message.CLASSIFICATION_THRESHOLD < 0 || message.CLASSIFICATION_THRESHOLD > 1)
  {
    alert('Invalid value for CLASSIFICATION THRESHOLD! Must be between 0 and 1.');
    return;
  }
  message.MIN_PACKET_THRESHOLD = parseInt(document.getElementsByName('MIN_PACKET_THRESHOLD')[0].value);
  
  if(document.getElementById('clearHostileYes').checked)
  {
    message.CLEAR_AFTER_HOSTILE_EVENT = '1';  
  }
  else
  {
    message.CLEAR_AFTER_HOSTILE_EVENT = '0';
  }
  
  message.CUSTOM_PCAP_FILTER = document.getElementsByName('CUSTOM_PCAP_FILTER')[0].value;
  
  if(document.getElementById('customPcapYes').checked)
  {
    message.CUSTOM_PCAP_MODE = '1';
  }
  else
  {
    message.CUSTOM_PCAP_MODE = '0';
  }
  
  message.CAPTURE_BUFFER_SIZE = parseInt(document.getElementsByName('CAPTURE_BUFFER_SIZE')[0].value);
  message.SAVE_FREQUENCY = parseInt(document.getElementsByName('SAVE_FREQUENCY')[0].value);
  message.DATA_TTL = parseInt(document.getElementsByName('DATA_TTL')[0].value);
  
  if(document.getElementById('dmEnabledYes').checked)
  {
    message.DM_ENABLED = '1';
  }
  else
  {
    message.DM_ENABLED = '0';
  }
  
  message.SERVICE_PREFERENCES = document.getElementsByName('SERVICE_PREFERENCES')[0].value;
  
  if((/^0:[0-7](\+|\-)?;1:[0-7](\+|\-)?;$/).test(message.SERVICE_PREFERENCES) == false)
  {
    document.getElementById('SERVICE_PREFERENCES').value = replace;
    alert('Service Preferences string is not formatted correctly.');
    return;
  }
  
  now.UpdateBaseConfig(message);
  now.MessageSend(message);
  location.reload(true);
}