function generatePortSetTable(portset, domElement) {
	var br = domElement.appendChild(document.createElement('br'));
	var table = domElement.appendChild(document.createElement('table'));
	table.className += "portset";
	var caption = table.appendChild(document.createElement('caption'));
	caption.innerHTML = portset.setName;

	var thead = table.appendChild(document.createElement('thead'));
	var theadtr = thead.appendChild(document.createElement('tr'));

	var header1 = theadtr.appendChild(document.createElement('th'));
	header1.innerHTML = "Port Number";
	var header2 = theadtr.appendChild(document.createElement('th'));
	header2.innerHTML = "Protocol";
	var header3 = theadtr.appendChild(document.createElement('th'));
	header3.innerHTML = "Behavior";

	var tr = table.appendChild(document.createElement('tr'));
	var td0 = tr.appendChild(document.createElement('td'));
	td0.innerHTML = "default";
	var td1 = tr.appendChild(document.createElement('td'));
	td1.innerHTML = "tcp";
	var td2 = tr.appendChild(document.createElement('td'));
	td2.innerHTML = portset.TCPBehavior;

	tr = table.appendChild(document.createElement('tr'));
	td0 = tr.appendChild(document.createElement('td'));
	td0.innerHTML = "default";
	td1 = tr.appendChild(document.createElement('td'));
	td1.innerHTML = "udp";
	td2 = tr.appendChild(document.createElement('td'));
	td2.innerHTML = portset.UDPBehavior;

	tr = table.appendChild(document.createElement('tr'));
	td0 = tr.appendChild(document.createElement('td'));
	td0.innerHTML = "default";
	td1 = tr.appendChild(document.createElement('td'));
	td1.innerHTML = "icmp";
	td2 = tr.appendChild(document.createElement('td'));
	td2.innerHTML = portset.ICMPBehavior;

	for (var port in portset.TCPExceptions) {
		var tr = table.appendChild(document.createElement('tr'));

		var td0 = tr.appendChild(document.createElement('td'));
		td0.innerHTML = portset.TCPExceptions[port].portNum;

		var td1 = tr.appendChild(document.createElement('td'));
		td1.innerHTML = portset.TCPExceptions[port].protocol;

		var td2 = tr.appendChild(document.createElement('td'));
		td2.innerHTML = portset.TCPExceptions[port].behavior;
	}

	for (var port in portset.UDPExceptions) {
		var tr = table.appendChild(document.createElement('tr'));

		var td0 = tr.appendChild(document.createElement('td'));
		td0.innerHTML = portset.UDPExceptions[port].portNum;

		var td1 = tr.appendChild(document.createElement('td'));
		td1.innerHTML = portset.UDPExceptions[port].protocol;

		var td2 = tr.appendChild(document.createElement('td'));
		td2.innerHTML = portset.UDPExceptions[port].behavior;
	}

}
