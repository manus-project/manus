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

    $.ajax('/api/app').done(function(data) {

        $('#appname').text(data.name);
        $('#appversion').text(data.version);

    });

    $('#tabs').append($('<div>').attr({'class' : 'tab-pane', 'id' : 'world'}));

    var worldPanel = {
        title : $('<h2>').text("World view").appendTo($('#world')),
        container : $('<div>').addClass('row').appendTo($('#world'))
    };

    viewer = $.manus.world.viewer({});
    $('<div class="col-lg-6">').appendTo(worldPanel.container).append(viewer.wrapper);
    $.manus.world.grid(viewer, vec3.fromValues(80, 0, 0));

    $.ajax('/api/camera/describe').done(function(data) {

        var cameraView = $.manus.world.views.camera('camera', '/api/camera/video', data);
        //$.manus.world.camera(viewer, 'camera', data);

        $('#menu').prepend($('<li>').attr({'id': 'menu-camera'})
            .append($('<a>').attr({'href' : '#world'}).data('toggle', 'tab').text("Camera").tab().click(function (e) {
            $(this).tab('show');
            viewer.view(cameraView);
            worldPanel.title.text("Camera view");
            return false;
        	}))
		);

        $.ajax('/api/camera/position').done(function(data) {
            PubSub.publish("camera.update", data);
        });

    }).fail(function () {});

    $('#menu').prepend($('<li>').attr({'id': 'menu-manipulator'})
        .append($('<a>').attr({'href' : '#world'}).data('toggle', 'tab').text("World").tab().click(function (e) {
            $(this).tab('show');
            viewer.view(null);
            worldPanel.title.text("World");
            return false;
    })));

    $.ajax('/api/manipulator/describe').done(function(data) {

		var sidebar = $('<div class="col-lg-4">').prependTo(worldPanel.container);
        var header = $('<div class="header">').appendTo(sidebar);

        /*emergency = $('<button type="button" class="btn emergency">Start</button>').click(function() {
            var command = "stop";
            if ($(this).text() == "Start") command = "start";
            $.ajax('/api/manipulator/' + command);

        });

        header.append(emergency);*/
        header.append($('<div class="information">').text(data.name + " (version: " + data.version.toFixed(2) + ")"));
        for (var v in data["joints"]) {
            joints[v] = $.manus.widgets.jointWidget(sidebar, "manipulator", v, data["joints"][v]);
        }

        $.manus.world.manipulator(viewer, "manipulator", data["joints"]);

        //manipulator.transform(mat4.translate(mat4.create(), mat4.create(), vec3.fromValues(0, 0, 0)));
        
		markers = $.manus.world.markers(viewer);
		markers.clear();

        $.ajax('/api/manipulator/state').done(function(data) {
            PubSub.publish("manipulator.update", data);
        });

    }).fail(function () { });

    var loc = window.location, new_uri;
    if (loc.protocol === "https:") {
        new_uri = "wss:";
    } else {
        new_uri = "ws:";
    }
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

	$('#menu-camera a').tab('show');

});
