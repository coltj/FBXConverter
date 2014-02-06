///////////////////////////////////////////////////////////////////////////////
//
//	Converter.h
//	Converts homgeneous transform matrices between coordinate systems.
//
//	Authors: Colt Johnson
//	Copyright 2012, Digipen Institute of Technology
//
///////////////////////////////////////////////////////////////////////////////
#pragma once


  class Converter
  {
  public:	
    Converter(KFbxScene* pScene);
    void ConvertMatrix( KFbxXMatrix& sourceMatX);
    void ConvertMeshMatrix( KFbxXMatrix& m);
    KFbxMatrix ConversionMatrix;
    bool NeedToChangedWinding;
  };

