
function handleEvent(input, event, handler) {
    if(input.addEventListener)
        input.addEventListener(event, handler, false);
    else
        input.attachEvent("on" + event, handler);
}

/*
function sendJsonRequest (id, url, vs) {
    var s = document.getElementById(id);
    if(s != null)
        document.body.removeChild(s);

    s = document.createElement("script");
    s.setAttribute("type", "text/javascript");
    s.setAttribute("src", url);
    s.setAttribute("id", id);
    document.body.appendChild(s);
}*/



function sendAjaxRequest(callback, url, vs) {


if( typeof XMLHttpRequest == "undefined" ) XMLHttpRequest = function() {
  try { return new ActiveXObject("Msxml2.XMLHTTP.6.0") } catch(e) {}
  try { return new ActiveXObject("Msxml2.XMLHTTP.3.0") } catch(e) {}
  try { return new ActiveXObject("Msxml2.XMLHTTP") } catch(e) {}
  try { return new ActiveXObject("Microsoft.XMLHTTP") } catch(e) {}
  throw new Error( "This browser does not support XMLHttpRequest." )
};

  var request =  new XMLHttpRequest();
  
  request.onreadystatechange = function() {
    if (request.readyState == 4 && request.status == 200) {
      if (request.responseText) {
        callback(request.responseText, vs);
      }
    }
  };
  
  request.open("get", url, true);
  request.send(null);
}


/*function prepareRegex(keys) {
    var regex = "([^A-Za-z0-9_&])(";
    var j = 0;
    for(var k = 0; k < keys.length; k++)
        if(keys[k].length) {
            if(j)regex += "|";
            regex += keys[k].replace(/\W/g, 
                function(sub){return "\\" + sub;});
            j++;
        }
    regex += ")";
    return new RegExp(regex, "gi");
}*/

function prepareRegex(keys, ed) {
    var regex1 = "([^A-Za-z0-9_&])(";
    var regex2 = "([^A-Za-z0-9_&])(";
    var j1 = 0;
    var j2 = 0;
    for(var k = 0; k < keys.length; k++)
        if(keys[k].length)
            if(ed[k] == 0) {
                if(j1)regex1 += "|";
                regex1 += keys[k].replace(/\W/g, 
                    function(sub){return "\\" + sub;});
                j1++;
            }
            else {
                if(j2)regex2 += "|";
                regex2 += keys[k].replace(/\W/g, 
                    function(sub){return "\\" + sub;});
                j2++;
            }

    regex1 += ")";
    regex2 += ")";
    return [ (j1)?new RegExp(regex1, "gi"):null, (j2)?new RegExp(regex2, "gi"):null ];
}

function highlight(string, regexp, stylec1, stylec2) {
    var str = " " + string.replace(/</g, "&lt;");
    if(regexp == null)
        return str;
    if(regexp[0] != null)
        str = str.replace(regexp[0], function(sub, m1, m2) {
                return m1 + "<span class='" + stylec1 + "'>" + m2 + "</span>";
            });
    if(regexp[1] != null)
        str = str.replace(regexp[1], function(sub, m1, m2) {
            return m1 + "<span class='" + stylec2 + "'>" + m2 + "</span>";
        });
    return str.replace(/^\s+/, '');;
}


/*function highlight(string, regexp, stylec) {
    var str = " " + string.replace(/</g, "&lt;");
    if(regexp == null)return str;
    var nstr = str.replace(regexp, function(sub, m1, m2) {
            return m1 + "<span class='" + stylec + "'>" + m2 + "</span>";
        });
    return nstr;
}*/

function truncate(str, regexp, left, right) {
    var pos = str.search(regexp);
    if(pos < 0)pos = 0;

    var i, j;
    i = pos - left;
    if(i < 0)i = 0;
    else while(i < pos && str[i] != ' ')i++;
    j = pos + right;
    if(j > str.length)j = str.length;
    else while(j < str.length && str[j] != ' ')j++;

    var str2 = str.substr(i, j);
    if(i > 0)str2 = " ... " + str2;
    if(j < str.length)str2 = str2 + " ... ";
    return str2;
}

ACUnit.prototype.relocate = function() {
    var left = 0;
    var top = 0;
    var e = this.input;
    while(e) {
        left += e.offsetLeft;
        top += e.offsetTop;
        e = e.offsetParent;
    }
    this.list.style.left = left;
    this.list.style.top = top + this.input.offsetHeight;
    this.list.style.width = this.input.offsetWidth;
}

ACUnit.prototype.colorCurrent = function(color) {
    if(this.current >= 0 && this.current < this.items.length) {
        this.items[this.current].className = color ? this.sitemc : this.itemc;
        
        if(color) {
            var ele = this.items[this.current];
            var top = 0;
            while(ele.offsetParent) {
                var ele2 = ele.parentNode;
                top += ele.offsetTop;
                while(ele2 != ele.offsetParent) {
                    if(ele2.scrollHeight > ele2.clientHeight) {
                        top -= ele2.offsetTop;
                        break;
                    }
                    ele2 = ele2.parentNode;
                }
                if(ele2.scrollHeight > ele2.clientHeight) {
                    if(ele2.scrollTop > top)
                        this.items[this.current].scrollIntoView();
                    else if(top + this.items[this.current].offsetHeight 
                        > ele2.scrollTop + ele2.clientHeight)
                        ele.scrollIntoView(false);
                    break;
                }
                ele = ele2;
            }
        }
    }
}

ACUnit.prototype.clearItems = function() {
    this.items = new Array();
    this.colorCurrent(false);
    this.current = -1;
}

ACUnit.prototype.addItem = function (item) {
    item.className = this.itemc;
    this.items.push(item);
    var i = this.items.length - 1;
    var self = this;
    handleEvent(this.items[i], "mousemove", function () {
        if(self.current != i) {
            self.colorCurrent(false);
            self.current = i;
            self.colorCurrent(true);
        }
    });    
    handleEvent(this.items[i], "click", function () {
        self.onclick(self.current);
    });
}

ACUnit.prototype.addItems = function(parent) {
    for(var i = 0; i < parent.childNodes.length; i++)
        this.addItem(parent.childNodes[i]);
}

ACUnit.prototype.showList = function (show) {
    if(show == null)show = true;
    if(show) {
        this.list.style.visibility = "visible";
    }
    else {
        this.list.style.visibility = "hidden";
        this.colorCurrent(false);
        this.current = -1;
    }
}

function ACUnit(input, listc, itemc, sitemc, onchange, onclick) {    
    this.input = document.getElementById(input);
    this.list = document.createElement("div");
    this.list.className = listc;
    this.list.style.position = "absolute";
    this.list.style.overflow = "auto";
    this.list.style.cursor = "default";
    this.showList(false);
    document.body.appendChild(this.list);
    this.relocate();
    
    this.itemc = itemc;
    this.sitemc = sitemc;
    this.items = new Array();
    this.oldValue = "";
    this.onclick = onclick;

    var self = this;    
    handleEvent(window, "resize", function() {
        self.relocate();
    });
    handleEvent(this.input, "keyup", function () {
        if(self.input.value != self.oldValue) {
            self.oldValue = self.input.value;
            onchange(self.input.value);
        }
    });
    window.setInterval(function () {
        if(self.input.value != self.oldValue) {
            self.oldValue = self.input.value;
            onchange(self.input.value);
        }
    }, 100);
    handleEvent(this.list, "mousedown", function(e) {
        if(e.stopPropagation){
            e.stopPropagation();
            e.preventDefault();
        }
        else self.onclick(self.current);
    });
    handleEvent(this.input, "blur", function () {
        self.showList(false);
    });
    handleEvent(this.input, "keydown", function (event) {
        // esc
        if(event.keyCode == 27) {
            self.showList(false);
        }
        // enter
        else if(event.keyCode == 13) {
            self.onclick(self.current);
        }
        // up
        else if(event.keyCode == 38 && self.current > 0) {
            self.colorCurrent(false);
            self.current--;
            self.colorCurrent(true);
        }
        // down
        else if(event.keyCode == 40 && self.current < self.items.length - 1) {
            self.list.style.visibility = "visible";
            self.colorCurrent(false);
            self.current++;
            self.colorCurrent(true);
        }
    });
}

function textPreview(string, regexp, padding, maxLength, noMatchesLength) {
    var str = " " + string.replace(/</g, "&lt;");
    if(regexp == null)
        return str;
        
    var matches = [];
       
    if(regexp[0] != null)
        str = str.replace(regexp[0], function(sub, m1, m2, offset) {
                matches.push({start: offset, length: m2.length});
                return sub;
            });
    if(regexp[1] != null)
        str = str.replace(regexp[1], function(sub, m1, m2, offset) {
            matches.push({start: offset, length: m2.length});
            return sub;
        });
        
    
    matches.sort(function(a,b) {
    
        return a.start - b.start;
    
    });
    
    
    var snippets = [];
    var curLength = 0;
    
    var i;
    for(i=0;i<matches.length;i++) {
        
        if (i > 0 && matches[i].start - padding <= snippets[snippets.length-1].end + 1) {
            // Just extend previous snippet
            curLength -= snippets[snippets.length-1].end - snippets[snippets.length-1].start + 1;
            snippets[snippets.length-1].end = Math.min(string.length-1, matches[i].start + matches[i].length + padding - 1);
            curLength += snippets[snippets.length-1].end - snippets[snippets.length-1].start + 1;
        } else {
            snippets.push({
                start: Math.max(0, matches[i].start - padding),
                end: Math.min(string.length-1, matches[i].start + matches[i].length + padding - 1)
            });
            // Don't truncate the word at the beginning of the snippet
            var added = snippets[snippets.length-1];
            if (!/\s/.test(string.charAt(added.start))) {
                while(added.start > 1 && !/\s/.test(string.charAt(added.start-1))) {
                    added.start--;
                }
            }
            curLength += snippets[snippets.length-1].end - snippets[snippets.length-1].start + 1;
        }
        
        if (curLength >= maxLength) {
            break;
        }
        
    }
    
    // join snippets and add '...'
    var ret = '';
    if (snippets.length > 0 && snippets[0].start > 0) {
        ret += '&hellip;';
    }
    var lastEnd = 0;
    for(i=0;i<snippets.length;i++) {
        ret += (ret == '' || ret == '&hellip;' ? '' : '&hellip;') + string.substring(snippets[i].start,snippets[i].end+1)
        lastEnd = snippets[i].end;
    }
    
    // If we have not reached the end of the string
    if (ret != '' && lastEnd < string.length-1) {
        // if we have still space... expand the last snippet
        if (ret.length < maxLength) {
            ret += string.substr(lastEnd+1, maxLength - ret.length);
            if (lastEnd + (maxLength - ret.length) < string.length - 1) {
                ret += '&hellip;';
            }
        } else {
            // Truncate to the curLength (which is close to maxLength since we stopped immediately after curLength was bigger then maxLength
            ret = ret.substr(0, curLength) + '&hellip;';
        }
    }
    
    // If there were no matches in the string just truncate it at noMatchesLength from the beginning
    if (ret == '') {
        ret = string.substr(0, noMatchesLength) + (noMatchesLength < string.length ? '&hellip;' : '');
    }
    
    return ret;
    
}

function searchIndex(searchElement, theArray)
{
    if (theArray === void 0 || theArray === null)
      throw new TypeError();

    var t = Object(theArray);
    var len = t.length >>> 0;
    if (len === 0)
      return -1;

    var n = 0;
    if (arguments.length > 0)
    {
      n = Number(arguments[1]);
      if (n !== n) // shortcut for verifying if it's NaN
        n = 0;
      else if (n !== 0 && n !== (1 / 0) && n !== -(1 / 0))
        n = (n > 0 || -1) * Math.floor(Math.abs(n));
    }

    if (n >= len)
      return -1;

    var k = n >= 0
          ? n
          : Math.max(len - Math.abs(n), 0);

    for (; k < len; k++)
    {
      if (k in t && t[k] === searchElement)
        return k;
    }
    return -1;
 }






