#!/bin/python

import re, sys
from os import path, listdir
from subprocess import check_call, check_output

if len(sys.argv) < 3:
    print 'create_proj.py proj_name proj_dir'
    sys.exit(1)

proj_name = sys.argv[1]
projects = list(d for d in listdir('.') if path.isdir(d))
if proj_name not in projects:
    print proj_name + ' is not a valid project, projects are listed below:'
    print '\n'.join(projects)
    sys.exit(2)

proj_dir = sys.argv[2]
if not path.exists(proj_dir):
    print proj_dir + ' doesn''t exist'
    sys.exit(3)

android_api_level_cmd = "android list targets|gawk '/API level/ {print $3}'"
android_api_level = check_output(android_api_level_cmd, shell=True)
android_api_level = re.match(r'\d+', android_api_level).group(0)

cocos_new_cmd = 'cocos new {0} -p {1} -l cpp -d {2}'.format(proj_name, android_api_level, proj_dir)
check_call(cocos_new_cmd, shell=True)

cmp_cmd = 'BCompare ./{0} {1}'.format(proj_name, path.join(proj_dir, proj_name))
check_call(cmp_cmd, shell=True)

