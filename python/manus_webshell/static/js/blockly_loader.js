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
        <category name="Manus" colour="0"> \
          <block type="manus_move_joint"></block> \
          <block type="manus_position_vector"></block> \
          <block type="manus_position_vector_var"></block> \
          <block type="manus_move_arm"></block> \
          <block type="manus_wait"></block> \
        </category> \
        <sep></sep> \
        <category name="Variables" colour="330" custom="VARIABLE"></category> \
        <category name="Functions" colour="290" custom="PROCEDURE"></category> \
      </xml> \
      <xml id="startBlocks" style="display: none"> \
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
});

function showCode() {
    // Generate Python code and display it.
    var code = Blockly.Python.workspaceToCode(workspace);
    alert(code);
}

function runCode() {
    var code = Blockly.Python.workspaceToCode(workspace);
    callwebapi("/api/code/submit", 
        {"code": code},
        function(data) {
            alert("code submition successful!");
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