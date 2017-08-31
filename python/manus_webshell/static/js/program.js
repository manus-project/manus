var workspace;

Program = {

  init: function() {

    $.ajax('/data/blockly-toolbox.xml').done(function(toolbox) {

      var blocklyContainer = $("#blockly");
      var blocklyArea = document.getElementById('blockly-area');
      var blocklyDiv = document.getElementById('blockly-workspace');
      blocklyContainer.append(Blockly.Xml.domToText(toolbox));

      Program.blockly = {};

      Program.blockly.running_app = null;
      Program.blockly.workspace = Blockly.inject('blockly-workspace',{
          grid:{
              spacing: 25,
              length: 3,
              colour: '#ddd',
              snap: true
          },
          media: 'media/',
          toolbox: document.getElementById('blockly-toolbox'),
          zoom: {controls: true, wheel: true}
      });

      // Register & call on resize function
      var onresize = function(e) {
          blocklyDiv.style.left = blocklyArea.offsetLeft + 'px';
          blocklyDiv.style.top = blocklyArea.offsetTop  + 'px';
          blocklyDiv.style.width = blocklyArea.offsetWidth + 'px';
          blocklyDiv.style.height = blocklyArea.offsetHeight + 'px';
      };

      window.addEventListener('resize', onresize, false);
      //onresize();
      //Blockly.svgResize(workspace);

      var initial = true;

      // Register on show callback
      $('a[data-toggle="tab"]').on('shown.bs.tab', function (e) {
          if (Program.blockly.workspace) {
              onresize();
              Blockly.svgResize(Program.blockly.workspace);
              if (initial) {
                initial = false;
                Program.pull();
              }
          }
      })

      Program.blockly.workspace.addChangeListener(function(event){
          if (event.type == Blockly.Events.CREATE || 
              event.type == Blockly.Events.DELETE || 
              event.type == Blockly.Events.CHANGE || 
              event.type == Blockly.Events.MOVE 
          ) {
              Program.push();
          }   
      });

    });

    var logconsole = $("#console .content");
    logconsole.click(function() {

      if ($("#console").hasClass("terminated")) {
          Program.show("blockly");
      }

    });

    var run_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.run();
          logconsole.empty();
      }, icon: "play", tooltip: "Run"
    });

    var stop_button = $.manus.widgets.fancybutton({
      callback : function() {  
          $.ajax('/api/apps?run=').done(function(data) {});
      }, icon: "stop", tooltip: "Stop"
    }).hide();

    var load_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.load();
      }, icon: "open", tooltip: "Load"
    });

    var save_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.save();
      }, icon: "save", tooltip: "Save"
    });

    Program.show("blockly");

    $("#program .toolbar.left").append(run_button).append(stop_button);
    $("#program .toolbar.right").append(save_button).append(load_button);

    PubSub.subscribe("apps.active", function(msg, identifier) {
      if (identifier !== undefined && (identifier == Program.blockly.running_app)) {
        run_button.hide(); stop_button.show();
        $("#console").removeClass("terminated");
        Program.show("console");
      } if (Program.blockly.running_app != null && identifier === undefined) {
        Program.blockly.running_app = null;
        stop_button.hide(); run_button.show();
        $("#console").addClass("terminated");
      }
    });

    PubSub.subscribe("apps.log", function(msg, data) {
      if (data.identifier == Program.blockly.running_app) {
        for (i in data.lines) {
          logconsole.append(data.lines[i]);
        }
      }
    });

  },

  current: function(data) {
    if (data === undefined) {
      var xml = Blockly.Xml.workspaceToDom(Program.blockly.workspace);
      return Blockly.Xml.domToText(xml);
    } else {
      var xml_dom = Blockly.Xml.textToDom(data);
      Program.blockly.workspace.clear(); // Don't forget to call clear before load. Othervie it will just add more elements to workspace.
      Blockly.Xml.domToWorkspace(xml_dom, Program.blockly.workspace);
      return true;
    }
  },

  push: function() {
    // Store to client storage
    if (typeof(Storage) !== "undefined") {
        localStorage.setItem('saved_code', Program.current());
    }
  },

  pull: function() {
    // Retrieve code from client storage
    if (typeof(Storage) !== "undefined") {
        var xml_text = localStorage.getItem('saved_code');
        // Clear & Load xml into workspace
        if (xml_text && xml_text.length > 0) {
            Program.current(xml_text);
        }
    } 
  },

  save: function() {
    Interface.dialog("Save program", 
      function(container) { 
        var onclose = Program._filemanager(container, "saving", function(key, name) {
          var timestamp = (new Date()).toISOString();
          RemoteStorage.set(key, {
            language: "blockly",
            name: name,
            timestamp: timestamp,
            code: Program.current()
          });
          Interface.dialog();
          onclose();
        }); 
      }, 
      { "Cancel" : function() { return true; } });
  },

  load: function() {
    Interface.dialog("Load program", 
      function(container) {
        var onclose = Program._filemanager(container, "loading", function(key, name) {
          RemoteStorage.get(key, function(key, data) {
            Program.current(data.code);
          });
          Program.show("blockly");
          Interface.dialog();
          onclose();
        });
      }, 
      { "Cancel" : function() { return true; } });
  },

  show: function(panel) {
    $("#program").children(".program-panel").hide();
    $("#program .runtime").hide();
    if (panel == "blockly") {
      $("#program").children("#blockly").show();
      $("#program .runtime").show();
      return;
    } 
    if (panel == "console") {
      $("#program").children("#console").show();
      $("#program .runtime").show();
      return;
    }
  },

  run : function() {
    var code = Blockly.Python.workspaceToCode(Program.blockly.workspace);
    $.ajax({
        'type': 'POST',
        'url': '/api/run',
        'contentType': 'application/json',
        'data': JSON.stringify({code: code, environment: "python"}),
        'dataType': 'json'
    }).done(function(data) {
        if (data.status != "ok"){
            console.log("Code execution failed. " + data.status + ": " + data.description);
        } else {
          Program.blockly.running_app = data.identifier;
        }
    }).fail(function(err) {
        Interface.notification("Error", err);

    });
  },

  _filemanager: function(container, type, callback) {

    var augmentItem = function(item) {
        var container = $(item.elm);
        if (container.hasClass('program-item') || item.values().key == "new")
            return;

        container.addClass('program-item');
        var tools = $('<div />').addClass('list-tools').prependTo(container);

        var timestamp = new Date(item.values().timestamp);
        var metadata = formatDateTime(timestamp) + " " + item.values().language;

        container.append($("<div/>").addClass("metadata").text(metadata));

        container.click(function () {
            var key = item.values().key;
            var name = item.values().name;
            callback(key, name);
        });

        if (type == "browse") {
          tools.append($('<i />').addClass('tool glyphicon glyphicon-trash').click(function() {
              var name = item.values().name;
              var key = item.values().key;
              Interface.confirmation("Deleting program",
                "Are you sure you want to delete program '" + name + "'? This action cannot be undone.",
                function() {
                  RemoteStorage.delete(key);
                }
              );
              return false;
            })
          );
        }
    }

    var updating = false;

    $(container).append($("<div>").attr({id : "filemanager"}).append($("<div>").addClass("list list-group")));

    var list = new List("filemanager", {
        valueNames : ["name", "modified"],
        item: "<a class='list-group-item'><div class='name'></div></a>"
    }, []);

    RemoteStorage.list(function(keys) {

        for (var i in keys) {
          if (!keys[i].startsWith("program_"))
            continue;
          RemoteStorage.get(keys[i], function(key, value) {
            value["key"] = key;
            list.add([value]);
            list.sort("timestamp", {order: 'desc'});
          });
        }
        
    });

    var subscribtions = {
     update : PubSub.subscribe("storage.update", function(msg, key) {

          if (!key.startsWith("program_")) 
              return;

          RemoteStorage.get(key, function(key, value) {

              list.remove("key", key);
              value["key"] = key;
              list.add([value]);
              list.sort("timestamp", {order: 'desc'});
          });
      }),
    delete : PubSub.subscribe("storage.delete", function(msg, key) {

          if (!key.startsWith("program_")) 
              return;
          list.remove("key", key);

      })
    }

    var index = 0;

    list.on("updated", function() {

        for (var i = 0; i < list.items.length; i++) {
            augmentItem(list.items[i]);
        }

    });

    if (type == "saving") { 
      var filename = $("<input>").attr({type: "text", placeholder: "New file"}).addClass("form-control");
      container.append($("<div>").addClass("form-group").append(filename));

      filename.keypress(function (e) {
          if(e.keyCode == 13) {
              e.stopPropagation();
              var name = filename.val();
              if (name.length < 1) return;
              var key = "program_" + uniqueIdentifier();
              callback(key, name);
          }
          return true;
        });
    } 

    return function() {
      PubSub.unsubscribe(subscribtions.update);
      PubSub.unsubscribe(subscribtions.delete);
    }
  }

};

$(function(){

  Program.init();

});


//function showCode() {
    // Generate Python code and display it.
    //var code = Blockly.Python.workspaceToCode(workspace);
//}

