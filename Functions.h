////////////////////////////////////////////////////////
//* Filename: functions.h                             //
//  Author: Colt Johnson                              //
//  Info:                                             //
// Copyright 2012, Digipen Institute of Technology    //
//                                                    //
//////////////////////////////////////////////////////*/

#pragma once



void InitializeSdkManager(KFbxSdkManager* pSdkManager, KFbxScene* pScene);

void DestroySdkManager(KFbxSdkManager* pSdkManager);

template<typename type>
void ConvertToStl(std::vector<type>& container, KFbxLayerElementArrayTemplate<type>* FbxContainter)
{
  int numberofObjects = FbxContainter->GetCount();
  void * pData = FbxContainter->GetLocked();
  container.resize( numberofObjects );
  memcpy(&container[0] , pData , sizeof(type)*numberofObjects);
  FbxContainter->Release( &pData );
}
template<typename type>
void FillStl(std::vector<type>& container, std::size_t size)
{
  container.resize(size);
  for(std::size_t i=0;i<size;++i)
    container[i] = i;
}

