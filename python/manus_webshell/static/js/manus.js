
function uniqueIdentifier() {
  return Math.round(new Date().getTime() + (Math.random() * 100));
}

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

        tools.append($('<i />').addClass('glyphicon glyphicon-pencil').click(function() {

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

        tools.append($('<i />').addClass('glyphicon glyphicon-trash').click(function() {
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

    $.ajax('/api/storage?key=poses').done(function(data) {

        updating = true;
        list.add(data);
        updating = false;
        
    });

    PubSub.subscribe("manipulator.update", function(msg, data) {

        currentPose = data;        

    });

    PubSub.subscribe("storage.update", function(msg, key) {

        if (key != "poses") 
            return;

        $.ajax('/api/storage?key=poses').done(function(data) {

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

        $.ajax({
            'type': 'POST',
            'url': '/api/storage?key=poses',
            'contentType': 'application/json',
            'data': JSON.stringify(data),
            'dataType': 'json'
        });

    });

    $("#poseslist").append($.manus.widgets.buttons({
        add : {
            text: "Save pose",
            callback: function() {
                if (!currentPose) return;
                index++;
                list.add([{
                    identifier : uniqueIdentifier(),
                    name : "New pose " + index,
                    pose: currentPose
                }]);
            } 
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

        tools.append($('<i />').addClass('glyphicon glyphicon-play').click(function() {
            if ($(this).hasClass('glyphicon-play')) {

                $.ajax('/api/apps?run=' + item.values().identifier).done(function(data) {});

            } else {

                $.ajax('/api/apps?stop=' + item.values().identifier).done(function(data) {});

            }
            return false;
        }));

    }

    var updating = false;

    var list = new List("appslist", {
        valueNames : ["name", "version", "description"],
        item: "<a class='list-group-item'><div class='name'></div><div class='version'></div><pre class='description'></pre></a>"
    }, []);

    $.ajax('/api/apps').done(function(data) {
        items = [];
        for (var key in data) { items.push(data[key]); }
        list.add(items);
    });

    var index = 0;

    list.on("updated", function() {

        for (var i = 0; i < list.items.length; i++) {
            augmentItem(list.items[i]);
        }

    });

    PubSub.subscribe("apps.activated", function(msg, identifier) {

        $(list.listContainer).children(".glyphicon-stop").removeClass("glyphicon-stop").addClass("glyphicon-play");

        $(list.listContainer).children("#app-" + identifier).removeClass("glyphicon-play").addClass("glyphicon-stop");

    });

}

function showOverlay(title, message) {

    $('#overlay .modal-title').text(title);
    $('#overlay .modal-body').text(message);

    $('#overlay').modal('show');
}

function hideOverlay() {
    $('#overlay').modal('hide');
}

$(function() {

    var joints = [];
    var emergency;
    var viewer;
    var manipulator;
	var markers;

    $('#overlay').modal({backdrop: 'static', keyboard : false, show: false});

    showOverlay("Loading ...", "Please wait, the interface is loading.");

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

    viewer = $.manus.world.viewer({});
    $('#viewer').append(viewer.wrapper);
    $.manus.world.grid(viewer, vec3.fromValues(80, 0, 0));
    var cameraView = null;

    var updateViewer = function() {
        viewer.resize($('#viewer').width(), $('#viewer').width() * 0.75);
    }

    $( window ).resize(updateViewer);
    updateViewer();

    $.ajax('/api/camera/describe').done(function(data) {

        cameraView = $.manus.world.views.camera('camera', '/api/camera/video', data);

        $.ajax('/api/camera/position').done(function(data) {
            PubSub.publish("camera.update", data);
        });

    }).fail(function () {});

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
    }).addClass('tools').appendTo($('#viewer'));


    /* Manipulator stuff */

    $.ajax('/api/manipulator/describe').done(function(data) {

        var container = $("#controls");
        var header = $('<div class="header">').appendTo(container);

        $('#manipulator').text(data.name + " (version: " + data.version.toFixed(2) + ")");

        for (var v in data["joints"]) {
            joints[v] = $.manus.widgets.jointWidget(container, "manipulator", v, data["joints"][v]);
        }

        $.manus.world.manipulator(viewer, "manipulator", data["joints"]);

		markers = $.manus.world.markers(viewer);
		markers.clear();

        $.ajax('/api/manipulator/state').done(function(data) {
            PubSub.publish("manipulator.update", data);
        });

    }).fail(function () { });

    posesList();

    appsList();

    /* Websocket events connection */

    var loc = window.location, new_uri;
    if (loc.protocol === "https:") { new_uri = "wss:"; } else { new_uri = "ws:"; }
    new_uri += "//" + loc.host;
    var socket = new WebSocket(new_uri + "/api/websocket");

    socket.onopen = function(event) {
        console.log("open");
        hideOverlay();
    }

    socket.onerror = function(event) {
        showOverlay("Connection lost", "Unable to communicate with the system.");
    }

    socket.onclose = function(event) {
        showOverlay("Connection lost", "Unable to communicate with the system.");
    }

    socket.onmessage = function (event) {
        var msg = JSON.parse(event.data);

        if (msg.channel == "camera") {

            PubSub.publish("camera.update", msg.data);

        } else if (msg.channel == "manipulator") {

            PubSub.publish("manipulator.update", msg.data);
            
        } else if (msg.channel == "storage") {

            PubSub.publish("storage.update", msg.key);
            
        } else if (msg.channel == "apps") {

            if (msg.action == "activated") {
                PubSub.publish("apps.activated", msg.identifier);
            } 

        }

    }

    PubSub.subscribe("manipulator.move_joint", function(msg, data) {

        $.ajax('/api/manipulator/move_joint?id=' + data.id + '&speed=' + data.speed + '&position=' + data.position);

    });

});
