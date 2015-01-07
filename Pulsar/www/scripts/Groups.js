function setUpGroups(bodyId)
{
  if(groups == undefined)
  {
    return;
  }
  
  var groupList = groups.groups.split(':');
  var memberList = groups.members.split('|');
  var ah = theDoc.getElementById(bodyId);
  
  for(var i in groupList)
  {
    if(groupList[i] != '')
    {
      maintainGroups.push({group:groupList[i],idx:i});
      var tr = theDoc.createElement('tr');
      tr.id = 'group' + i;
      
      var td1 = theDoc.createElement('td');
      var td2 = theDoc.createElement('td');
      
      var label = theDoc.createElement('label');
      label.id = 'groupLabel' + i;
      label.innerHTML = groupList[i];
      label.value = groupList[i];
      var input = theDoc.createElement('input');
      input.id = 'members' + i;
      input.value = memberList[i];
      if(groupList[i] == 'all')
      {
        input.setAttribute('readonly', 'true');
      }
      
      td1.appendChild(label);
      td2.appendChild(input);
      tr.appendChild(td1);
      tr.appendChild(td2);
      
      if(groupList[i] != 'all')
      {
        var td3 = theDoc.createElement('td');
        var deleteButton = theDoc.createElement('button');
        deleteButton.setAttribute('onclick', 'removeGroup("groupLabel' + i + '")');
        deleteButton.innerHTML = 'Delete';
        td3.appendChild(deleteButton);
        tr.appendChild(td3);
        
        var td4 = theDoc.createElement('td');
        var updateButton = theDoc.createElement('button');
        updateButton.setAttribute('onclick', 'updateGroup(document.getElementById("groupLabel' + i + '").value, document.getElementById("members' + i + '").value)');
        updateButton.innerHTML = 'Update';
        td4.appendChild(updateButton);
        tr.appendChild(td4);
      }
      
      ah.appendChild(tr);
    }
  }
}

function getMembers()
{
  if(clients == '' || clients.length == 0)
  {
    return undefined;
  }
  else
  {
    var members = '';
    for(var i in clients)
    {
      if(theDoc.getElementById('client' + clients[i]) != null && theDoc.getElementById('client' + clients[i]).checked)
      {
        members += theDoc.getElementById('client' + clients[i]).value + ',';
      }
    }
    if(members[members.length - 1] === ',')
    {
      members = members.substr(0, members.length - 1);
    }
    if(members == '')
    {
      return undefined;
    }
    
    return members;
  }
}

function removeGroup(group)
{
  var groupToRemove = theDoc.getElementById(group).value;
  now.RemoveGroup(groupToRemove, function(err){
    if(err != undefined)
    {
      alert(err);
      return;
    }
  });
  for(var i in maintainGroups)
  {
    if(maintainGroups[i].group == groupToRemove)
    {
      delete maintainGroups[i];
    } 
  }
  theDoc.getElementById('appendHook').removeChild(theDoc.getElementById('group' + group.substr(10)));
}

function updateGroup(group, newMembers)
{
  now.UpdateGroup(group, newMembers, function(err){
    if(err != undefined)
    {
      alert(err);
      return;
    }
  });
  var idx = 0;
  for(var i in maintainGroups)
  {
    if(group == maintainGroups[i].group)
    {
      idx = maintainGroups[i].idx;
    }
  }
  theDoc.getElementById('members' + idx).value = newMembers;
  var checkGroupCount = newMembers.split(',');
  if(theDoc.getElementById('groupcheck' + idx) !== null && (checkGroupCount[1] == '' || checkGroupCount[1] == undefined))
  {
    theDoc.getElementById('groupcheck' + idx).setAttribute('disabled', true);
  }
}

function addGroup(group, members)
{
  if(members != undefined && group != undefined)
  {
    for(var i in maintainGroups)
    {
      if(group == maintainGroups[i].group)
      {
        alert('Cannot have identical group names');
        theDoc.getElementById('groupName').value = '';
        return;
      }
      if(members == theDoc.getElementById('members' + maintainGroups[i].idx).value)
      {
        alert('Already have a group "' + maintainGroups[i].group + '" with members "' + members + '"');
        theDoc.getElementById('groupName').value = '';
        return;
      }
    }
    
    var checkMemberCount = members.split(',');
    if(checkMemberCount[1] == '' || checkMemberCount[1] == undefined)
    {
      alert('Groups must consist of two or more clients');
      return;
    }
    
    maintainGroups.push({group:group,idx:parseInt(clientCount) + 1});
   
    if(/^[a-z0-9]+$/i.test(group) || group.length <= 1)
    {
      alert('Group name must be between 2 and 10 alphanumeric characters');
      return;
    }
   
    consle.log('group is ' + group);
   
    now.AddGroup(group, members, function(err){
      if(err != undefined)
      {
        alert(err);
        return;
      }
    });
    
    theDoc.getElementById('groupName').value = '';
    
    var ah = theDoc.getElementById('appendHook');
    
    var tr = theDoc.createElement('tr');
    tr.id = 'group' + (parseInt(clientCount) + 1);
    
    var td1 = theDoc.createElement('td');
    var td2 = theDoc.createElement('td');
    
    var label = theDoc.createElement('label');
    label.id = 'groupLabel' + (parseInt(clientCount) + 1);
    label.innerHTML = group;
    label.value = group;
    var input = theDoc.createElement('input');
    input.id = 'members' + (parseInt(clientCount) + 1);
    input.value = members;
    
    td1.appendChild(label);
    td2.appendChild(input);
    tr.appendChild(td1);
    tr.appendChild(td2);
    
    if(group != 'all')
    {
      var td3 = theDoc.createElement('td');
      var deleteButton = theDoc.createElement('button');
      deleteButton.setAttribute('onclick', 'removeGroup("groupLabel' + (parseInt(clientCount) + 1) + '")');
      deleteButton.innerHTML = 'Delete';
      td3.appendChild(deleteButton);
      tr.appendChild(td3);
      
      var td4 = theDoc.createElement('td');
      var updateButton = theDoc.createElement('button');
      updateButton.setAttribute('onclick', 'updateGroup(document.getElementById("groupLabel' + (parseInt(clientCount) + 1) + '").value, document.getElementById("members' + (parseInt(clientCount) + 1) + '").value)');
      updateButton.innerHTML = 'Update';
      td4.appendChild(updateButton);
      tr.appendChild(td4);
    }
    
    ah.appendChild(tr);
  }
  else
  {
    alert('To add a group, at least one client must be checked and a group name must be entered!');
  }
}