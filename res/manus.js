$(function() {

    var joints = [];
    var emergency;
    var viewer;
    var arm;
	var markers;

    function queryArmStatus() {

        $.ajax('/api/arm/status', {timeout : 300}).done(function(data) {

            for (var v in data["joints"]) {
                joints[v](data["joints"][v], data["goals"][v]);
            }

            arm.update(data["joints"]);

            if (data["state"] == "active") {
                emergency.removeClass("btn-success").addClass("btn-danger");
                emergency.text("Stop");
            } else {
                emergency.removeClass("btn-danger").addClass("btn-success");
                emergency.text("Start");
            }

            setTimeout(queryArmStatus, 250);

        }).fail(function () {

            setTimeout(queryArmStatus, 1000);

        });

    }

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

    $.ajax('/api/app/information').done(function(data) {

        $('#appname').text(data.name);
        $('#appversion').text(data.version);
        $('#appbuild').text(data.build);

    });

    $('#tabs').append($('<div>').attr({'class' : 'tab-pane', 'id' : 'world'}));

    var worldPanel = {
        title : $('<h2>').text("World view").appendTo($('#world')),
        container : $('<div>').addClass('row').appendTo($('#world'))
    };

    viewer = $.manus.world.viewer({});

    $('<div class="col-lg-6">').appendTo(worldPanel.container).append(viewer.wrapper);

    $.ajax('/api/camera/describe').done(function(data) {

        var cameraView = $.manus.world.views.camera('/api/camera');

        $('#menu').prepend($('<li>').attr({'id': 'menu-camera'})
            .append($('<a>').attr({'href' : '#world'}).data('toggle', 'tab').text("Camera").tab().click(function (e) {
            $(this).tab('show');
            viewer.view(cameraView);
            worldPanel.title.text("Camera view");
            return false;
        	}))
		);

    }).fail(function () {});

    $.ajax('/api/arm/describe').done(function(data) {

    	$('#menu').prepend($('<li>').attr({'id': 'menu-arm'})
		    .append($('<a>').attr({'href' : '#world'}).data('toggle', 'tab').text("Arm").tab().click(function (e) {
		        $(this).tab('show');
		        viewer.view(null);
		        worldPanel.title.text("Arm status");
		        return false;
        })));

		var sidebar = $('<div class="col-lg-4">').prependTo(worldPanel.container);
        var header = $('<div class="header">').appendTo(sidebar);

        emergency = $('<button type="button" class="btn emergency">Start</button>').click(function() {
            var command = "stop";
            if ($(this).text() == "Start") command = "start";
            $.ajax('/api/arm/' + command);

        });

        header.append(emergency);

        header.append($('<div class="information">').text(data.name + " (version: " + data.version.toFixed(2) + ")"));

        for (var v in data["joints"]) {
            joints[v] = $.manus.widgets.jointWidget(sidebar, parseInt(v), data["joints"][v]);
        }

        arm = $.manus.world.arm(viewer, data["joints"]);

        arm.transform(mat4.translate(mat4.create(), mat4.create(), vec3.fromValues(0, 0, 0)));
        $.manus.world.grid(viewer, vec3.fromValues(80, 0, 0));

		markers = $.manus.world.markers(viewer);
		markers.clear();

        queryArmStatus();
		queryMarkerStatus();

    }).fail(function () {

            //setTimeout(queryStatus, 1000);

    });

	$('#menu-camera a').tab('show');

});
