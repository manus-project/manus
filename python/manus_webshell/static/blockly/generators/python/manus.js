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

Blockly.Python['manus_move_joint'] = function(block) {
  var dropdown_joint_id = block.getFieldValue('joint_id');
  var angle_joint_angle = block.getFieldValue('joint_angle');
  // TODO: Assemble Python into code variable.
  var code = 'manus_move_joint('+ dropdown_joint_id +', '+ angle_joint_angle +')\n';
  return code;
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
  var value_coordinates = Blockly.Python.valueToCode(block, 'coordinates', Blockly.Python.ORDER_ATOMIC) || "(0,0,0)";
  // TODO: Assemble Python into code variable.
  var code = 'manus_move_arm_to_coordinates('+value_coordinates+')\n';
  return code;
};

Blockly.Python['manus_wait'] = function(block) {
  var value_wait_val = Blockly.Python.valueToCode(block, 'wait_val', Blockly.Python.ORDER_ATOMIC) || "0";
  // TODO: Assemble Python into code variable.
  var code = 'manus_wait('+value_wait_val+')\n';
  return code;
};
/*
Blockly.Python['manus_any_block_detector'] = function(block) {
  // TODO: Assemble Python into code variable.
  var code = 'manus_any_block_detected()';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_colored_block_detector'] = function(block) {
  var dropdown_block_color = block.getFieldValue('block_color');
  // TODO: Assemble Python into code variable.
  var code = 'manus_block_with_color_detected("'+dropdown_block_color+'")';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};
*/
Blockly.Python['manus_detected_blocks_array'] = function(block) {
  // TODO: Assemble Python into code variable.
  var code = 'manus_detect_blocks()';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_detect_and_store_blocks'] = function(block) {
  var variable_blocks = Blockly.Python.variableDB_.getName(block.getFieldValue('BLOCKS'), Blockly.Variables.NAME_TYPE);
  // TODO: Assemble Python into code variable.
  var code = variable_blocks+' = manus_detect_blocks()\n';
  return code;
};

Blockly.Python['manus_retrieve_component'] = function(block) {
  var dropdown_component_dropdown = block.getFieldValue('component_dropdown');
  var variable_selected_block_for_component_access = Blockly.Python.variableDB_.getName(block.getFieldValue('SELECTED_BLOCK_FOR_COMPONENT_ACCESS'), Blockly.Variables.NAME_TYPE);
  // TODO: Assemble Python into code variable.
  var code = 'manus_retrieve_component_from_block("'+dropdown_component_dropdown+'", '+variable_selected_block_for_component_access+')';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};