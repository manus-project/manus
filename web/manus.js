$(function() {

    function robotArmVisualizer(parent, parameters) {

        var canvas = $('<canvas class="visualization" width="600" height="600">');
        $(parent).append(canvas);

        canvas = canvas[0];

         // create the scene and setup camera, perspective and viewport
        var scene = new Phoria.Scene();
        scene.camera.position = {x:0.0, y:0.0, z:1.0};
        scene.camera.up = {x:0.0, y:0.0, z:1.0};
        scene.perspective.aspect = canvas.width / canvas.height;
        scene.viewport.width = canvas.width;
        scene.viewport.height = canvas.height;
        // create a canvas renderer
        var renderer = new Phoria.CanvasRenderer(canvas);

        // add a grid to help visualise camera position etc.
        var plane = Phoria.Util.generateTesselatedPlane(8,8,0,20);
        
        scene.graph.push(Phoria.Entity.create({
            points: plane.points,
            edges: plane.edges,
            polygons: plane.polygons,
            style: {
                drawmode: "wireframe",
                shademode: "plain",
                linewidth: 0.5,
                objectsortmode: "back"
            }
            }
        ).rotateX(90 * Phoria.RADIANS));

        var dirty = true;

        var cameraSphere = {"x" : 10, "y" : 90, "distance": 80};

        var mouse = {"dragging" : false, "clickX" : 0, "clickY" : 0, "dragX" : 0, "dragY" : 0};
        mouse.onMouseMove = function onMouseMove(evt) {
            mouse.dragging = true;
         	mouse.dragX = evt.clientX - mouse.clickX;
         	mouse.dragY = evt.clientY - mouse.clickY;
         };
             
         mouse.onMouseUp = function onMouseUp(evt) {
            mouse.endDragging();
         	canvas.removeEventListener('mousemove', mouse.onMouseMove, false);
         };
         
         mouse.onMouseOut = function onMouseOut(evt) {
            mouse.endDragging();
         	canvas.removeEventListener('mousemove', mouse.onMouseMove, false);
         };
         
         mouse.onMouseDown = function onMouseDown(evt) {
         	evt.preventDefault();
         	canvas.addEventListener('mousemove', mouse.onMouseMove, false);
         	mouse.clickX = evt.clientX;
         	mouse.clickY = evt.clientY;
         };

        mouse.endDragging = function() {

            mouse.dragging = false;
            cameraSphere.x = mouse.dragX;
            cameraSphere.y = mouse.dragY;
        }
     
        canvas.addEventListener('mousedown', mouse.onMouseDown, false);
        canvas.addEventListener('mouseup', mouse.onMouseUp, false);
        canvas.addEventListener('mouseout', mouse.onMouseOut, false);

        cameraSphere.update = function() {
                var x = this.x + mouse.dragX;
                var y = this.y + mouse.dragY;

                var rotation = mat4.create();
                rotation = mat4.rotateX(rotation, rotation, -y * Phoria.RADIANS);
                rotation = mat4.rotateY(rotation, rotation, x * Phoria.RADIANS);

                var position = vec4.fromValues(0, 0, this.distance, 1);
                position = vec4.transformMat4(position, position, rotation);

                scene.camera.position.x = position[0];
                scene.camera.position.y = position[1];
                scene.camera.position.z = position[2];


        }

        function render() {

            if (mouse.dragging) {
                cameraSphere.update();
                dirty = true;
            }

            if (dirty) {
                scene.modelView();
                renderer.render(scene);
                dirty = false;
            }

            setTimeout(render, 100);

        }

        render();
        cameraSphere.update();

        var joints = [];

        var jointMesh = Phoria.Util.generateUnitCube(1.5);

        var parentJoint;

        for (var v in parameters["joints"]) {

            var a = parameters["joints"][v].a / 10;
            var d = parameters["joints"][v].d / 10;
            var alpha = parameters["joints"][v].alpha;
            var theta = parameters["joints"][v].theta;

            var segmentMesh = Phoria.Util.generateCuboid({"scalex" : Math.max(1, a / 2), "scaley" : 1, "scalez" : 1, "offsetx" : - Math.max(1, a / 2), "offsety" : 0, "offsetz": 0});

            var segment = Phoria.Entity.create({
                points: segmentMesh.points,
                edges: segmentMesh.edges,
                polygons: segmentMesh.polygons,
                style: {
                    drawmode: "wireframe",
                    shademode: "plain",
                    linewidth: 1,
                }
            });



            var joint = Phoria.Entity.create({
                id : "Joint" + v,
                points: jointMesh.points,
                edges: jointMesh.edges,
                polygons: jointMesh.polygons,
                style: {
                    color: [100,100,255],
                    drawmode: "wireframe",
                    shademode: "plain",
                    linewidth: 2,
                }
            });

            joints[v] = {"segment" : segment, "joint" : joint, "data" : parameters["joints"][v]};
            if (parameters["joints"][v].type != "rotation") break;


            joint.identity().rotateZ(theta).translateZ(d);

            segment.identity().rotateX(alpha).translateX(a);

            /*Phoria.Entity.debug(joint, {
                showId: true,
                showAxis: true,
                showPosition: true
            });*/

            if (parentJoint === undefined) {
                scene.graph.push(joint);
            } else {
                parentJoint.children.push(joint);
            }

            joint.children.push(segment);
            parentJoint = segment;
            
        }


        scene.graph.push(new Phoria.DistantLight());

        return function(status) {

            for (var v in status["joints"]) {
                var d = joints[v].data.d / 10;
                var theta = joints[v].data.theta;
                var update = true;

                switch (joints[v].data.type) {
                case "rotation": {
                    theta = status["joints"][v];
                    break;
                }
                case "translation": {
                    d = status["joints"][v] / 10;
                    break;
                }
                default: {
                    update = false;
                    break;
                }

                }

                if (!update) break;

                joints[v].joint.identity().rotateZ(theta).translateZ(d);

            }

            dirty = true;

        };

    }

    function createJointController(parent, id, parameters) {

        var value = 0;

        var status;
        var information = $('<div class="information">').append($('<span class="title">').text("Joint " + id)).append($('<span class="type">').text("Type: " + parameters.type));

        var container = $('<div class="joint">').append(information);

        switch (parameters.type) {
        case "translation":
        case "rotation": {

            status = $('<span class="status">').text("0");

            container.append($('<button type="button" class="btn btn-primary">&lt;</button>').click(function() {

                $.ajax('/api/arm?command=move&joint=' + id + '&position=' + (value - 0.5));
                
            }));

            container.append(status);

            container.append($('<button type="button" class="btn btn-primary">&gt;</button>').click(function() {
                
                $.ajax('/api/arm?command=move&joint=' + id + '&position=' + (value + 0.5));

            }));
            break;
        }
        case "gripper_dual": {

            status = $('<button type="button" class="btn">&gt;</button>').click(function() {
                if (value > 0.5) value = 0;
                else value = 1;                

                $.ajax('/api/arm?command=move&joint=' + id + '&position=' + value);

            });

            container.append(status);
            break;
        }

        }


        $(parent).append(container);

        return function(v) {
            
            value = parseFloat(v);

            switch (parameters.type) {
            case "translation": {
                status.html(value.toFixed(2) + "mm");
                break;
            }
            case "rotation": {
                status.html(((value * 180) / Math.PI ).toFixed(2) + "&deg;");
                break;
            }
            case "gripper_dual": {

                if (value < 1) {
                    status.removeClass("btn-success").addClass("btn-danger");
                    status.text("Grip");
                } else {
                    status.removeClass("btn-danger").addClass("btn-success");
                    status.text("Release");
                }
                break;
            }

            }

        };

    }

    
    var joints = [];
    var visualizer;

    function queryStatus() {

        $.ajax('/api/arm?command=status', {timeout : 100}).done(function(data) {

            for (var v in data["joints"]) {
                joints[v](data["joints"][v]);
            }

            visualizer(data);

            setTimeout(queryStatus, 500);

        }).fail(function () {

            //setTimeout(queryStatus, 1000);

        });

    }

    $.ajax('/api/arm?command=describe').done(function(data) {

        $('#arm').append($("<h2>").text(data.name));

        var container = $('<div class="row">').appendTo($('#arm'));

        var sidebar = $('<div class="col-lg-5">').appendTo(container);

        sidebar.append($('<div class="information">').text("Version: " + data.version.toFixed(2)));

        for (var v in data["joints"]) {
            joints[v] = createJointController(sidebar, v, data["joints"][v]);
        }

        var visualize = $('<div class="col-lg-7">').appendTo(container);

        visualizer = robotArmVisualizer(visualize, data);

        queryStatus();

    });




/*
    createJointController(container, 0);
    createJointController(container, 1);
    createJointController(container, 2);
*/

});
