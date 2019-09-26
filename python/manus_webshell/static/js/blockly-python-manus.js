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
  return 'workspace.manipulator.joint('+ dropdown_joint_id +', '+ angle_joint_angle +' * math.pi / 180)\n';
};

Blockly.Python['manus_move_joint_variable'] = function(block) {
  var dropdown_joint_id = block.getFieldValue('joint_id');
  var joint_angle = Blockly.Python.valueToCode(block, 'joint_angle', Blockly.Python.ORDER_ATOMIC) || "0";
  return 'workspace.manipulator.joint('+ dropdown_joint_id +', '+ joint_angle +' * math.pi / 180)\n';
};

Blockly.Python['manus_retrieve_joint'] = function(block) {
  var dropdown_joint_id = block.getFieldValue('joint_id');
  // TODO: Assemble Python into code variable.
  var code = 'workspace.manipulator.state().joints['+ dropdown_joint_id + '].position';
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_retrieve_joint_position'] = function(block) {
  var dropdown_joint_id = block.getFieldValue('joint_id');
  var code = 'workspace.manipulator.position('+ dropdown_joint_id + ') * 180 / math.pi';
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_retrieve_gripper_position'] = function(block) {
  var code = 'workspace.manipulator.position(6)';
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_position_vector'] = function(block) {
  var number_x = block.getFieldValue('X');
  var number_y = block.getFieldValue('Y');
  var number_z = block.getFieldValue('Z');
  // TODO: Assemble Python into code variable.
  var code = '(' + number_x+', '+number_y+', '+number_z + ')';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_position_vector_var'] = function(block) {
  var value_x = Blockly.Python.valueToCode(block, 'X', Blockly.Python.ORDER_ATOMIC) || "0";
  var value_y = Blockly.Python.valueToCode(block, 'Y', Blockly.Python.ORDER_ATOMIC) || "0";
  var value_z = Blockly.Python.valueToCode(block, 'Z', Blockly.Python.ORDER_ATOMIC) || "0";
  // TODO: Assemble Python into code variable.
  var code = '(' + value_x+', '+value_y+', '+value_z + ')';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_move_arm'] = function(block) {
  var value_coordinates = Blockly.Python.valueToCode(block, 'coordinates', Blockly.Python.ORDER_ATOMIC) || "(0.0,0.0,0.0)";
  // TODO: Assemble Python into code variable.
  return 'workspace.manipulator.trajectory([ manus.MoveTo(('+value_coordinates+'), None, workspace.manipulator.gripper())])\n';
};

Blockly.Python['manus_safe_position'] = function(block) {
  return 'workspace.manipulator.safe()\n';
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

  var functionName = Blockly.Python.provideFunction_(
      'get_coordinate',
      ['def ' + Blockly.Python.FUNCTION_NAME_PLACEHOLDER_ + '(obj, coordinate):',
       '  if isinstance(obj, (tuple, list)):',
       '    mapping = {"x" : 0, "y" : 1, "z" : 2, "w": 3}',
       '    return obj[mapping[coordinate]]',
       '  if hasattr(obj, "__coordinate__"):',
       '    return obj.__coordinate__(coordinate)']);
  var dropdown_component_dropdown = block.getFieldValue('coordinate_dropdown');
  var varname = Blockly.Python.variableDB_.getName(block.getFieldValue('varname'), Blockly.Variables.NAME_TYPE);
  var code = functionName + "(" + varname + ', "' + dropdown_component_dropdown + '")';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_FUNCTION_CALL];
};

Blockly.Python['manus_retrieve_rotation'] = function(block) {

  var functionName = Blockly.Python.provideFunction_(
      'get_rotation',
      ['def ' + Blockly.Python.FUNCTION_NAME_PLACEHOLDER_ + '(obj, coordinate):',
       '  if isinstance(obj, (tuple, list)):',
       '    mapping = {"x" : 0, "y" : 1, "z" : 2, "w": 3}',
       '    return obj[mapping[coordinate]] * 180 / math.pi',
       '  if hasattr(obj, "__rotation__"):',
       '    return obj.__rotation__(coordinate) * 180 / math.pi']);
  var dropdown_component_dropdown = block.getFieldValue('coordinate_dropdown');
  var varname = Blockly.Python.variableDB_.getName(block.getFieldValue('varname'), Blockly.Variables.NAME_TYPE);
  var code = functionName + "(" + varname + ', "' + dropdown_component_dropdown + '")';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_FUNCTION_CALL];
};


Blockly.Python['manus_retrieve_color'] = function(block) {
  var varname = Blockly.Python.variableDB_.getName(block.getFieldValue('varname'), Blockly.Variables.NAME_TYPE);
  // TODO: Assemble Python into code variable.
  var code = 'block_color_name(' + varname + ')';
  // TODO: Change ORDER_NONE to the correct strength.
  return [code, Blockly.Python.ORDER_NONE];
};

Blockly.Python['manus_open_close_gripper'] = function(block) {
  var dropdown_open_close_option = block.getFieldValue('open_close_option');
  var mapping = {"open" : 0.1, "half" : 0.5, "close" : 0.8}
  return 'workspace.manipulator.gripper(' + mapping[dropdown_open_close_option] + ')\n';
};