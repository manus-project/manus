/**
 * @license
 * Visual Blocks Language
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
 * @fileoverview Generating Python for text blocks.
 * @author q.neutron@gmail.com (Quynh Neutron)
 */
'use strict';

goog.require('Blockly.Python');

Blockly.Python['text_newline'] = function(block) {
  return ["\"\\n\"", Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_move_joint'] = function(block) {
  var dropdown_joint_id = block.getFieldValue('joint_id');
  var angle_joint_angle = block.getFieldValue('joint_angle');
  // TODO: Assemble Python into code variable.

  angle_joint_angle = angle_joint_angle * Math.PI / 180.0;
  return 'workspace.manipulator.joint('+ dropdown_joint_id +', '+ angle_joint_angle +')\n';
};

Blockly.Python['manus_retrieve_joint'] = function(block) {
  var dropdown_joint_id = block.getFieldValue('joint_id');
  // TODO: Assemble Python into code variable.
  var code = 'workspace.manipulator.state().joints['+ dropdown_joint_id + '].position';
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_position_vector'] = function(block) {
  var number_x = block.getFieldValue('X');
  var number_y = block.getFieldValue('Y');
  var number_z = block.getFieldValue('Z');
  // TODO: Assemble Python into code variable.
  var code = number_x+', '+number_y+', '+number_z;
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_position_vector_var'] = function(block) {
  var value_x = Blockly.Python.valueToCode(block, 'X', Blockly.Python.ORDER_ATOMIC) || "0";
  var value_y = Blockly.Python.valueToCode(block, 'Y', Blockly.Python.ORDER_ATOMIC) || "0";
  var value_z = Blockly.Python.valueToCode(block, 'Z', Blockly.Python.ORDER_ATOMIC) || "0";
  // TODO: Assemble Python into code variable.
  var code = value_x+', '+value_y+', '+value_z;
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_move_arm'] = function(block) {
  var value_coordinates = Blockly.Python.valueToCode(block, 'coordinates', Blockly.Python.ORDER_ATOMIC) || "(0.0,0.0,0.0)";
  // TODO: Assemble Python into code variable.
  return 'workspace.manipulator.trajectory([ manus.MoveTo(('+value_coordinates+'), (0.0, 0.0, 0.0), workspace.manipulator.gripper())])\n';
};

Blockly.Python['manus_wait'] = function(block) {
  var value_wait_val = Blockly.Python.valueToCode(block, 'wait_val', Blockly.Python.ORDER_ATOMIC) || "0";
  // TODO: Assemble Python into code variable.
  return 'workspace.wait('+value_wait_val+')\n';
};
Blockly.Python['manus_detected_blocks_array'] = function(block) {
  // TODO: Assemble Python into code variable.
  var code = 'workspace.detect_blocks()';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_detect_and_store_blocks'] = function(block) {
  var variable_blocks = Blockly.Python.variableDB_.getName(block.getFieldValue('BLOCKS'), Blockly.Variables.NAME_TYPE);
  // TODO: Assemble Python into code variable.
  return variable_blocks+' = workspace.detect_blocks()\n';
};

Blockly.Python['manus_retrieve_coordinate'] = function(block) {
  var dropdown_component_dropdown = block.getFieldValue('coordinate_dropdown');
  var variable_selected_block_for_component_access = Blockly.Python.variableDB_.getName(block.getFieldValue('SELECTED_BLOCK_FOR_COMPONENT_ACCESS'), Blockly.Variables.NAME_TYPE);
  // TODO: Assemble Python into code variable.
  var mapping = {"x" : 0, "y" : 1, "z" : 2}
  var code = variable_selected_block_for_component_access + '.position[' +mapping[dropdown_component_dropdown] + ']';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_retrieve_color'] = function(block) {
  var variable_selected_block_for_component_access = Blockly.Python.variableDB_.getName(block.getFieldValue('SELECTED_BLOCK_FOR_COMPONENT_ACCESS'), Blockly.Variables.NAME_TYPE);
  // TODO: Assemble Python into code variable.
  var code = 'block_color_name(' + variable_selected_block_for_component_access + ')';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_open_close_gripper'] = function(block) {
  var dropdown_open_close_option = block.getFieldValue('open_close_option');
  var mapping = {"open" : 0, "half" : 0.5, "close" : 1}
  return 'workspace.manipulator.gripper(' + mapping[dropdown_open_close_option] + ')\n';
};