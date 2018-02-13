#include <stdlib.h>
#include <iostream>

#include <glad/glad.h>

#include "meshobject.h"

#define bufferOffest(x) ((char*)NULL+(x))

MeshObject::MeshObject()
    : IndiceCnt(0)
    , VertCnt(0)
    , EnabledArrays(0)
    , VAO(0)
    , VBO(0)
    , IBO(0)
    , PivotPoint(0,0,0)
    , AABBMin( 9e23f)
    , AABBMax(-9e23f)
    , IndexRangeStart(0)
    , IndexRangeEnd(0)
    , VertArray(0)
    , CleanedUp(true)
{
    /************************************************************************************
      According to:
      http://www.opengl.org/registry/specs/NV/vertex_program.txt

      vertex attribute indices are:

        Vertex
        Attribute  Conventional                                           Conventional
        Register   Per-vertex        Conventional                         Component
        Number     Parameter         Per-vertex Parameter Command         Mapping
        ---------  ---------------   -----------------------------------  ------------
         0         vertex position   Vertex                               x,y,z,w
         1         vertex weights    VertexWeightEXT                      w,0,0,1
         2         normal            Normal                               x,y,z,1
         3         primary color     Color                                r,g,b,a
         4         secondary color   SecondaryColorEXT                    r,g,b,1
         5         fog coordinate    FogCoordEXT                          fc,0,0,1
         6         -                 -                                    -
         7         -                 -                                    -
         8         texture coord 0   MultiTexCoord(GL_TEXTURE0_ARB, ...)  s,t,r,q
         9         texture coord 1   MultiTexCoord(GL_TEXTURE1_ARB, ...)  s,t,r,q
         10        texture coord 2   MultiTexCoord(GL_TEXTURE2_ARB, ...)  s,t,r,q
         11        texture coord 3   MultiTexCoord(GL_TEXTURE3_ARB, ...)  s,t,r,q
         12        texture coord 4   MultiTexCoord(GL_TEXTURE4_ARB, ...)  s,t,r,q
         13        texture coord 5   MultiTexCoord(GL_TEXTURE5_ARB, ...)  s,t,r,q
         14        texture coord 6   MultiTexCoord(GL_TEXTURE6_ARB, ...)  s,t,r,q
         15        texture coord 7   MultiTexCoord(GL_TEXTURE7_ARB, ...)  s,t,r,q

        Table X.2:  Aliasing of vertex attributes with conventional per-vertex
        parameters.
    **************************************************************************************/
}

MeshObject::~MeshObject()
{
    shutdown();
}

void MeshObject::init(const MeshBuffer& meshObj)
{
    // setup the mesh 
    setMesh(meshObj);
    CleanedUp = false;
}

void MeshObject::update()
{

}

void MeshObject::render()
{
    glBindVertexArray(VAO);

    if (IndiceCnt)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glDrawElements(GL_TRIANGLES, IndexRangeEnd, GL_UNSIGNED_INT, (const void*)(IndexRangeStart * sizeof(unsigned int)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, VertCnt);
    }

    glBindVertexArray(0);
}

void MeshObject::shutdown()
{
    if (CleanedUp)
        return;

	glBindVertexArray(VAO);
    for (GLuint i=0; i<EnabledArrays; ++i)
        glDisableVertexAttribArray(i);
	glBindVertexArray(0);

    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &IBO);
    glDeleteBuffers(1, &IBO);
    glDeleteVertexArrays(1, &VAO);
    CleanedUp = true;

//    delete[] VertArray;
//    VertArray = 0;
}

void MeshObject::setMesh(const MeshBuffer& meshBuffer)
{
    VertCnt = meshBuffer.getVertCnt();
    computeBoundingBox(meshBuffer);

    unsigned int vertArrayCnt = VertCnt;
    int VertComponentCount = sizeof(glm::vec3) / sizeof(glm::vec3::value_type);
    int NormalComponentCount = sizeof(glm::vec3) / sizeof(glm::vec3::value_type);
    int UVComponentCount = sizeof(glm::vec2) / sizeof(glm::vec2::value_type);
    Stride = VertComponentCount; // will always have positions
    NormOffset = 0;
    UvOffset = 0;
    Normalidx = 0;
    UVidx = 0;
    EnabledArrays = 1; // 1 is for positions
    if (meshBuffer.UsesNormals)
    {
        Normalidx = EnabledArrays++;
        vertArrayCnt += VertCnt;
        NormOffset = Stride;
        Stride += NormalComponentCount;
    }
    if (meshBuffer.UsesUVs)
    {
        UVidx = EnabledArrays++;
        vertArrayCnt += VertCnt;
        UvOffset = Stride;
        Stride += UVComponentCount;
    }

    const float* pos = (float*)meshBuffer.getVerts().data();
    const float* norm = (float*)meshBuffer.getNorms().data();
    const float* uv = (float*)meshBuffer.getTexCoords(0).data();
    VertArray = new float[VertCnt*Stride];

    for (unsigned int i=0, idx=0, uvidx=0; i<VertCnt; i++, idx+=3, uvidx+=2)
    {
        int vi = i*Stride;
        VertArray[vi + 0] = pos[idx + 0];
        VertArray[vi + 1] = pos[idx + 1];
        VertArray[vi + 2] = pos[idx + 2];

        if (NormOffset)
        {
            int ni = i*Stride+NormOffset;
            VertArray[ni + 0] = norm[idx + 0];
            VertArray[ni + 1] = norm[idx + 1];
            VertArray[ni + 2] = norm[idx + 2];
        }

        if (UvOffset)
        {
            int ti = i*Stride+UvOffset;
            VertArray[ti + 0] = uv[uvidx + 0];
            VertArray[ti + 1] = uv[uvidx + 1];
        }
    }
    // make values go from number of componets to number of bytes
    StrideBytes = Stride * 4;
    NormOffset *= 4;
    UvOffset *= 4;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);
    glGenBuffers(1, &IBO);

    for (GLuint i=0; i<EnabledArrays; ++i)
        glEnableVertexAttribArray(i);

    //glNamedBufferData(VBO, VertCnt * Stride, (GLvoid*)vertArray, GL_STATIC_DRAW);

    // set vert buffer    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, VertCnt*StrideBytes, 0, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, VertCnt*StrideBytes, (const GLvoid*)VertArray);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, StrideBytes, bufferOffest(0));

    if (Normalidx)
        glVertexAttribPointer(Normalidx, 3, GL_FLOAT, GL_FALSE, StrideBytes, bufferOffest(NormOffset));

    if (UVidx)
        glVertexAttribPointer(UVidx, 2, GL_FLOAT, GL_FALSE, StrideBytes, bufferOffest(UvOffset));

    IndiceCnt = meshBuffer.getIdxCnt();
    if (IndiceCnt)
    {        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            IndiceCnt * sizeof(GLuint),
            (GLvoid*)meshBuffer.getIndices().data(),
            GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        IndexRangeStart = 0;
        IndexRangeEnd = IndiceCnt;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void MeshObject::updateBuffers(const MeshBuffer& meshBuffer)
{
    const float* pos = (float*)meshBuffer.getVerts().data();
    const float* norm = (float*)meshBuffer.getNorms().data();

    if (0 == VertArray)
        VertArray = new float[VertCnt * Stride];

    for (unsigned int i = 0, idx = 0, uvidx = 0; i<VertCnt; i++, idx += 3, uvidx += 2)
    {
        int vi = i*Stride;
        VertArray[vi + 0] = pos[idx + 0];
        VertArray[vi + 1] = pos[idx + 1];
        VertArray[vi + 2] = pos[idx + 2];

        if (NormOffset)
        {
            int ni = i*Stride + NormOffset;
            VertArray[ni + 0] = norm[idx + 0];
            VertArray[ni + 1] = norm[idx + 1];
            VertArray[ni + 2] = norm[idx + 2];
        }
    }

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, VertCnt*StrideBytes, 0, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, VertCnt*StrideBytes, (const GLvoid*)VertArray);
}

void MeshObject::computeBoundingBox(const MeshBuffer& meshBuffer)
{
    const std::vector<glm::vec3>& verts = meshBuffer.getVerts();
    for (int i=0; i<(int)verts.size(); ++i)
    {
        if (verts[i][0] < AABBMin[0]) AABBMin[0] = verts[i][0];
        if (verts[i][1] < AABBMin[1]) AABBMin[1] = verts[i][1];
        if (verts[i][2] < AABBMin[2]) AABBMin[2] = verts[i][2];

        if (verts[i][0] > AABBMax[0]) AABBMax[0] = verts[i][0];
        if (verts[i][1] > AABBMax[1]) AABBMax[1] = verts[i][1];
        if (verts[i][2] > AABBMax[2]) AABBMax[2] = verts[i][2];
    }

    glm::vec3 diagonal = AABBMax - AABBMin;
    PivotPoint = AABBMin + (diagonal * 0.5f);
}


