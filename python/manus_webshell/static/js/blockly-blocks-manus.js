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

/**
 * @fileoverview Text blocks for Blockly.
 * @author fraser@google.com (Neil Fraser)
 */
'use strict';

goog.require('Blockly.Blocks');

Blockly.Blocks['manus_move_joint'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Move joint ")
        .appendField(new Blockly.FieldDropdown([["1","0"], ["2","1"], ["3","2"], ["4","3"], ["5","5"]]), "joint_id");
    this.appendDummyInput()
        .appendField(" to ")
        .appendField(new Blockly.FieldNumber(0), "joint_angle")
        .appendField(" degrees.");
    this.setInputsInline(true);
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(0);
    this.setTooltip('');
    this.setHelpUrl('Set the angle for one joint.');
  }
};

Blockly.Blocks['manus_retrieve_joint'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Get joint ")
        .appendField(new Blockly.FieldDropdown([["1","0"], ["2","1"], ["3","2"], ["4","3"], ["5","5"]]), "joint_id")
        .appendField(" position.");
    this.setOutput(true, null);
    this.setColour(0);
    this.setTooltip('Retrieves position of a block.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_position_vector'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Coordinates");
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
    this.setColour(0);
    this.setTooltip('Manus constant position vector');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_position_vector_var'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Coordinates");
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
    this.setColour(0);
    this.setTooltip('Manus variable position vector');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_move_arm'] = {
  init: function() {
    this.appendValueInput("coordinates")
        .setCheck("manus_position_vector")
        .appendField("Move arm to");
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(0);
    this.setTooltip('Move robot arm to coordinates');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_wait'] = {
  init: function() {
    this.appendValueInput("wait_val")
        .setCheck("Number")
        .appendField("Wait");
    this.setInputsInline(true);
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(0);
    this.setTooltip('');
    this.setHelpUrl('Wait halts execution for number of miliseconds');
  }
};

Blockly.Blocks['manus_any_block_detector'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Any block is visible");
    this.setInputsInline(true);
    this.setOutput(true, "Boolean");
    this.setColour(0);
    this.setTooltip('Returns true if it sees any block');
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
    this.setColour(0);
    this.setTooltip('Returns true if it sees block with specified colour');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_detected_blocks_array'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("All detected blocks");
    this.setOutput(true, "Array");
    this.setColour(0);
    this.setTooltip('Array of all detected blocks');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_detect_and_store_blocks'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Detect and store blocks into:")
        .appendField(new Blockly.FieldVariable("blocks"), "BLOCKS");
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(0);
    this.setTooltip('Detects all blocks and stores them in array');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_retrieve_color'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Get color from")
        .appendField(new Blockly.FieldVariable(null), "SELECTED_BLOCK_FOR_COMPONENT_ACCESS");
    this.setOutput(true, null);
    this.setColour(0);
    this.setTooltip('Retrieves color of block.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_retrieve_coordinate'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Get")
        .appendField(new Blockly.FieldDropdown([["x","x"], ["y","y"], ["z","z"]]), "coordinate_dropdown")
        .appendField("from")
        .appendField(new Blockly.FieldVariable(null), "SELECTED_BLOCK_FOR_COMPONENT_ACCESS");
    this.setOutput(true, null);
    this.setColour(0);
    this.setTooltip('Retrieves coordinate from point.');
    this.setHelpUrl('');
  }
};

Blockly.Blocks['manus_open_close_gripper'] = {
  init: function() {
    this.appendDummyInput()
        .appendField("Gripper")
        .appendField(new Blockly.FieldDropdown([["open","open"], ["half","half"], ["close","close"]]), "open_close_option");
    this.setPreviousStatement(true, null);
    this.setNextStatement(true, null);
    this.setColour(0);
    this.setTooltip('Open or close gripper');
    this.setHelpUrl('');
  }
};

