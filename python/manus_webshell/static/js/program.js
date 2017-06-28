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

      var augmentItem = function(item) {
          var container = $(item.elm);
          if (container.hasClass('program-item') || item.values().key == "new")
              return;

          container.addClass('program-item');
          var tools = $('<div />').addClass('list-tools').prependTo(container);

          container.click(function () {
              var key = item.values().key;
              if ($("#programs").hasClass("loading")) {
                RemoteStorage.get(key, function(key, data) {
                  Program.current(data.code);
                });
              } 

              if ($("#programs").hasClass("saving")) {
                  var name = item.values().name;
                  var timestamp = (new Date()).toISOString();
                  RemoteStorage.set(key, {language: "blockly",
                      name: name,
                      timestamp: timestamp,
                      code: Program.current()});
              }

              Program.show("blockly");
          });

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

      var updating = false;

      var list = new List("programs", {
          valueNames : ["name", "modified"],
          item: "<a class='list-group-item'><div class='name'></div><div class='modified'></div></a>"
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

      PubSub.subscribe("storage.update", function(msg, key) {

          if (!key.startsWith("program_")) 
              return;

          RemoteStorage.get(key, function(key, value) {

              list.remove("key", key);
              value["key"] = key;
              list.add([value]);
              list.sort("timestamp", {order: 'desc'});
          });

      });

      PubSub.subscribe("storage.delete", function(msg, key) {

          if (!key.startsWith("program_")) 
              return;
          list.remove("key", key);

      });

      var index = 0;

      list.on("updated", function() {

          for (var i = 0; i < list.items.length; i++) {
              augmentItem(list.items[i]);
          }

      });

      $("#programs .toolbar").append($.manus.widgets.fancybutton({
        icon: "plus", tooltip: "Save as new program",
        callback: function() {

          if ($("#programs").hasClass("loading")) 
            return;
          list.add([{key : "new", name: "", modified: "", timestamp: "zzzz"}]);
          list.update();
        
          var container = $(list.get("key", "new")[0].elm);
          container.addClass('editable');
          var textbox = container.children(".name");
          textbox.text("New program");
          textbox.attr('contenteditable', 'true');

          var sel = window.getSelection();
          textbox.bind('blur', function() {
              list.remove("key", "new");
          }).focus();

          var sel = window.getSelection();
          var range = document.createRange();
          range.setStart(textbox[0], 1);
          range.collapse(true);
          sel.removeAllRanges();
          sel.addRange(range);
          
          textbox.bind('keyup', function(event) {
              if(event.keyCode == 13) {
                  name = textbox.text();
                  key = "program_" + uniqueIdentifier();
                  timestamp = (new Date()).toISOString();
                  RemoteStorage.set(key, {language: "blockly",
                      name: name,
                      timestamp: timestamp,
                      code: Program.current()});
                  list.remove("key", "new");
                  Program.show("blockly");
                  event.stopPropagation();
              } else if (event.keyCode == 27) {
                  list.remove("key", "new");
                  event.stopPropagation();
              }
              return true;
          });

          return false;
        } 

    }).addClass("addnew"));

   
    $("#programs .toolbar").append($.manus.widgets.fancybutton({
        icon: "remove", tooltip: "Cancel",
        callback: function() {
          Program.show("blockly");
        }
    }));

    var logconsole = $("#console");
    logconsole.click(function() {

      if ($(this).hasClass("terminated")) {
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

    $("#program .toolbar.runtime").append(run_button).append(stop_button);
    $("#program .toolbar.storage").append(save_button).append(load_button);

    PubSub.subscribe("apps.active", function(msg, identifier) {
      if (identifier == Program.blockly.running_app) {
        run_button.hide(); stop_button.show();
        $("#console").removeClass("terminated");
        Program.show("console");
      } else {
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
    $("#programs").addClass("saving").removeClass("loading");
    $("#programs").children(".title").text("Save program");
    Program.show("programs");
  },

  load: function() {
    $("#programs").addClass("loading").removeClass("saving");
    $("#programs").children(".title").text("Load program");
    Program.show("programs");
  },

  show: function(panel) {
    $("#program").children(".program-panel").hide();
    $("#program .storage").hide();
    $("#program .runtime").hide();
    if (panel == "blockly") {
      $("#program").children("#blockly").show();
      $("#program .runtime").show();
      $("#program .storage").show();
      return;
    } 
    if (panel == "console") {
      $("#program").children("#console").show();
      $("#program .runtime").show();
      return;
    }
    if (panel == "programs") {
      $("#program").children("#programs").show();
      $("#program .storage").show();
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
    }

};

$(function(){

  Program.init();

});


//function showCode() {
    // Generate Python code and display it.
    //var code = Blockly.Python.workspaceToCode(workspace);
//}

