
//This js file is used to show the dropdown list.
function handleEvent(input, event, handler) {
    if(input.addEventListener)
        input.addEventListener(event, handler, false);
    else
        input.attachEvent("on" + event, handler);
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
    //this.list.style.left = left;
    //this.list.style.top = top + this.input.offsetHeight;
    //this.list.style.width = this.input.offsetWidth;
    this.list.style.left = left+"px";
    this.list.style.top = top + this.input.offsetHeight+"px";    this.list.style.width = this.input.offsetWidth+"px";
}

ACUnit.prototype.colorCurrent = function(color) {
    if(this.current >= 0 && this.current < this.items.length) {
	if(this.current%2)
{
	this.items[this.current].className = color ? this.sitemc : this.itemc1;
}
else
{
	this.items[this.current].className = color ? this.sitemc : this.itemc2;
}
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
    this.items.push(item);
    var i = this.items.length - 1;
if (i%2)
{
    item.className = this.itemc1;
}
else
{
    item.className = this.itemc2;
}
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
    this.relocate();
    if(show == null)show = true;
    if(show) {
        this.list.style.visibility = "visible";
	this.current = 0;
	this.colorCurrent(true);
    }
    else {
        this.list.style.visibility = "hidden";
        this.colorCurrent(false);
        this.current = -1;
    }
}

function ACUnit(input, listc, itemc1,itemc2, sitemc, onchange, onclick) {    
    this.input = document.getElementById(input);
    this.list = document.createElement("div");
    this.list.className = listc;
    this.list.style.position = "absolute";
    /**this.list.style.overflow = "auto";**/
    this.list.style.cursor = "default";
    this.showList(false);
    document.body.appendChild(this.list);
    this.relocate();
    
    this.itemc1 = itemc1;
    this.itemc2 = itemc2;
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
//        self.showList(false);
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







