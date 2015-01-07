function clearSuspects()       
{
  var check = message.id.split(':');
  message.type = 'clearSuspects';
  now.MessageSend(message);
  for(var i in suspectGrid.m_elements)
  {
    for(var j in check)
    {
      if(suspectGrid.m_elements[i][1].indexOf(check[j]) != -1 && check[j] != '')
      {
        suspectGrid.DeleteRow(i);
        break;
      } 
    }
  }
}

function getDetails(suspect)
{
  disableBackground(theDoc.getElementById('setup'));
  var send = {};
  var splitString = suspect.split('@');
  var ip = splitString[0];
  splitString = splitString[1].split('_');
  var clientId = splitString[0];
  var iface = splitString[1];
  
  send.ip = ip;
  send.clientId = clientId;
  send.interface = iface;
  
  now.GetSuspectDetails(send);
  
  splitString = null;
  
  theDoc.getElementById('lightbox').style.display = 'block';
  theDoc.getElementById('setup').style.opacity = '0.5';
  var opts = {
      lines: 17,
      length: 0,
      width: 4,
      radius: 27,
      corners: 0.6,
      rotate: 0,
      color: '#E8A02F',
      speed: 1,
      trail: 34,
      shadow: false,
      hwaccel: false,
      className: 'spinner',
      zIndex: 1003,
      top: 20,
      left: 'auto'
  };
  
  spinner = new Spinner(opts).spin(theDoc.getElementById('spinnerDiv'));
  theDoc.getElementById('spinnerDiv').style.height = ((opts.radius * 2) + 30)+ 'px';
  theDoc.getElementById('details').style.height = '95%';
}

function closeLightbox()
{
  theDoc.getElementById('lightbox').style.display = 'none';
  theDoc.getElementById('setup').style.opacity = '1'; 
  while(theDoc.getElementById('details').hasChildNodes())
  {
    theDoc.getElementById('details').removeChild(theDoc.getElementById('details').lastChild); 
  }
  enableBackground(theDoc.getElementById('setup'));
}

function disableBackground(source)
{
  var disableUs = source.childNodes;
  for(var i in disableUs)
  {
    if(typeof disableUs[i] == 'object' && disableUs[i].tagName != undefined)
    {
      disableUs[i].disabled = true;
      if(disableUs[i].hasChildNodes())
      {
        disableBackground(disableUs[i]);
      }
    }
  }
}

function enableBackground(source)
{
  var enableUs = source.childNodes;
  for(var i in enableUs)
  {
    if(typeof enableUs[i] == 'object' && enableUs[i].tagName != undefined)
    {
      enableUs[i].disabled = false;
      if(enableUs[i].hasChildNodes())
      {
        enableBackground(enableUs[i]);
      }
    }
  }
}

now.RenderSuspectDetails = function(detailString)
{
  if(theDoc.getElementById('lightbox').style.display == 'block')
  {
    spinner.stop();
    spinner = null;
    
    theDoc.getElementById('spinnerDiv').style.height = '0px';
    
    var list = detailString.split('\\n');
    
    if(list[0].indexOf('Unable') != -1)
    {
      var sorry = theDoc.createElement('p');
      sorry.innerHTML = 'Unable to complete request, please check that Novad';
      var sorry1 = theDoc.createElement('p');
      sorry1.innerHTML = ' is running on the client the suspect was requested from';
      theDoc.getElementById('details').appendChild(sorry);
      theDoc.getElementById('details').appendChild(sorry1);
      return; 
    }
    
    for(var i in list)
    {
      if(list[i] != '')
      {
        var p = theDoc.createElement('p');
        p.innerHTML = list[i];
        theDoc.getElementById('details').appendChild(p);
      }  
    }
  }
  else
  {
    console.log('did not hit block check');
    return; 
  }
};

function startNovad()
{
    message.type = 'startNovad';
    now.MessageSend(message);
}

function stopNovad()
{
    message.type = 'stopNovad';
    now.MessageSend(message);
}

function startHaystack()
{
    message.type = 'startHaystack';
    now.MessageSend(message);
}

function stopHaystack()
{
    message.type = 'stopHaystack';
    now.MessageSend(message);
}

function requestBenign()
{
  theDoc.getElementById('benignList').value = message.id;
  var iter = message.id.split(':');
  
  now.GetClientBenignRequest(function(list){
    for(var i in iter)
    {
      if(list.join().indexOf(iter[i]) == -1)
      {
        now.AddClientBenignRequest(iter[i]);
      }
    }
  });
  
  message.type = 'requestBenign';
  now.MessageSend(message);
}

function stopRequestBenign()
{
  message.type = 'cancelRequestBenign';
  now.MessageSend(message);
  var assign = message.id;
  var iter = message.id.split(':');
  
  now.GetClientBenignRequest(function(list){
    for(var i in iter)
    {
      if(list.join().indexOf(iter[i]) != -1)
      {
        now.RemoveClientBenignRequest(iter[i]);
      }
    }
  });
  
  var remove = message.id.split(':');
  for(var i in remove)
  {
    if((remove[i] != '' || remove[i] != undefined) && assign.indexOf(remove[i]) != -1)
    {
      var regex = new RegExp(remove[i] + ':', 'i')
      assign = assign.replace(regex, '');
    }
  }
  theDoc.getElementById('benignList').value = assign;
}

function setTarget(source, target, group)
{
    if(group != undefined)
    {
      if(theDoc.getElementById(source).checked)
      {
        message.id = target + ':';
        var targets = target.split(':');

        for(var i in theDoc.getElementById('groupsList').childNodes)
        {
          if(theDoc.getElementById('groupcheck' + group) != undefined && theDoc.getElementById('groupcheck' + group).id.indexOf(source) == -1)
          {
            theDoc.getElementById('groupcheck' + group).setAttribute('disabled', true);
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
        for(var i = 0; i < 7; i++)
        {
          theDoc.getElementById('button' + i).removeAttribute('disabled'); 
        }
      }
      else
      {
        message.id = '';
        for(var i in theDoc.getElementById('groupsList').childNodes)
        {
          if(theDoc.getElementById('groupcheck' + group) != undefined && theDoc.getElementById('groupcheck' + group).id.indexOf(source) == -1)
          {
            theDoc.getElementById('groupcheck' + group).removeAttribute('disabled');
          }
        }
        for(var i in clients)
        {
          if(clients[i] != '' && clients[i] != undefined && document.getElementById('client' + clients[i]) != undefined)
          {
            document.getElementById('client' + clients[i]).disabled = false;
          } 
        }
        for(var i = 0; i < 7; i++)
        {
          theDoc.getElementById('button' + i).setAttribute('disabled', true); 
        }
      }
    }
    else
    {
      if(theDoc.getElementById(source).checked)
      {
        message.id += target + ':';
        for(var i = 0; i < 7; i++)
        {
          theDoc.getElementById('button' + i).removeAttribute('disabled'); 
        }
      }
      else
      {
        var regex = new RegExp(target + ':', 'i');
        message.id = message.id.replace(regex, '');
      }
      
      if(message.id == '')
      {
        for(var i = 0; i < 7; i++)
        {
          theDoc.getElementById('button' + i).setAttribute('disabled', true); 
        }
      }
    }
    theDoc.getElementById('showtargets').value = message.id.replace(new RegExp(':', 'g'), ',').substr(0, message.id.length - 1);
}

require(['dijit/form/TextBox']);

var ClearAllSuspects = function() 
{
    try 
    {
        now.ClearAllSuspects();
    } 
    catch(err) 
    {
        alert('Action failed because unable to connect to server! Please try refreshing the page and trying again.');
        console.log('err was: ' + err);
    } 
}

now.AllSuspectsCleared = function() 
{
  suspectGrid.Clear();
};

now.SuspectCleared = function(suspect) 
{
    console.log('Cleared suspect ');
    console.log(suspect);
    suspectGrid.DeleteRow(suspect.ip);
    suspectGrid.Render();
    return;
};
  
function pad(num) 
{
    return ('0' + num.toString()).slice(-2);
}

now.OnNewSuspect = function(suspect)
{
  if(suspectGrid == undefined)
  {
    console.log('suspectGrid has not been initialized');
    return;
  }
  doRender = true;
  var type = suspect.type;

  var row = new Array();
  // This is probably a really bad way to do this...
  var inner = '<button onclick=getDetails("' + String(suspect.ip) + '@' + String(suspect.client) + '_' + String(suspect.interface) + '");>' + String(suspect.ip) + '@' + String(suspect.client) + '_' + String(suspect.interface) + '</button>';
  row.push(String(inner));
  row.push(String(suspect.client));
  row.push(parseFloat(suspect.classification));
  row.push(String(suspect.lastpacket));
  row.push(String(suspect.ishostile));

   if(advancedFilterEnabled) 
   {
     if(advancedIpFilter != null) 
     {
        if(!advancedIpFilter.test(suspect.ip)) 
        {
            return;
        }
     }
     if(clientIdFilter != null)
     {
       if(!clientIdFilter.test(suspect.client))
       {
         return; 
       }
     }
   }

  suspectGrid.PushEntry(row);
};

var applyAdvancedFilter = function() 
{
    var patt = new RegExp('^[1234567890x\. ()<>=!&|]+$');

    var filter = theDoc.getElementById('ipFilter').value;
    if(filter != '') 
    {
      try 
      {
          advancedIpFilter = new RegExp(filter);
      } 
      catch(err) 
      {
          alert('Invalid filter: ' + filter);
          return;
      }
    }
    else
    {
      advancedIpFilter = null; 
    }
    var clientFilter = theDoc.getElementById('clientFilter').value;
    if(clientFilter != '')
    {
      try
      {
        clientIdFilter = new RegExp(clientFilter);
      }
      catch(err)
      {
        alert('Invalid filter: ' + clientFilter);
        return; 
      }
    }
    else
    {
      clientIdFilter = null; 
    }

    advancedFilterEnabled = true;
    suspectGrid.Clear();  
    now.ClearSuspectIPs(function(length){
      console.log('in callback, suspectIPs reports length ' + length);
      now.GetHostileSuspects();
    });
};

var disableAdvancedFilter = function() 
{
    theDoc.getElementById('advancedFiltering').style.display='none';
    
    advancedFilterEnabled = false;
    var advancedFilters = new Object();
    
    suspectGrid.Clear();
    now.ClearSuspectIPs(function(length){
      console.log('in callback, suspectIPs reports length ' + length);
      now.GetHostileSuspects();
    });
}
          
function featureFormatter(d) 
{
    var num = new Number(d);
    return (num.toFixed(2));
}

function classificationFormatter(d) 
{
    var num = 100 * d;
    if (num >= 0) 
    {
        // Minor performance boost, don't bother with coloring those < than 5% hostile, barely shows anyway
        if (num > 5) 
        {
            return '<div style="background: rgba(255, 0, 0, ' + d.toFixed(2) + ');">' + num.toFixed(2) + "%</div>";
        } 
        else 
        {
            return num.toFixed(2) + '%';
        }
    } 
    else 
    {
        if (num == -1) 
        {
            return 'Invalid';
        } 
        else if (num == -2) 
        {
            return 'Not Enough Data';
        } 
        else 
        {
            return 'Error';
        }
    }
}

function onGridRendered() 
{
   var gridSize = suspectGrid.GetNumberOfPages();
   var currentPage = suspectGrid.GetCurrentPage();

   var temp = theDoc.createElement('a');
   theDoc.getElementById('tablePages').innerHTML = '';

   var pageLink = theDoc.createElement('a');
   pageLink.setAttribute('href', '#');
   pageLink.setAttribute('onclick', 'suspectGrid.PreviousPage();');
   var pageLinkText = theDoc.createTextNode(' <-- ');
   pageLink.appendChild(pageLinkText);
   theDoc.getElementById('tablePages').appendChild(pageLink);

   for (var i = 0; i < gridSize; i++) 
   {
       var pageLink = theDoc.createElement('a');
       pageLink.setAttribute('href', '#');
       pageLink.setAttribute('onclick', 'suspectGrid.SetCurrentPage( ' + i + ');');

       var pageLinkText = theDoc.createTextNode(' ' + (i + 1) + ' ');
       pageLink.appendChild(pageLinkText);

       if (i == currentPage) 
       {
           var boldTag = theDoc.createElement('b');
           boldTag.appendChild(pageLink);

           theDoc.getElementById('tablePages').appendChild(boldTag);
       } 
       else 
       {
           theDoc.getElementById('tablePages').appendChild(pageLink);
       }    
   }

   var pageLink = theDoc.createElement('a');
   pageLink.setAttribute('href', '#');
   pageLink.setAttribute('onclick', 'suspectGrid.NextPage();');
   var pageLinkText = theDoc.createTextNode(' --> ');
   pageLink.appendChild(pageLinkText);
   theDoc.getElementById('tablePages').appendChild(pageLink);
}

now.RenderBenignRequests = function()
{
  now.GetClientBenignRequest(function(list){
    var set = '';
    for(var i in list)
    {
      if(list[i] != '')
      {
        set += list[i] + ':';
      }
    }
    theDoc.getElementById('benignList').value = set;
  }); 
}
