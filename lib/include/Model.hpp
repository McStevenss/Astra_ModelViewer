#ifndef MODEL_HPP
#define MODEL_HPP

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Shader.hpp"
#include "Mesh.hpp"
#include "AnimationUtils.h"
#include "assimp_glm_helpers.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <vector>
using namespace std;

class Model 
{
public:
    // model data 
    vector<Texture> textures_loaded;	// stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh>    meshes;
    
    string directory;
    bool gammaCorrection;
    

    auto& GetBoneInfoMap() { return mBoneInfoMap; }
    int& GetBoneCount() { return mBoneCounter; }   

    bool hasDiffuse = false;
    bool hasNormal = false;
    bool hasSpecular = false;

    bool dirty = true;
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
    glm::mat4 ModelMatrix = glm::mat4(1.0f);

    float scaleFactor = 0.01f; // what makes the model fit
    // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleFactor));
    // rootTransform = scaleMat * rootTransform; // multiply the root node's transform

    glm::vec3 aabbMin = glm::vec3(FLT_MAX);
    glm::vec3 aabbMax = glm::vec3(-FLT_MAX);

    glm::vec3 localAabbMin = glm::vec3(FLT_MAX);
    glm::vec3 localAabbMax = glm::vec3(-FLT_MAX);

    Model() = default;

    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false, bool flipUVs = false) : gammaCorrection(gamma)
    {
        loadModel(path,flipUVs);
        std::cout << "Model has diffuse:" << hasDiffuse << std::endl;
        std::cout << "Model has normal:" << hasNormal << std::endl;
        std::cout << "Model has specular:" << hasSpecular << std::endl;
    }

    ~Model()
    {
        // Free textures
        for (auto& tex : textures_loaded)
        {
            if (glIsTexture(tex.id)) // make sure it’s a valid texture ID
                glDeleteTextures(1, &tex.id);
        }

        for (auto& mesh : meshes){
            mesh.Delete();
        }

    }


    void UpdateModelMatrix(){
        glm::mat4 transformX = glm::rotate(glm::mat4(1.0f),
                        glm::radians(Rotation.x),
                        glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 transformY = glm::rotate(glm::mat4(1.0f),
                        glm::radians(Rotation.y),
                        glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 transformZ = glm::rotate(glm::mat4(1.0f),
                        glm::radians(Rotation.z),
                        glm::vec3(0.0f, 0.0f, 1.0f));

        // Y * X * Z
        glm::mat4 roationMatrix = transformY * transformX * transformZ;
        ModelMatrix = glm::translate(glm::mat4(1.0f), Position) * roationMatrix * glm::scale(glm::mat4(1.0f), Scale);

        UpdateAABB();
    }

    // draws the model, and thus all its meshes
    void Draw(Shader &shader)
    {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path,bool flipUVs)
    {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene;// = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        if(flipUVs){
            scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);
            // scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        }
        else{
            scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace  | aiProcess_JoinIdenticalVertices);
            // scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);
        }
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
    }

    unsigned int TextureFromFile(const char *path, const string &directory, bool gamma=false, bool binaryTexture=false)
    {
        string filename = string(path);

        if(!binaryTexture){
            filename = directory + '/' + filename;
        }

        // std::cout << "Loading texture from: " << filename << std::endl;
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << " used file name: " << filename.c_str() << std::endl;
            
            std::cout << "Current working directory: " 
              << std::filesystem::current_path() 
              << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }
    
private:

    // --- Animation variables ---
    std::map<string, BoneInfo> mBoneInfoMap;
    int mBoneCounter = 0;

    void SetVertexBoneDataToDefault(Vertex& vertex)
    {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
        {
            vertex.m_BoneIDs[i] = -1;
            vertex.m_Weights[i] = 0.0f;
        }
    }

    void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
	{
		for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
		{
			if (vertex.m_BoneIDs[i] < 0)
			{
				vertex.m_Weights[i] = weight;
				vertex.m_BoneIDs[i] = boneID;
				break;
			}
		}
	}

    // void UpdateAABB(const glm::mat4& modelMatrix)
    void UpdateAABB()
    {
        glm::vec3 min = localAabbMin;
        glm::vec3 max = localAabbMax;

        glm::vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {min.x, max.y, min.z},
            {max.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {min.x, max.y, max.z},
            {max.x, max.y, max.z}
        };

        glm::vec3 newMin(FLT_MAX);
        glm::vec3 newMax(-FLT_MAX);

        for (int i = 0; i < 8; i++) {
            glm::vec4 transformed = ModelMatrix * glm::vec4(corners[i], 1.0f);
            glm::vec3 world = glm::vec3(transformed);
            newMin = glm::min(newMin, world);
            newMax = glm::max(newMax, world);
        }

        aabbMin = newMin;
        aabbMax = newMax;
    }



    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene. 
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        glm::vec3 minPoint(FLT_MAX);
        glm::vec3 maxPoint(-FLT_MAX);
        // walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            SetVertexBoneDataToDefault(vertex);
            vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
            
            minPoint = glm::min(minPoint, vertex.Position);
            maxPoint = glm::max(maxPoint, vertex.Position);
            // normals
            if (mesh->HasNormals())
            {
                vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
            }
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x; 
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                vertex.Tangent = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);
                vertex.Bitangent = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);

            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);


            vertices.push_back(vertex);
            aabbMin =  glm::min(minPoint, aabbMin);
            aabbMax =  glm::max(maxPoint, aabbMax);         
            localAabbMin =  glm::min(minPoint, aabbMin);
            localAabbMax =  glm::max(maxPoint, aabbMax);  
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];    

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
        if (normalMaps.size() == 0){
             normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        }
        
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        

        if (diffuseMaps.size() > 0) hasDiffuse = true;
        if (normalMaps.size() > 0) hasNormal = true;
        if (specularMaps.size() > 0) hasSpecular = true;

        
        ExtractBoneWeightForVertices(vertices,mesh,scene);
        Mesh processedMesh = Mesh(vertices, indices, textures);
        processedMesh.aabbMax = maxPoint;
        processedMesh.aabbMin = minPoint;

        return processedMesh;
        // return Mesh(vertices,indices,textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecessary load duplicate textures.
            }
        }
        return textures;
    }

    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
	{
		auto& boneInfoMap = mBoneInfoMap;
		int& boneCount = mBoneCounter;

		for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				BoneInfo newBoneInfo;
				newBoneInfo.id = boneCount;
				newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				boneInfoMap[boneName] = newBoneInfo;
				boneID = boneCount;
				boneCount++;
			}
			else
			{
				boneID = boneInfoMap[boneName].id;
			}
			assert(boneID != -1);
			auto weights = mesh->mBones[boneIndex]->mWeights;
			int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId <= vertices.size());
				SetVertexBoneData(vertices[vertexId], boneID, weight);
			}
		}
	}


    glm::mat4 ConvertMatrixToGLM(const aiMatrix4x4& from) {
        glm::mat4 to;

        to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
        to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
        to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
        to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;

        return to;
    }
};



#endif