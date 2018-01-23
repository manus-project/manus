#!/usr/bin/env python
import os
import traceback


class AppGenerator(object):

    def __init__(self, target_dir):
        self.template = ""
        current_dir = os.path.dirname(os.path.realpath(__file__))
        self.read_template(os.path.join(current_dir, "code_template.tpl"))
        self.app_file_content = """Auto-generated App
1
generated_app.py
Automatically generated app

Automatically generated app
"""

    def read_template(self, template_path):
        try:
            if not os.path.isfile(template_path):
                return False
            tmpl_file = open(template_path, "r")
            self.template = tmpl_file.read()
            tmpl_file.close()
            return True
        except Exception as e:
            return False

    def prefix_lines_with_spaces(self, code, num_of_spaces):
        res = list()
        prefix = " " * num_of_spaces
        for line in code.split("\n"):
            if len(line) == 0:
                res.append(line)
            else:
                res.append(prefix + line)
        return "\n".join(res)

    def generate(self, code, destination=None):
        try:

            # Indent code with 4 spaces
            code = self.prefix_lines_with_spaces(code, 4)
            code = self.template.replace("{{code_container}}", code)

            if (not destination == None):
                app_file_path = os.path.join(destination, "generated_app.app")
                python_file_path = os.path.join(destination, "generated_app.py")

                app_file = open(app_file_path, "w")
                python_file = open(python_file_path, "w")

                app_file.write(self.app_file_content)
                python_file.write(code)

                app_file.close()
                python_file.close()
            return True
        except Exception as e:
            return False
