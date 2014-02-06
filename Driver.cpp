////////////////////////////////////////////////////////////////////
// 
//  Authors: Colt Johnson
//
//  All content © 2012 DigiPen (USA) Corporation, all rights reserved.
///////////////////////////////////////////////////////////////////
#include "Precompiled.h"
#include "Functions.h"
#include "Scene.h"


int main(int argc, char** argv)
{
  KFbxSdkManager* pSdkManager = NULL;
  KFbxScene* pFbxScene = NULL;

  
	//Check for command line arguments
	if(argc <= 1)
	{
    printf("Please type FBXConverter filename\n");
    return 0;
  }

  std::string filename(argv[1]);
  
  printf("Loading %s\n", filename.c_str());

  Scene scene(filename.c_str());

  bool sceneLoaded = scene.LoadScene();

  if (sceneLoaded)
  {
    bool extracted = scene.ExtractScene();

    if(extracted)
  	{
      scene.SaveScene();
		}
    else 
    {
      printf("Failed to extract scene!\n");
    }
  }
  else 
  {
    printf("Failed to load scene!\n");
  }

  DestroySdkManager(pSdkManager);

  return 0;
}