function checkSelectedClients(clientName)
{
  for(var i in selectedClients)
  {
    if(selectedClients[i] == clientName)
    {
      return true;
    }
  }
  return false;
}
    
function setTarget(source, target, group)
{
  if(group != undefined)
  {
    if(document.getElementById(source).checked)
    {
      message.id = target + ':';
      var targets = target.split(':');
      
      while(document.getElementById('autoconfElements').hasChildNodes())
      {
        document.getElementById('autoconfElements').removeChild(document.getElementById('autoconfElements').lastChild);
      }
      
      for(var i in targets)
      {
        if(targets != undefined && targets != '')
        {
          createAutoconfigElement(targets[i]);
        }
      }
      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + group) != undefined && document.getElementById('groupcheck' + group).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + group).setAttribute('disabled', true);
        }
      }
      for(var i in clients)
      {
        if(clients[i] != '' && clients[i] != undefined && document.getElementById('client' + clients[i]) != undefined)
        {
          document.getElementById('client' + clients[i]).checked = false;
          document.getElementById('client' + clients[i]).disabled = true;
        } 
      }
    }
    else
    {
      message.id = '';
      while(document.getElementById('autoconfElements').hasChildNodes())
      {
        document.getElementById('autoconfElements').removeChild(document.getElementById('autoconfElements').lastChild);
      }
      for(var i in document.getElementById('groupsList').childNodes)
      {
        if(document.getElementById('groupcheck' + group) != undefined && document.getElementById('groupcheck' + group).id.indexOf(source) == -1)
        {
          document.getElementById('groupcheck' + group).removeAttribute('disabled');
        }
      }
      for(var i in clients)
      {
        if(clients[i] != '' && clients[i] != undefined && document.getElementById('client' + clients[i]) != undefined)
        {
          document.getElementById('client' + clients[i]).disabled = false;
        } 
      }
    }
  }
  else
  {
    if(document.getElementById(source).checked)
    {
      message.id += target + ':';
      var targets = target.split(':');
      for(var i in targets)
      {
        if(targets[i] != undefined && targets[i] != '')
        {
          createAutoconfigElement(targets[i]);
        }
      }
    }
    else
    {
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      
      var targets = target.split(':');
      for(var i in targets)
      {
        if(targets[i] != undefined && targets[i] != '')
        {
          removeAutoconfigElement(targets[i]);
        }
      }
    }
  }
}

function changeLabel(type, index)
{
  if(type == 'ratio')
  {
    document.getElementById('label' + index).innerHTML = ': Create ';
    document.getElementById('change' + index).innerHTML = ' times the amount of real nodes found on ';
    document.getElementById('input' + index).type = 'number';
    document.getElementById('input' + index).min = '0';
    document.getElementById('input' + index).max = '1024';
    document.getElementById('input' + index).value = '0';
  }
  else if(type == 'number')
  {
    document.getElementById('label' + index).innerHTML = ': Create ';
    document.getElementById('change' + index).innerHTML = ' nodes on '; 
    document.getElementById('input' + index).type = 'number';
    document.getElementById('input' + index).min = '0';
    document.getElementById('input' + index).max = '1024';
    document.getElementById('input' + index).value = '0';
  }
  else if(type == 'range')
  {
    document.getElementById('label' + index).innerHTML = ': Create nodes on IP ';
    document.getElementById('change' + index).innerHTML = ' on interface '; 
    document.getElementById('input' + index).type = 'text';
    document.getElementById('input' + index).value = '';
    document.getElementById('input' + index).placeholder = '##.##.##.##-##.##.##.##';
  }
}

function createAutoconfigElement(clientName, group)
{
  selectedClients.push(clientName);
  var tr = document.createElement('tr');
  tr.id = 'autohook' + clientName;
  tr.setAttribute('style', 'background: #E8A02F;');
  var td0 = document.createElement('td');
  var labelBold = document.createElement('label');
  labelBold.innerHTML = clientName;
  labelBold.value = clientName;
  labelBold.setAttribute('style', 'font-weight: bold;');
  var label = document.createElement('label');
  label.innerHTML = ': Create ';
  label.id = 'label' + clientName;
  var inputType = document.createElement('select');
  inputType.id = 'type' + clientName;
  var types = ['number', 'ratio', 'range'];
  
  for(var i = 0; i < types.length; i++)
  {
    var option = document.createElement('option');
    option.value = types[i];
    option.innerHTML = types[i];
    inputType.appendChild(option);
  }
  inputType.setAttribute('onclick', 'changeLabel(document.getElementById("type' + clientName + '").value, "' + clientName + '")');
  
  var input = document.createElement('input');
  input.id = 'input' + clientName;
  input.type = 'number';
  input.min = '0';
  //This needs to be found dynamically
  input.max = '1024';
  input.value = '0';
  //dropDown needs to be a dojo select containing a list of the interfaces on the 
  //host with clientId clientName
  var label1 = document.createElement('label');
  label1.innerHTML = ' nodes on ';
  label1.id = 'change' + clientName;
  var dropDown = document.createElement('select');
  
  now.GetInterfacesOfClient(clientName, function(interfaces){
    for(var i in interfaces)
    {
      var option = document.createElement('option');
      option.value = interfaces[i];
      option.innerHTML = interfaces[i];
      dropDown.appendChild(option);
    }
  });
  
  td0.appendChild(labelBold);
  td0.appendChild(label);
  td0.appendChild(inputType);
  td0.appendChild(input);
  td0.appendChild(label1);
  td0.appendChild(dropDown);
  tr.appendChild(td0);
  
  if(group != '')
  {
    document.getElementById('autoconfElements').appendChild(tr);
  }
  else
  {
    document.getElementById('autoconfElements').appendChild(tr);
  }
  document.getElementById('sendAutoconfig').removeAttribute('disabled');
  document.getElementById('changeStyle').setAttribute('style', 'border: 2px solid; border-collapse: collapse;')
}

function removeAutoconfigElement(target)
{
  var loopIter = document.getElementById('autoconfElements');
  do 
  {
    if(document.getElementById('autohook' + target) != undefined)
    {
      loopIter.removeChild(document.getElementById('autohook' + target));
      var regex = new RegExp(target + ':', 'i');
      message.id = message.id.replace(regex, '');
      break;
    }
  }while(loopIter.hasChildNodes());
  
  for(var j in selectedClients)
  {
    if(selectedClients[j] == target)
    {
      delete selectedClients[j];
    }
  }
}

function isIp(ip)
{
  var d = ip.split('.'), i = d.length;
  if (i != 4)
  {
    return false;
  }
  var ok = true;
  while (i-- && ok) 
  {
    ok = false;
    if (d[i].length != 0) if (parseInt(d[i])) if (d[i] > -1 && d[i] < 256) ok = true;
  }
  return ok;
}

function isIpRange(source)
{
  if(document.getElementById(source).value == '')
  {
    return false;
  }
  var range = document.getElementById(source).value.strip().split('-');
  if(isIp(range[0]) && isIp(range[1]))
  {
    return true;
  }
  else
  {
    return false;
  }
}

function processAutoconfigElements()
{
  var loopIter = document.getElementById('autoconfElements');
  while(loopIter.hasChildNodes()) 
  {
    if(loopIter.lastChild != undefined)
    {
      var simple = loopIter.lastChild.childNodes[0];
      var autoconfMessage = {};
      autoconfMessage.type = 'haystackConfig';
      autoconfMessage.id = simple.childNodes[0].value;
      var s = simple.childNodes[2];
      autoconfMessage.numNodesType = s.options[s.selectedIndex].value;
      autoconfMessage.numNodes = simple.childNodes[3].value;
      if(/^[0-9]+$/i.test(autoconfMessage.numNodes.toString()) == false 
         && (autoconfMessage.numNodesType == 'number' || autoconfMessage.numNodesType == 'ratio')
         && parseInt(autoconfMessage.numNodes) <= 0)
      {
        simple.childNodes[3].value = '0';
        alert('Number of nodes/ratio of nodes to create must be a digit');
        return;
      }
      else if(autoconfMessage.numNodesType == 'range' 
              && !isIpRange('input' + autoconfMessage.id))
      {
        simple.childNodes[3].value = '';
        alert('Ip range formatted incorrectly');
        return;
      }
      autoconfMessage.ethinterface = simple.childNodes[5].value;
      now.MessageSend(autoconfMessage);
    }
    loopIter.removeChild(loopIter.lastChild);
  }
  var uncheck = document.getElementById('clientsList');
  var count = 0;
  for(var i in uncheck.children)
  {
    if(document.getElementById('check' + count) == undefined)
    {
      break;
    }
    document.getElementById('check' + count).checked = false;
    count++;
  }
  uncheck = document.getElementById('groupsList');
  count = 0;
  for(var i in uncheck.children)
  {
    if(document.getElementById('groupcheck' + count) == undefined)
    {
      break;
    }
    document.getElementById('groupcheck' + count).checked = false;
    count++;
  }
}