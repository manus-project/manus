var workspace;

$(document).ready(function(){
    var $blockly_container = $( "#blockly-container" );
    if ($blockly_container.length == 0) {
        console.error("Missing blockly container!");
        return;
    } 

    $blockly_container.append('\
      <p> \
        <button class="btn btn-primary" onclick="showCode()">Show Python</button> \
        <button class="btn btn-primary" onclick="runCode()">Run Python</button> \
        <button class="btn btn-primary" onclick="saveCode()">Save</button> \
        <button class="btn btn-primary" onclick="loadCode()">Load</button> \
      </p> \
      <div id="blockly-workspace" style="height: 480px; width: 600px;"></div> \
      <xml id="toolbox" style="display: none"> \
        <category name="Logic" colour="210"> \
          <block type="controls_if"></block> \
          <block type="controls_ifelse"></block> \
          <block type="logic_compare"></block> \
          <block type="logic_operation"></block> \
          <block type="logic_negate"></block> \
          <block type="logic_boolean"></block> \
          <block type="logic_null"></block> \
          <block type="logic_ternary"></block> \
        </category> \
        <category name="Loops" colour="120"> \
          <block type="controls_repeat_ext"></block> \
          <block type="controls_whileUntil"></block> \
          <block type="controls_for"></block> \
          <block type="controls_forEach"></block> \
          <block type="controls_flow_statements"></block> \
        </category> \
        <category name="Math" colour="230"> \
          <block type="math_number"></block> \
          <block type="math_arithmetic"></block> \
          <block type="math_single"></block> \
          <block type="math_trig"></block> \
          <block type="math_constant"></block> \
          <block type="math_number_property"></block> \
          <block type="math_round"></block> \
          <block type="math_on_list"></block> \
          <block type="math_modulo"></block> \
          <block type="math_constrain"></block> \
          <block type="math_random_int"></block> \
          <block type="math_random_float"></block> \
        </category> \
        <category name="Text" colour="160"> \
          <block type="text"></block> \
          <block type="text_join"></block> \
          <block type="text_append"></block> \
          <block type="text_length"></block> \
          <block type="text_isEmpty"></block> \
          <block type="text_indexOf"></block> \
          <block type="text_charAt"></block> \
          <block type="text_getSubstring"></block> \
          <block type="text_changeCase"></block> \
          <block type="text_trim"></block> \
          <block type="text_print"></block> \
          <block type="text_prompt_ext"></block> \
          <block type="text_count"></block> \
          <block type="text_replace"></block> \
          <block type="text_reverse"></block> \
        </category> \
        <category name="Lists" colour="260"> \
          <block type="lists_create_empty"></block> \
          <block type="lists_create_with"></block> \
          <block type="lists_repeat"></block> \
          <block type="lists_length"></block> \
          <block type="lists_isEmpty"></block> \
          <block type="lists_indexOf"></block> \
          <block type="lists_getIndex"></block> \
          <block type="lists_setIndex"></block> \
          <block type="lists_getSublist"></block> \
          <block type="lists_sort"></block> \
          <block type="lists_split"></block> \
          <block type="lists_reverse"></block> \
        </category> \
        <category name="Colour" colour="20"> \
          <block type="colour_picker"></block> \
          <block type="colour_random"></block> \
          <block type="colour_rgb"></block> \
          <block type="colour_blend"></block> \
        </category> \
        <category name="Robot Arm" colour="0"> \
            <block type="manus_move_joint"></block> \
            <block type="manus_move_arm"></block> \
            <block type="manus_position_vector"></block> \
            <block type="manus_position_vector_var"></block> \
            <block type="manus_detected_blocks_array"></block> \
            <block type="manus_detect_and_store_blocks"></block> \
            <block type="manus_wait"></block> \
            <block type="manus_retrieve_component"></block> \
        </category> \
        <sep></sep> \
        <category name="Variables" colour="330" custom="VARIABLE"></category> \
        <category name="Functions" colour="290" custom="PROCEDURE"></category> \
      </xml> \
      <xml id="startBlocks" style="display: none"> \
        <block type="controls_if" x="20" y="20" > \
            <value name="IF0"> \
                <block type="manus_any_block_detector" /> \
            </value> \
            <statement name="DO0"> \
                <block type="manus_move_arm" > \
                    <value name="coordinates"> \
                    <block type="manus_position_vector_var"> \
                        <value name="X"> \
                            <block type="variables_get" > \
                                <field name="VAR">detection_x</field> \
                            </block> \
                        </value> \
                        <value name="Y"> \
                            <block type="variables_get"> \
                                <field name="VAR">detection_y</field> \
                            </block> \
                        </value> \
                        <value name="Z"> \
                            <block type="variables_get"> \
                                <field name="VAR">detection_z</field> \
                            </block> \
                        </value> \
                    </block> \
                    </value> \
                </block> \
            </statement> \
        </block> \
      </xml>');

    workspace = Blockly.inject('blockly-workspace',
      {grid:
         {spacing: 25,
          length: 3,
          colour: '#ccc',
          snap: true},
       media: 'blockly/media/',
       toolbox: document.getElementById('toolbox'),
       zoom: {controls: true, wheel: true}
      });

    // Register shown callback to resize the workspace (othervise it remains hidden until window resize)
    $('a[data-toggle="tab"]').on('shown.bs.tab', function (e) {
        if (workspace)
          Blockly.svgResize(workspace);
    })
    // Register on create and delete callbacks
    function onEvent(event) {
        if (event.type == Blockly.Events.CHANGE &&
            event.element == 'comment' &&
            !event.oldValue && event.newValue) {
            alert('Congratulations on creating your first comment!')
            workspace.removeChangeListener(onFirstComment);
        }
}
    workspace.addChangeListener(function(event){
        if (event.type == Blockly.Events.CREATE)
            onBlockCreate(event);
        else if (event.type == Blockly.Events.DELETE)
            onBlockDelete(event);
    });

});

function showCode() {
    // Generate Python code and display it.
    var code = Blockly.Python.workspaceToCode(workspace);
    alert(code);
}

function runCode() {
    // Generate code
    var code = Blockly.Python.workspaceToCode(workspace);
    // Also generate xml
    var xml = Blockly.Xml.workspaceToDom(workspace);
    var xml_text = Blockly.Xml.domToText(xml);
    // Validate xml code
    err = error_in_xml_code(xml);
    if (err){
        alert(err);
        return;
    }
    callwebapi("/api/code/submit", 
        {
            "code": code,
            "xml_code": xml_text,
        },
        function(data) {
            console.log("code submition successful!");
            if (!('status' in data)){
                console.error("status field missing from respons data");
            }
            if (data.status != "OK"){
                if (!('description' in data)){
                    console.error("description field missing from non-OK response data");
                }
                alert("Code execution failed. "+data.status+": "+data.description);
            }
        },
        function(){
            alert("code submition FAILED!");
        }
    );
}

function saveCode(){
     // Generate xml
    var xml = Blockly.Xml.workspaceToDom(workspace);
    var xml_text = Blockly.Xml.domToText(xml);
    // Store to client storage
    if (typeof(Storage) !== "undefined") {
        localStorage.setItem('saved_code', xml_text);
    } else {
        alert("Client storage not supported by browser");
        console.error("Client storage not supported by browser");
    }
}

function loadCode(){
    // Retrieve code from client storage
    if (typeof(Storage) !== "undefined") {
        var xml_text = localStorage.getItem('saved_code');
        // Clear & Load xml into workspace
        if (xml_text && xml_text.length > 0) {
            var xml_dom = Blockly.Xml.textToDom(xml_text);
            workspace.clear(); // Don't forget to call clear before load. Othervie it will just add more elements to workspace.
            Blockly.Xml.domToWorkspace(xml_dom, workspace);
        }
    } else {
        alert("Client storage not supported by browser");
        console.error("Client storage not supported by browser");
    }
}

function callwebapi(url, reqdata_raw, ok_func, err_func) {
    var reqdata = JSON.stringify(reqdata_raw, null);

    // Send data using POST
    $.ajax({
        url: url,
        dataType: 'json',
        type: 'POST',
        data: reqdata,
        jsonp: false,
        timeout: 2000,
        success: function (data) {
            console.log("Successfully posted to "+url+". It responded with "+data);
            if ('function' === typeof (ok_func)) {
                ok_func(data);
            }
        },
        error: function (jqXHR, textStatus, errorThrown ) {
          console.error("Failed to call "+url+" because "+textStatus+": "+errorThrown);
          if ('function' === typeof (err_func)) {
              err_func();
          }
        }
    });
}



var detector_count = 0;
function onBlockCreate(event){
    var block_type = event.xml.getAttribute("type");
    if (block_type == "manus_any_block_detector" || block_type == "manus_colored_block_detector"){
        if (detector_count == 0){
            workspace.createVariable("detection_x");
            workspace.createVariable("detection_y");
            workspace.createVariable("detection_z");
            workspace.createVariable("detection_color");
        }
        detector_count++;
    }
}

function onBlockDelete(event){
    var block_type = event.oldXml.getAttribute("type");
    if (block_type == "manus_any_block_detector" || block_type == "manus_colored_block_detector"){
        detector_count--;
        if (detector_count == 0){
            workspace.deleteVariable("detection_x");
            workspace.deleteVariable("detection_y");
            workspace.deleteVariable("detection_z");
            workspace.createVariable("detection_color");
        }
    }
}

function error_in_xml_code(xml){
    return false; // Code is ok
}

function treverse_xml(depth, xml, element_callback){
    if (!element_callback){
        console.error("element_callback function not set");
        return;
    }
    if (depth > 300) {
        console.error("Recursion limit reached. Something must have gone wrong while traversing xml tree.");
        return;
    }
    var c = xml.firstChild;
    while (c) {
        // Call element_callback
        should_continue = element_callback(c);
        if (!should_continue)
            return false;
        // Continue treversing
        if (c.hasChildNodes())
            if (!treverse_xml(depth+1, c, element_callback))
                return false;
        c = c.nextSibling;
    }
    return true;
}