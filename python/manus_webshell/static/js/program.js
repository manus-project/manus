var workspace;

BLOCKLY_LANGUAGE = 'blockly'
PYTHON_LANGUAGE = 'python'

CodeParser = {
    tab_size: 2,

    tab : function() {
        return " ".repeat(CodeParser.tab_size);
    },

    indent_lines: function (code) {
        var tab = CodeParser.tab();
        var code_lines = code.split("\n");
        for (var i in code_lines){
            code_lines[i] = tab + code_lines[i]
        }
        return code_lines.join("\n")
    },

    std_code_prepend: function (){
        var tab = CodeParser.tab();
        return `from manus.robot_arm import *


arm = RobotArm()

# Wait for one second ...
arm.wait(1000)
# ... then start execution
`
    },

    prepare_std_code: function(code){
        // code = CodeParser.indent_lines(code);
        return CodeParser.std_code_prepend()+code+"\n\n";
    }

}


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

      // Ace code editor init
      Program.python = {};
      Program.python.shown = false;
      Program.python.in_use = false;
      Program.python.ace = ace.edit("python-editor");
      Program.python.ace.setTheme("ace/theme/xcode");
      Program.python.ace.getSession().setMode("ace/mode/python");
      Program.python.ace.getSession().setTabSize(Program.python.tab_size);
      Program.python.ace.setOptions({
          enableBasicAutocompletion: true,
          enableLiveAutocompletion: true
      });
      Program.python.ace.completers.push({
        getCompletions: function(editor, session, pos, prefix, callback) {
          callback(null, [
              {value: "arm.move_joint(joint, angle)", score: 1000, meta: "Robot arm: Move one joint to specific angle."},
              {value: "arm.cotrol_gripper(cmd)", score: 1000, meta: "Robot arm: Open, half close or close gripper."},
              {value: "arm.wait(milisecs)", score: 1000, meta: "Robot arm: Wait/Sleep for milisecs."},
              {value: "arm.move_to_coordinates(p)", score: 1000, meta: "Robot arm: Move arm to coordinates/point."},
              {value: "arm.detect_blocks()", score: 1000, meta: "Robot arm: Detect all blocks."},
              {value: "arm.get_joint_position(joint)", score: 1000, meta: "Robot arm: Get joints position."},
              {value: "retrieve_coordinate_from_point(comp, point)", score: 1000, meta: "Robot arm: Retrieve coordinate from point/coordinates."},
              {value: "retrieve_color_from_block(block)", score: 1000, meta: "Robot arm: Retrieve color from block."}
          ]);
        }
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
          Program.show();
      }

    });

    var run_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.run();
          logconsole.empty();
      }, icon: "play", tooltip: "Run"
    }).addClass("button-codetime");

    var new_program = $.manus.widgets.fancybutton({
      callback : function() {  
          Interface.dialog("New document",
              "Do you want to start from scratch? Make sure that you have saved your work first.",
              {
                "Cancel" : function() { return true; },
                "Blocky" : function() {
                  Program.python.shown = false;
                  Program.python.in_use = false;
                  Program.show("blockly");
                  return true;
                },
                "Python" : function() {
                  Program.python.shown = false;
                  Program.python.in_use = false;
                  Program.show("blockly");
                  return true;
                }

              }
            );
      },
      icon: "file", 
      tooltip: "New program"
    }).addClass("button-codetime");

    var convert_python = $.manus.widgets.fancybutton({
      callback : function() {  
        if (Program.python.shown){
            Interface.confirmation("Back to Blockly?",
                "If you haven't saved your python code you will lose it. Are you sure you want to go back to blockly?",
                function() {
                    Program.python.shown = false;
                    Program.python.in_use = false;
                    Program.show("blockly");
                }
              );
        } else {
            Program.python.ace.setValue(CodeParser.prepare_std_code(Blockly.Python.workspaceToCode(Program.blockly.workspace)));
            Program.python.shown = true;
            Program.python.in_use = true;
            Program.show("python");
        }
      },
      icon: "align-left", 
      tooltip: "Convert to Python"
    }).addClass("button-blockly button-codetime");

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

    $("#program .toolbar.left").append(run_button).append(convert_python).append(stop_button);
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

  current: function(data, language) {
    if (data === undefined) {
      if (Program.python && Program.python.in_use){
          return { code: Program.python.ace.getValue(), language: PYTHON_LANGUAGE }
      }
      var xml = Blockly.Xml.workspaceToDom(Program.blockly.workspace);
      return { code: Blockly.Xml.domToText(xml), language: BLOCKLY_LANGUAGE };
    } else {
      if (language == undefined)
        language = BLOCKLY_LANGUAGE;
        
      if (language == BLOCKLY_LANGUAGE){
        var xml_dom = Blockly.Xml.textToDom(data);
        Program.blockly.workspace.clear(); // Don't forget to call clear before load. Othervie it will just add more elements to workspace.
        Blockly.Xml.domToWorkspace(xml_dom, Program.blockly.workspace);
        return true;
      } else if (language == PYTHON_LANGUAGE) {
        Program.python.ace.setValue(data)
        return true;
      } else {
        console.error("Unknown code language: "+language);
        return false;
      }
    }
  },

  push: function() {
    // Store to client storage
    if (typeof(Storage) !== "undefined") {
        localStorage.setItem('saved_code', Program.current().code);
    }
  },

  pull: function() {
    // Retrieve code from client storage
    if (typeof(Storage) !== "undefined") {
        var xml_text = localStorage.getItem('saved_code');
        // Clear & Load xml into workspace
        if (xml_text && xml_text.length > 0) {
            Program.current(xml_text, BLOCKLY_LANGUAGE);
        }
    } 
  },

  save: function() {
    Interface.dialog("Save program", 
      function(container) { 
        var onclose = Program._filemanager(container, "saving", function(key, name) {
          var timestamp = (new Date()).toISOString();
          var curr = Program.current();
          RemoteStorage.set(key, {
            language: curr.language,
            name: name,
            timestamp: timestamp,
            code: curr.code,
            read_only: false
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
            if (data.language == PYTHON_LANGUAGE){
                Program.show("python");
            } else {
                Program.show("blockly");
            }
            Program.current(data.code, data.language);
          });
          Program.show("blockly");
          Interface.dialog();
          onclose();
        });
      }, 
      { "Cancel" : function() { return true; } });
  },

  show: function(panel) {
    if (panel == undefined){
        if (Program.python) { 
            if (Program.python.in_use)
                panel = "python"
            else
                panel = "blockly"
        } else {
            panel = "blockly"
        }
    }
    if (Program.python) Program.python.shown = false;
    $("#program").children(".program-panel").hide();
    $("#program .runtime").hide();
    if (panel == "blockly") {
      $("#program").children("#blockly").show();
      $("#program .runtime").show();
    } else if (panel == "python") {
      if (Program.python) Program.python.shown = true;
      $("#program").children("#python").show();
      $("#program .runtime").show();
    } else if (panel == "console") {
      $("#program").children("#console").show();
      $("#program .runtime").show();
    }
    Program.update_state();
  },

  run : function() {
    var code = "";
    if (Program.python.in_use) 
        code = Program.python.ace.getValue();
    else
        code = CodeParser.prepare_std_code(Blockly.Python.workspaceToCode(Program.blockly.workspace));
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
        var metadata = formatDateTime(timestamp);
        var read_only = item.values().read_only;

        if (read_only && type == "saving")
          container.addClass("readonly");

        var metadiv = $("<div/>").addClass("metadata").text(metadata);
        container.append(metadiv);

        if (!read_only || type != "saving") {
          container.click(function () {
              var key = item.values().key;
              var name = item.values().name;
              callback(key, name);
          });
        }


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

        // Add language icon
        if (item.values().language == PYTHON_LANGUAGE){
          metadiv.prepend($("<span/> ").addClass("glyphicon glyphicon-align-left"));
        }else{
          metadiv.prepend($("<span/> ").addClass("glyphicon glyphicon-th-large"));
        }

    }

    var updating = false;

    $(container).append($("<div>").attr({id : "filemanager"}).append($("<div>").addClass("list list-group")));

    var list = new List("filemanager", {
        valueNames : ["name", "modified", { name: 'language', attr: 'data-language' }, { name: 'read_only', attr: 'data-read-only' }],
        item: "<a class='list-group-item'><div><span class='name'></span></div></a>"
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
  },

  update_state : function() {
        var ace_visible = $("#python").is(":visible");
        var blockly_visible = $("#blockly").is(":visible");

        if (ace_visible && !blockly_visible){
            Program.python.shown = true;
            Program.python.in_use = true;
            var  $i = $("a.btn i.glyphicon-arrow-right");
            if ($i) {
                $i.removeClass("glyphicon-arrow-right");
                $i.addClass("glyphicon-arrow-left");
            }
        } else if (!ace_visible && blockly_visible) {
            Program.python.shown = false;
            Program.python.in_use = false;
            var  $i = $("a.btn i.glyphicon-arrow-left");
            if ($i) {
                $i.removeClass("glyphicon-arrow-left");
                $i.addClass("glyphicon-arrow-right");
            }
        }
    },

    save_current_as_read_only: function(name){
        if (name == undefined || name == "") {
            console.error("Name must be set");
            return;
        }
        key = "program_" + uniqueIdentifier();
        timestamp = (new Date()).toISOString();
        curr = Program.current();
        RemoteStorage.set(key, {language: curr.language,
            name: name,
            timestamp: timestamp,
            code: curr.code,
            read_only: true});
    }

};

$(function(){

  Program.init();

});


//function showCode() {
    // Generate Python code and display it.
    //var code = Blockly.Python.workspaceToCode(workspace);
//}

