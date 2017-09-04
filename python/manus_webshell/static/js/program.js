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
        Program.code_editor = {};
        Program.code_editor.shown = false;
        Program.code_editor.in_use = false;
        Program.code_editor.ace = ace.edit("ace-code-editor-workspace");
        Program.code_editor.ace.setTheme("ace/theme/xcode");
        Program.code_editor.ace.getSession().setMode("ace/mode/python");
        Program.code_editor.ace.getSession().setTabSize(Program.code_editor.tab_size);
        Program.code_editor.ace.setOptions({
            enableBasicAutocompletion: true,
            enableLiveAutocompletion: true
        });
         Program.code_editor.ace.completers.push({
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
        })

        // Register & call on resize function
        var onresize = function(e) {
            blocklyDiv.style.left = blocklyArea.offsetLeft + 'px';
            blocklyDiv.style.top = blocklyArea.offsetTop  + 'px';
            blocklyDiv.style.width = blocklyArea.offsetWidth + 'px';
            blocklyDiv.style.height = blocklyArea.offsetHeight + 'px';
            //Program.code_editor.ace.resize();
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

          var timestamp = new Date(item.values().timestamp);
          var metadata = formatDateTime(timestamp) + " " + item.values().language;

          container.append($("<div/>").addClass("metadata").text(metadata));

          container.click(function () {
              var key = item.values().key;
              if ($("#programs").hasClass("loading")) {
                RemoteStorage.get(key, function(key, data) {
                  if (data.language == PYTHON_LANGUAGE){
                      Program.show("code_editor");
                  } else {
                      Program.show("blockly");
                  }
                  Program.current(data.code, data.language);
                });
                return;
              } 

              if ($("#programs").hasClass("saving")) {
                  // Check if read only
                  var read_only = item.values().read_only;
                  if (typeof read_only !== 'undefined' && read_only === true) {
                    Interface.notification("Read only program",
                        "This is a read-only program. Please choose another program.")
                    return;
                  }
                  var name = item.values().name;
                  var timestamp = (new Date()).toISOString();
                  var curr = Program.current();
                  RemoteStorage.set(key, {language: curr.language,
                      name: name,
                      timestamp: timestamp,
                      code: curr.code,
                      read_only: false});
              }

              Program.show();
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

          // Add language icon
          $lang_span = container.find("span.language")
          if ($lang_span && $lang_span.length == 1){
              if ($lang_span.data("language") == PYTHON_LANGUAGE){
                  $lang_span.html("<span class='glyphicon glyphicon-align-left'></span> ");
              }else{
                  $lang_span.html("<span class='glyphicon glyphicon-th'></span> ");
              }
          }

          // Add lock icon
          $read_only_span = container.find("span.read_only")
          if ($read_only_span && $read_only_span.length == 1){
              if ($read_only_span.attr("data-read-only") == "true"){
                  $read_only_span.html("<span class='glyphicon glyphicon-lock' style='color:red;'></span> ");
              }
          }

      }

      var updating = false;

      var list = new List("programs", {
          valueNames : ["name", "modified", { name: 'language', attr: 'data-language' }, { name: 'read_only', attr: 'data-read-only' }],
          item: "<a class='list-group-item'><div><span class='read_only'></span><span class='language'></span><span class='name'></span></div></a>"
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
          list.add([{key : "new", name: "", modified: "", timestamp: "zzzz", language: Program.current().language}]);
          list.update();
        
          var container = $(list.get("key", "new")[0].elm);
          container.addClass('editable');
          var textbox = container.find("span.name");
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
                  curr = Program.current();
                  RemoteStorage.set(key, {language: curr.language,
                      name: name,
                      timestamp: timestamp,
                      code: curr.code,
                      read_only: false});
                  list.remove("key", "new");
                  Program.show();
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
          Program.show();
        }
    }));

    var logconsole = $("#console");
    logconsole.click(function() {

      if ($(this).hasClass("terminated")) {
          Program.show();
      }

    });

    var run_button = $.manus.widgets.fancybutton({
      callback : function() {  
          Program.run();
          logconsole.empty();
      }, icon: "play", tooltip: "Run"
    });

    var to_python_code_button = $.manus.widgets.fancybutton({
      callback : function() {  
        if (Program.code_editor.shown){
            Interface.confirmation("Back to Blockly?",
                "If you haven't saved your python code you will lose it. Are you sure you want to go back to blockly?",
                function() {
                    Program.code_editor.shown = false;
                    Program.code_editor.in_use = false;
                    Program.show("blockly");
                    var $i = $("a.btn i.glyphicon-arrow-left");
                    if ($i) {
                        $i.removeClass("glyphicon-arrow-left");
                        $i.addClass("glyphicon-arrow-right");
                    }
                }
              );
        } else {
            Program.code_editor.ace.setValue(CodeParser.prepare_std_code(Blockly.Python.workspaceToCode(Program.blockly.workspace)));
            Program.code_editor.shown = true;
            Program.code_editor.in_use = true;
            Program.show("code_editor");
            var  $i = $("a.btn i.glyphicon-arrow-right");
            if ($i) {
                $i.removeClass("glyphicon-arrow-right");
                $i.addClass("glyphicon-arrow-left");
            }
        }
      },
      icons: ["th", "arrow-right", "align-left"], 
      tooltip: "Convert to python code"
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

    $("#program .toolbar.runtime").append(run_button).append(to_python_code_button).append(stop_button);
    $("#program .toolbar.storage").append(save_button).append(load_button);

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
      if (Program.code_editor && Program.code_editor.in_use){
            return { code: Program.code_editor.ace.getValue(), language: PYTHON_LANGUAGE }
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
        Program.code_editor.ace.setValue(data)
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
    if (panel == undefined){
        if (Program.code_editor) { 
            if (Program.code_editor.in_use)
                panel = "code_editor"
            else
                panel = "blockly"
        } else {
            panel = "blockly"
        }
        
    }
    if (Program.code_editor) Program.code_editor.shown = false;
    $("#program").children(".program-panel").hide();
    $("#program .storage").hide();
    $("#program .runtime").hide();
    if (panel == "blockly") {
      $("#program").children("#blockly").show();
      $("#program .runtime").show();
      $("#program .storage").show();
    } else if (panel == "code_editor") {
      if (Program.code_editor) Program.code_editor.shown = true;
      $("#program").children("#ace-code-editor-container").show();
      $("#program .runtime").show();
      $("#program .storage").show();
    } else if (panel == "console") {
      $("#program").children("#console").show();
      $("#program .runtime").show();
    } else  if (panel == "programs") {
      $("#program").children("#programs").show();
      $("#program .storage").show();
    }
    Program.update_state();
  },

  run : function() {
        var code = "";
        if (Program.code_editor.in_use) 
            code = Program.code_editor.ace.getValue();
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

    update_state : function() {
        var ace_visible = $("#ace-code-editor-container").is(":visible");
        var blockly_visible = $("#blockly").is(":visible");

        if (ace_visible && !blockly_visible){
            Program.code_editor.shown = true;
            Program.code_editor.in_use = true;
            var  $i = $("a.btn i.glyphicon-arrow-right");
            if ($i) {
                $i.removeClass("glyphicon-arrow-right");
                $i.addClass("glyphicon-arrow-left");
            }
        } else if (!ace_visible && blockly_visible) {
            Program.code_editor.shown = false;
            Program.code_editor.in_use = false;
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
        name = name;
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

