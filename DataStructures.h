////////////////////////////////////////////////////////
//* Filename: DataStructures.h                        //
//  Author: Colt Johnson                              //
//  Info:                                             //
// Copyright 2012, Digipen Institute of Technology    //
//                                                    //
//////////////////////////////////////////////////////*/

#pragma once
#include <fbxsdk.h>
#include <string>
#include <vector>

  struct FbxBone
  {
    KFbxSkeleton *bone_;

    KFbxVector4 localPos_;
    KFbxQuaternion localRot_;
    KFbxMatrix invTransform_;

    KFbxVector4 shearing_;
    KFbxVector4 scaling_;
    double sign_;

    unsigned int index_;
    int pIndex_;

    std::string name_;
  };

  struct FbxKeyFrame
  {
    FbxKeyFrame(const KFbxXMatrix &localTrans) : 
      localPos_(localTrans.GetT()), localRot_(localTrans.GetQ()){}

    float time_;
    KFbxVector4 localPos_;
    KFbxQuaternion localRot_;
  };

  struct FbxAnimation
  {
    std::string name_;
    float length_;
    std::vector<std::vector<FbxKeyFrame>> frames_;
  };

  enum ModelType
  {
    Static,
    Skinned
  };

  struct FbxVert
  {
    FbxVert(void) : boneIndex_(0){}

    KFbxVector4 pos_;
    KFbxVector4 nrm_;
    KFbxVector2 uv_;
    KFbxVector4 tan_;
    KFbxVector4 bitan_;
    int boneIndices_ [4];
    float boneWeights_ [4];
    unsigned short boneIndex_;
  };

  struct IndexedVert
  {
    int posIndex;
    int normIndex;
    int uvIndex;
    int tanIndex;
    int bitanIndex;
  };

  struct Tri
  {
    union
    {
      struct  
      {
        unsigned p0_, p1_, p2_;
      };

      unsigned v_[3];
    };
  };

  struct FbxParent
  {
    FbxParent(int parent, KFbxNode *node) 
      : parent_(parent), node_(node){}

    int parent_;
    KFbxNode *node_;
  };

  //Skinning Data
  struct JointWeight
  {
    float weight;
    unsigned int index;
  };
  typedef std::vector< JointWeight > WeightVector;

  struct SkinData
  {
    //Index in PointWeight vector correspond to mesh control points which
    //are mapped by the Position Indices
    std::vector< WeightVector > PointWeights;
  };

  struct FbxMesh
  {
    FbxMesh(KFbxMesh *mesh) : mesh_(mesh) {}

    KFbxMesh *mesh_;
    std::vector<FbxVert> verts_;
    std::vector<int> indices_;
    KFbxMatrix nrmMatrix_;
    KFbxMatrix transformMtx_;
    std::string name_;

    //For triangulation on each mesh (input)
    std::vector<int> posInd;
    std::vector<int> uvInd;
    std::vector<int> nInd;
    std::vector<int> tInd;
    std::vector<int> bInd;


    std::vector<KFbxVector4> positions;
    std::vector<KFbxVector4> norms;
    std::vector<KFbxVector4> tans;
    std::vector<KFbxVector4> bitans;
    std::vector<KFbxVector2> uvs;	
    std::vector<int> polySizeArray;

    std::vector<std::vector<int>> mappedVerts;
    std::vector<IndexedVert> source;

    //Resulting Data
    std::vector<FbxVert> ProcessedVertices;
    std::vector<int> ProcessedIndices;

    std::vector<Tri> triangles;
    unsigned numTris;

    //Type information for getting the tangents/bitangents and such
    ModelType meshType;

    //Skinning stuff
    SkinData skin;

    void CombineInto(FbxMesh& mesh);
  };