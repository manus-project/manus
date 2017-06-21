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
        .appendField(new Blockly.FieldDropdown([["Move joint 1","joint_1"], ["Move joint 2","joint_3"], ["Move joint 3","joint_3"], ["Move joint 4","joint_4"], ["Move joint 5","joint_5"], ["Move joint 7","joint_7"]]), "joint_dropdown");
    this.appendValueInput("position_value")
        .setCheck("Number")
        .appendField("to position");
    this.setInputsInline(true);
    this.setColour(0);
    this.setTooltip('');
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
    this.setInputsInline(true);
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