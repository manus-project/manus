/**
 * @license
 * Visual Blocks Editor
 *
 * Copyright 2012 Google Inc.
 * https://developers.google.com/blockly/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

'use strict';

goog.require('Blockly.Blocks');

Blockly.Blocks['text_newline'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("new line")
    this.setOutput(true, "String");
    this.setColour(160);
    this.setTooltip('New line character.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_move_joint'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("move joint ")
        .appendField(new Blockly.FieldDropdown([["1","0"], ["2","1"], ["3","2"], ["4","3"], ["5","5"]]), "joint_id");
    this.appendDummyInput()
        .appendField(" to ")
        .appendField(new Blockly.FieldNumber(0), "joint_angle")
        .appendField(" degrees.");
    this.setInputsInline(true);
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(20);
    this.setTooltip('');
    this.setHelpUrl('Set the angle for one joint of the manipulator.');
  }
};

Blockly.Blocks['manus_retrieve_joint'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("get joint ")
        .appendField(new Blockly.FieldDropdown([["1","0"], ["2","1"], ["3","2"], ["4","3"], ["5","5"]]), "joint_id")
        .appendField(" position.");
    this.setOutput(true, null);
    this.setColour(20);
    this.setTooltip('Retrieves position of a manipulator\'s joint.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_position_vector'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("coordinates");
    this.appendDummyInput()
        .appendField("x:")
        .appendField(new Blockly.FieldNumber(0), "X");
    this.appendDummyInput()
        .appendField("y:")
        .appendField(new Blockly.FieldNumber(0), "Y");
    this.appendDummyInput()
        .appendField("z:")
        .appendField(new Blockly.FieldNumber(0), "Z");
    this.setInputsInline(true);
    this.setOutput(true, "manus_position_vector");
    this.setColour(230);
    this.setTooltip('Constant position vector');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_position_vector_var'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("coordinates");
    this.appendValueInput("X")
        .setCheck("Number")
        .appendField("x:");
    this.appendValueInput("Y")
        .setCheck("Number")
        .appendField("y:");
    this.appendValueInput("Z")
        .setCheck("Number")
        .appendField("z:");
    this.setInputsInline(false);
    this.setOutput(true, "manus_position_vector");
    this.setColour(230);
    this.setTooltip('Variable position vector');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_move_arm'] = {
  init: function() {
    this.appendValueInput("coordinates")
        .setCheck("manus_position_vector")
        .appendField("move manipulator to");
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(20);
    this.setTooltip('Move manipulator\'s gripper to coordinates');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_wait'] = {
  init: function() {
    this.appendValueInput("wait_val")
        .setCheck("Number")
        .appendField("wait for");
    this.appendDummyInput()
        .appendField("milliseconds");
    this.setInputsInline(true);
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(100);
    this.setTooltip('');
    this.setHelpUrl('Wait halts execution for number of milliseconds');
  }
};

Blockly.Blocks['manus_any_block_detector'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("any block is visible");
    this.setInputsInline(true);
    this.setOutput(true, "Boolean");
    this.setColour(60);
    this.setTooltip('Returns true if it sees at least one block');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_colored_block_detector'] = {
  init: function() {
    this.appendDummyInput()
        .appendField(new Blockly.FieldDropdown([["Red","red"], ["Blue","blue"], ["Green","green"]]), "block_color")
        .appendField("block is visible");
    this.setInputsInline(true);
    this.setOutput(true, "Boolean");
    this.setColour(60);
    this.setTooltip('Returns true if it sees block with specified color');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_detected_blocks_array'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("all detected blocks");
    this.setOutput(true, "Array");
    this.setColour(60);
    this.setTooltip('Array of all detected blocks');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_detect_and_store_blocks'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("detect and store blocks into:")
        .appendField(new Blockly.FieldVariable("blocks"), "BLOCKS");
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(60);
    this.setTooltip('Detects all blocks and stores them in an array');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_retrieve_color'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("get color of")
        .appendField(new Blockly.FieldVariable(null), "SELECTED_BLOCK_FOR_COMPONENT_ACCESS");
    this.setOutput(true, null);
    this.setColour(60);
    this.setTooltip('Retrieves color of a given block.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_retrieve_coordinate'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("get")
        .appendField(new Blockly.FieldDropdown([["x","x"], ["y","y"], ["z","z"]]), "coordinate_dropdown")
        .appendField("of block")
        .appendField(new Blockly.FieldVariable(null), "SELECTED_BLOCK_FOR_COMPONENT_ACCESS");
    this.setOutput(true, null);
    this.setColour(60);
    this.setTooltip('Retrieves coordinate of a given block.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_open_close_gripper'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("gripper")
        .appendField(new Blockly.FieldDropdown([["open","open"], ["half","half"], ["close","close"]]), "open_close_option");
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(20);
    this.setTooltip('Open or close manipulator\'s gripper');
    this.setHelpUrl('');
  }
};

