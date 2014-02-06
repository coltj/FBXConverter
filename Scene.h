////////////////////////////////////////////////////////
//* Filename: Scene.h                                 //
//  Author: Colt Johnson                              //
//  Info:                                             //
// Copyright 2012, Digipen Institute of Technology    //
//                                                    //
//////////////////////////////////////////////////////*/

#pragma once
#include <fbxsdk.h>
#include "DataStructures.h"

class Converter;

class Scene
{
  public:
    Scene(const char* filename);
    bool LoadScene(void);
    bool ExtractScene(void);
    void SaveScene(void);
    void PrintScene(KFbxNode* pRootNode);
    void CollectMeshes(KFbxNode* pRootNode);
    void CollectBones(KFbxNode* pRootNode);
    void CollectAnimations(void);
    void ProcessMeshes(void);
    void GetPositions(FbxMesh& inmesh, KFbxXMatrix& trans);
    void GetNormalsUvs(FbxMesh& inmesh, KFbxXMatrix& transform);
    KFbxVector4 Transform(const KFbxXMatrix& pXMatrix, const KFbxVector4& point);
    void ExtractMesh(KFbxNode *pNode);
    void GenerateTriangleIndices(FbxMesh& mesh);
    void GrabSkinWeights(FbxMesh& mesh);
    int FindBone(KFbxNode *node);
    void GenerateVertices(FbxMesh& mesh);

    void PrintMesh(KFbxNode* pRootNode);
    void PrintNode(KFbxNode* pNode);
    void PrintAttribute(KFbxNodeAttribute* pAttribute);
    void PrintTabs(void);
    void CombineMeshes(void);
    KString GetAttributeTypeName(KFbxNodeAttribute::EAttributeType type);
    KFbxNodeAttribute::EAttributeType GetNodeAttributeType(KFbxNode* pNode);
    void CollectKeyTimes(std::set<KTime> &keyTimes, KFbxTypedProperty<fbxDouble3> &attribute, const char *curveName, const char *takeName, KFbxAnimLayer* layer);
    int FindMatchingVertex(IndexedVert& v, FbxMesh& mesh);
    bool IsMatchingVertex(FbxVert& vertex, IndexedVert& cur, IndexedVert& v);
    int AddNewVertex(IndexedVert v, FbxMesh& mesh);
    void ProcessBones(void);
    void GetKeyFrames(FbxBone &bone, const char *takeName, std::set<KTime> &keyTimes, int animlayer);
    void Triangulate(FbxMesh& mesh);
    void ConvertTriWinding(FbxMesh& mesh);
    void CalculateTansAndBitans(FbxMesh& mesh);
    void CalcTriTanBinNorm(const KFbxVector4& P1, const KFbxVector4& P2, const KFbxVector4& P3, 
                           const KFbxVector2& UV1, const KFbxVector2& UV2, const KFbxVector2& UV3,
                           KFbxVector4 &tangent, KFbxVector4 &binormal);
    void AvgTanBins(std::vector<Tri>& triangles, unsigned int numTris, std::vector<FbxVert>& verts, unsigned int numVerts);
    void TransformTansBitans(FbxMesh& mesh, KFbxXMatrix& transform);
    KFbxSdkManager* sdkManager_;
    KFbxScene* scene_;
    KFbxIOSettings* ios_;
    std::string filename_;
    std::string output_;
    FILE *fp_;
    KFbxPose* pose_;
    KArrayTemplate<KString*> takes_;
    KFbxGeometryConverter* converter_;
    int numTabs_;
    int extension_;
    int takeCount_;

    unsigned maxWeights_;
    bool tans_;
    bool bitans_;

    std::vector<FbxMesh> meshes_;
    std::vector<FbxBone> bones_;
    std::vector<FbxAnimation> anims_;
    std::vector<FbxVert> verts_;
    std::vector<int> indices_;
    std::vector<SkinData> skins_;

    bool extractAnimations_;
	  bool extractMesh_;
	  bool extractSkinData_;
	  bool extractSkeletonData_;

    ModelType type_;

    //For converting from FBX space to DX Space thanks to Chris Peters
    Converter* mtxConverter_;
};