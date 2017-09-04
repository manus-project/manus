
function uniqueIdentifier() {
  return Math.round(new Date().getTime() + (Math.random() * 100));
}

function formatDateTime(date) {
  var monthNames = [
    "January", "February", "March",
    "April", "May", "June", "July",
    "August", "September", "October",
    "November", "December"
  ];

  var day = date.getDate();
  var monthIndex = date.getMonth();
  var year = date.getFullYear();

  return day + ' ' + monthNames[monthIndex] + ' ' + year + " " + date.getHours() + ":" + date.getMinutes();
}

RemoteStorage = {
    get : function(key, callback) {
        $.ajax('/api/storage?key=' + key).done(function(data) {
            callback(key, data);
        });

    },
    list: function(callback) {
        $.ajax('/api/storage').done(function(data) {
            callback(data);
        });
    },
    set: function(key, value) {
        $.ajax({
            'type': 'POST',
            'url': '/api/storage?key=' + key,
            'contentType': 'application/json',
            'data': JSON.stringify(value),
            'dataType': 'json'
        });
    },
    delete(key) {
        $.ajax({
            'type': 'POST',
            'url': '/api/storage?key=' + key,
            'contentType': 'application/json',
            'data': '',
            'dataType': 'json'
        });
    }
};

Interface = {

    init: function() {
        $('#block-overlay').modal({backdrop: 'static', keyboard : false, show: false});
        $('#block-dialog').modal({backdrop: 'static', keyboard : true, show: false});
    },

    overlay: function (title, message) {

        if (title === undefined) {
            $('#block-overlay').modal('hide');

        } else {
            $('#block-overlay .modal-title').text(title);
            $('#block-overlay .modal-body').text(message);

            $('#block-overlay').modal('show');
        }

    },

    confirmation: function (title, message, callback) {

        $('#block-dialog .modal-title').text(title);
        $('#block-dialog .modal-body').text(message);
        $('#block-dialog .modal-footer').empty();

        $('#block-dialog .modal-footer').append($("<button/>")
            .addClass("btn btn-default").text("Cancel")
            .click(function() { $('#block-dialog').modal('hide'); }));

        $('#block-dialog .modal-footer').append($("<button/>")
            .addClass("btn btn-primary").text("Confirm")
            .click(function() { 
                callback();
                $('#block-dialog').modal('hide');
            }));

        $('#block-dialog').modal('show');
    },

    notification: function (title, message) {

        $('#block-dialog .modal-title').text(title);
        $('#block-dialog .modal-body').text(message);
        $('#block-dialog .modal-footer').empty();

        $('#block-dialog .modal-footer').append($("<button/>")
            .addClass("btn btn-default").text("Close")
            .click(function() { $('#block-dialog').modal('hide'); }));

        $('#block-dialog').modal('show');
    }

};

function posesList() {

    var augmentItem = function(item) {
        var container = $(item.elm);
        if (container.hasClass('pose-item'))
            return;

        container.addClass('pose-item');
        var tools = $('<div />').addClass('list-tools').prependTo(container);

        container.click(function () {

            var data = item.values().pose;
            
            var params = "";
            for (var i = 0; i < data.joints.length; i++)
                params += "j" + (i+1) + "=" + data.joints[i].position + "&";
            $.ajax('/api/manipulator/move?' + params);

        });

        tools.append($('<i />').addClass('tool glyphicon glyphicon-pencil').click(function() {

            if (container.hasClass('editable'))
                return;

            container.addClass('editable');
            var textbox = container.children(".name");
            textbox.text(item.values().name);
            textbox.attr('contenteditable', 'true');

            var sel = window.getSelection();

            textbox.bind('blur', function() {
                textbox.attr('contenteditable', 'false');
                container.removeClass('editable');
            }).focus();

            var sel = window.getSelection();
            var range = document.createRange();
            range.setStart(textbox[0], 1);
            range.collapse(true);
            sel.removeAllRanges();
            sel.addRange(range);
            
            textbox.bind('keyup', function(event) {
                if(event.keyCode == 13) {
                    container.removeClass('editable');
                    item.values({"name": textbox.text()});
                    list.update();
                    event.stopPropagation();
                } else if (event.keyCode == 27) {
                    textbox.attr('contenteditable', 'false');
                    container.removeClass('editable');
                    textbox.text(item.values().name);
                    container.focus();
                    event.stopPropagation();
                }
                return true;
            });

            return false;
        }));

        tools.append($('<i />').addClass('tool glyphicon glyphicon-trash').click(function() {
            list.remove("identifier", item.values().identifier);
            return false;
        }));

    }

    var currentPose = null;
    var updating = false;

    var list = new List("poseslist", {
        valueNames : ["name"],
        item: "<a class='list-group-item'><div class='name'></div></a>"
    }, []);

    RemoteStorage.get("poses", function(key, data) {

        updating = true;
        list.clear();
        list.add(data);
        updating = false;
        
    });

    PubSub.subscribe("manipulator.update", function(msg, data) {

        currentPose = data;        

    });

    PubSub.subscribe("storage.update", function(msg, key) {

        if (key != "poses") 
            return;

        RemoteStorage.get("poses", function(key, data) {

            updating = true;
            list.clear();
            list.add(data);
            updating = false;
            
        });

    });

    var index = 0;

    list.on("updated", function() {

        if (list.items.length > 0) {
            $(list.listContainer).children(".alert").hide();
        } else {
            $(list.listContainer).children(".alert").show();
        }

        var data = [];
        for (var i = 0; i < list.items.length; i++) {
            data.push(list.items[i].values());
            augmentItem(list.items[i]);
        }

        if (updating) return;

        RemoteStorage.set("poses", data);

    });

    $("#poseslist").append($.manus.widgets.fancybutton({
        icon: "plus", tooltip: "Add current pose",
        callback: function() {
            if (!currentPose) return;
            index++;
            list.add([{
                identifier : uniqueIdentifier(),
                name : "New pose " + index,
                pose: currentPose
            }]);
        } 
    }));

}

function appsList() {

    var augmentItem = function(item) {
        var container = $(item.elm);
        if (container.hasClass('pose-item'))
            return;

        container.attr("id", "app-" + item.values().identifier);

        container.addClass('apps-item');

        container.click(function () {
            if (container.hasClass("expanded")) {
                container.removeClass("expanded");
            } else {
                container.siblings().removeClass("expanded");
                container.addClass("expanded");
            }
        });

        var tools = $('<div />').addClass('list-tools').prependTo(container);

        tools.append($('<i />').addClass('tool glyphicon glyphicon-play').click(function() {
            if ($(this).hasClass('glyphicon-play')) {

                $.ajax('/api/apps?run=' + item.values().identifier).done(function(data) {});

            } else {

                $.ajax('/api/apps?run=').done(function(data) {});

            }
            return false;
        }).tooltip({title: "Run/Stop", delay: 1}));
        
    }

    var updating = false;

    var list = new List("appslist", {
        valueNames : ["name", "version", "description"],
        item: "<a class='list-group-item'><div class='name'></div><div class='version'></div><pre class='description'></pre></a>"
    }, []);

    var index = 0;

    list.on("updated", function() {

        for (var i = 0; i < list.items.length; i++) {
            augmentItem(list.items[i]);
        }

    });

    $.ajax('/api/apps').done(function(data) {
        items = [];
        for (var key in data.list) { items.push(data.list[key]); }
        list.add(items);
        changeActive(data.active);
    });

    var changeActive = function(identifier) {

        $(list.listContainer).find("i.glyphicon-stop").removeClass("glyphicon-stop").addClass("glyphicon-play");
        if (identifier)
            $("#app-" + identifier + " i.tool").removeClass("glyphicon-play").addClass("glyphicon-stop");

    }

    PubSub.subscribe("apps.active", function(msg, identifier) {changeActive(identifier)});

}

$(function() {

    var joints = [];
    var emergency;
    var viewer;
    var manipulator;
	var markers;

    Interface.init();
    Interface.overlay("Loading ...", "Please wait, the interface is loading.");

    function queryMarkerStatus() {

        $.ajax('/api/markers/get', {timeout : 300}).done(function(data) {

			markers.clear();
			for (var m in data) {
				markers.add(data[m]["position"], data[m]["orientation"], data[m]["color"]);
			}

            setTimeout(queryMarkerStatus, 250);

        }).fail(function () {

            setTimeout(queryMarkerStatus, 1000);

        });

    }

    $.ajax('/api/info').done(function(data) {

        $('#appname').text(data.name);
        $('#appversion').text(data.version);

    });



    /* Camera stuff */

    var viewer = $.manus.world.viewer({});
    $('#viewer').append(viewer.wrapper);
    $.manus.world.grid(viewer, vec3.fromValues(80, 0, 0));

    var updateViewer = function() {
        viewer.resize($('#viewer').width(), $('#viewer').width() * 0.75);
    }

    var markers = $.manus.world.markers(viewer);

    $( window ).resize(updateViewer);
    updateViewer();

    $.ajax('/api/camera/describe').done(function(data) {

        cameraView = $.manus.world.views.camera('camera', '/api/camera/video', data);

        $.ajax('/api/camera/position').done(function(data) {
            PubSub.publish("camera.update", data);
        });

        $.manus.widgets.buttons({
            free: {
                text: "World",
                callback: function(e) {
                    viewer.view(null);
                }
            },
            camera: {
                text: "Camera",
                callback: function(e) {
                    if (cameraView) viewer.view(cameraView);
                }
            }
        }).addClass('toolbar').prependTo($('#viewer'));

    }).fail(function () {});

    /* Manipulator stuff */

    $.ajax('/api/manipulator/describe').done(function(data) {

        var container = $("#controls");
        var header = $('<div class="header">').appendTo(container);

        $('#manipulator').text(data.name + " (version: " + data.version.toFixed(2) + ")");

        var id = 1;
        for (var v in data["joints"]) {
            if (data["joints"][v].type.toLowerCase() == "fixed")
                continue;
            joints[v] = $.manus.widgets.jointWidget(container, "manipulator", v, "Joint " + id, data["joints"][v]);
            id = id + 1;
        }

        $.manus.world.manipulator(viewer, "manipulator", data["joints"]);

		markers = $.manus.world.markers(viewer);
		markers.clear();

        $.ajax('/api/manipulator/state').done(function(data) {
            PubSub.publish("manipulator.update", data);
        });

    }).fail(function () {

    });

    posesList();

    appsList();

    /* Websocket events connection */

    var loc = window.location, new_uri;
    if (loc.protocol === "https:") { new_uri = "wss:"; } else { new_uri = "ws:"; }
    new_uri += "//" + loc.host;
    var socket = new WebSocket(new_uri + "/api/websocket");

    function waitForConnection() {

        $.ajax('/api/info').done(function(data) {

            location.reload();    

        }).fail(function () {

            setTimeout(waitForConnection, 1000);

        });

    }

    var reconnect = true;

    $(window).on('beforeunload', function(){
          reconnect = false;
    });

    socket.onopen = function(event) {
        Interface.overlay();
    }

    socket.onerror = function(event) {
        if (!reconnect) return;
        Interface.overlay("Connection lost", "Unable to communicate with the system.");
        waitForConnection();
    }

    socket.onclose = function(event) {
        if (!reconnect) return;
        Interface.overlay("Connection lost", "Unable to communicate with the system.");
        waitForConnection();
    }

    var err_count = 0;
    socket.onmessage = function (event) {
        var msg;
        try {
            msg = JSON.parse(event.data);
        }catch(err) {
            err_count++;
            if (err_count > 200){
                console.error("Failed to parse JSON in onmessage: " + err.message);
                err_count = 0;
            }
            return; 
        }

        if (msg.channel == "camera") {

            PubSub.publish("camera.update", msg.data);

        } else if (msg.channel == "manipulator") {

            PubSub.publish("manipulator.update", msg.data);
            
        } else if (msg.channel == "storage") {

            if (msg.action == "update") {
                PubSub.publish("storage.update", msg.key);
            } else if (msg.action == "delete") {
                PubSub.publish("storage.delete", msg.key);
            }

        } else if (msg.channel == "apps") {

            if (msg.action == "activated") {
                PubSub.publish("apps.active", msg.identifier);
            } else if (msg.action == "deactivated") {
                PubSub.publish("apps.active", undefined);
            } else if (msg.action == "log") {
                PubSub.publish("apps.log", {identifier: msg.identifier, lines: msg.lines});
            } 

        } else if (msg.channel == "markers") {

            markers.clear();
            for (i in msg.markers) {
                var marker = msg.markers[i];
                markers.add(marker.location, marker.rotation, [20, 20, 20], [0, 255, 0]);
            }

        }

    }

    PubSub.subscribe("manipulator.move_joint", function(msg, data) {

        $.ajax('/api/manipulator/move_joint?id=' + data.id + '&speed=' + data.speed + '&position=' + data.position);

    });

});
