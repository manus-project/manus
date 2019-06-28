(function($) {

function cameraToWorld(transform) {

    var rotation = mat3.fromMat4(mat3.create(), transform);
    var translation = vec3.fromValues(transform[12], transform[13], transform[14]);
    rotation = mat3.transpose(mat3.create(), rotation);

    translation = vec3.negate(vec3.create(), vec3.transformMat3(vec3.create(), translation, rotation));

    var result = mat4.create();

    result[0] = rotation[0];
    result[1] = rotation[1];
    result[2] = rotation[2];
    result[4] = rotation[3];
    result[5] = rotation[4];
    result[6] = rotation[5];
    result[8] = rotation[6];
    result[9] = rotation[7];
    result[10] = rotation[8];
    result[12] = translation[0];
    result[13] = translation[1];
    result[14] = translation[2];

    return result;

}

function lookAtParameters(transform) {

    var inv = transform;
    var up = vec4.fromValues(0, -1, 0, 1);
    var position = vec4.fromValues(0, 0, 0, 1);
    var target = vec4.fromValues(0, 0, 1, 1);

    position = vec4.transformMat4(vec4.create(), position, inv);
    target = vec4.transformMat4(vec4.create(), target, inv);
    up = vec4.sub(vec4.create(), vec4.transformMat4(vec4.create(), up, inv), position);

    return {up: up, lookat: target, eye: position};

}

if (!$.manus) {
    $.manus = {};
}

if (!$.manus.world) {
    $.manus.world = {};
}

$.manus.world = {

    _transparent : "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7",

    views : {
        free : function(options) {
            options = $.extend({azimuth : 20, altitude : -10, distance: 1000, elevation: 100, offsetx: 200, offsety: 0}, options);

            var cameraSphere = options;
            var mouse = {};

            mouse.onMouseMove = function onMouseMove(evt) {
                mouse.dragging = true;
                mouse.dragX = evt.clientX - mouse.clickX;
                mouse.dragY = evt.clientY - mouse.clickY;
            };

             mouse.onMouseUp = function onMouseUp(evt) {
                mouse.endDragging();
                evt.target.removeEventListener('mousemove', mouse.onMouseMove, false);
             };

             mouse.onMouseOut = function onMouseOut(evt) {
                mouse.endDragging();
                evt.target.removeEventListener('mousemove', mouse.onMouseMove, false);
             };

             mouse.onMouseDown = function onMouseDown(evt) {
                evt.preventDefault();
                evt.target.addEventListener('mousemove', mouse.onMouseMove, false);
                mouse.clickX = evt.clientX;
                mouse.clickY = evt.clientY;
             };

            mouse.endDragging = function() {
                mouse.dragging = false;
                cameraSphere.azimuth = mouse.dragX;
                cameraSphere.altitude = mouse.dragY;
            }

            cameraSphere.update = function(world) {

                    var azimuth = this.azimuth + mouse.dragX;
                    var altitude = Math.min(90, Math.max(-90, this.altitude + mouse.dragY));

                    var rotation = mat4.create();
                    rotation = mat4.rotateZ(rotation, rotation, azimuth * Phoria.RADIANS);
                    rotation = mat4.rotateY(rotation, rotation, altitude * Phoria.RADIANS);

                    var position = vec4.fromValues(this.distance, 0, 0, 1);
                    position = vec4.transformMat4(position, position, rotation);

                    world.scene.camera.position.x = position[0] + cameraSphere.offsetx;
                    world.scene.camera.position.y = position[1] + cameraSphere.offsety;
                    world.scene.camera.position.z = position[2] + cameraSphere.elevation;

            }

            return {
                install: function(world) {
                    mouse.dragging = false;
                    mouse.clickX = 0;
                    mouse.clickY = 0;
                    mouse.dragX = 0;
                    mouse.dragY = 0;

                    world.scene.camera.lookat.x = cameraSphere.offsetx;
                    world.scene.camera.lookat.y = cameraSphere.offsety;
                    world.scene.camera.lookat.z = cameraSphere.elevation;

                    world.scene.camera.up.x = 0;
                    world.scene.camera.up.y = 0;
                    world.scene.camera.up.z = 1;

                    world.scene.perspective.fov = 35.0;

                    world.scene.viewport.x = 0;
                    world.scene.viewport.y = 0;

                    world.canvas.addEventListener('mousedown', mouse.onMouseDown, false);
                    world.canvas.addEventListener('mouseup', mouse.onMouseUp, false);
                    world.canvas.addEventListener('mouseout', mouse.onMouseOut, false);
                    cameraSphere.update(world);
                    world.render();
                },
                uninstall: function(world) {
                    world.canvas.removeEventListener('mousedown', mouse.onMouseDown, false);
                    world.canvas.removeEventListener('mouseup', mouse.onMouseUp, false);
                    world.canvas.removeEventListener('mouseout', mouse.onMouseOut, false);
                    world.canvas.removeEventListener('mousemove', mouse.onMouseMove, false);
                },
                prerender: function(world) {
                    if (mouse.dragging) {
                        cameraSphere.update(world);
                        world.render();
                    }
                },
                clear: null,
                postrender: function(world) {
                    /*var ctx = world.canvas.getContext('2d');
                    ctx.fillStyle = "#aaaaaa";
                    ctx.fillText("Drag mouse to rotate the view.", 10, 20);*/
                }
            }
        },
        camera: function(name, url, data) {

            var transform = mat4.create();

            function callback(msg, data) {

                mat4.identity(transform);

                transform[0] = data.rotation[0][0];
                transform[1] = data.rotation[1][0];
                transform[2] = data.rotation[2][0];
                transform[4] = data.rotation[0][1];
                transform[5] = data.rotation[1][1];
                transform[6] = data.rotation[2][1];
                transform[8] = data.rotation[0][2];
                transform[9] = data.rotation[1][2];
                transform[10] = data.rotation[2][2];

                transform[12] = data.translation[0][0];
                transform[13] = data.translation[0][1];
                transform[14] = data.translation[0][2];

                transform = mat4.scale(mat4.create(), transform, vec3.fromValues(1, 1, -1)); // Flip Z axis
                transform = cameraToWorld(transform); // Invert matrix

            }

            PubSub.subscribe(name + ".update", callback);

            return {
                install: function(world) {

                    var fov1 = 360 * Math.atan2(data.image.width, 2*data.intrinsics[0][0]) / Math.PI;
                    var fov2 = 360 * Math.atan2(data.image.height, 2*data.intrinsics[1][1]) / Math.PI;
                    world.scene.perspective.aspect = data.image.width / data.image.height;
                    world.scene.perspective.fov = fov2; // Compute average fov

                    // TODO: hardcoded - calculate it from intrinsics
                    world.scene.viewport.y = -15;
                    world.scene.viewport.x = -5;

                    //console.log(data.intrinsics);
                    world.projection.attr({src: url});

                },
                uninstall: function(world) {
                    world.projection.attr({src: $.manus.world._transparent});

                },
                prerender: function(world) {
                    var ctx = world.canvas.getContext('2d');

                    //if (!image) return;
                    ctx.clearRect(0, 0, world.canvas.width, world.canvas.height);

                    parameters = lookAtParameters(transform);

                    world.scene.camera.position.x = parameters.eye[0];
                    world.scene.camera.position.y = parameters.eye[1];
                    world.scene.camera.position.z = parameters.eye[2];

                    world.scene.camera.lookat.x = parameters.lookat[0];
                    world.scene.camera.lookat.y = parameters.lookat[1];
                    world.scene.camera.lookat.z = parameters.lookat[2];

                    world.scene.camera.up.x = parameters.up[0];
                    world.scene.camera.up.y = parameters.up[1];
                    world.scene.camera.up.z = parameters.up[2];

                    world.render();

                },
                clear:  function(ctx) {},
                postrender: function(world) {}
            }
        }

    },

    viewer: function (options) {

        options = $.extend({width : 800, height: 600}, options);

        var canvas = $('<canvas/>').attr({width: options.width, height: options.height});
        var projection = $('<img/>').attr({width: options.width, height: options.height, src: $.manus.world._transparent});
        var wrapper = $('<div/>').addClass("viewer").append(projection).append(canvas);

        viewCamera = -1;
        canvas = canvas[0];

        var scene = new Phoria.Scene();
        scene.camera.position = {x:0.0, y:0.0, z:1.0};
        scene.camera.up = {x:0.0, y:0.0, z:1.0};
        scene.camera.lookat = {x:0.0, y:0.0, z:10.0};
        scene.perspective.aspect = canvas.width / canvas.height;
        scene.viewport.width = canvas.width;
        scene.viewport.height = canvas.height;
        var renderer = new Phoria.CanvasRenderer(canvas);

        var defaultView = $.manus.world.views.free();
        var currentView = defaultView;
        var dirty = true;

        var world = {
            scene: scene,
            canvas: canvas,
            projection: projection,
            wrapper: wrapper,
            render: function() { dirty = true; },
            view: function(v) { if (v == null) v = defaultView; currentView.uninstall(this); currentView = v; v.install(this); },
            resize: function(width, height) {
                options.width = width;
                options.height = height;
                $(canvas).attr({width: options.width, height: options.height});
                projection.attr({width: options.width, height: options.height});
                scene.perspective.aspect = canvas.width / canvas.height;
                scene.viewport.width = canvas.width;
                scene.viewport.height = canvas.height;
                var dirty = true;
                world.render();
            },
            snapshot: function() {
                var buffer = document.createElement('canvas');
                buffer.width = canvas.width;
                buffer.height = canvas.height;
                var context = buffer.getContext('2d');
                context.drawImage(projection[0], 0, 0, canvas.width, canvas.height);
                context.drawImage(canvas, 0, 0, canvas.width, canvas.height);
                var dataURL = buffer.toDataURL('image/png');
                return dataURL;
            }
        }

        function render() {

            currentView.prerender(world);

            if (dirty) {
                scene.modelView();
                renderer.render(scene, currentView.clear);
                dirty = false;
            }

            currentView.postrender(world);
            setTimeout(render, 100);

        }

        currentView.install(world);

        render();
        scene.graph.push(new Phoria.DistantLight());

        return world;

    },
    camera: function(world, name, data) {

        var transform = mat4.create();
        var cameraProxy;

        function callback(msg, data) {
            console.log(data);
            transform[0] = data.rotation[0][0];
            transform[1] = data.rotation[0][1];
            transform[2] = data.rotation[0][2];
            transform[4] = data.rotation[1][0];
            transform[5] = data.rotation[1][1];
            transform[6] = data.rotation[1][2];
            transform[8] = data.rotation[2][0];
            transform[9] = data.rotation[2][1];
            transform[10] = data.rotation[2][2];

            transform[12] = data.translation[0][0];
            transform[13] = data.translation[0][1];
            transform[14] = data.translation[0][2];

            transform = mat4.scale(mat4.create(), transform, vec3.fromValues(1, 1, -1));  // Flip Z axis
            transform = cameraToWorld(transform); // Invert matrix
            cameraProxy.matrix = transform;
        }

        //$.manus.world.debug(cameraProxy, {showAxis: true, showId: true});
        var fov1 = Math.atan(data.image.width / (2*data.intrinsics[0][0])) / Math.PI;
        var fov2 = Math.atan(data.image.height / (2*data.intrinsics[1][1])) / Math.PI;
        var fov = (fov1 + fov2) / 2; // Compute average fov

        xsize = 0.1 * data.image.width;
        ysize = 0.1 * data.image.height;
        zsize = -0.01 * data.image.width / Math.tan(fov / 2);

        cameraProxy = Phoria.Entity.create({
            id : name,
            points: [{x:0,y:0,z:0}, {x:xsize,y:ysize,z:-zsize}, {x:xsize,y:-ysize,z:-zsize},{x:-xsize,y:-ysize,z:-zsize},{x:-xsize,y:ysize,z:-zsize}],
            edges: [{a:0,b:1}, {a:0,b:2}, {a:0,b:3}, {a:0,b:4}, {a:1,b:2}, {a:2,b:3}, {a:3,b:4}, {a:4,b:1}],
            polygons: [],
            style: {
                color: [255,0,0],
                drawmode: "wireframe",
                shademode: "plain",
                linewidth: 2,
            }
        });

        world.scene.graph.push(cameraProxy);

        PubSub.subscribe(name + ".update", callback);

    },
    manipulator: function(world, manipulator, description) {

        var joints = [];
        var rootTransform = mat4.create();
        var parentJoint;
        var manipulatorRoot;

        joints_description = description["joints"];

        for (var v in joints_description) {

            var a = joints_description[v].a;
            var d = joints_description[v].d;
            var alpha = joints_description[v].alpha;
            var theta = joints_description[v].theta;

            var overlay = undefined;
            var hover = undefined;

            var jointContainer = Phoria.Entity.create({});

            switch (joints_description[v].type.toLowerCase()) {
                case "fixed":
                case "translation":
                case "rotation": {

                    var segmentMesh = null;
                    var jointMesh = null;

                    if (a > 1) {

                        segmentMesh = Phoria.Util.generateCuboid({"scalex" : Math.max(1, a / 2),
                                 "scaley" : 10, "scalez" : 10, "offsetx" : - Math.max(1, a / 2),
                                 "offsety" : 0, "offsetz": 0});

                    } else {
                        segmentMesh = {
                             points: [],
                             edges: [],
                             polygons: []
                        };
                    }

                    if (d > 1) {
                         jointMesh = Phoria.Util.generateCuboid({"scalez" : Math.max(1, d / 2),
                                 "scaley" : 10, "scalex" : 10, "offsetz" : - Math.max(1, d / 2),
                                 "offsety" : 0, "offsetx": 0});

                    } else {

                         jointMesh = Phoria.Util.generateUnitCube(10);

                    }

                    var segment = Phoria.Entity.create({
                        points: segmentMesh.points,
                        edges: segmentMesh.edges,
                        polygons: segmentMesh.polygons,
                        style: {
                            color: [255,0,100],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 3,
                        }
                    });

                    var joint = Phoria.Entity.create({
                        id : "Joint" + (v+1),
                        points: jointMesh.points,
                        edges: jointMesh.edges,
                        polygons: jointMesh.polygons,
                        style: {
                            color: [53,126,189],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 4,
                        }
                    });


                    var overlay_points = [{"x": 0, "y": 0, "z": 0}];
                    var overlay_edges = [];

                    var segment_count = Math.max(3, Math.round((joints_description[v].max - joints_description[v].min) / 0.2));
                    var segment_space = (joints_description[v].max - joints_description[v].min) / segment_count;

                    for (var i = 0; i <= segment_count; i++) {
                        var angle = segment_space * i + joints_description[v].min;
                        overlay_points.push({"x": Math.cos(angle) * 100, "y": Math.sin(angle) * 100, "z": 0});
                        overlay_edges.push({"a": i, "b": i+1});
                    }

                    overlay_edges.push({"a": segment_count + 1, "b": 0});

                    overlay = Phoria.Entity.create({
                        points: overlay_points,
                        edges: overlay_edges,
                        polygons: [],
                        disabled: true,
                        style: {
                            color: [200,200,200],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 3,
                        }
                    });

                    hover = Phoria.Entity.create({
                        points: [{"x": 0, "y": 0, "z": 0}, {"x": 100, "y": 0, "z": 0}],
                        edges: [{"a": 0, "b": 1}],
                        polygons: [],
                        style: {
                            color: [200,0,0],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 5,
                        }
                    });

                    overlay.children.push(hover);

                    break;
                }
                case "gripper": {

                    jointMesh = Phoria.Util.generateUnitCube(10);

                    var joint = Phoria.Entity.create({
                        id : "Joint" + (v+1),
                        points: jointMesh.points,
                        edges: jointMesh.edges,
                        polygons: jointMesh.polygons,
                        style: {
                            color: [53,126,189],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 4,
                        }
                    });

                    segmentMesh = {
                             points: [],
                             edges: [],
                             polygons: []
                        };

                    var segment = Phoria.Entity.create({
                        points: segmentMesh.points,
                        edges: segmentMesh.edges,
                        polygons: segmentMesh.polygons,
                        style: {
                            color: [100,255,100],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 5,
                        }
                    });

                    break;
                }
            }

            joints[v] = {"segment" : segment, "joint" : joint, "data" : joints_description[v], "overlay": overlay, "hover": hover};
            joints[v].data.type = joints[v].data.type.toLowerCase();
            joint.identity().rotateZ(theta).translateZ(d);
            segment.identity().rotateX(alpha).translateX(a);

            if (overlay !== undefined)
                jointContainer.children.push(overlay);
            jointContainer.children.push(joint);

            if (parentJoint === undefined) {
                manipulatorRoot = jointContainer;
                world.scene.graph.push(manipulatorRoot);
            } else {
                parentJoint.children.push(jointContainer);
            }

            joint.children.push(segment);
            parentJoint = segment;

        }

        PubSub.subscribe(manipulator + ".hover", function(msg, data) {

            for (var i in joints) {
                if (joints[i].overlay !== undefined)
                    joints[i].overlay.disabled = true;
            }

            if (data.id === undefined) return;

            if (joints[data.id].overlay !== undefined)
                joints[data.id].overlay.disabled = false;

            if (joints[data.id].hover !== undefined) {
                switch (joints[data.id].data.type) {
                    case "rotation": { joints[data.id].hover.identity().rotateZ(data.position); break; }
                   // case "translation": {d = data.position; break;}
                }
            }

            world.render();

        });

        PubSub.subscribe(manipulator + ".update", function(msg, data) {

            for (var v in data.joints) {
                var d = joints[v].data.d;
                var theta = joints[v].data.theta;
                var update = true;
                switch (joints[v].data.type) {
                    case "rotation": {theta = data.joints[v].position; break; }
                    case "translation": {d = data.joints[v].position; break;}
                    case "gripper": { break; }
                    case "fixed": { break; }
                    default: { update = false; break; }
                }
                if (!update) break;
                joints[v].joint.identity().rotateZ(theta).translateZ(d);
            }

            manipulatorRoot.matrix = mat4.create();

            manipulatorRoot.matrix = mat4.multiply(manipulatorRoot.matrix, rootTransform, manipulatorRoot.matrix);

            world.render();

        });

        rootTransform = mat4.create();

        origin = vec3.fromValues(description.offset.origin.x, description.offset.origin.y, description.offset.origin.z);

        rootTransform = mat4.rotateX(rootTransform, rootTransform, description.offset.rotation.x / 180.0 * Math.PI);
        rootTransform = mat4.rotateY(rootTransform, rootTransform, description.offset.rotation.y / 180.0 * Math.PI);
        rootTransform = mat4.rotateZ(rootTransform, rootTransform, description.offset.rotation.z / 180.0 * Math.PI);

        rootTransform = mat4.translate(rootTransform, rootTransform, origin);

    },
    markers: function(world) {
        //var markerMesh = Phoria.Util.generateUnitCube(5);
        var markers = Phoria.Entity.create({
            id : "Markers",
            points: [],
            edges: [],
            polygons: [],
            style: {
                color: [255,0,0],
                drawmode: "wireframe",
                shademode: "plain",
                linewidth: 4,
            }
        });

        world.scene.graph.push(markers);

        return {
            clear : function() {
                markers.children = [];
            },
            add : function(position, orientation, scale, color) {
                mesh = Phoria.Util.generateCuboid({"scalex" : scale[0] / 2,
                         "scaley" : scale[1] / 2, "scalez" : scale[2] / 2});

                var marker = Phoria.Entity.create({
                    points: mesh.points,
                    edges: mesh.edges,
                    polygons: mesh.polygons,
                    style: {
                        color: color,
                        drawmode: "wireframe",
                        shademode: "plain",
                        linewidth: 3,
                    }
                });

                marker.identity().translate(position).rotateZ(orientation[2]); //.translate([-scale[0]/4, -scale[1]/4, -scale[2]/4]);
                markers.children.push(marker);
            }
        };

    },
    grid: function(world, vsegs, hsegs, size, position) {

        var points = [], edges = [], polys = [],
            hinc = size, vinc = size, c = 0;
        for (var i=0, x, y = vsegs * vinc /2 ; i<=vsegs; i++) {
         x = -hsegs * hinc /2;
         for (var j=0; j<=hsegs; j++)
         {
            // generate a row of points
            points.push( {x: x, y: y, z: 0} );
            
            // edges
            if (j !== 0)
            {
               edges.push( {a:c, b:c-1} );
            }
            if (i !== 0)
            {
               edges.push( {a:c, b:c-hsegs-1} );
            }

            if (i !== 0 && j !== 0)
            {
               // generate quad
               var p = {vertices:[c-hsegs-1, c, c-1, c-hsegs-2]};
               polys.push(p);
            }
            
            x += hinc;
            c++;
         }
         y -= vinc;
        }


        var wireframe = Phoria.Entity.create({
            points: points,
            edges: edges,
            polygons: polys,
            style: {
                drawmode: "wireframe",
                shademode: "plain",
                linewidth: 2,
                objectsortmode: "back"
            }
        }).translate(position);

        world.scene.graph.push(wireframe);

    },
    debug: function(entity, config)
    {
      // search child list for debug entity
      var id = "Phoria.Debug" + (entity.id ? (" "+entity.id) : "");
      var debugEntity = null;
      for (var i=0; i<entity.children.length; i++)
      {
         if (entity.children[i].id === id)
         {
            debugEntity = entity.children[i];
            break;
         }
      }

      // create debug entity if it does not exist
      if (debugEntity === null)
      {
         // add a child entity with a custom renderer - that renders text of the parent id at position
         debugEntity = new Phoria.Entity();
         debugEntity.id = id;
         debugEntity.points = [ {x:0,y:0,z:0} ];
         debugEntity.style = {
            drawmode: "point",
            shademode: "callback",
            geometrysortmode: "none",
            objectsortmode: "front"    // force render on-top of everything else
         };

         // config object - will be combined with input later
         debugEntity.config = {};

         debugEntity.onRender(function(ctx, x, y) {
            // render debug text
            ctx.fillStyle = "#333";
            ctx.font = "14pt Helvetica";
            var textPos = y;
            if (this.config.showId)
            {
               ctx.fillText(entity.id ? entity.id : "unknown - set Entity 'id' property", x, textPos);
               textPos += 16;
            }
            if (this.config.showPosition)
            {
               var p = entity.worldposition ? entity.worldposition : debugEntity._worldcoords[0];
               ctx.fillText("{x:" + p[0].toFixed(2) + ", y:" + p[1].toFixed(2) + ", z:" + p[2].toFixed(2) + "}", x, textPos);
            }
         });
         entity.children.push(debugEntity);

         // add visible axis geometry (lines) as children of entity for showAxis
         var fnCreateAxis = function(letter, vector, color) {
            var axisEntity = new Phoria.Entity();
            axisEntity.points = [ {x:0,y:0,z:0}, {x:2*vector[0],y:2*vector[1],z:2*vector[2]} ];
            axisEntity.edges = [ {a:0,b:1} ];
            axisEntity.style = {
               drawmode: "wireframe",
               shademode: "plain",
               geometrysortmode: "none",
               objectsortmode: "front",
               linewidth: 2.0,
               color: color
            };
            axisEntity.disabled = true;
            return axisEntity;
         };
         debugEntity.children.push(fnCreateAxis("X", vec3.fromValues(100,0,0), [255,0,0]));
         debugEntity.children.push(fnCreateAxis("Y", vec3.fromValues(0,100,0), [0,255,0]));
         debugEntity.children.push(fnCreateAxis("Z", vec3.fromValues(0,0,100), [0,0,255]));
      }

      // set the config
      Phoria.Util.combine(debugEntity.config, config);
      for (var i=0; i<debugEntity.children.length; i++)
      {
         debugEntity.children[i].disabled = !debugEntity.config.showAxis;
      }
   }

}

})(jQuery);