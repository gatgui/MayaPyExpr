import os
import re
import sys
import glob
import excons
from excons.tools import maya

Version = "0.1.0"

DefaultId = "0x64374" # G = 6, A = 4, E = 3, T = 7
Id = excons.GetArgument("node-id", DefaultId)
if Id == DefaultId:
  print("!!! Using default node id 0x64374 from site internal range 0 - 0x7ffff. Override using node-id= !!!")          

# Maya plugin
targets = [
  {"name"    : "maya%s/plug-ins/pyexpr" % maya.Version(),
   "alias"   : "pyexpr",
   "defs"    : ["PYEXPR_VERSION=\\\"%s\\\"" % Version,
                "PYEXPR_ID=%s" % Id],
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "srcs"    : glob.glob("src/*.cpp"),
   "install" : {"scripts": glob.glob("src/*.mel")},
   "custom"  : [maya.Require, maya.Plugin]}
]

env = excons.MakeBaseEnv()
excons.DeclareTargets(env, targets)

Default(["pyexpr"])
