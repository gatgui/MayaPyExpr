/*
Copyright (C) 2015  Gaetan Guidet

This file is part of MayaPyExpr.

MayaPyExpr is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

MayaPyExpr is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <maya/MPxNode.h>
#include <maya/MFnPlugin.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MGlobal.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MStringArray.h>
#include <maya/MNodeMessage.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnStringData.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnUInt64ArrayData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MMatrix.h>
#include <maya/MDistance.h>
#include <maya/MTime.h>
#include <maya/MAngle.h>
#include <maya/MPoint.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>
#include <maya/MDGMessage.h>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------------

template <typename FnAttribute>
struct ToStream
{
   static void OutputSingle(MPlug &plug, std::ostringstream &, bool verbose)
   {
      if (verbose)
      {
         MGlobal::displayWarning("[pyexpr] ToStream not implemented for attribute \"" + plug.partialName(false, false, false, false, false, true) + "\"");
      }
   }
};

template <> struct ToStream<MFnMessageAttribute>
{
   static void OutputSingle(MPlug &plug, std::ostringstream &oss, bool)
   {
      MPlugArray srcs;
      
      if (plug.connectedTo(srcs, true, false) && srcs.length() == 1)
      {
         MStatus stat;
         MObject srcNode = srcs[0].node();
         
         MFnDagNode dag(srcNode, &stat);
         
         if (stat != MS::kSuccess)
         {
            MFnDependencyNode dep(srcNode);
            
            oss << "'" << dep.name().asChar() << "'";
         }
         else
         {
            MDagPath path;
            
            dag.getPath(path);
            oss << "'" << path.partialPathName().asChar() << "'";
         }
      }
      else
      {
         oss << "''";
      }
   }
};

template <> struct ToStream<MFnUnitAttribute>
{
   static void OutputSingle(MPlug &plug, std::ostringstream &oss, bool verbose)
   {
      MFnUnitAttribute fnAttr(plug.attribute());
      
      switch (fnAttr.unitType())
      {
      case MFnUnitAttribute::kAngle:
         oss << plug.asMAngle().as(MAngle::uiUnit());
         break;
         
      case MFnUnitAttribute::kDistance:
         oss << plug.asMDistance().as(MDistance::uiUnit());
         break;
         
      case MFnUnitAttribute::kTime:
         oss << plug.asMTime().as(MTime::uiUnit());
         break;
         
      default:
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Unsupported unit type for attribute \"" + fnAttr.name() + "\"");
         }
      }
   }
};

template <> struct ToStream<MFnEnumAttribute>
{
   static void OutputSingle(MPlug &plug, std::ostringstream &oss, bool)
   {
      MFnEnumAttribute fnAttr(plug.attribute());
      
      oss << fnAttr.fieldName(plug.asShort()).asChar();
   }
};

template <> struct ToStream<MFnMatrixAttribute>
{
   static void OutputSingle(MPlug &plug, std::ostringstream &oss, bool)
   {
      MObject oData = plug.asMObject();
      MFnMatrixData fnData(oData);
      
      MMatrix M = fnData.matrix();
      
      oss << "((" << M[0][0] << ", " << M[0][1] << ", " << M[0][2] << ", " << M[0][3] << "),";
      oss << " (" << M[1][0] << ", " << M[1][1] << ", " << M[1][2] << ", " << M[1][3] << "),";
      oss << " (" << M[2][0] << ", " << M[2][1] << ", " << M[2][2] << ", " << M[2][3] << "),";
      oss << " (" << M[3][0] << ", " << M[3][1] << ", " << M[3][2] << ", " << M[3][3] << "))";
   }
};

template <> struct ToStream<MFnNumericAttribute>
{
   static void OutputSingle(MPlug &plug, std::ostringstream &oss, bool verbose)
   {
      MFnNumericAttribute fnAttr(plug.attribute());
      
      MObject oData = plug.asMObject();
      MFnNumericData fnData(oData);
      
      switch (fnAttr.unitType())
      {
      case MFnNumericData::kBoolean:
         oss << (plug.asBool() ? "True" : "False");
         break;
         
      case MFnNumericData::kChar:
         oss << plug.asChar();
         break;
         
      case MFnNumericData::kByte:
      case MFnNumericData::kShort:
         oss << plug.asShort();
         break;
         
      case MFnNumericData::k2Short:
         {
            short v0, v1;
            fnData.getData(v0, v1);
            oss << "(" << v0 << ", " << v1 << ")";
         }
         break;
         
      case MFnNumericData::k3Short:
         {
            short v0, v1, v2;
            fnData.getData(v0, v1, v2);
            oss << "(" << v0 << ", " << v1 << ", " << v2 << ")";
         }
         break;
         
      case MFnNumericData::kLong:
      //case MFnNumericData::kInt:
         oss << plug.asInt();
         break;
         
      case MFnNumericData::k2Long:
      //case MFnNumericData::k2Int:
         {
            int v0, v1;
            fnData.getData(v0, v1);
            oss << "(" << v0 << ", " << v1 << ")";
         }
         break;
      case MFnNumericData::k3Long:
      //case MFnNumericData::k3Int:
         {
            int v0, v1, v2;
            fnData.getData(v0, v1, v2);
            oss << "(" << v0 << ", " << v1 << ", " << v2 << ")";
         }
         break;
         
      case MFnNumericData::kFloat:
         oss << plug.asFloat();
         break;
         
      case MFnNumericData::k2Float:
         {
            float v0, v1;
            fnData.getData(v0, v1);
            oss << "(" << v0 << ", " << v1 << ")";
         }
         break;
         
      case MFnNumericData::k3Float:
         {
            float v0, v1, v2;
            fnData.getData(v0, v1, v2);
            oss << "(" << v0 << ", " << v1 << ", " << v2 << ")";
         }
         break;
         
      case MFnNumericData::kDouble:
         oss << plug.asDouble();
         break;
         
      case MFnNumericData::k2Double:
         {
            double v0, v1;
            fnData.getData(v0, v1);
            oss << "(" << v0 << ", " << v1 << ")";
         }
         break;
         
      case MFnNumericData::k3Double:
         {
            double v0, v1, v2;
            fnData.getData(v0, v1, v2);
            oss << "(" << v0 << ", " << v1 << ", " << v2 << ")";
         }
         break;
         
      case MFnNumericData::k4Double:
         {
            double v0, v1, v2, v3;
            fnData.getData(v0, v1, v2, v3);
            oss << "(" << v0 << ", " << v1 << ", " << v2 << ", " << v3 << ")";
         }
         break;
         
      default:
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Unsupported numeric type for attribute \"" + fnAttr.name() + "\"");
         }
      }
   }
};

template <> struct ToStream<MFnTypedAttribute>
{
   static void OutputSingle(MPlug &plug, std::ostringstream &oss, bool verbose)
   {
      MFnTypedAttribute fnAttr(plug.attribute());
      
      switch (fnAttr.attrType())
      {
      case MFnData::kNumeric:
         {
            ToStream<MFnNumericAttribute>::OutputSingle(plug, oss, verbose);
         }
         break;
         
      case MFnData::kString:
         oss << "'" << plug.asString().asChar() << "'";
         break;
         
      case MFnData::kMatrix:
         {
            MObject oData = plug.asMObject();
            MFnMatrixData fnData(oData);
            
            MMatrix M = fnData.matrix();
            
            oss << "((" << M[0][0] << ", " << M[0][1] << ", " << M[0][2] << ", " << M[0][3] << "),";
            oss << " (" << M[1][0] << ", " << M[1][1] << ", " << M[1][2] << ", " << M[1][3] << "),";
            oss << " (" << M[2][0] << ", " << M[2][1] << ", " << M[2][2] << ", " << M[2][3] << "),";
            oss << " (" << M[3][0] << ", " << M[3][1] << ", " << M[3][2] << ", " << M[3][3] << "))";
         }
         break;
         
      case MFnData::kStringArray:
         {
            MObject oData = plug.asMObject();
            MFnStringArrayData fnData(oData);
            
            unsigned int count = fnData.length();
            
            oss << "[";
            
            for (unsigned int i=0; i<count; ++i)
            {
               oss << "'" << fnData[i].asChar() << "'";
               
               if (i + 1 < count)
               {
                  oss << ", ";
               }
            }
            
            oss << "]";
         }
         break;
         
      case MFnData::kDoubleArray:
         {
            MObject oData = plug.asMObject();
            MFnDoubleArrayData fnData(oData);
            
            unsigned int count = fnData.length();
            
            oss << "[";
            
            for (unsigned int i=0; i<count; ++i)
            {
               oss << fnData[i];
               
               if (i + 1 < count)
               {
                  oss << ", ";
               }
            }
            
            oss << "]";
         }
         break;
         
      case MFnData::kIntArray:
         {
            MObject oData = plug.asMObject();
            MFnIntArrayData fnData(oData);
            
            unsigned int count = fnData.length();
            
            oss << "[";
            
            for (unsigned int i=0; i<count; ++i)
            {
               oss << fnData[i];
               
               if (i + 1 < count)
               {
                  oss << ", ";
               }
            }
            
            oss << "]";
         }
         break;
         
      case MFnData::kPointArray:
         {
            MObject oData = plug.asMObject();
            MFnPointArrayData fnData(oData);
            
            unsigned int count = fnData.length();
            
            oss << "[";
            
            for (unsigned int i=0; i<count; ++i)
            {
               MPoint pnt = fnData[i];
               
               oss << "(" << pnt.x << ", " << pnt.y << ", " << pnt.z << ")";
               
               if (i + 1 < count)
               {
                  oss << ", ";
               }
            }
            
            oss << "]";
         }
         break;
         
      case MFnData::kVectorArray:
         {
            MObject oData = plug.asMObject();
            MFnVectorArrayData fnData(oData);
            
            unsigned int count = fnData.length();
            
            oss << "[";
            
            for (unsigned int i=0; i<count; ++i)
            {
               MVector vec = fnData[i];
               
               oss << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
               
               if (i + 1 < count)
               {
                  oss << ", ";
               }
            }
            
            oss << "]";
         }
         break;
         
      default:
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Unsupported type for attribute \"" + fnAttr.name() + "\"");
         }
      }
   }
};

template <typename FnAttribute>
void Output(MObject &node, MObject &attr, std::ostringstream &oss, bool verbose)
{
   MPlug plug(node, attr);
            
   oss << plug.partialName(false, false, false, false, false, true).asChar() << " = ";
   
   if (plug.isArray())
   {
      unsigned int count = plug.numElements();
      
      oss << "[";
      
      for (unsigned int i=0; i<count; ++i)
      {
         MPlug elem = plug[i];
         
         ToStream<FnAttribute>::OutputSingle(elem, oss, verbose);
         
         if (i + 1 < count)
         {
            oss << ", ";
         }
      }
      
      oss << "]" << std::endl;
   }
   else
   {
      ToStream<FnAttribute>::OutputSingle(plug, oss, verbose);
      oss << std::endl;
   }
}

// -----------------------------------------------------------------------------

class PyExpr : public MPxNode
{
public:
   
   static void *Create();
   static MStatus Initialize();
   
   static MTypeId Id;
   
   static MObject aExpression;
   static MObject aOutputType;
   static MObject aEvalOnTimeChanged;
   static MObject aVerbose;
   
   static MObject aIntOutput;
   static MObject aIntArrayOutput;
   static MObject aDoubleOutput;
   static MObject aDoubleArrayOutput;
   static MObject aStringOutput;
   static MObject aStringArrayOutput;
   static MObject aSucceeded;
   static MObject aErrorString;
   
   enum OutputType
   {
      OT_int = 0,
      OT_int_array,
      OT_double,
      OT_double_array,
      OT_string,
      OT_string_array,
      OT_undefined
   };

public:
   
   PyExpr();
   virtual ~PyExpr();
   
   virtual MStatus setDependentsDirty(const MPlug &plug, MPlugArray &plugArray);
   virtual MStatus compute(const MPlug &plug, MDataBlock &block);
   virtual void postConstructor();
   
   virtual bool getInternalValueInContext(const MPlug &plug, MDataHandle &hdl, MDGContext &ctx);
   virtual bool setInternalValueInContext(const MPlug &plug, const MDataHandle &hdl, MDGContext &ctx);
   virtual void copyInternalData(MPxNode *other);
   
   void evalExpression();
   
private:
   
   bool evalExpression(const MString &expr, short outputType, bool verbose);
   
private:
   
   bool mEval;
   
   bool mSucceeded;
   MString mErrorString;
   int mIntOutput;
   MIntArray mIntArrayOutput;
   double mDoubleOutput;
   MDoubleArray mDoubleArrayOutput;
   MString mStringOutput;
   MStringArray mStringArrayOutput;
   bool mHasTimeChangedCB;
   MCallbackId mTimeChangedCB;
};

// -----------------------------------------------------------------------------

void TimeChanged(MTime &, void *clientData)
{
   PyExpr *node = (PyExpr*) clientData;
   node->evalExpression();
}

// -----------------------------------------------------------------------------

MTypeId PyExpr::Id(PYEXPR_ID);
MObject PyExpr::aExpression;
MObject PyExpr::aOutputType;
MObject PyExpr::aEvalOnTimeChanged;
MObject PyExpr::aVerbose;
MObject PyExpr::aIntOutput;
MObject PyExpr::aIntArrayOutput;
MObject PyExpr::aDoubleOutput;
MObject PyExpr::aDoubleArrayOutput;
MObject PyExpr::aStringOutput;
MObject PyExpr::aStringArrayOutput;
MObject PyExpr::aSucceeded;
MObject PyExpr::aErrorString;

// -----------------------------------------------------------------------------

void* PyExpr::Create()
{
   return new PyExpr();
}

MStatus PyExpr::Initialize()
{
   MStatus stat;
   MFnTypedAttribute tattr;
   MFnNumericAttribute nattr;
   MFnUnitAttribute uattr;
   MFnEnumAttribute eattr;
   
   // --- Inputs ---
   
   aExpression = tattr.create("expression", "expr", MFnData::kString, MObject::kNullObj, &stat);
   addAttribute(aExpression);
   
   aOutputType = eattr.create("outputType", "outt", 4, &stat);
   eattr.addField("int", OT_int);
   eattr.addField("int[]", OT_int_array);
   eattr.addField("double", OT_double);
   eattr.addField("double[]", OT_double_array);
   eattr.addField("string", OT_string);
   eattr.addField("string[]", OT_string_array);
   addAttribute(aOutputType);
   
   aEvalOnTimeChanged = nattr.create("evalOnTimeChanged", "evltc", MFnNumericData::kBoolean, 0.0, &stat);
   nattr.setInternal(true);
   addAttribute(aEvalOnTimeChanged);
   
   aVerbose = nattr.create("verbose", "verb", MFnNumericData::kBoolean, 0.0, &stat);
   addAttribute(aVerbose);
   
   // --- Outputs ---
   
   aIntOutput = nattr.create("outInt", "oint", MFnNumericData::kLong, 0, &stat);
   nattr.setWritable(false);
   nattr.setStorable(false);
   addAttribute(aIntOutput);
   
   aIntArrayOutput = nattr.create("outInts", "oins", MFnNumericData::kLong, 0, &stat);
   nattr.setArray(true);
   nattr.setWritable(false);
   nattr.setStorable(false);
   nattr.setUsesArrayDataBuilder(true);
   addAttribute(aIntArrayOutput);
   
   aDoubleOutput = nattr.create("outDouble", "odbl", MFnNumericData::kDouble, 0, &stat);
   nattr.setWritable(false);
   nattr.setStorable(false);
   addAttribute(aDoubleOutput);
   
   aDoubleArrayOutput = nattr.create("outDoubles", "odbs", MFnNumericData::kDouble, 0, &stat);
   nattr.setArray(true);
   nattr.setWritable(false);
   nattr.setStorable(false);
   nattr.setUsesArrayDataBuilder(true);
   addAttribute(aDoubleArrayOutput);
   
   aStringOutput = tattr.create("outString", "ostr", MFnData::kString, MObject::kNullObj, &stat);
   tattr.setWritable(false);
   tattr.setStorable(false);
   addAttribute(aStringOutput);
   
   aStringArrayOutput = tattr.create("outStrings", "osts", MFnData::kString, MObject::kNullObj, &stat);
   tattr.setArray(true);
   tattr.setWritable(false);
   tattr.setStorable(false);
   nattr.setUsesArrayDataBuilder(true);
   addAttribute(aStringArrayOutput);
   
   aSucceeded = nattr.create("succeeded", "succ", MFnNumericData::kBoolean, 1.0, &stat);
   nattr.setWritable(false);
   nattr.setStorable(false);
   addAttribute(aSucceeded);
   
   aErrorString = tattr.create("errorString", "errs", MFnData::kString, MObject::kNullObj, &stat);
   tattr.setWritable(false);
   tattr.setStorable(false);
   addAttribute(aErrorString);
   
   attributeAffects(aExpression, aIntOutput);
   attributeAffects(aExpression, aIntArrayOutput);
   attributeAffects(aExpression, aDoubleOutput);
   attributeAffects(aExpression, aDoubleArrayOutput);
   attributeAffects(aExpression, aStringOutput);
   attributeAffects(aExpression, aStringArrayOutput);
   attributeAffects(aExpression, aSucceeded);
   attributeAffects(aExpression, aErrorString);
   
   attributeAffects(aOutputType, aIntOutput);
   attributeAffects(aOutputType, aIntArrayOutput);
   attributeAffects(aOutputType, aDoubleOutput);
   attributeAffects(aOutputType, aDoubleArrayOutput);
   attributeAffects(aOutputType, aStringOutput);
   attributeAffects(aOutputType, aStringArrayOutput);
   attributeAffects(aOutputType, aSucceeded);
   attributeAffects(aOutputType, aErrorString);
   
   return MS::kSuccess;
}

// -----------------------------------------------------------------------------

PyExpr::PyExpr()
   : MPxNode()
   , mEval(true)
   , mSucceeded(false)
   , mIntOutput(0)
   , mDoubleOutput(0.0)
   , mHasTimeChangedCB(false)
   , mTimeChangedCB(0)
{
}

PyExpr::~PyExpr()
{
   if (mHasTimeChangedCB)
   {
      MMessage::removeCallback(mTimeChangedCB);
   }
}

void PyExpr::postConstructor()
{
   setMPSafe(false);
}

MStatus PyExpr::setDependentsDirty(const MPlug &plug, MPlugArray &affectedPlugs)
{
   MObject oAttr = plug.attribute();
   
   MFnAttribute fnAttr(oAttr);
   
   if (fnAttr.isDynamic())
   {
      MObject oNode = thisMObject();
      
      MDataBlock block = forceCache();
      
      MDataHandle hOutputType = block.outputValue(aOutputType);
      
      switch (hOutputType.asShort())
      {
      case OT_int_array:
         {
            MPlug pIntArrayOutput(oNode, aIntArrayOutput);
            affectedPlugs.append(pIntArrayOutput);
            
            unsigned int n = pIntArrayOutput.numElements();
            for (unsigned int i=0; i<n; ++i)
            {
               MPlug pIntArrayElem = pIntArrayOutput.elementByPhysicalIndex(i);
               affectedPlugs.append(pIntArrayElem);
            }
         }
         break;
      case OT_double_array:
         {
            MPlug pDoubleArrayOutput(oNode, aDoubleArrayOutput);
            affectedPlugs.append(pDoubleArrayOutput);
            
            unsigned int n = pDoubleArrayOutput.numElements();
            for (unsigned int i=0; i<n; ++i)
            {
               MPlug pDoubleArrayElem = pDoubleArrayOutput.elementByPhysicalIndex(i);
               affectedPlugs.append(pDoubleArrayElem);
            }
         }
         break;
      case OT_string_array:
         {
            MPlug pStringArrayOutput(oNode, aStringArrayOutput);
            affectedPlugs.append(pStringArrayOutput);
            
            unsigned int n = pStringArrayOutput.numElements();
            for (unsigned int i=0; i<n; ++i)
            {
               MPlug pStringArrayElem = pStringArrayOutput.elementByPhysicalIndex(i);
               affectedPlugs.append(pStringArrayElem);
            }
         }
         break;
      case OT_int:
         {
            MPlug pIntOutput(oNode, aIntOutput);
            affectedPlugs.append(pIntOutput);
         }
         break;
      case OT_double:
         {
            MPlug pDoubleOutput(oNode, aDoubleOutput);
            affectedPlugs.append(pDoubleOutput);
         }
         break;
      case OT_string:
      default:
         {
            MPlug pStringOutput(oNode, aStringOutput);
            affectedPlugs.append(pStringOutput);
         }
         break;
      }
      
      MPlug pSucceeded(oNode, aSucceeded);
      affectedPlugs.append(pSucceeded);
      
      mEval = true;
   }
   else if (oAttr == aExpression || oAttr == aOutputType)
   {
      mEval = true;
   }
   
   return MS::kSuccess;
}

bool PyExpr::getInternalValueInContext(const MPlug &plug, MDataHandle &hdl, MDGContext &ctx)
{
   if (plug == aEvalOnTimeChanged)
   {
      hdl.set(mHasTimeChangedCB);
      return true;
   }
   else
   {
      return MPxNode::getInternalValueInContext(plug, hdl, ctx);
   }
}

bool PyExpr::setInternalValueInContext(const MPlug &plug, const MDataHandle &hdl, MDGContext &ctx)
{
   MStatus stat = MS::kSuccess;
   
   if (plug == aEvalOnTimeChanged)
   {
      if (hdl.asBool())
      {
         if (!mHasTimeChangedCB)
         {  
            // or addForceUpdateCallback
            mTimeChangedCB = MDGMessage::addTimeChangeCallback(TimeChanged, (void*)this, &stat);
            
            if (stat == MS::kSuccess)
            {
               mHasTimeChangedCB = true;
            }
            else
            {
               // failed to add
               return false;
            }
         }
         else
         {
            // already set
         }
      }
      else
      {
         if (mHasTimeChangedCB)
         {
            if (MMessage::removeCallback(mTimeChangedCB) == MS::kSuccess)
            {
               mHasTimeChangedCB = false;
            }
            else
            {
               // failed to remove
               return false;
            }
         }
      }
      
      return true;
   }
   else
   {
      return MPxNode::setInternalValueInContext(plug, hdl, ctx);
   }  
}

void PyExpr::copyInternalData(MPxNode *other)
{
   PyExpr *rhs = (PyExpr*) other;
   
   rhs->mEval = mEval;
   rhs->mSucceeded = mSucceeded;
   rhs->mErrorString = mErrorString;
   rhs->mIntOutput = mIntOutput;
   rhs->mIntArrayOutput = mIntArrayOutput;
   rhs->mDoubleOutput = mDoubleOutput;
   rhs->mDoubleArrayOutput = mDoubleArrayOutput;
   rhs->mStringOutput = mStringOutput;
   rhs->mStringArrayOutput = mStringArrayOutput;
}

void PyExpr::evalExpression()
{
   MDataBlock block = forceCache();
   MObject oSelf = thisMObject();
   MFnDependencyNode nSelf(oSelf);
   
   MDataHandle hOutputType = block.inputValue(aOutputType);
   
   short outputType = hOutputType.asShort();
   
   MPlug outPlug;
   
   switch (outputType)
   {
   case OT_int:
      outPlug = nSelf.findPlug("outInt");
      break;
   case OT_int_array:
      outPlug = nSelf.findPlug("outInts");
      break;
   case OT_double:
      outPlug = nSelf.findPlug("outDouble");
      break;
   case OT_double_array:
      outPlug = nSelf.findPlug("outDoubles");
      break;
   case OT_string:
      outPlug = nSelf.findPlug("outString");
      break;
   case OT_string_array:
      outPlug = nSelf.findPlug("outStrings");
   default:
      break;
   }
   
   if (!outPlug.isNull())
   {
      compute(outPlug, block);
   }
}

bool PyExpr::evalExpression(const MString &expr, short outputType, bool verbose)
{
   if (mEval)
   {
      MStatus stat;
      
      MObject oSelf = thisMObject();
      MFnDependencyNode nSelf(oSelf);
      
      mIntOutput = 0;
      mDoubleOutput = 0.0;
      mStringOutput = "";
      mIntArrayOutput.setLength(0);
      mDoubleArrayOutput.setLength(0);
      mStringArrayOutput.setLength(0);
      
      std::ostringstream oss;
      
      unsigned int count = nSelf.attributeCount();
      
      for (unsigned int i=0; i<count; ++i)
      {
         MObject oAttr = nSelf.attribute(i);
         MFnAttribute fnAttr(oAttr);
         
         if (!fnAttr.isDynamic())
         {
            continue;
         }
         
         if (oAttr.hasFn(MFn::kMessageAttribute))
         {
            Output<MFnMessageAttribute>(oSelf, oAttr, oss, verbose);
         }
         else if (oAttr.hasFn(MFn::kUnitAttribute))
         {
            Output<MFnUnitAttribute>(oSelf, oAttr, oss, verbose);
         }
         else if (oAttr.hasFn(MFn::kEnumAttribute))
         {
            Output<MFnEnumAttribute>(oSelf, oAttr, oss, verbose);
         }
         else if (oAttr.hasFn(MFn::kMatrixAttribute))
         {
            Output<MFnMatrixAttribute>(oSelf, oAttr, oss, verbose);
         }
         else if (oAttr.hasFn(MFn::kNumericAttribute))
         {
            Output<MFnNumericAttribute>(oSelf, oAttr, oss, verbose);
         }
         else if (oAttr.hasFn(MFn::kTypedAttribute))
         {
            Output<MFnTypedAttribute>(oSelf, oAttr, oss, verbose);
         }
         else
         {
            if (verbose)
            {
               MGlobal::displayWarning("[pyexpr] Unsupported type for attribute \"" + fnAttr.name()  + "\"");
            }
         }
      }
      
      // Build function declaration
      
      // The error string returned by MStatus after an executePythonCommand doesn't contain
      //   any usefull information
      // Catch any exception and set a global error string variable
      // In verbose mode, re-raise the caught exception and let maya handle the output
      // When not in verbose mode, return a valid default value and set success status
      //   depending on the global error string content. Empty means no error happened
      
      MString func = "_pyexpr_eval_" + nSelf.name() + "()";
      MString errv = "_pyexpr_err_" + nSelf.name();
      
      MString vars = oss.str().c_str();
      
      MString decl = errv + " = ''\n";
      
      decl += "def " + func + ":\n"
              "  global " + errv + "\n"
              "  " + errv + " = ''\n\n"
              "  try:\n";
      
      MString remain = vars + expr;
      
      int i = remain.indexW('\n');
      
      while (i != -1)
      {
         decl += "    " + remain.substringW(0, i);
         remain = remain.substringW(i + 1, remain.numChars() - 1);
         i = remain.indexW('\n');
      }
      
      if (remain.length() > 0)
      {
         decl += "    " + remain;
      }
      
      decl += "\n  except Exception, e:\n"
              "    " + errv + " = '%s: %s' % (e.__class__.__name__, str(e))\n";
      
      if (verbose)
      {
         decl += "    raise e\n";
      }
      else
      {
         switch (outputType)
         {
         case OT_int:
         case OT_double:
            decl += "    return 0\n";
            break;
         case OT_int_array:
         case OT_double_array:
         case OT_string_array:
            decl += "    return []\n";
            break;
         case OT_string:
         default:
            decl += "    return ''\n";
         }
      }
      
      if (verbose)
      {
         MGlobal::displayInfo("[pyexpr] Declare function:\n" + decl);
      }
      
      if (MGlobal::executePythonCommand(decl) == MS::kSuccess)
      {
         // Only one-liner have return values
         
         switch (outputType)
         {
         case OT_int:
            if (verbose)
            {
               MGlobal::displayInfo("[pyexpr] Evaluating int expression");
            }
            stat = MGlobal::executePythonCommand(func, mIntOutput);
            break;
         case OT_int_array:
            if (verbose)
            {
               MGlobal::displayInfo("[pyexpr] Evaluating int[] expression");
            }
            stat = MGlobal::executePythonCommand(func, mIntArrayOutput);
            break;
         case OT_double:
            if (verbose)
            {
               MGlobal::displayInfo("[pyexpr] Evaluating double expression");
            }
            stat = MGlobal::executePythonCommand(func, mDoubleOutput);
            break;
         case OT_double_array:
            if (verbose)
            {
               MGlobal::displayInfo("[pyexpr] Evaluating double[] expression");
            }
            stat = MGlobal::executePythonCommand(func, mDoubleArrayOutput);
            break;
         case OT_string_array:
            if (verbose)
            {
               MGlobal::displayInfo("[pyexpr] Evaluating string[] expression");
            }
            stat = MGlobal::executePythonCommand(func, mStringArrayOutput);
            break;
         case OT_string:
         default:
            if (verbose)
            {
               if (outputType != OT_string)
               {
                  MGlobal::displayWarning("[pyexpr] Default output type to 'string'");
               }
               
               MGlobal::displayInfo("[pyexpr] Evaluating string expression");
            }
            stat = MGlobal::executePythonCommand(func, mStringOutput);
         }
      }
      
      mSucceeded = (stat == MS::kSuccess);
      
      if (!verbose || !mSucceeded)
      {
         mErrorString = MGlobal::executePythonCommandStringResult(errv);
         
         // Note: when not in verbose mode, error string will only be set if an exception was caught
         if (mErrorString.length() > 0)
         {
            mSucceeded = false;
         }
      }
      else
      {
         // Avoid unnecessary script execution
         mErrorString = "";
      }
      
      mEval = false;
   }
   
   return mSucceeded;
}

MStatus PyExpr::compute(const MPlug &plug, MDataBlock &block)
{
   MDataHandle hExpression = block.inputValue(aExpression);
   MDataHandle hOutputType = block.inputValue(aOutputType);
   MDataHandle hVerbose = block.inputValue(aVerbose);
   
   MString expr = hExpression.asString();
   bool verbose = hVerbose.asBool();
   short outputType = hOutputType.asShort();
   
   bool success = evalExpression(expr, outputType, verbose);
   
   if (plug.attribute() == aIntOutput)
   {
      if (outputType != OT_int)
      {
         success = false;
      }
      
      MDataHandle hIntOutput = block.outputValue(aIntOutput);
      hIntOutput.set(success ? mIntOutput : 0);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aDoubleOutput)
   {
      if (outputType != OT_double)
      {
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Querying wrong output type");
         }
         success = false;
      }
      
      MDataHandle hDoubleOutput = block.outputValue(aDoubleOutput);
      hDoubleOutput.set(success ? mDoubleOutput : 0.0);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aStringOutput)
   {
      if (outputType != OT_string)
      {
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Querying wrong output type");
         }
         success = false;
      }
      
      MDataHandle hStringOutput = block.outputValue(aStringOutput);
      hStringOutput.set(success ? mStringOutput : "");
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aIntArrayOutput)
   {
      if (outputType != OT_int_array)
      {
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Querying wrong output type");
         }
         success = false;
      }
      
      MArrayDataHandle hIntArrayOutput = block.outputArrayValue(aIntArrayOutput);
      
      MArrayDataBuilder builder(&block, aIntArrayOutput, (success ? mIntArrayOutput.length() : 0));
         
      if (success)
      {
         for (unsigned int i=0; i<mIntArrayOutput.length(); ++i)
         {
            MDataHandle hIntOutput = builder.addElement(i);
            hIntOutput.set(mIntArrayOutput[i]);
         }
      }
      
      hIntArrayOutput.set(builder);
      hIntArrayOutput.setAllClean();
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aDoubleArrayOutput)
   {
      if (outputType != OT_double_array)
      {
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Querying wrong output type");
         }
         success = false;
      }
      
      MArrayDataHandle hDoubleArrayOutput = block.outputArrayValue(aDoubleArrayOutput);
      
      MArrayDataBuilder builder(&block, aDoubleArrayOutput, (success ? mDoubleArrayOutput.length() : 0));
         
      if (success)
      {
         for (unsigned int i=0; i<mDoubleArrayOutput.length(); ++i)
         {
            MDataHandle hDoubleOutput = builder.addElement(i);
            hDoubleOutput.set(mDoubleArrayOutput[i]);
         }
      }
      
      hDoubleArrayOutput.set(builder);
      hDoubleArrayOutput.setAllClean();
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aStringArrayOutput)
   {
      if (outputType != OT_string_array)
      {
         if (verbose)
         {
            MGlobal::displayWarning("[pyexpr] Querying wrong output type");
         }
         success = false;
      }
      
      MArrayDataHandle hStringArrayOutput = block.outputArrayValue(aStringArrayOutput);
      
      MArrayDataBuilder builder(&block, aStringArrayOutput, (success ? mStringArrayOutput.length() : 0));
         
      if (success)
      {
         for (unsigned int i=0; i<mStringArrayOutput.length(); ++i)
         {
            MDataHandle hStringOutput = builder.addElement(i);
            hStringOutput.set(mStringArrayOutput[i]);
         }
      }
      
      hStringArrayOutput.set(builder);
      hStringArrayOutput.setAllClean();
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aSucceeded)
   {
      MDataHandle hSucceeded = block.outputValue(aSucceeded);
      
      hSucceeded.set(success);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else if (plug.attribute() == aErrorString)
   {
      MDataHandle hErrorString = block.outputValue(aErrorString);
      
      hErrorString.set(mErrorString);
      
      block.setClean(plug);
      
      return MS::kSuccess;
   }
   else
   {
      return MS::kUnknownParameter;
   }
}

// -----------------------------------------------------------------------------

PLUGIN_EXPORT MStatus initializePlugin(MObject oPlugin)
{
   MFnPlugin fnPlugin(oPlugin, "Gaetan Guidet", PYEXPR_VERSION, "2013");
   
   return fnPlugin.registerNode("pyexpr", PyExpr::Id, PyExpr::Create, PyExpr::Initialize);
}

PLUGIN_EXPORT MStatus uninitializePlugin(MObject oPlugin)
{
   MFnPlugin fnPlugin(oPlugin);
   
   return fnPlugin.deregisterNode(PyExpr::Id);
}
