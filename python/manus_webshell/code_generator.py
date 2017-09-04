#!/usr/bin/env python
import os
import traceback

class CodeGenerator(object):

    def __init__(self, template_path, target_dir):
        self.template_path = template_path
        if target_dir[len(target_dir)-1] is not "/":
            target_dir = target_dir+"/"
        self.target_dir = target_dir
        self.template = ""
        self.read_template()
        
        self.app_file_content = """Generated App
1
generated_app.py
Blockly generated app

App generated from Blockly code
"""

    def read_template(self):
        try:
            if not os.path.isfile(self.template_path):
                raise RuntimeError("template file at path "+self.template_path+" not found!")
            tmpl_file = open(self.template_path, "r")
            self.template = tmpl_file.read()
            tmpl_file.close() 
        except Exception as e:
            raise RuntimeError("failed to read template file: "+e.message)

    def generate_code_with_code(self, code):
        # Indent code with 4 spaces
        code = self.prefix_lines_with_spaces(code, 4)
        return self.template.replace("{{code_container}}", code)
    
    def prefix_lines_with_spaces(self, code, num_of_spaces):
        res = list()
        prefix = " " * num_of_spaces
        for line in code.split("\n"):
            if len(line) == 0:
                res.append(line)
            else:
                res.append(prefix+line)
        return "\n".join(res)

    def generate_app_with_code(self, code, write_to_disk=False, wrap_with_template=False):
        try:
            res_code = code
            if wrap_with_template:
                res_code = self.generate_code_with_code(code)
            if (write_to_disk):
                app_file_path = self.target_dir+"generated_app.app"
                python_file_path = self.target_dir+"generated_app.py"

                app_file = open(app_file_path, "w")
                python_file = open(python_file_path, "w")

                app_file.write(self.app_file_content)
                python_file.write(res_code)

                app_file.close()
                python_file.close
            return res_code
        except Exception as e:
            raise RuntimeError("failed to generate app: "+e.message+"\n"+traceback.format_exc())

