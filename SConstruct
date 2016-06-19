import os
import re
import sys
import glob
import platform
import excons
from excons.tools import maya

Version = "0.1.1"

DefaultId = "0x64374" # G = 6, A = 4, E = 3, T = 7
Id = excons.GetArgument("node-id", DefaultId)
if Id == DefaultId:
  print("!!! Using default node id 0x64374 from site internal range 0 - 0x7ffff. Override using node-id= !!!")          

# Maya plugin
mels = glob.glob("src/*.mel")

targets = [
  {"name"    : "maya%s/plug-ins/pyexpr" % maya.Version(),
   "alias"   : "pyexpr",
   "defs"    : ["PYEXPR_VERSION=\\\"%s\\\"" % Version,
                "PYEXPR_ID=%s" % Id],
   "type"    : "dynamicmodule",
   "ext"     : maya.PluginExt(),
   "srcs"    : glob.glob("src/*.cpp"),
   "install" : {"maya%s/scripts" % maya.Version(): mels},
   "custom"  : [maya.Require, maya.Plugin]}
]

env = excons.MakeBaseEnv()
targets = excons.DeclareTargets(env, targets)

# Add ecosystem distribution target
dist_dir = excons.GetArgument("dist-dir", "dist")
dist_ver_dir = dist_dir + "/pyexpr/" + Version
dist_plat = platform.system().lower()
dist_env = env.Clone()
Alias("dist", dist_env.Install(dist_dir, "pyexpr_%s.env" % Version.replace(".", "_")))
Alias("dist", dist_env.Install(dist_ver_dir + "/plug-ins/" + dist_plat, targets["pyexpr"][0]))
Alias("dist", dist_env.Install(dist_ver_dir + "/scripts", mels))
dist_env.Clean("dist", dist_ver_dir)

excons.ConservativeClean(env, "dist", targets)

Default(["pyexpr"])
