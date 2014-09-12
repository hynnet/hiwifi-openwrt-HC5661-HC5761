#!/bin/env python
# -*- coding: utf-8 -*-
"""Copyright (c) 2014 GeekGeek, Inc. All Rights Reserved
    @author: Fibrizof(dongyu.fang@hiwifi.tw)
    @brief 递归的将lua的文本程序转成字节码程序, 并替换原来的名字
    ./rluac.py $(STAGING_DIR_HOST) $$(IDIR_$(1))
"""
import sys
import os

WHITE_LIST = ["debug.lua",]

def change_lua_to_bytecode(name):
    mode = os.stat(name)[0]
    after_name = name + ".after"
    cmd = LUAC_CMD % (after_name, name)
    if 0 != os.system(cmd):
        raise Exception, "%s not exit on 0" % cmd
    os.remove(name)
    os.rename(after_name, name)
    os.chmod(name, mode)

def recursive_change(rootdir):
    for root, dirs, files in os.walk(rootdir):
        for name in files:
            if name in WHITE_LIST:
                continue
            if name.endswith(".lua"):
                change_lua_to_bytecode(os.path.join(root, name))

if __name__ == '__main__':
    STAGING_DIR_HOST = sys.argv[1]
    LUAC_CMD = os.path.join(STAGING_DIR_HOST, "bin/luac -s -o %s %s")
    recursive_change(sys.argv[2])
    sys.exit(0)
