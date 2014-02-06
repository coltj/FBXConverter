////////////////////////////////////////////////////////////////////
// 
//  Authors: Colt Johnson
//
//  All content © 2012 DigiPen (USA) Corporation, all rights reserved.
///////////////////////////////////////////////////////////////////
#include "DataStructures.h"

void FbxMesh::CombineInto(FbxMesh& otherMesh)
{
  //The base size of the original mesh
  //The base size of the original mesh
  unsigned int baseSize = verts_.size();
  printf("%d\n", baseSize);
  unsigned int otherSize = otherMesh.verts_.size();
  printf("%d\n", otherSize);

  //Append the other meshes vertices
  verts_.resize(baseSize + otherSize);
  for(unsigned int i = 0; i < otherSize; ++i)
    verts_[baseSize + i] = otherMesh.verts_[i];

  //Copy over the indices increasing them by the size of the original vertex buffer
  int baseIndicesSize = indices_.size();
  int otherIndicesSize = otherMesh.indices_.size();
  indices_.resize(baseIndicesSize + otherIndicesSize);

  for(unsigned int i = 0; i < otherIndicesSize; ++i)
    indices_[baseIndicesSize + i] = baseSize + otherMesh.indices_[i];

  //Append the other meshes skin weights
  int oPointSize = skin.PointWeights.size();
  skin.PointWeights.resize(skin.PointWeights.size() + otherMesh.skin.PointWeights.size());
    
  for(unsigned int i = 0; i < otherMesh.skin.PointWeights.size(); ++i)
    skin.PointWeights[oPointSize + i] = otherMesh.skin.PointWeights[i];

  //Copy over the indices incrementing the position index so the the weights
  //are still correct (Note the other values in th source buffer are not adjusted)
  source.resize(baseSize + otherSize);

  for(unsigned int i = 0; i < otherMesh.source.size(); ++i)
  {
    source[baseSize + i] = otherMesh.source[i];
    source[baseSize + i].posIndex += oPointSize;
  }
}