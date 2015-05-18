# MayaPyExpr
Simple python expression node for Maya

# Build
```
git submodule update --init
scons with-maya=maya_version|maya_root_dir
```

# Usage

Write the content of your expression as if it were the body of a function.

All user defined attributes on the __pyexpr__ node are accessible directly using their name.
