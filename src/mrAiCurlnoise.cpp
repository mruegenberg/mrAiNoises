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

enum CurlParams {
    p_space,
    p_scale,
    p_octaves, // = turbulence
    p_lacunarity,
    p_time,
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
    AiParameterFlt("time", 0.0f);
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

node_finish
{
    ShaderData *data = (ShaderData *)AiNodeGetLocalData(node);
    delete data;
}

node_update
{
    ShaderData *data = (ShaderData *)AiNodeGetLocalData(node);
    data->space   = AiNodeGetInt(node, "space");
    data->octaves = AiNodeGetInt(node, "octaves");
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
    ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

    AtVector scale   = AiShaderEvalParamVec(p_scale);
    float lacunarity = AiShaderEvalParamFlt(p_lacunarity);
    // float gain       = AiShaderEvalParamFlt(p_gain);
    float time       = AiShaderEvalParamFlt(p_time);
      
    AtVector P;
    // space transform
    static const AtString pref("Pref");
    {
        switch (data->space) {
        case NS_OBJECT: P = sg->Po; break;
        case NS_PREF: if (!AiUDataGetVec(pref, P)) P = sg->Po; break;
        default: P = sg->P; break; // NS_WORLD
        }
    }

    // scaling
    P *= scale;

    // finite difference curl, as seen in Bridson's curl noise paper
    // psi_1 = x, psi_2: y, psi_3: z of the vector noise
    // d psi_x/d y: finite difference of psi_x along direction y
    // so d psi_3 / d y = valYNxt.z - valYPre.z
    
    float delta = 1e-4f;
    AtVector valXPre = AiVNoise4(P + AtVector(-delta, 0, 0), time, data->octaves, 0, lacunarity);
    AtVector valXNxt = AiVNoise4(P + AtVector( delta, 0, 0), time, data->octaves, 0, lacunarity);
    AtVector valYPre = AiVNoise4(P + AtVector(0, -delta, 0), time, data->octaves, 0, lacunarity);
    AtVector valYNxt = AiVNoise4(P + AtVector(0,  delta, 0), time, data->octaves, 0, lacunarity);
    AtVector valZPre = AiVNoise4(P + AtVector(0, 0, -delta), time, data->octaves, 0, lacunarity);
    AtVector valZNxt = AiVNoise4(P + AtVector(0, 0,  delta), time, data->octaves, 0, lacunarity);

    // as described in Bridson's paper section 2.1, see the comments above
    AtRGB result = AtRGB((valYNxt.z - valYPre.z) - (valZNxt.y - valZPre.y),
                         (valZNxt.x - valZPre.x) - (valXNxt.z - valXPre.z),
                         (valXNxt.y - valXPre.y) - (valYNxt.x - valYPre.x));
    result /= delta;
    
    sg->out.RGB() = result;
}

