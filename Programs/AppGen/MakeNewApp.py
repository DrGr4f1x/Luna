'''
Copyright (c) Microsoft. All rights reserved.
This code is licensed under the MIT License (MIT).
THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.

Developed by Minigraph

Author:  James Stanard
'''

import os
import sys
import shutil
from glob import glob
from uuid import uuid4

APPS_FOLDER = "./Apps"
TEMPLATES_FOLDER = "./Programs/AppGen/Templates"

def copy_template_file(filename, project, guid):
    '''Copies one template file and replaces templated values'''
    template_filename = os.path.join(TEMPLATES_FOLDER, filename)
    output_filename = os.path.join(APPS_FOLDER, project)
    output_filename = os.path.join(output_filename, filename)
    output_filename = output_filename.replace('TEMPLATE', project)
    with open(template_filename, 'r', encoding='utf-8') as infile:
        with open(output_filename, 'w', encoding='utf-8') as outfile:
            contents = infile.read()
            contents = contents.replace('TEMPLATE_GUID', guid)
            contents = contents.replace('TEMPLATE_NAME', project)
            contents = contents.replace('TEMPLATE', project)
            outfile.write(contents)

def copy_app_template(project, guid):
    '''Instantiates a new solution from a template'''
    copy_template_file('Stdafx.h', project, guid)
    copy_template_file('Stdafx.cpp', project, guid)
    copy_template_file('TEMPLATEApp.h', project, guid)
    copy_template_file('TEMPLATEApp.cpp', project, guid)
    copy_template_file('Main.cpp', project, guid)
    copy_template_file('TEMPLATE.vcxproj', project, guid)
    copy_template_file('TEMPLATE.vcxproj.filters', project, guid)
    copy_template_file('packages.config', project, guid)

def create_project():
    if len(sys.argv) != 2:
        print('Usage:  {0} <ProjectName>'.format(sys.argv[0]))
        return

    project_name = sys.argv[1]
    folder_contents = set(os.listdir())
    app_folder_contents = set(os.listdir(APPS_FOLDER))
    expected_contents = set(['README.md', 'Apps', 'Engine', 'External', 'Programs'])
    if not expected_contents.issubset(folder_contents):
        print('Run this script from the root of Luna')
    elif project_name in app_folder_contents:
        print('Project already exists')
    else:
        new_app_dir = os.path.join(APPS_FOLDER, project_name)
        os.mkdir(new_app_dir)
        copy_app_template(project_name, str(uuid4()).upper())
        

if __name__ == "__main__":
    create_project()
