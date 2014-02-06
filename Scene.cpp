////////////////////////////////////////////////////////
//* Filename: Scene.cpp                               //
//  Author: Colt Johnson                              //
//  Info:                                             //
// Copyright 2012, Digipen Institute of Technology    //
//                                                    //
//////////////////////////////////////////////////////*/

#include "Precompiled.h"
#include "Scene.h"
#include "Functions.h"
#include "Converter.h"


 Scene::Scene(const char* filename) : filename_(filename)
 {
    maxWeights_ = 4;
    tans_ = true;
    bitans_ = true;

    output_ = filename;
    extension_ = output_.find_last_of(".");
    output_.erase(extension_);
    output_.append(".gmf");

    //Initialize the SDK
	  sdkManager_ = KFbxSdkManager::Create();

  	// create an IOSettings object
	  ios_ = KFbxIOSettings::Create(sdkManager_, IOSROOT );
	  sdkManager_->SetIOSettings(ios_);

  	// Create the empty scene.
	  scene_ = KFbxScene::Create(sdkManager_,"");
    KFbxGeometryConverter con(sdkManager_);
    converter_ = &con;
 }

 void Scene::PrintTabs(void)
 {
   for(int i = 0; i < numTabs_; ++i)
    printf("\t");
 }
bool Scene::LoadScene(void)
{
    int lFileMajor, lFileMinor, lFileRevision;
    int lSDKMajor,  lSDKMinor,  lSDKRevision;
    bool lStatus;

    // Get the version number of the FBX files generated by the
    // version of FBX SDK that you are using.
    KFbxSdkManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

    // Create an importer.
    KFbxImporter* lImporter = KFbxImporter::Create(sdkManager_,"");

    // Initialize the importer by providing a filename.
    const bool lImportStatus = lImporter->Initialize(filename_.c_str(), -1, sdkManager_->GetIOSettings());

    // number of takes (i.e., animation stacks) in the file:
    takeCount_ = lImporter->GetAnimStackCount();
    printf("Number of takes %d\n", takeCount_);

    // Get the version number of the FBX file format.
    lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

    if(!lImportStatus)  // Problem with the file to be imported
    {
      printf("Call to KFbxImporter::Initialize() failed.\n");
      printf("Error returned: %s", lImporter->GetLastErrorString());
      return false;
    }

    printf("FBX version number for this FBX SDK is %d.%d.%d\n", lSDKMajor, lSDKMinor, lSDKRevision);

    if (lImporter->IsFBX())
    {
      printf("FBX version number for file %s is %d.%d.%d\n", filename_.c_str(), lFileMajor, lFileMinor, lFileRevision);
        
      // Import the scene.
      lStatus = lImporter->Import(scene_);

      if(lStatus == false)
      {
        printf("Error in Importing Scene!!! %d", lImporter->GetLastErrorID());
        return false;
      }

      printf("Importing scene is complete...\n");
    }

    // Destroy the importer
    lImporter->Destroy();

    return lStatus;

}
bool Scene::ExtractScene(void)
{
  KFbxNode* pRootNode = scene_->GetRootNode();

  //Grab the animation names and the bind pose
  pose_ = scene_->GetPose(0);
  scene_->FillAnimStackNameArray(takes_);

  // determine if the scene has static meshes or skinned

  if (scene_->GetPoseCount() > 0)
    type_ = Skinned;

  else
    type_ = Static;



  //Create a converter to change coordinate spaces for DirectX
  mtxConverter_ = new Converter(scene_);
  PrintScene(scene_->GetRootNode());
  CollectMeshes(scene_->GetRootNode());
  CollectBones(scene_->GetRootNode());
  CollectAnimations();
  ProcessMeshes();
  ProcessBones();

  CombineMeshes();

 
  return true;
}

void WriteDoubleToFloat(FILE *fp, const double *dubs, int size)
{
  for(int i = 0; i < size; ++i)
  {
    float fTd = static_cast<float>(dubs[i]);
    fwrite(&fTd, sizeof(float), 1, fp);
  }
}
void Scene::SaveScene(void)
{
  printf("Writing %s\n", output_.c_str());
  //All of the meshes are now stored in meshes_[0]
  FbxMesh& mesh = meshes_[0];
  SkinData& skin = meshes_[0].skin;

  fp_ = fopen(output_.c_str(), "wb");

  fwrite(&type_, sizeof(unsigned int), 1, fp_);
    
  unsigned int vertCount = mesh.verts_.size();
  unsigned int indexCount = mesh.indices_.size();

  fwrite(&vertCount, sizeof(unsigned int), 1, fp_);
  fwrite(&indexCount, sizeof(unsigned int), 1, fp_);

  for(unsigned int i = 0; i < vertCount; ++i)
  {
    WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&mesh.verts_[i].pos_), 3);   //write position
    WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&mesh.verts_[i].nrm_), 3);   //write normal
    WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&mesh.verts_[i].uv_), 2);    //write uv

    WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&mesh.verts_[i].tan_), 3);   //write tangent
    WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&mesh.verts_[i].bitan_), 3); //write bitangent

    if(type_ == Skinned)
    {
      int originalPosInd = mesh.source[i].posIndex;
      WeightVector& weights = skin.PointWeights[originalPosInd];

      //Write bone indices out as unsigned values.
      for(unsigned int j = 0; j < 4; ++j)
      {
        unsigned char index = static_cast<unsigned char>(weights[j].index);
        fwrite(&index, sizeof(unsigned char), 1, fp_);
      }
      //write bone weights
      for(unsigned int j = 0; j < 4; ++j)
        fwrite(&weights[j].weight, sizeof(float), 1, fp_);  
    }
  }
  //write out the indices
  fwrite(&mesh.indices_[0], sizeof(int) * indexCount, 1, fp_);

  if(type_ == Skinned)
  {
    unsigned int boneCount = bones_.size();
    fwrite(&boneCount, sizeof(unsigned int), 1, fp_);

    //Write out all the bone info.
    for(unsigned int i = 0; i < boneCount; ++i)
    {
      WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&bones_[i].localPos_), 3);      //write bone position
      WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&bones_[i].localRot_), 4);      //write bone quaternion
      WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&bones_[i].invTransform_), 16); //write inverse bone transform
      fwrite(&bones_[i].index_, sizeof(unsigned int), 1, fp_);              //write index
      fwrite(&bones_[i].pIndex_, sizeof(int), 1, fp_);             //write parent index

      //Write out the bone name and its name's length
      unsigned int strsize = bones_[i].name_.size();
      fwrite(&strsize, sizeof(unsigned int), 1, fp_);
      fwrite(bones_[i].name_.c_str(), strsize, 1, fp_);
    }
    
    unsigned int animCount = anims_.size();
    fwrite(&animCount, sizeof(unsigned int), 1, fp_); //write animation count
    for(unsigned int i = 0; i < animCount; ++i)      //write animation info
    {
      //Write out the animation name and its name's length
      unsigned int strsize = anims_[i].name_.size();
      fwrite(&strsize, sizeof(unsigned int), 1, fp_);
      fwrite(anims_[i].name_.c_str(), strsize, 1, fp_);

      fwrite(&anims_[i].length_, sizeof(float), 1, fp_);          //write animation length
      unsigned int frameVecCount = anims_[i].frames_.size();
      fwrite(&frameVecCount, sizeof(unsigned int), 1, fp_);           //write key frame vector count
      
      for(unsigned int j = 0; j < frameVecCount; ++j)                 //write key frame information
      {
        unsigned int frameCount = anims_[i].frames_[j].size();
        fwrite(&frameCount, sizeof(unsigned int), 1, fp_);             //write key frame count
        
        for(unsigned int k = 0; k < frameCount; ++k)
        {
          fwrite(&anims_[i].frames_[j][k].time_, sizeof(float), 1, fp_);          //write time of key frame
          WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&anims_[i].frames_[j][k].localPos_), 3); //write translation
          WriteDoubleToFloat(fp_, reinterpret_cast<double*>(&anims_[i].frames_[j][k].localRot_), 4); //write rotation
        }
      }
    }
  }

  fclose(fp_);
  delete mtxConverter_;
}

void Scene::PrintScene(KFbxNode* pRootNode)
{
    // Print the nodes of the scene and their attributes recursively.
    // Note that we are not printing the root node, because it should
    // not contain any attributes.

    printf("This Scene has %d Node(s)\n", pRootNode->GetChildCount());

    if(pRootNode) 
    {
      for(int i = 0; i < pRootNode->GetChildCount(); ++i)
        PrintNode(pRootNode->GetChild(i));
    }
}

void Scene::PrintNode(KFbxNode* pNode)
{   
    PrintTabs();
    const char* nodeName = pNode->GetName();
    fbxDouble3 translation = pNode->LclTranslation.Get();
    fbxDouble3 rotation = pNode->LclRotation.Get();
    fbxDouble3 scaling = pNode->LclScaling.Get();

    // print the contents of the node.
    printf("<node name='%s'\ntranslation='(%f, %f, %f)'\nrotation='(%f, %f, %f)'\nscaling='(%f, %f, %f)'>\n", 
          nodeName, 
          translation[0], translation[1], translation[2],
          rotation[0], rotation[1], rotation[2],
          scaling[0], scaling[1], scaling[2]);

    ++numTabs_;

    // Print the node's attributes.
    for(int i = 0; i < pNode->GetNodeAttributeCount(); ++i)
        PrintAttribute(pNode->GetNodeAttributeByIndex(i));

    // Recursively print the children nodes.
    for(int j = 0; j < pNode->GetChildCount(); ++j)
        PrintNode(pNode->GetChild(j));

    numTabs_--;
    PrintTabs();

    printf("</node>\n");
}
void Scene::PrintAttribute(KFbxNodeAttribute* pAttribute)
{
    if(!pAttribute) 
      return;
 
    KString typeName = GetAttributeTypeName(pAttribute->GetAttributeType());
    KString attrName = pAttribute->GetName();
    //PrintTabs();
    // Note: to retrieve the chararcter array of a KString, use its Buffer() method.
    printf("<attribute type='%s' name='%s'/>\n", typeName.Buffer(), attrName.Buffer());
}

KString Scene::GetAttributeTypeName(KFbxNodeAttribute::EAttributeType type)
{
   switch(type) 
   {
    case KFbxNodeAttribute::eUNIDENTIFIED: return "unidentified";
    case KFbxNodeAttribute::eNULL: return "null";
    case KFbxNodeAttribute::eMARKER: return "marker";
    case KFbxNodeAttribute::eSKELETON: return "skeleton";
    case KFbxNodeAttribute::eMESH: return "mesh";
    case KFbxNodeAttribute::eNURB: return "nurb";
    case KFbxNodeAttribute::ePATCH: return "patch";
    case KFbxNodeAttribute::eCAMERA: return "camera";
    case KFbxNodeAttribute::eCAMERA_STEREO:    return "stereo";
    case KFbxNodeAttribute::eCAMERA_SWITCHER: return "camera switcher";
    case KFbxNodeAttribute::eLIGHT: return "light";
    case KFbxNodeAttribute::eOPTICAL_REFERENCE: return "optical reference";
    case KFbxNodeAttribute::eOPTICAL_MARKER: return "marker";
    case KFbxNodeAttribute::eNURBS_CURVE: return "nurbs curve";
    case KFbxNodeAttribute::eTRIM_NURBS_SURFACE: return "trim nurbs surface";
    case KFbxNodeAttribute::eBOUNDARY: return "boundary";
    case KFbxNodeAttribute::eNURBS_SURFACE: return "nurbs surface";
    case KFbxNodeAttribute::eSHAPE: return "shape";
    case KFbxNodeAttribute::eLODGROUP: return "lodgroup";
    case KFbxNodeAttribute::eSUBDIV: return "subdiv";
    default: return "unknown";
   }
}
void Scene::PrintMesh(KFbxNode* pRootNode)
{}
void Scene::TransformTansBitans(FbxMesh& mesh, KFbxXMatrix& transform)
{
  for(unsigned int i = 0; i < mesh.verts_.size(); ++i)
  {
    mesh.verts_[i].tan_.Normalize();
    mesh.verts_[i].bitan_.Normalize();
  }
}
void Scene::AvgTanBins(std::vector<Tri>& triangles, unsigned int numTris, std::vector<FbxVert>& verts, unsigned int numVerts)
{
  //For every vert
  for(unsigned int i = 0; i < numVerts; ++i)
  {
    unsigned int sharedTris = 0;

    //Figure out how many triangles share this vertex
    for(unsigned int j = 0; j < numTris; ++j)
      if(i == triangles[j].p0_ || i == triangles[j].p1_ || i == triangles[j].p2_)
        ++sharedTris;

    //Average the tangent and binormal.
      verts[i].tan_   /= sharedTris;
      verts[i].bitan_ /= sharedTris;
  }
}
void Scene::CalcTriTanBinNorm(const KFbxVector4& P1, const KFbxVector4& P2, const KFbxVector4& P3, 
                       const KFbxVector2& UV1, const KFbxVector2& UV2, const KFbxVector2& UV3,
                       KFbxVector4 &tangent, KFbxVector4 &binormal)
{
  //Create a vector from p1_ to p2 and p1_ to p3, same with the uv coordinates
  KFbxVector4 Edge1 = P2 - P1;
  KFbxVector4 Edge2 = P3 - P1;
  KFbxVector4 Edge1uv = UV2 - UV1;
  KFbxVector4 Edge2uv = UV3 - UV1;

  //Grab the determinant of the matrix created by the uv edges.
  float det = Edge1uv[1] * Edge2uv[0] - Edge1uv[0] * Edge2uv[1];

  if(det) 
  {
    //Use it to "solve" for the tangent and binormal.
    float inverse = 1.0f / det;
    tangent   = (Edge1 * -Edge2uv[1] + Edge2 *  Edge1uv[1]) * inverse;
    binormal  = (Edge1 *  Edge2uv[0] + Edge2 * -Edge1uv[0]) * inverse;
  }
}
void Scene::CalculateTansAndBitans(FbxMesh& mesh)
{
  //Initialize all tangents and binormals.
  for(unsigned int i = 0; i < mesh.verts_.size(); ++i)
    mesh.verts_[i].bitan_ = mesh.verts_[i].tan_ = KFbxVector4(0,0,0,0);

  //Generate tangents and binormals for each triangle.
  for(unsigned int i = 0; i < mesh.numTris; ++i)
  {
    //Grab the vertexes out of the vertex list to work with.
    FbxVert& p1 = mesh.verts_[mesh.triangles[i].p0_];
    FbxVert& p2 = mesh.verts_[mesh.triangles[i].p1_];
    FbxVert& p3 = mesh.verts_[mesh.triangles[i].p2_];

    //Calculate tangent and binormal for this triangle
    KFbxVector4 tan, bin;
    CalcTriTanBinNorm(p1.pos_, p2.pos_, p3.pos_, 
                      p1.uv_, p2.uv_, p3.uv_, 
                      tan, bin);

    //Add it to the average of all the vertexes in the triangle.
    p1.bitan_ += bin;
    p2.bitan_ += bin;
    p3.bitan_ += bin;

    p1.tan_ += tan;
    p2.tan_ += tan;
    p3.tan_ += tan;
  }

  //Now go back through the vert list, and average every triangle that has that vert to get
  //its specific tangent and binormal, then store it into the vertex.
  AvgTanBins(mesh.triangles, mesh.numTris, mesh.verts_, mesh.verts_.size());
}
void Scene::ProcessMeshes(void)
{
  printf("Processing Meshes.\n");

  //Check to see if we even have any to process.
  if(!meshes_.size())
  {
    printf("No meshes gathered.\n");
    return;
  }
  //Iterate over all the meshes and get the data we want.
  for(unsigned int i = 0; i < meshes_.size(); ++i)
  {
    KFbxXMatrix transform;
    GetPositions(meshes_[i], transform);
    GetNormalsUvs(meshes_[i], transform);
    //Generate the correct vertices for this thing.
    GenerateVertices(meshes_[i]);
    meshes_[i].verts_.swap(meshes_[i].ProcessedVertices);
    meshes_[i].indices_.swap(meshes_[i].ProcessedIndices);
    GrabSkinWeights(meshes_[i]);

    //Flip the ys on the UV coordinates for DX
    for(unsigned int j = 0; j < meshes_[i].verts_.size(); ++j)
      meshes_[i].verts_[j].uv_[1] = 1.0f - meshes_[i].verts_[j].uv_[1];


    //If we didn't get tangents or bitangents for the mesh, we should generate them.
    if(!tans_ || !bitans_)
    {
      //Create a triangle list for our newly triangulated model.
      GenerateTriangleIndices(meshes_[i]);
      //Grab all the tangents and bitangents after triangulation
      CalculateTansAndBitans(meshes_[i]);
      //Transform the tangents/bitangents to be in the same space the normals are.
      TransformTansBitans(meshes_[i], transform);
    }
  }

  printf("Done processing meshes.\n");
}
void Scene::GrabSkinWeights(FbxMesh& mesh)
{
  mesh.skin.PointWeights.resize( mesh.verts_.size() );

  //Now grab skin weights
  int skinCount = mesh.mesh_->GetDeformerCount(KFbxDeformer::eSKIN);
  for(int j = 0; j < skinCount; ++j)
  {
    //Grab the skin, then iterate over each of its clusters grabbing weights
    KFbxSkin *skin = reinterpret_cast<KFbxSkin*>(mesh.mesh_->GetDeformer(j, KFbxDeformer::eSKIN));
    int clusterCount = skin->GetClusterCount();
    for(int k = 0; k < clusterCount; ++k)
    {
      //Grab the current cluster and check for a valid link.
      KFbxCluster *cluster = skin->GetCluster(k);
      KFbxNode *link = cluster->GetLink();
      if(!link)
        continue;

      //Find that link in the pose, or add the link if it's not present.
      int nodeIndex = pose_->Find(link);
      if(nodeIndex == -1)
      {
        KFbxXMatrix linkMtx;
        cluster->GetTransformLinkMatrix(linkMtx);
        pose_->Add(link, KFbxMatrix(linkMtx));
      }

      //Now grab the actual weights from the skin
      double *weights = cluster->GetControlPointWeights();
      int *ctrlPtIndices = cluster->GetControlPointIndices();
      int ctrlPtIndCount = cluster->GetControlPointIndicesCount();
      int boneIndex = FindBone(link);

      for(int l = 0; l < ctrlPtIndCount; ++l)
      {
        //Grab the current vertex index
        int vertIndex = ctrlPtIndices[l];

        //Grab its weight and assign it to our temporary vertex buffer.
        float weight = static_cast<float>(weights[l]);

        JointWeight jw = {weight, boneIndex};
        mesh.skin.PointWeights[vertIndex].push_back(jw);
      }
    }
  }

  //Normalize the skin weights for 4 weights for vertex that sum to one
  for(unsigned int j = 0; j < mesh.skin.PointWeights.size(); ++j)
  {
    //Make sure there is MaxWeights by inserting dummies
    while(mesh.skin.PointWeights[j].size() < maxWeights_)
    {						
      JointWeight jw = {0,0};
      mesh.skin.PointWeights[j].push_back(jw);
    }
    //Normalize the weights
    float sum = 0.0f;
    for(unsigned int w = 0; w < maxWeights_; ++w)
      sum += mesh.skin.PointWeights[j][w].weight;
    for(unsigned int w = 0; w < maxWeights_; ++w)
      mesh.skin.PointWeights[j][w].weight /= sum;
  }
}
void Scene::GetNormalsUvs(FbxMesh& inmesh, KFbxXMatrix& transform)
{
  transform.SetT(KFbxVector4(0, 0, 0, 0));
  inmesh.nrmMatrix_ = transform;

  KFbxLayer *layer = inmesh.mesh_->GetLayer(0);

  int vertexCount = inmesh.mesh_->GetPolygonVertexCount();

  //Now grab normals and uvs.
  KFbxLayerElementNormal *normLayer = layer->GetNormals();
  if(normLayer)
  {
    ConvertToStl(inmesh.norms, &normLayer->GetDirectArray() );

    for(unsigned int n = 0; n < inmesh.norms.size(); ++n)
    {
      inmesh.norms[n] = Transform( transform , inmesh.norms[n] );
      inmesh.norms[n].Normalize();
    }

    KFbxLayerElement::EReferenceMode refMode = normLayer->GetReferenceMode();

    //Grab uv coords now as that's in common in all other cases.
    KFbxLayerElementUV *uvlayer = layer->GetUVs();
    if(refMode == KFbxLayerElement::eINDEX_TO_DIRECT)
    {
      ConvertToStl(inmesh.nInd, &normLayer->GetIndexArray() );

      //If there is a uv layer.
      if(uvlayer)
      {
        ConvertToStl( inmesh.uvs , &uvlayer->GetDirectArray() );
        KFbxLayerElement::EMappingMode Mapping = uvlayer->GetMappingMode();
        if( uvlayer->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT )
          ConvertToStl( inmesh.uvInd , &uvlayer->GetIndexArray() );
        else
          inmesh.uvInd = inmesh.posInd;
      }
    }
    else
    {
      KFbxLayerElement::EMappingMode mapMode = normLayer->GetMappingMode();
      if(mapMode == KFbxLayerElement::eBY_CONTROL_POINT)
      {
        inmesh.nInd = inmesh.posInd;
        if(uvlayer)
        {
          ConvertToStl( inmesh.uvs , &uvlayer->GetDirectArray() );
          KFbxLayerElement::EMappingMode Mapping = uvlayer->GetMappingMode();
          if(uvlayer->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT)
            ConvertToStl( inmesh.uvInd , &uvlayer->GetIndexArray() );
          else
            inmesh.uvInd = inmesh.posInd;
        }
      }
      else
      {
        FillStl(inmesh.nInd, vertexCount);
        if(uvlayer)
        {
          ConvertToStl( inmesh.uvs , &uvlayer->GetDirectArray() );
          KFbxLayerElement::EMappingMode Mapping = uvlayer->GetMappingMode();
          if(uvlayer->GetReferenceMode() == KFbxLayerElement::eINDEX_TO_DIRECT)
            ConvertToStl( inmesh.uvInd , &uvlayer->GetIndexArray());
          else
            inmesh.uvInd = inmesh.posInd;
        }
      }
    }
  }
  //Get Tangents
  KFbxLayerElementTangent *tanLayer = layer->GetTangents();
  if(tanLayer)
  {
    ConvertToStl(inmesh.tans, &tanLayer->GetDirectArray());

    for(unsigned int t = 0; t < inmesh.tans.size(); ++t)
    {
      inmesh.tans[t] = Transform( transform , inmesh.tans[t] );
      inmesh.tans[t].Normalize();
    }

    KFbxLayerElement::EReferenceMode refMode = tanLayer->GetReferenceMode();

    if(refMode == KFbxLayerElement::eINDEX_TO_DIRECT)
    {
      ConvertToStl(inmesh.tInd, &tanLayer->GetIndexArray());
    }
    else
    {
      KFbxLayerElement::EMappingMode mapMode = tanLayer->GetMappingMode();
      if(mapMode == KFbxLayerElement::eBY_CONTROL_POINT)
        inmesh.tInd = inmesh.posInd;
          
      else
        FillStl(inmesh.tInd, vertexCount);
    }
  }
  else
  {
    printf("Didn't get tangents.\n");
    tans_ = false;
  }

  //Get Bitangents
  KFbxLayerElementBinormal *binLayer = layer->GetBinormals();
  if(binLayer)
  {
    ConvertToStl(inmesh.bitans, &binLayer->GetDirectArray());

    for(unsigned int b = 0; b < inmesh.bitans.size(); ++b)
    {
      inmesh.bitans[b] = Transform( transform , inmesh.bitans[b]);
      inmesh.bitans[b].Normalize();
    }

    KFbxLayerElement::EReferenceMode refMode = binLayer->GetReferenceMode();

    if(refMode == KFbxLayerElement::eINDEX_TO_DIRECT)
      ConvertToStl(inmesh.bInd, &binLayer->GetIndexArray());
    else
    {
      KFbxLayerElement::EMappingMode mapMode = binLayer->GetMappingMode();
      if(mapMode == KFbxLayerElement::eBY_CONTROL_POINT)
        inmesh.bInd = inmesh.posInd;
      else
        FillStl(inmesh.bInd, vertexCount);
    }
  }
  else
  {
    printf("Didn't get bitangents.\n");
    bitans_ = false;
  }
}
void Scene::ConvertTriWinding(FbxMesh& mesh)
{
  for(unsigned int i = 0; i < mesh.ProcessedIndices.size(); i += 3)
  {
    std::swap(mesh.ProcessedIndices[i], mesh.ProcessedIndices[i+2]);
  }
}
void Scene::Triangulate(FbxMesh& mesh)
{
  std::vector<int> NewIndices;
  int c = 0;
   
  for(unsigned int i = 0; i  < mesh.polySizeArray.size(); ++i)
  {
    int size = mesh.polySizeArray[i];		
    //Simple Convex Polygon Triangulation: always n-2 tris
    const int NumberOfTris = size - 2;

    for(int p = 0; p < NumberOfTris; ++p)
    {
      NewIndices.push_back(mesh.ProcessedIndices[c+0]);
      NewIndices.push_back(mesh.ProcessedIndices[c+1+p]);
      NewIndices.push_back(mesh.ProcessedIndices[c+2+p]);
    }	
    c += size;
  }

  //Swap the new triangulated indices with the old vertices
  mesh.ProcessedIndices.swap(NewIndices);
}
int Scene::AddNewVertex(IndexedVert v, FbxMesh& mesh)
{
  FbxVert nv;
  nv.nrm_ = mesh.norms[v.normIndex];
  nv.pos_ = mesh.positions[v.posIndex];
  nv.uv_ = mesh.uvs[v.uvIndex];

  if(tans_ && bitans_)
  {
    nv.tan_ = mesh.tans[v.tanIndex];
    nv.bitan_ = mesh.bitans[v.bitanIndex];
  }

  mesh.ProcessedVertices.push_back(nv);
  mesh.source.push_back(v);

  return mesh.ProcessedVertices.size() - 1;
}
bool Scene::IsMatchingVertex(FbxVert& vertex, IndexedVert& cur, IndexedVert& v)
{
  //Need to check to see if the normals and uvs match if not generate a new vertex
  if(v.normIndex != cur.normIndex)
    return false;

  if(v.uvIndex != cur.uvIndex)
    return false;

  return true;
}
int Scene::FindMatchingVertex(IndexedVert& v, FbxMesh& mesh)
{
  std::vector<int>& mappedVertices = mesh.mappedVerts[v.posIndex];
  for(unsigned int i = 0; i < mappedVertices.size(); ++i)
  {
    if(IsMatchingVertex(mesh.ProcessedVertices[mappedVertices[i]], mesh.source[mappedVertices[i]], v))
      return mappedVertices[i];
  }

  //No matching vertices mapped for this index, just add one
  int newIndex = AddNewVertex(v, mesh);
  mappedVertices.push_back(newIndex);
  
  return newIndex;
}
void Scene::GenerateVertices(FbxMesh& mesh)
{
  printf("Generating triangulated verts.");
  if(mesh.uvInd.empty())
  {
    //Fill UvIndices with zeros
    mesh.uvInd.resize( mesh.posInd.size());
    //Put in a dummy UV
    mesh.uvs.push_back( KFbxVector2(0,0));
  }
  //Create a vertex and an index buffer
  mesh.mappedVerts.resize(mesh.posInd.size());
  printf("%d\n", mesh.posInd.size());
  printf("%d\n", mesh.tInd.size());
  printf("%d\n", mesh.bInd.size());
  for(unsigned int i = 0; i < mesh.posInd.size(); ++i)
  {
    IndexedVert v;
    v.posIndex   = mesh.posInd[i];
    v.normIndex  = mesh.nInd[i];
    v.uvIndex    = mesh.uvInd[i];

    if(tans_)
      v.tanIndex   = mesh.tInd[i];
      
    if(bitans_)
      v.bitanIndex = mesh.bInd[i];
  
    int index = FindMatchingVertex(v, mesh);
    mesh.ProcessedIndices.push_back( index );
  }
	
  Triangulate(mesh);
  ConvertTriWinding(mesh);
}
void Scene::ProcessBones(void)
{
  printf("Processing bones.");
  int boneCount = bones_.size();
  
  if(boneCount == 0)
  {
    printf("No bones in Scene.\n");
      return;
  }

  //Grab all the relevant info we want from every bone.
  for(int i = 0; i < boneCount; ++i)
  {
    //For storing matrix data we'll be working with for inverse transform
    KFbxXMatrix matrix;
    if(pose_)
    {
      int nodeIndex = pose_->Find(bones_[i].bone_->GetNode());
      if(nodeIndex > -1)
      {
        KFbxMatrix tmp = pose_->GetMatrix(nodeIndex);
        matrix = *(reinterpret_cast<KFbxXMatrix*>(&tmp));
      }

      //Take care of non uniform scaling
      KFbxVector4 scale = matrix.GetS();
      for(int j = 0; j < 4; ++j)
        scale.SetAt(j, 1 / scale[j]);
        
      KFbxXMatrix scaleMatrix;
      scaleMatrix.SetS(scale);
      matrix = scaleMatrix * matrix;
    }
      
    //Transform the matrix to the correct space.
    mtxConverter_->ConvertMatrix(matrix);

    //Grab all of the local data we need for this bone
    bones_[i].invTransform_ = matrix.Inverse();
    bones_[i].localPos_ = matrix.GetT();
    bones_[i].localRot_ = matrix.GetQ();
  }

  printf("Done processing bones.\n");
}
void Scene::GenerateTriangleIndices(FbxMesh& mesh)
{
  mesh.numTris = mesh.indices_.size() / 3;
  mesh.triangles.resize(mesh.numTris);

  for(unsigned int j = 0, k = 0; j < mesh.indices_.size(); j += 3, ++k)
  {
    //Every 3 indices is a new triangle
    Tri tri;
    tri.p0_ = mesh.indices_[j];
    tri.p1_ = mesh.indices_[j + 1];
    tri.p2_ = mesh.indices_[j + 2];

    mesh.triangles[k] = tri;
  }
}
int Scene::FindBone(KFbxNode *node)
{
  //Return the index of the bone you find or -1
  for(unsigned i = 0; i < bones_.size(); ++i)
    if(bones_[i].bone_->GetNode() == node)
      return i;
    return -1;
}

void Scene::GetPositions(FbxMesh& inmesh, KFbxXMatrix& trans)
{
  //Convert this mesh's transform mtx to DirectX space before we start using it.
  KFbxXMatrix transformMatrix = *(reinterpret_cast<KFbxXMatrix *>(&inmesh.transformMtx_));
  mtxConverter_->ConvertMeshMatrix(transformMatrix);
  inmesh.transformMtx_ = transformMatrix;

  KFbxMesh *mesh = inmesh.mesh_;

  int polyCount = mesh->GetPolygonCount();

  //Grab the indices of all our verts
  int *indices = mesh->GetPolygonVertices();
  int vertexCount = mesh->GetPolygonVertexCount();

  inmesh.posInd.resize(vertexCount);
  memcpy(&inmesh.posInd[0], indices, sizeof(int) * vertexCount);

  for(int p = 0; p < polyCount; ++p)
    inmesh.polySizeArray.push_back(mesh->GetPolygonSize(p));

  //Grab the control points, triangle count, and use them to get the vertex positions.
  int ctrlPtCount = mesh->GetControlPointsCount();
  KFbxVector4 *ctrlPts = mesh->GetControlPoints();
  int tris = polyCount;

  //Grab the normal matrix of each mesh and the "world positions" of each vertex
  KFbxNode *node = mesh->GetNode();
  KFbxXMatrix mtw;

  if(pose_)
  {
    int nodeIndex = pose_->Find(mesh->GetNode());
    KFbxMatrix mtx = pose_->GetMatrix(nodeIndex);
    mtw = *(reinterpret_cast<KFbxXMatrix*>(&mtx));

    //Grab the scaling vector and normalize it.
    KFbxVector4 scaleVec = mtw.GetS();
    for(int i = 0; i < 4; ++i)
      scaleVec.SetAt(i, 1 / scaleVec[i]);

    KFbxXMatrix scaleMatrix;
    scaleMatrix.SetS(scaleVec);

    mtw = scaleMatrix * mtw;
  }
  else 
    mtw = node->GetScene()->GetEvaluator()->GetNodeGlobalTransform(node);

  //Create an offset matrix that is the local transform
  KFbxVector4 t = node->GetGeometricTranslation(KFbxNode::eSOURCE_SET);
  KFbxVector4 r = node->GetGeometricRotation(KFbxNode::eSOURCE_SET);
  KFbxVector4 s = node->GetGeometricScaling(KFbxNode::eSOURCE_SET);

  KFbxXMatrix geoTransform (t,r,s);

  //create the transform by appending the world to the local transform
  trans = mtw * geoTransform;

  //Converter to DX space
  mtxConverter_->ConvertMeshMatrix(trans);

  inmesh.positions.resize(ctrlPtCount);
  for(int cp = 0; cp < ctrlPtCount; ++cp)
    inmesh.positions[cp] = Transform(trans, ctrlPts[cp]);
}
void Scene::CombineMeshes(void)
{
  printf("Combining meshes\n");
  printf("%d", meshes_.size());
  
  for(unsigned int i = 1; i < meshes_.size(); ++i)
    meshes_[0].CombineInto( meshes_[i] );
}
KFbxVector4 Scene::Transform(const KFbxXMatrix& pXMatrix, const KFbxVector4& point)
{
  KFbxMatrix * m = (KFbxMatrix*)&pXMatrix;
  return m->MultNormalize( point );
}
void Scene::CollectBones(KFbxNode* pRootNode)
{
    printf("Collecting Bones from the scene...\n");

    //Get the queue of bones set up
    int bIndex = -1;
    std::queue<FbxParent> boneQ;
    boneQ.push(FbxParent(bIndex, pRootNode));

    //Extract relevant data from each bone into our structures
    //Then get its children into the queue
    while(!boneQ.empty())
    {
      //Grab the top bone
      FbxParent curNode = boneQ.front();
      boneQ.pop();

      //Get the attributes off the node if it's valid
      if(GetNodeAttributeType(curNode.node_) == KFbxNodeAttribute::eSKELETON)
      {
        //Fill in a bone and add it to our list of bones
        FbxBone bone;
        bone.bone_ = curNode.node_->GetSkeleton();
        bone.name_ = bone.bone_->GetName();
        printf("Collecting bone: %s\n", bone.name_.c_str());
        bone.pIndex_ = curNode.parent_;
        bIndex = bones_.size();
        bone.index_ = bIndex;
        bones_.push_back(bone);
      }

      //Push all the children bones onto the queue
      int childCount =  curNode.node_->GetChildCount();
      for(int i = 0; i < childCount; ++i)
        boneQ.push(FbxParent(bIndex, curNode.node_->GetChild(i)));
    }

    printf("Done...\n");
}
  void Scene::GetKeyFrames(FbxBone &bone, const char *takeName, std::set<KTime> &keyTimes, int animlayer)
  {
    
   

    //Grab the current take's node.
  // scene_->ActiveAnimStackName = const_cast<char*>(takeName);
  //  bone.bone_->SetCurrentTakeNode(const_cast<char*>(takeName));
    bone.bone_->GetNode()->GetScene()->ActiveAnimStackName =  takeName;
    KString lActiveAnimStackName = KFbxGet<KString>(scene_->ActiveAnimStackName);
   KFbxAnimStack* lAnimStack = scene_->FindMember(FBX_TYPE(KFbxAnimStack), lActiveAnimStackName.Buffer());
   KFbxAnimLayer* lAnimLayer = lAnimStack->GetMember(FBX_TYPE(KFbxAnimLayer), animlayer);
    //Grab all the key times for the local translation and rotation for x y and z
    CollectKeyTimes(keyTimes, bone.bone_->GetNode()->LclRotation, KFCURVENODE_R_X, takeName, lAnimLayer);
    CollectKeyTimes(keyTimes, bone.bone_->GetNode()->LclRotation, KFCURVENODE_R_Y, takeName, lAnimLayer);
    CollectKeyTimes(keyTimes, bone.bone_->GetNode()->LclRotation, KFCURVENODE_R_Z, takeName, lAnimLayer);
    CollectKeyTimes(keyTimes, bone.bone_->GetNode()->LclTranslation, KFCURVENODE_T_X, takeName, lAnimLayer);
    CollectKeyTimes(keyTimes, bone.bone_->GetNode()->LclTranslation, KFCURVENODE_T_Y, takeName, lAnimLayer);
    CollectKeyTimes(keyTimes, bone.bone_->GetNode()->LclTranslation, KFCURVENODE_T_Z, takeName, lAnimLayer);
  }
void Scene::CollectAnimations(void)
  {
    printf("Collecting animations from bones.\n");
    int takeCount = takes_.GetCount();
    if(takeCount < 1)
    {
      printf("No animations to collect.\n");
      return;
    }

    for(int i = 0; i < takeCount; ++i)
    {
      //For animation length
      KTime start;
      KTime end;
      KTime duration;

      //Extract the take info and calculate the duration if possible.
     
      scene_->ActiveAnimStackName = takes_[i]->Buffer();
      KFbxTakeInfo *info = scene_->GetTakeInfo(*takes_[i]);
      if(info)
      {
        start = info->mLocalTimeSpan.GetStart();
        end = info->mLocalTimeSpan.GetStop();
        duration = end - start;
      }

      //Get a blank animation ready to be filled in
      anims_.push_back(FbxAnimation());

      //Fill in the blank animations
      anims_.back().length_ = static_cast<float>(duration.GetSecondDouble());
      anims_.back().name_ = takes_[i]->Buffer();

      int boneCount = bones_.size();
      for(int j = 0; j < boneCount; ++j)
      {
        std::set<KTime> keyTimes;
        keyTimes.insert(start);
        GetKeyFrames(bones_[j], takes_[i]->Buffer(), keyTimes, i);
        anims_.back().frames_.push_back(std::vector<FbxKeyFrame>());
        for(std::set<KTime>::iterator it = keyTimes.begin(); it != keyTimes.end(); ++it)
        {
          KFbxXMatrix pInv;
          KFbxXMatrix lTrans;

          if(GetNodeAttributeType(bones_[j].bone_->GetNode()->GetParent()) == KFbxNodeAttribute::eSKELETON )
            pInv = bones_[j].bone_->GetNode()->GetScene()->GetEvaluator()->GetNodeGlobalTransform(bones_[j].bone_->GetNode()->GetParent(), *it).Inverse();

          //lTrans = pInv * bones_[j].bone_->GetNode()->GetGlobalFromCurrentTake(*it);
          lTrans = pInv * bones_[j].bone_->GetNode()->GetScene()->GetEvaluator()->GetNodeGlobalTransform(bones_[j].bone_->GetNode(), *it);
          //Deal with scaling (normalize it)
          KFbxVector4 scale = lTrans.GetS();
          for(int k = 0; k < 4; ++k)
            scale.SetAt(k, 1 / scale[k]);
          KFbxXMatrix scaleMatrix;
          scaleMatrix.SetS(scale);
          lTrans = scaleMatrix * lTrans;

          //Convert the matrix from FBX space to DirectX Space
          mtxConverter_->ConvertMatrix(lTrans);

          //Finally give the new transformation matrix to the animations.
          anims_.back().frames_.back().push_back(lTrans);
          anims_.back().frames_.back().back().time_ = it->GetSecondDouble();
        }
      }
    }
    printf("Done getting the animations.\n");
  }
void Scene::CollectMeshes(KFbxNode* pRootNode)
{
  printf("Collecting Meshes from the scene...\n");

  //Get all the mesh data for every node in our scene.
  KFbxNode *root = scene_->GetRootNode();
  int childCount = root->GetChildCount();

  for(int i = 0; i < childCount; ++i)
  {
    KFbxNode *node = root->GetChild(i);
    ExtractMesh(node);
  }

  printf("Done...\n");
}

void Scene::ExtractMesh(KFbxNode *pNode)
{
    //Recurse to the bottom of the scene tree
    int childCount = pNode->GetChildCount();

    if(childCount > 0)
      for(int i = 0; i < childCount; ++i)
        ExtractMesh(pNode->GetChild(i));

    //Once no more children are left we do work from the bottom up.

    //Grab name and the rest of the attributes we want
    const char *name = pNode->GetName();
    int attrCount = pNode->GetNodeAttributeCount();
    
    //Go through all this node's attributes looking for what we want.
    for(int i = 0; i < attrCount; ++i)
    {
           //Grab the type of the current attribute on this node
      KFbxNodeAttribute::EAttributeType attribute = pNode->GetNodeAttributeByIndex(i)->GetAttributeType();

            //Keeping this as a switch because this may be where I get skinning/lighting data, not sure yet.
      switch(attribute)
      {
        case KFbxNodeAttribute::eMESH:
        {
          meshes_.push_back(pNode->GetMesh());
          meshes_.back().name_ = name;
          meshes_.back().meshType = type_;
          break;
        }
      }
    }
}
KFbxNodeAttribute::EAttributeType Scene::GetNodeAttributeType(KFbxNode* pNode)
{
	KFbxNodeAttribute* nodeAtt = pNode->GetNodeAttribute();

	if( nodeAtt != NULL )
		return nodeAtt->GetAttributeType();

	return KFbxNodeAttribute::eNULL;
}
void Scene::CollectKeyTimes(std::set<KTime> &keyTimes, KFbxTypedProperty<fbxDouble3> &attribute, const char *curveName, const char *takeName, KFbxAnimLayer* layer)
  {
    //Grab the curve
   // KFCurve *curve = attribute.GetKFCurve(curveName, takeName);
    KFbxAnimCurve* lAnimCurve = attribute.GetCurve<KFbxAnimCurve>(layer, curveName);
    if(lAnimCurve)
    {
      //Grab all the key times
      int keyCount = lAnimCurve->KeyGetCount();
      for(int i = 0; i < keyCount; ++i)
      {
        KFbxAnimCurveKey curveKey = lAnimCurve->KeyGet(i);
        keyTimes.insert(curveKey.GetTime());
      }
    }
  }