$(function() {

    var joints = [];
    var emergency;
    var viewer;
    var arm;

    function queryStatus() {

        $.ajax('/api/arm/status', {timeout : 100}).done(function(data) {

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

            setTimeout(queryStatus, 500);

        }).fail(function () {

            //setTimeout(queryStatus, 1000);

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

    $('#menu').prepend($('<li>').attr({'id': 'menu-world'})
        .append($('<a>').attr({'href' : '#world'}).data('toggle', 'tab').text("World").tab().click(function (e) {
            $(this).tab('show');
            viewer.view(null);
            worldPanel.title.text("World view");
            return false;
        })));
    
    $('<div class="col-lg-6">').appendTo(worldPanel.container).append(viewer.canvas);

    $('#menu-world a').tab('show');

    $.ajax('/api/camera/describe').done(function(data) {

        var cameraView = $.manus.world.views.camera('/api/camera');

        $('#menu').prepend($('<li>').attr({'id': 'menu-camera'})
            .append($('<a>').attr({'href' : '#camera'}).data('toggle', 'tab').text("Camera").tab().click(function (e) {
            $(this).tab('show');
            viewer.view(cameraView);
            worldPanel.title.text("Camera view");
            return false;
            })));

    }).fail(function () {});

    $.ajax('/api/arm/describe').done(function(data) {

        var sidebar = $('<div class="col-lg-5">').prependTo(worldPanel.container);

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
       // $.manus.world.camera(viewer, '/api/camera');
//        $.manus.world.marker(viewer, vec3.create());
        $.manus.world.grid(viewer, vec3.fromValues(80, 0, 0));
        queryStatus();

    });

});
