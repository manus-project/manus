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
        return `import math, manus
from manus_apps.workspace import Workspace
from manus_apps.blocks import Block, block_color_name

workspace = Workspace(bounds=[(100, -200), (100, 200), (370, 200), (370, -200)])
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

      Program.running_app = null;
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
              {value: "workspace", score: 1000, meta: "Main workspace reference"},
              {value: "workspace.manipulator", score: 1000, meta: "Manipulator reference"},
              {value: "workspace.camera", score: 1000, meta: "Camera reference"},
              {value: "workspace.wait(10)", score: 1000, meta: "Wait for some time"}
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

      Program.python.ace.on("input", function() { Program.push(); } );

    });

    var logconsole = $("#console .content");
    $("#console").dblclick(function() {

      if ($("#console").hasClass("terminated")) {
          Program.show();
      }

    });

    $("#console .inputline").keypress(function (e) {
            if(e.which == 13) { 
              var line = $("#console .inputline").val();

              PubSub.publish("apps.send", {"identifier" : Program.running_app, "lines" : [line + "\n"]});

              $("#console .inputline").val("");
              return false; 
            }
        });

    $(document).keyup(function(e) {
      if (Program.running_app !== undefined && !$("#console").hasClass("terminated")) {
        //console.log(e.keyCode);
      }

      if (e.keyCode == 27) {
        if ($("#console").hasClass("terminated")) {
          Program.show();
        }
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
                  Program.current(null, "blockly");
                  return true;
                },
                "Python" : function() {
                  Program.current(null, "python");
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

        Interface.confirmation("Convert to Python", "If you would like to keep your Blockly program make sure that you have saved it first.",
          function() {
            Program.python.ace.setValue(CodeParser.prepare_std_code(Blockly.Python.workspaceToCode(Program.blockly.workspace)));
            Program.python.shown = true;
            Program.python.in_use = true;
            Program.show("python");
          });
      },
      icon: "align-left", 
      tooltip: "Convert to Python"
    }).addClass("button-blockly");

    var stop_button = $.manus.widgets.fancybutton({
      callback : function() {  
          postJSON('/api/apps', {}, function(data) {});
      }, icon: "stop", tooltip: "Stop"
    }).addClass("button-runtime").hide();

    var load_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.load();
      }, icon: "open", tooltip: "Load"
    }).addClass("button-codetime");

    var save_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.save();
      }, icon: "save", tooltip: "Save"
    }).addClass("button-codetime");

    Program.show("blockly");

    $("#program .toolbar.left").append(run_button).append(stop_button).append(new_program).append(convert_python);
    $("#program .toolbar.right").append(save_button).append(load_button);

    PubSub.subscribe("apps.active", function(msg, identifier) {
      if (identifier !== undefined && (identifier == Program.running_app)) {
        run_button.hide(); stop_button.show();
        $("#console").removeClass("terminated");
        Program.show("console");
      } if (Program.running_app != null && identifier === undefined) {
        Program.running_app = null;
        stop_button.hide(); run_button.show();
        $("#console").addClass("terminated");
      }
    });

    PubSub.subscribe("apps.console", function(msg, data) {
      if (data.identifier == Program.running_app) {
        for (i in data.lines) {
          logconsole.append($("<span>").addClass(data.source).text(data.lines[i]));
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
    } else if (data === null) {
      if (language == undefined)
        language = BLOCKLY_LANGUAGE;
      if (language == BLOCKLY_LANGUAGE) {
        Program.blockly.workspace.clear(); // Don't forget to call clear before load. Othervie it will just add more elements to workspace.
        Program.show("blockly");
        return true;
      } else if (language == PYTHON_LANGUAGE) {
        Program.python.ace.setValue("");
        Program.show("python");
        return true;
      } else {
        return false;
      }
    } else {
      if (language == undefined)
        language = BLOCKLY_LANGUAGE;
        
      if (language == BLOCKLY_LANGUAGE){
        var xml_dom = Blockly.Xml.textToDom(data);
        Program.blockly.workspace.clear(); // Don't forget to call clear before load. Othervie it will just add more elements to workspace.
        Blockly.Xml.domToWorkspace(xml_dom, Program.blockly.workspace);
        Program.show("blockly");
        return true;
      } else if (language == PYTHON_LANGUAGE) {
        Program.python.ace.setValue(data);
        Program.show("python");
        return true;
      } else {
        return false;
      }
    }
  },

  push: function() {
    // Store to client storage
    if (typeof(Storage) !== "undefined") {
        localStorage.setItem('saved_program', JSON.stringify(Program.current()));
    }
  },

  pull: function() {
    // Retrieve code from client storage
    if (typeof(Storage) !== "undefined") {
        var data = localStorage.getItem('saved_program');
        // Clear & Load xml into workspace
        if (data && data.length > 0) {
          data = JSON.parse(data);
          Program.current(data.code, data.language);
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
    //$("#program .runtime").hide();
    $("#program .toolbar .btn").hide();
    if (panel == "blockly") {
      $("#program").children("#blockly").show();
      //$("#program .runtime").show();
      $("#program .toolbar .button-blockly").show();
      $("#program .toolbar .button-codetime").show();
    } else if (panel == "python") {
      if (Program.python) Program.python.shown = true;
      $("#program").children("#python").show();
      //$("#program .runtime").show();
      $("#program .toolbar .button-codetime").show();
    } else if (panel == "console") {
      $("#program").children("#console").show();
      //$("#program .runtime").show();
      $("#program .toolbar .button-runtime").show();
    }
    Program.update_state();
  },

  run : function() {
    var code = "";
    if (Program.python.in_use) 
        code = Program.python.ace.getValue();
    else
        code = CodeParser.prepare_std_code(Blockly.Python.workspaceToCode(Program.blockly.workspace));
        postJSON('/api/apps', {code: code, environment: "python"}, function(data) {
          if (data.status != "ok") {
              console.log("Code execution failed. " + data.status + ": " + data.description);
          } else {
              Program.running_app = data.identifier;
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

        if (read_only) {
          metadiv.prepend($("<span/> ").addClass("glyphicon glyphicon-lock"));
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

