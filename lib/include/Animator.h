#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include "Animation.h"
#include "Bone.h"

class Animator
{
    public:
        Animator(Animation* animation)
        {
            m_CurrentTime = 0.0;
            m_CurrentAnimation = animation;

            m_FinalBoneMatrices.reserve(100);

            for (int i = 0; i < 100; i++)
                m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
        }

        void UpdateAnimation(float dt)
        {
            m_DeltaTime = dt;
            if (m_CurrentAnimation)
            {
                m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
                m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
				// CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), m_CurrentAnimation->GetRootTransform());
				CalculateBoneTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f));
            }
        }

        void PlayAnimation(Animation* pAnimation)
        {
            m_CurrentAnimation = pAnimation;
            m_CurrentTime = 0.0f;
        }

        void CalculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform)
        {
            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone* Bone = m_CurrentAnimation->FindBone(nodeName);

            if (Bone)
            {
                Bone->Update(m_CurrentTime);
                nodeTransform = Bone->GetLocalTransform();
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;

            auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
            if (boneInfoMap.find(nodeName) != boneInfoMap.end())
            {
                int index = boneInfoMap[nodeName].id;
                glm::mat4 offset = boneInfoMap[nodeName].offset;
                m_FinalBoneMatrices[index] = globalTransformation * offset;
            }

            for (int i = 0; i < node->childrenCount; i++)
                CalculateBoneTransform(&node->children[i], globalTransformation);
        }

        std::vector<glm::mat4> GetFinalBoneMatrices()
        {
            return m_FinalBoneMatrices;
        }

        glm::vec3 GetBoneGlobalPosition(const std::string& boneName)
        {
            auto boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
            if (!m_CurrentAnimation) return glm::vec3(-1.0f);

            glm::mat4 globalTransform = GetBoneGlobalTransform(boneName);
            return glm::vec3(globalTransform[3]);
        }

        glm::mat4 GetBoneGlobalTransform(const std::string& boneName)
        {
            glm::mat4 result = glm::mat4(1.0f);
            CalculateBoneGlobalTransform(&m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), boneName, result);
            return result;
        }

        void CalculateBoneGlobalTransform(const AssimpNodeData* node, glm::mat4 parentTransform, 
                                        const std::string& targetBone, glm::mat4& outTransform)
        {
            glm::mat4 nodeTransform = node->transformation;

            Bone* bone = m_CurrentAnimation->FindBone(node->name);
            if (bone)
            {
                bone->Update(m_CurrentTime);
                nodeTransform = bone->GetLocalTransform();
            }

            glm::mat4 globalTransform = parentTransform * nodeTransform;

            if (node->name == targetBone)
                outTransform = globalTransform;

            for (int i = 0; i < node->childrenCount; i++)
                CalculateBoneGlobalTransform(&node->children[i], globalTransform, targetBone, outTransform);
        }

    private:
        std::vector<glm::mat4> m_FinalBoneMatrices;
        Animation* m_CurrentAnimation;
        float m_CurrentTime;
        float m_DeltaTime;

};
#endif