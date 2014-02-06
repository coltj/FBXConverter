////////////////////////////////////////////////////////////////////
// 
//  Authors: Colt Johnson
//
//  All content © 2012 DigiPen (USA) Corporation, all rights reserved.
///////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "Functions.h"

void InitializeSdkManager(KFbxSdkManager* pSdkManager, KFbxScene* pScene)
{
  // Create the FBX SDK memory manager object.
  // The SDK Manager allocates and frees memory
  // for almost all the classes in the SDK.
  pSdkManager = KFbxSdkManager::Create();

  	// create an IOSettings object
	KFbxIOSettings * ios = KFbxIOSettings::Create(pSdkManager, IOSROOT );
	pSdkManager->SetIOSettings(ios);

  // Create the empty scene.
	pScene = KFbxScene::Create(pSdkManager,"");   
}

void DestroySdkManager(KFbxSdkManager* pSdkManager)
{
  if (pSdkManager)
    pSdkManager->Destroy();

  pSdkManager = NULL;
}