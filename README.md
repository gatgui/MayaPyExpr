# MayaPyExpr
Simple python expression node for Maya

# Build
```
git submodule update --init
scons with-maya=maya_version|maya_root_dir
```

# Usage

Write the content of your expression as if it were the body of a function and set it in the _expression_ attribute.

All user defined attributes on the __pyexpr__ node are accessible directly using their name.

The expected output type can be set using the _outputType_ attribute. (0: int, 1: int[], 2: double, 3: double[], 4: string, 5: string[])

Result should be queried according to the _outputType_ using the _outInt_, _outInts_, _outDouble_, _outDoubles_, _outString_ and _outStrings_ attributes respectively.

The expression evaluation success or failure is reported by the _succeeded_ attribute and the _errorString_ attribute will contain the error message when the evaluation failed.

# Example

```c
loadPlugin pyexpr;

string $n = `createNode pyexpr`;

// Add user attributes...
addAttr -ln "frame" -at "float" $n;
connectAttr time1.outTime ($n+".frame");

addAttr -ln "multiplier" -at "float" $n;
setAttr ($n+".multiplier") 2;

// Set an expression using both of the user defined attributes
setAttr -type "string" ($n+".expression") "return frame * multiplier";
// Change output type to double
setAttr ($n+".outputType") 2;

// Change current frame
currentTime 10;

// Query expression result
print(`getAttr ($n+".outDouble")` + "\n");

// Set an invalid expression on purpose and output error message
setAttr -type "string" ($n+".expression") "return frame * mult";
float $rv = `getAttr ($n+".outDouble")`;
if (!`getAttr ($n+".succeeded")`)
{
  print(`getAttr ($n+".errorString")` + "\n");
}
```
