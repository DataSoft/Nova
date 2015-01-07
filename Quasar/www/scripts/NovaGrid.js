//============================================================================
// Name        : NovaGrid.js
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
// Description : Simple grid for displaying suspects and other web UI stuff
//============================================================================

// Constructor
//   columns: array of column objects
//     Each should contain a "name" attribute and optionally a "formatter"
//   keyIndex: index of column used as a UID for a row
//   tableElement: DOM object of the <table> 
//   selection (optional): boolean. Enable or disable the ability to select table rows
//   rightclick (optional): TODO document this
var NovaGrid = function(columns, keyIndex, tableElement, gridName, selection, rightclick) {
    this.m_columns = columns;
    this.m_keyIndex = keyIndex;
    this.m_sortByKey = keyIndex;
    this.m_sortDescending = true;
    this.m_tableElement = tableElement;
    this.m_elements = new Object();
    this.m_pageElements = [];
    this.m_pagesElement = null;

    // Current page
    this.m_arrayRep = [];
    
    // Called whenever the grid is redrawn
    this.m_renderCallback = function() {};

    // Called when the grid is redrawn without explicit Render call (page change, sort order change, etc)
    this.m_autoRenderCallback = function() {};
    
    
    this.m_selected = [];
    this.m_currentPage = 0;
    this.m_relativePageNumbersToShow = 2;
    this.m_rowsPerPage = Number.MAX_VALUE;
    this.m_name = gridName;

    this.m_remotePaging = false;
    this.m_numberOfPages = 1;

    if (selection == undefined)
    {
        this.m_selection = false;
    }
    else
    {
        this.m_selection = selection;
    }

    if(rightclick == undefined)
    {
      this.m_rightClick = false;
    }
    else
    {
      this.m_rightClick = rightclick;
    }


    this.GenerateDivs();
    this.GenerateTableHeader();
}

Object.keys = Object.keys || (function () {
    var hasOwnProperty = Object.prototype.hasOwnProperty,
        hasDontEnumBug = !{toString:null}.propertyIsEnumerable("toString"),
        DontEnums = [
            'toString',
            'toLocaleString',
            'valueOf',
            'hasOwnProperty',
            'isPrototypeOf',
            'propertyIsEnumerable',
            'constructor'
        ],
        DontEnumsLength = DontEnums.length;
 
    return function (o) {
        if (typeof o != "object" && typeof o != "function" || o === null)
            throw new TypeError("Object.keys called on a non-object");
 
        var result = [];
        for (var name in o) {
            if (hasOwnProperty.call(o, name))
                result.push(name);
        }
 
        if (hasDontEnumBug) {
            for (var i = 0; i < DontEnumsLength; i++) {
                if (hasOwnProperty.call(o, DontEnums[i]))
                    result.push(DontEnums[i]);
            }
        }
 
        return result;
    };
})();

NovaGrid.prototype = {
    // Adds a new row to the table (or updates row if it exists with same keyIndex)
          PushEntry: function (entry) {
               if (entry.length != this.m_columns.length) {
                   throw "Can't push entry of size " + entry.length + " into table of size " + this.m_columns.length
               } else {
                   this.m_elements[entry[this.m_keyIndex]] = entry;
                   // Disabling row blinking for now
                   //this.m_elements[entry[this.m_keyIndex]]._newRow = true;
               }
           }

           , GenerateDivs: function() {
                this.m_tableElement.innerHTML = "";
             
                this.m_pagesElement = document.createElement("div");
                this.m_pagesElement.setAttribute("class", "novaGridPagesDiv");

                var tableDiv = document.createElement("div");
                tableDiv.setAttribute("class", "novaGrid");

                if (this.m_pagesBelowChart) {
                    this.m_tableElement.appendChild(this.m_pagesElement);
                    this.m_tableElement.appendChild(tableDiv);
                } else {
                    this.m_tableElement.appendChild(tableDiv);
                    this.m_tableElement.appendChild(this.m_pagesElement);
                }

                this.m_tableElement = tableDiv;
           }

           // Used internally to generate the TH tags
           , GenerateTableHeader: function() {
               this.headerHTML = '<TABLE class="novaGrid">';
               this.headerHTML += '<THEAD>';
               this.headerHTML += '<TR class="novaGrid">';
               for (var c = 0; c < this.m_columns.length; c++) 
               {
                   if (this.m_columns[c].isDisabled == true) {continue;}

                   var title = this.m_columns[c].name;

                   if (this.m_sortByKey == c) 
                   {
                       title = '<span>' + title + '</span>' + '<span class="sortArrow">' + (this.m_sortDescending ? '&#8744;' : '&#8743;') + '</span>';
                   }
                   this.headerHTML += '<TH class="novaGrid" onclick="' + this.m_name + '.SetSortByKey(' + c + ')"><div style="font-size: 12px; cursor: pointer; white-space: nowrap;">' + title + '</div></TH>';
               }
               this.headerHTML += '</TR>';
               this.headerHTML += '</THEAD>';
           }

           , GetRenderCallback: function() {return this.m_renderCallback;}
           , SetRenderCallback: function(cb) {this.m_renderCallback = cb;}
           , GetRowsPerPage: function() {return this.m_rowsPerPage;}
           , SetRowsPerPage: function(rows) {this.m_rowsPerPage = rows;}
           , GetCurrentPage: function() {return this.m_currentPage;}

           , SetCurrentPage: function(page) {
               if (page >= 0 && page < this.GetNumberOfPages()) {
                   this.m_currentPage = page;
                   this.m_selected.length = 0;
                   this.m_autoRenderCallback();
                   this.Render();
               }
           }

           , NextPage: function() {
               if (this.GetNumberOfPages() == 1) {return};
               if (this.m_currentPage + 1 >= this.GetNumberOfPages()) {
                   this.SetCurrentPage(0);
               } else {
                   this.SetCurrentPage(this.m_currentPage + 1);
               }
           }

           , PreviousPage: function() {
               if (this.GetNumberOfPages() == 1) {return};
               if (this.m_currentPage - 1 < 0) {
                   this.SetCurrentPage(this.GetNumberOfPages() - 1);
               } else {
                   this.SetCurrentPage(this.m_currentPage - 1);
               }
           }

           , GetNumberOfPages: function() {
               if (!this.m_remotePaging) {
                    this.m_numberOfPages = Math.ceil(Object.keys(this.m_elements).length/this.m_rowsPerPage);
               }

               return this.m_numberOfPages;
           }

           // Returns the HTML for the table
           , GetTable: function() {
               if (this.m_rightClick) {
                var innerTableString = this.headerHTML + '<TBODY' + ' oncontextmenu="' + this.m_rightClick + '" onclick="' + this.m_rightClick + '">';
               } else {
                if (this.m_selection) {
                  var innerTableString = this.headerHTML + '<TBODY onclick="' + this.m_name + '.AddToSelected(event)">';
                } else {
                  var innerTableString = this.headerHTML + '<TBODY>';
                }
               }
               var keys = Object.keys(this.m_elements);
               this.m_arrayRep = new Array();
               for (var i = 0; i < keys.length; i++) 
               {
                 this.m_arrayRep.push(this.m_elements[keys[i]]);
               }
               
               // work around for scoping issues
               var so = this;

               this.m_arrayRep.sort(function(a, b) {
                 if(a[so.m_sortByKey] > b[so.m_sortByKey]) 
                 {
                   return so.m_sortDescending == true ? -1 : 1;
                 } 
                 else if(a[so.m_sortByKey] < b[so.m_sortByKey])
                 {
                   return so.m_sortDescending == true ? 1 : -1;
                 } 
                 else 
                 {
                   return 0;
                 }
               });


               if (!this.m_remotePaging) {
                    if (this.m_arrayRep.length < this.m_currentPage * this.m_rowsPerPage) {
                        return innerTableString; 
                    }
               }


               var i = this.m_currentPage * this.m_rowsPerPage;
               var maxi = (this.m_currentPage + 1)* this.m_rowsPerPage;
               if (this.m_remotePaging) {
                 i = 0;
                 maxi = this.m_rowsPerPage;
               }

               for (; (i < this.m_arrayRep.length) && i < maxi; i++) {
                   var sub = '';
                   var idx = keys[i].indexOf('>');
                   // All this magic because the grid is keyed to an html
                   // element and not a string for the main.jade page.
                   if(idx != -1)
                   {
                       var put = keys[i].substring(idx + 1, keys[i].indexOf('<', idx));
                       if(put[0] == ' ')
                       {
                           put = put.substring(1);
                       }
                       if(put[put.length] == ' ')
                       {
                           put = put.substring(0, put.length - 2);
                       }
                       this.m_pageElements.push(put);
                       sub = put;  
                   }
                   else
                   {
                       this.m_pageElements.push(this.m_arrayRep[i][this.m_keyIndex]);
                       sub = this.m_arrayRep[i][this.m_keyIndex];
                   }
                   if(this.m_selection)
                   {
                       if (this.m_selected.indexOf(this.m_arrayRep[i][this.m_keyIndex]) != -1)
                       {
                         innerTableString += '<TR class="novaGrid" style="background: #d0e9fc">';
                       } else {
                         innerTableString += '<TR class="novaGrid">';
                       }
                   }
                   else
                   {
                     var classes = "novaGrid";
                     if (this.m_arrayRep[i]._newRow)
                     {
                        this.m_arrayRep[i]._newRow = false;
                        classes += " newRow";
                     }

                     innerTableString += '<TR class="' + classes + '" ';
                     if(this.m_rightClick != undefined)
                     {
                       if(this.m_arrayRep[i].style != undefined)
                       {
                         innerTableString += 'style="' + this.m_arrayRep[i].style + '">';
                       } 
                       else
                       {
                         innerTableString += '>';
                       }     
                     }
                     else
                     {
                       if(this.m_arrayRep[i].style != undefined)
                       {
                         innerTableString += 'style="' + this.m_arrayRep[i].style + '">';
                       } 
                       else
                       {
                         innerTableString += '>';
                       }                         
                     }
                   }
                     for (var c = 0; c < this.m_columns.length; c++) {
                        if (this.m_columns[c].isDisabled == true) {continue;}
                        if (this.m_columns[c].formatter !== undefined) {
                          innerTableString += '<TD class="novaGrid">' + this.m_columns[c].formatter(this.m_arrayRep[i][c]) + '</TD>';
                        } else {
                          if (this.m_columns[c].noEscape) {
                            innerTableString += '<TD class="novaGrid">' + this.m_arrayRep[i][c] + '</TD>';
                          } else {
                            innerTableString += '<TD class="novaGrid">' + String(this.m_arrayRep[i][c]).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") + '</TD>';
                          }
                        }
                    }
                    innerTableString += '</TR>';
                }
                innerTableString += "</TBODY></TABLE>";
        
                return innerTableString;
            }

    // Sets which column to sort the table by
    , SetSortByKey: function(key) {
        // Why? This is really inconvenient...
        if (this.m_sortByKey == key) {
            this.m_sortDescending = !this.m_sortDescending;
        }
        this.m_sortByKey = key;
        this.GenerateTableHeader();
        this.m_autoRenderCallback();
        this.Render();
    }

    , EnableColumn: function(columnIndex) {
        this.m_columns[columnIndex].isDisabled = false;
    }
    
    , DisableColumn: function(columnIndex) {
        this.m_columns[columnIndex].isDisabled = true;
    }

    // Clears all elements from the table
    , Clear: function() {
        this.m_elements = new Array();
        this.Render();
    }

    , DeleteRow: function(key, noRender) {
        delete this.m_elements[key];

        if (this.Size() % this.m_rowsPerPage == 0 && this.m_currentPage != 0) {
            this.PreviousPage();
        }

        if (!noRender) {
            this.Render();
        }
    }

    // Returns the number of rows in the table
    , Size: function() {
        return Object.keys(this.m_elements).length;
    }
    
  , GetSelected: function() {
    var ret = this.m_selected;
    return ret;
  }
  
  , GetPageElements: function() {
    var ret = this.m_pageElements;
    return ret;
  }
  
  , GetElements: function() {
    var ret = this.m_elements;
    return ret;
  }
  
  , AddToSelected: function(oEvent) {
    var tableRow = oEvent.srcElement || oEvent.target;
    while (tableRow != null && tableRow.nodeName != "TR") {
        tableRow = tableRow.parentNode;
    }

    if (tableRow == null) {
        console.log("ERROR: Unable to get <TR> element from click");
        return;
    }

    var key = this.m_arrayRep[tableRow.rowIndex - 1][this.m_keyIndex];
    if(oEvent.ctrlKey)
    {
      var add = true;
      var idx = 0;
      for(var i in this.m_selected)
      {
        if(this.m_selected[i] === key)
        {
          add = false;
          idx = i;
        }
      }
      if(add)
      {
        this.m_selected.push(key);
        this.ChangeRowColor(key, true);
      }
      else
      {
        delete this.m_selected[idx];
        this.ChangeRowColor(key, false);
      }
    }
    else if(oEvent.shiftKey)
    {
      var idxStart = 0;
      var idxEnd = 0;
      var add = [];
      var i = this.m_pageElements.length - 1;

      if(this.m_selected.length == 1)
      {

        for (var i = 0; i < this.m_pageElements.length; i++) {
          if(this.m_pageElements[i] == this.m_selected[0])
          {
            idxStart = i;
          }
          else if(this.m_pageElements[i] == key)
          {
            idxEnd = i;
          }
        }
        
        if(idxEnd < idxStart)
        {
          var temp = idxEnd;
          idxEnd = idxStart;
          idxStart = temp;
        }
       
        this.m_selected = [];
        for(var i = idxStart; i <= idxEnd; i++)
        {
          this.m_selected.push(this.m_pageElements[i]);
          this.ChangeRowColor(this.m_pageElements[i], true);
        }
      }
      else
      {
        for(var i in this.m_selected)
        {
          this.ChangeRowColor(this.m_selected[i], false);
        }
        this.m_selected = [];
        this.m_selected.push(key);       
      }
      document.getSelection().removeAllRanges();
    }
    else
    {
      for(var i in this.m_selected)
      {
       this.ChangeRowColor(this.m_selected[i], false);
      }
      this.m_selected = [];
      this.m_selected.push(key);
      this.ChangeRowColor(this.m_selected[0], true);
    }
  }

  // Pass this a div DOM element and it will throw the page numbers in it
  ,populateTablePages: function() {
       tablePages = this.m_pagesElement;

       if (!tablePages) {return;}
       tablePages.innerHTML = "";

       var gridSize = this.GetNumberOfPages();
       var currentPage = this.GetCurrentPage();

       var temp = document.createElement('a');
           
       var startpageLink = document.createElement('span');
       startpageLink.setAttribute('onclick', this.m_name + '.SetCurrentPage(0);');
       startpageLink.className += "pageNumberLink";
       startpageLink.title = "First page";
       var startpageLinkText = document.createTextNode("First");
       startpageLink.appendChild(startpageLinkText);
       tablePages.appendChild(startpageLink);

       var pageLink = document.createElement('span');
       pageLink.setAttribute('onclick', this.m_name + '.PreviousPage();');
       pageLink.className += "pageNumberLink";
       pageLink.title = "Previous page";
       var pageLinkText = document.createTextNode(" Back ");
       pageLink.appendChild(pageLinkText);
       tablePages.appendChild(pageLink);
       
    
       // We only show the current page and the current page +/- m_relativePageNumbersToShow
       if (gridSize < (this.m_relativePageNumbersToShow*2 + 1)) {
           var minPage = 0;
           var maxPage = gridSize - 1;
       } else {
           var minPage = currentPage - this.m_relativePageNumbersToShow;
           var maxPage = currentPage + this.m_relativePageNumbersToShow;
       }

       // Handle edge case where we can't center the current page in the list because it's too low
       if (minPage < 0) {
           minPage = 0;
           maxPage = this.m_relativePageNumbersToShow*2;
       }
       
       // Handle edge case where we can't center the current page in the list because it's too high
       if (maxPage > gridSize - 1) {
           maxPage = gridSize - 1;
           minPage = maxPage - this.m_relativePageNumbersToShow*2;
       }

       for (var i = minPage; i <= maxPage; i++) {
           var pageLink = document.createElement('span');
           pageLink.setAttribute('onclick', this.m_name + '.SetCurrentPage(' + i + ');');

           var pageLinkText = document.createTextNode(i + 1);
           pageLink.appendChild(pageLinkText);

           if (i == currentPage) {
               pageLink.className += " selectedpageNumberLink";
           }
           pageLink.className += " pageNumberLink";
           tablePages.appendChild(pageLink);
       }
       
       var pageLink = document.createElement('span');
       pageLink.setAttribute('onclick', this.m_name + '.NextPage();');
       pageLink.className += "pageNumberLink";
       pageLink.title = "Next page";
       var pageLinkText = document.createTextNode(" Next ");
       pageLink.appendChild(pageLinkText);
       tablePages.appendChild(pageLink);
           
       var lastpageLink = document.createElement('span');
       lastpageLink.setAttribute('onclick', this.m_name + '.SetCurrentPage(' + (this.GetNumberOfPages() - 1) + ');');
       lastpageLink.className += "pageNumberLink";
       lastpageLink.title = "Last page";
       var lastpageLinkText = document.createTextNode("Last");
       lastpageLink.appendChild(lastpageLinkText);
       tablePages.appendChild(lastpageLink);
  }
  
  , PushKeyToSelected: function(key)
  {
    var add = true;
    for(var i in this.m_selected)
    {
      if(key == this.m_selected[i])
      {
        add = false;
      }
    } 
    if(add)
    {
      this.m_selected.push(key);
      this.ChangeRowColor(key, true);
    }
  }
  
  , ChangeRowColor: function(elementId, makeBlue) {
    var keyIndex = -1;
    for (var i = 0; i < this.m_arrayRep.length; i++) {
        if (this.m_arrayRep[i][this.m_keyIndex] == elementId) {
            keyIndex = i;
            break;
        }
    }

    if (keyIndex == -1) {
        return;
    }

    var rowElemen = this.m_tableElement.firstChild.childNodes[1].getElementsByTagName("TR")[keyIndex];

    if(makeBlue == false)
    {
     rowElemen.style.background = 'white';
    }
    else
    {
      rowElemen.style.background = '#d0e9fc';
    }
  }
  
  , SetRightClickEventListener: function(rightClickFunction) {
    this.m_rightClick = rightClickFunction.toString();
  }
  , SetKeyIndex: function(keyIndex) {
    this.m_keyIndex = keyIndex;
  }
  // Renders the table
  , Render: function() {
      // Simple way (slow in Chrome, fine in FF)
      //theDoc.getElementById("suspectTable").innerHTML = suspectGrid.GetTable();
      this.m_tableElement = replaceHtml(this.m_tableElement, this.GetTable());
      this.populateTablePages();
      this.m_renderCallback();
  }
}

// This is an ugly hack which happens to double performance in Chrome for the suspect grid. No diff in firefox
// from here, http://blog.stevenlevithan.com/archives/faster-than-innerhtml
function replaceHtml(el, html) {
    var oldEl = typeof el === "string" ? document.getElementById(el) : el;
    /*@cc_on // Pure innerHTML is slightly faster in IE
      oldEl.innerHTML = html;
      return oldEl;
      @*/
    var newEl = oldEl.cloneNode(false);
    newEl.innerHTML = html;
    oldEl.parentNode.replaceChild(newEl, oldEl);
    /* Since we just removed the old element from the DOM, return a reference
       to the new element, which can be used to restore variable references. */      
    return newEl;
};
