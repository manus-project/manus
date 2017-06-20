
function posesList() {

    var currentPose = null;

    var list = new List("poseslist", {
        valueNames : ["name"],
        item: "<a class='list-group-item'><span class='name'></span></a>"

    }, []);

    $.ajax('/api/storage?key=poses').done(function(data) {

        list.add(data);
        
    });

    PubSub.subscribe("manipulator.update", function(msg, data) {

        currentPose = data;        

    });

    var index = 0;

    list.on("updated", function() {

        var data = [];
        for (var i = 0; i < list.items.length; i++)
            data.push(list.items[i].values());
        $.ajax({
            'type': 'POST',
            'url': '/api/storage?key=poses',
            'contentType': 'application/json',
            'data': JSON.stringify(data),
            'dataType': 'json'
        });

    });

    $("#poseslist").prepend($.manus.widgets.buttons({
        add : {
            text: "Save pose",
            callback: function() {
                if (!currentPose) return;
                index++;
                list.add([{
                    index : index,
                    name : "New pose " + index,
                    pose: currentPose
                }]);
                var item = list.items[list.items.length-1];
                var container = $(list.items[list.items.length-1].elm);
                var tools = $('<div />').addClass('list-tools').appendTo(container);

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
                    list.remove("index", item.values().index);
                    return false;
                }));

            } 
        }

    }));

}

$(function() {

    var joints = [];
    var emergency;
    var viewer;
    var manipulator;
	var markers;

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

    function update_apps(data) {
        $('#app-list').empty();
        $.each(data, function(i, a) {
            $('#app-list').append($('<li/>').append($("<a />").text(a).click(function (e) {

                $.ajax('/api/apps?run=' + i).done(function(data) {
                    update_apps(data);
                });

            })));
        });

    }

    $.ajax('/api/apps').done(function(data) {

        update_apps(data);
        
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

    /* Websocket events connection */

    var loc = window.location, new_uri;
    if (loc.protocol === "https:") { new_uri = "wss:"; } else { new_uri = "ws:"; }
    new_uri += "//" + loc.host;
    var socket = new WebSocket(new_uri + "/api/websocket");

    socket.onmessage = function (event) {
        var msg = JSON.parse(event.data);

        if (msg.type == "update") {
        
            if (msg.object == "camera") {

                PubSub.publish("camera.update", msg.data);

            } else if (msg.object == "manipulator") {

                PubSub.publish("manipulator.update", msg.data);
                
            }

        }

    }

    PubSub.subscribe("manipulator.move_joint", function(msg, data) {

        $.ajax('/api/manipulator/move_joint?id=' + data.id + '&speed=' + data.speed + '&position=' + data.position);

    });

});
