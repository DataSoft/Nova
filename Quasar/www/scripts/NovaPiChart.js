//============================================================================
// Name        : NovaPiChart.js
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
// Description : Simple pi chart
//============================================================================

var lastRed = 0, lastGreen = 0, lastBlue = 0; //Integer representations of color

// divId: id of div to put the pi chart
// size: Size of pi chart in pixels (will be size X size)
// items: Array of objects with each object consisting of,
//     name: String name of the item
//     value: Number of this item present
var NovaPiChart = function(divId, title, size, deleteButtonFunction) {
    this.m_title = title;
    this.m_deleteFunction = deleteButtonFunction;
    this.SetSize(size);
    
    var mainDiv = document.createElement("div");
    mainDiv.setAttribute("style", "text-align: left");

    this.popupdiv = document.createElement("div");
    $(this.popupdiv).css('display','none');  
    $(this.popupdiv).css('background-color','#D0E9FC');  
    $(this.popupdiv).css('padding','2px');  
    $(this.popupdiv).css('border','1px solid black');  
    $(this.popupdiv).css('border-radius','4px');  
    $(this.popupdiv).css("position", "absolute"); 
    $(this.popupdiv).css("z-index", "120"); 

    var div = document.getElementById(divId);
    div.appendChild(mainDiv);
    $('body')[0].appendChild(this.popupdiv);
    this.m_id = mainDiv;
}

NovaPiChart.prototype = {
    SetSize: function(size) {
        // Used so when we redrow we use the same colors
        this.m_usedColors = new Object();
        this.m_size = size;
        this.m_halfSize = parseInt(size/2);
    },

    // Renders the chart
    Render: function(items) {
        if (items !== undefined) {
            this.m_items = items;
        }

        this.m_numberOfItems = 0;
        for (var i = 0; i < items.length; i++) {
           this.m_numberOfItems += items[i].value;
        }
        // Reset the div
        this.m_id.innerHTML = "";

        var title = document.createElement("h2");
        title.innerHTML = this.m_title;
        title.setAttribute("class", "novaGridTitle");
        this.m_id.appendChild(title);

        // Make a canvas
        var canvas = document.createElement("canvas");
        canvas.setAttribute("width", this.m_size + "px");
        canvas.setAttribute("height", this.m_size + "px");
        this.m_id.appendChild(canvas);

        
        // Draw the pi chart on the canvas
        var ctx = canvas.getContext("2d");
        var lastend = 0;
        ctx.clearRect(0, 0, canvas.width, canvas.height);

        var self = this;


        if (this.m_items.length >= 1) {
        
        // Hide the popup box when not in the canvas with mouse
        $(canvas).mouseout(function() {
            $(self.popupdiv).css('display', 'none'); 
        });

        // Make a popup box that follows mouse and tells you which slice is hovered over
        canvas.onmousemove = function(e) {
            if (!e) var e = window.event;
           
            var x = e.pageX - $(this).offset().left;
            var y = self.m_size - (e.pageY - $(this).offset().top);

            // Don't display the popup if we're not inside the circle
            var sqrDistance = Math.pow((self.m_halfSize - x), 2) + Math.pow((self.m_halfSize - y), 2);
            if (sqrDistance >= Math.pow(self.m_halfSize, 2)) {
                $(self.popupdiv).css('display', 'none'); 
                return;
            }

            // Angle only valid right now if Quadrant I
            var m = (self.m_halfSize - y)/(self.m_halfSize - x);
            var angle = Math.atan(m);

            // Get proper angle based on quadrant
            // Quadrant II
            if (x <= self.m_halfSize && y > self.m_halfSize) {
                angle += Math.PI;
            // Quadrant III
            } else if (x <= self.m_halfSize && y <= self.m_halfSize) {
                angle += Math.PI;
            // Quadrant IV
            } else if (x >= self.m_halfSize && y < self.m_halfSize) {
                angle += 2*Math.PI;
            }
           

            // html5 canvas is being drawn clockwise instead of the usual counterclockwise, this fixes our angle for that
            angle = 2*Math.PI - angle;

            // Find the index of the currently hovered over pie slice
            var current = 0;
            for (var i = 0; i < self.m_items.length; i++) {
                if (angle >= self.m_items[i].startArc && angle <= self.m_items[i].endArc) {
                    current = i;
                    break;
                }
            }

            $(self.popupdiv).html(self.m_items[current].name + " (" + Number(100*self.m_items[current].value/self.m_numberOfItems).toFixed(2) + "%)"); 
            $(self.popupdiv).css('left',e.pageX); 
            $(self.popupdiv).css('top',e.pageY - 22);
            $(self.popupdiv).css('display', 'block'); 
        };

        }

        if (this.m_numberOfItems == 0) {
            ctx.fillStyle = "#A1A1A1";
            ctx.beginPath();
            ctx.moveTo(this.m_halfSize,this.m_halfSize);
            ctx.arc(this.m_halfSize,this.m_halfSize, this.m_halfSize,lastend,lastend+Math.PI*2,false);
            ctx.lineTo(this.m_halfSize, this.m_halfSize);
            ctx.fill();

            for (var i = 0; i < this.m_items.length; i++) {
                var legend = document.createElement("div");
                legend.setAttribute("class", "pieLegendElementDiv");
                var text = document.createElement("p");
                text.setAttribute('style', 'display: inline-block; margin: 2px');
                text.innerHTML = "<span style='background-color: " + "#A1A1A1" + ";'>&nbsp &nbsp &nbsp</span>&nbsp 0% " + this.m_items[i].name;
                legend.appendChild(text);
                this.m_id.appendChild(legend);
            }
            return;
        }
        var randomColor = "";

        for (var pfile = 0; pfile < this.m_items.length; pfile++) {
            if (this.m_usedColors[this.m_items[pfile].name] === undefined) {
                delta = ((1<<7)*Math.random()|0) + (1<<6);
                randomColor = ((lastRed + delta) % (1<<8)).toString(16);
                lastRed = (lastRed + delta) % (1<<8);

                delta = ((1<<7)*Math.random()|0) + (1<<6);
                randomColor += ((lastGreen + delta) % (1<<8)).toString(16);
                lastGreen = (lastGreen + delta) % (1<<8);

                delta = ((1<<7)*Math.random()|0) + (1<<6);
                randomColor += ((lastBlue + delta) % (1<<8)).toString(16);
                lastBlue = (lastBlue + delta) % (1<<8);

                // Pad color with 0's on the left
                if (randomColor.length != 6) {
                    for (var i = randomColor.length; i < 6; i++) {
                        randomColor = "0" + randomColor;
                    }
                }
                randomColor = "#" + randomColor;
                this.m_usedColors[this.m_items[pfile].name] = randomColor;
            } else {
                randomColor = this.m_usedColors[this.m_items[pfile].name];
            }

            ctx.fillStyle = randomColor;
            ctx.beginPath();
            ctx.moveTo(this.m_halfSize,this.m_halfSize);
            ctx.arc(this.m_halfSize,this.m_halfSize, this.m_halfSize,lastend,lastend+(Math.PI*2*(this.m_items[pfile].value/this.m_numberOfItems)),false);
            ctx.lineTo(this.m_halfSize, this.m_halfSize);
            ctx.fill();

            this.m_items[pfile].startArc = lastend;
            lastend += Math.PI*2*(this.m_items[pfile].value/this.m_numberOfItems);
            this.m_items[pfile].endArc = lastend;

            // Draw the legend and values
            if (!this.m_disableLegend) {
                var legend = document.createElement("div");
                legend.setAttribute("class", "pieLegendElementDiv");
                var text = document.createElement("p");
                text.innerHTML = "<span style='background-color: " + randomColor + ";'>&nbsp &nbsp &nbsp</span>&nbsp " +  (100*this.m_items[pfile].value/this.m_numberOfItems).toFixed(2) + "% (" + this.m_items[pfile].value + ") " + this.m_items[pfile].name;
                text.setAttribute('style', 'display: inline-block; margin: 2px');
                if(this.m_deleteFunction != undefined) {
                    var deleteButton = document.createElement("button");
                    deleteButton.setAttribute('style', 'display: inline-block;');
                    deleteButton.innerHTML = "<img src='images/delete.png' class='buttonIcon'/><span class='buttonSpan'>Delete All</span>";
                    deleteButton.setAttribute('onclick', this.m_deleteFunction + '("' + this.m_items[pfile].name + '")');
                    legend.appendChild(deleteButton);
                }
                legend.appendChild(text);
                this.m_id.appendChild(legend);
            }
        }
        
    }
}
