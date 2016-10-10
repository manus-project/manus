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

                    world.canvas.addEventListener('mousedown', mouse.onMouseDown, false);
                    world.canvas.addEventListener('mouseup', mouse.onMouseUp, false);
                    world.canvas.addEventListener('mouseout', mouse.onMouseOut, false);
                    cameraSphere.update(world);
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
                    var ctx = world.canvas.getContext('2d');
                    ctx.fillStyle = "#aaaaaa";
                    ctx.fillText("Drag mouse to rotate the view.", 10, 20);
                }
            }
        },
        camera: function(url, options) {

            options = $.extend({'rate' : 250}, options);

            var image = null;
            var timeout = null;
            var transform = mat4.create();

            function callback() {
                var tmp = new Image();
                tmp.onload = function() {
                    image = tmp;
                    timeout = setTimeout(callback, options.rate);
                }
                tmp.src = url + '/image?' + Math.random();

                $.ajax(url + '/position').done(function(data) {

                    mat4.identity(transform);
                    transform[0] = data.rotation.m00;
                    transform[1] = data.rotation.m10;
                    transform[2] = data.rotation.m20;
                    transform[4] = data.rotation.m01;
                    transform[5] = data.rotation.m11;
                    transform[6] = data.rotation.m21;
                    transform[8] = data.rotation.m02;
                    transform[9] = data.rotation.m12;
                    transform[10] = data.rotation.m22;
                    transform[12] = data.translation.m0;
                    transform[13] = data.translation.m1;
                    transform[14] = data.translation.m2;

                    transform = mat4.scale(mat4.create(), transform, vec3.fromValues(1, 1, -1)); // Flip Z axis
                    transform = cameraToWorld(transform); // Invert matrix

                });
            }

            return {
                install: function(world) {
                    $.ajax(url + '/describe').done(function(data) {

                        var fov1 = 360 * Math.atan2(data.image.width, 2*data.intrinsics.m00) / Math.PI;
                        var fov2 = 360 * Math.atan2(data.image.height, 2*data.intrinsics.m11) / Math.PI;
                        world.scene.perspective.aspect = fov1 / fov2;
                        world.scene.perspective.fov = fov2; // (fov1 + fov2) / 2; // Compute average fov
                        callback();

                    });

                },
                uninstall: function(world) {
                    if (timeout)
                        clearTimeout(timeout);

                },
                prerender: function(world) {
					var ctx = world.canvas.getContext('2d');

                    if (!image) return;
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


					var scale = Math.max(image.width / world.canvas.width, image.height / world.canvas.height);


                    ctx.drawImage(image, (world.canvas.width - (image.width / scale)) / 2,
						(world.canvas.height - (image.height / scale)) / 2, image.width / scale, image.height / scale);
                    world.render();
                    image = null;

                },
                clear:  function(ctx) {},
                postrender: function(world) {

                }
            }
        }

    },

    viewer: function (options) {

		options = $.extend({width : 800, height: 600}, options);

        var canvas = $('<canvas class="visualization" width="' +
			options.width + '" height="' + options.height + '">');

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
            render: function() { dirty = true; },
            view: function(v) { if (v == null) v = defaultView; currentView.uninstall(this); currentView = v; v.install(this); }
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
    camera: function(world, url) {

        var timeout = null;
        var transform = mat4.create();
        var cameraProxy;

        function callback() {

            $.ajax(url + '/position').done(function(data) {

                transform[0] = data.rotation.m00;
                transform[1] = data.rotation.m10;
                transform[2] = data.rotation.m20;
                transform[4] = data.rotation.m01;
                transform[5] = data.rotation.m11;
                transform[6] = data.rotation.m21;
                transform[8] = data.rotation.m02;
                transform[9] = data.rotation.m12;
                transform[10] = data.rotation.m22;

                transform[12] = data.translation.m0;
                transform[13] = data.translation.m1;
                transform[14] = data.translation.m2;

                transform = mat4.scale(mat4.create(), transform, vec3.fromValues(1, 1, -1));  // Flip Z axis
                transform = cameraToWorld(transform); // Invert matrix

                cameraProxy.matrix = transform;

                timeout = setTimeout(callback, 250);
            });
        }

        $.manus.world.debug(cameraProxy, {showAxis: true, showId: true});

        $.ajax(url + '/describe').done(function(data) {

            var fov1 = Math.atan(data.image.width / (2*data.intrinsics.m00)) / Math.PI;
            var fov2 = Math.atan(data.image.height / (2*data.intrinsics.m11)) / Math.PI;
            var fov = (fov1 + fov2) / 2; // Compute average fov

            xsize = data.image.width;
            ysize = data.image.height;
            zsize = data.image.width / Math.tan(fov / 2);

            cameraProxy = Phoria.Entity.create({
                id : "Camera",
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
            callback();
        });
    },
    arm: function(world, status) {

        var joints = [];
        var jointMesh = Phoria.Util.generateUnitCube(10);
        var rootTransform = mat4.create();
        var parentJoint;
        var rootJoint;

        for (var v in status) {

            var a = status[v].a;
            var d = status[v].d;
            var alpha = status[v].alpha;
            var theta = status[v].theta;

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

            switch (status[v].type) {
                case "translation":
                case "rotation": {
                    var segmentMesh = Phoria.Util.generateCuboid({"scalex" : Math.max(1, a / 2),
                             "scaley" : 10, "scalez" : 10, "offsetx" : - Math.max(1, a / 2),
                             "offsety" : 0, "offsetz": 0});

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

                    break;
                }
                case "gripper": {

                    var segmentMesh = Phoria.Util.generateCuboid({"scalex" : Math.max(1, a / 2),
                             "scaley" : Math.max(1, a / 2), "scalez" : 10, "offsetx" : - Math.max(1, a / 2),
                             "offsety" : 0, "offsetz": 0});

                    var segment = Phoria.Entity.create({
                        points: segmentMesh.points,
                        edges: segmentMesh.edges,
                        polygons: segmentMesh.polygons,
                        style: {
                            color: [100,255,100],
                            drawmode: "wireframe",
                            shademode: "plain",
                            linewidth: 3,
                        }
                    });

                    break;
                }
            }

            joints[v] = {"segment" : segment, "joint" : joint, "data" : status[v]};
            joint.identity().rotateZ(theta).translateZ(d);
            segment.identity().rotateX(alpha).translateX(a);

            if (parentJoint === undefined) {
                world.scene.graph.push(joint);
                rootJoint = joint;
            } else {
                parentJoint.children.push(joint);
            }

            joint.children.push(segment);
            parentJoint = segment;

        }

        return {
            transform : function(transform) {
                rootTransform = mat4.clone(transform);
            },
            update : function(status) {

                for (var v in status) {
                    var d = joints[v].data.d;
                    var theta = joints[v].data.theta;
                    var update = true;
                    switch (joints[v].data.type) {
                        case "rotation": {theta = status[v]; break; }
                        case "translation": {d = status[v]; break;}
                        case "gripper": { break; }
                        default: { update = false; break; }
                    }
                    if (!update) break;
                    joints[v].joint.identity().rotateZ(theta).translateZ(d);
                }

                rootJoint.matrix = mat4.multiply(rootJoint.matrix, rootTransform, rootJoint.matrix);


                world.render();

            }
        };
    },
    marker: function(world, position) {
        var markerMesh = Phoria.Util.generateUnitCube(5);
        var marker = Phoria.Entity.create({
            id : "Marker",
            points: markerMesh.points,
            edges: markerMesh.edges,
            polygons: markerMesh.polygons,
            style: {
                color: [255,0,0],
                drawmode: "wireframe",
                shademode: "plain",
                linewidth: 4,
            }
        });

        marker.identity().translate(position);

        world.scene.graph.push(marker);

    },
    grid: function(world, position) {
        var plane = Phoria.Util.generateTesselatedPlane(16,16,0,400);
        var wireframe = Phoria.Entity.create({
            points: plane.points,
            edges: plane.edges,
            polygons: plane.polygons,
            style: {
                drawmode: "wireframe",
                shademode: "plain",
                linewidth: 2,
                objectsortmode: "back"
            }
        }
        ).translateX(200).rotateX(90 * Phoria.RADIANS).translate(position);

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
