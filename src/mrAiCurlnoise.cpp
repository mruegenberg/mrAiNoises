/*
MIT License

Copyright (c) 2016 Marcel Ruegenberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <ai.h>
#include <string.h>
#include <cstdio>
#include "Simplex.h"

enum CurlParams {
    p_space,
    p_scale,
    p_octaves, // = turbulence
    p_lacunarity,
    p_gain, // = roughness
};

#define TRUE 1
#define FALSE 0


AI_SHADER_NODE_EXPORT_METHODS(CurlnoiseMethods);

enum CurlnoiseSpaceEnum
{
	NS_WORLD = 0,
	NS_OBJECT,
	NS_PREF,
};

static const char* curlSpaceNames[] =
{
	"world",
	"object",
	"Pref",
	NULL
};

node_parameters
{
    AiParameterEnum("space", 1, curlSpaceNames);
    AiParameterVec("scale", 1.0f, 1.0f, 1.0f);
    AiParameterInt("octaves", 1);
    AiParameterFlt("lacunarity", 1.92f);
    AiParameterFlt("gain", 0.5f); 
}

struct ShaderData {
    int space;
    int octaves;
};
        

node_initialize
{
    ShaderData *data = new ShaderData;
    AiNodeSetLocalData(node, data);
}

node_update
{
    ShaderData *data = (ShaderData *)AiNodeGetLocalData(node);
    data->space = params[p_space].INT;
    data->octaves = params[p_octaves].INT;
}

node_finish
{
    ShaderData *data = (ShaderData *)AiNodeGetLocalData(node);
    delete data;
}

node_loader
{
   if (i > 0) return FALSE;
   
   node->methods      = CurlnoiseMethods;
   node->output_type  = AI_TYPE_RGB;
   node->name         = "mrAiCurlnoise";
   node->node_type    = AI_NODE_SHADER;
   strcpy(node->version, AI_VERSION);
   return TRUE;
}


shader_evaluate
{
    const AtParamValue *params = AiNodeGetParams(node);
    ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

    AtPoint scale    = AiShaderEvalParamVec(p_scale);
    float lacunarity = AiShaderEvalParamFlt(p_lacunarity);
    float gain       = AiShaderEvalParamFlt(p_gain);
      
    AtPoint P;
    // space transform
    {
        switch (data->space) {
        case NS_OBJECT: P = sg->Po; break;
        case NS_PREF: if (!AiUDataGetPnt("Pref", &P)) P = sg->Po; break;
        default: P = sg->P; break; // NS_WORLD
        }
    }

    // scaling
    P *= scale;

    glm::vec3 p1(P.x, P.y, P.z);
    glm::vec3 noise = Simplex::curlNoise(p1, data->octaves, lacunarity, gain);

    AtRGB result; result.r = noise.x; result.g = noise.y; result.b = noise.z;
    sg->out.RGB = result;
}

