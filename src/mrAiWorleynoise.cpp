/*
MIT License

Copyright (c) 2016 Marcel Ruegenberg and Filmakademie Baden-Wuerttemberg, Institute of Animation

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

enum SolidtexParams {
    p_space,
    p_scale,
    p_innerColor,
    p_outerColor,
    p_gapColor,
    p_octaves,
    p_lacunarity,
    p_distanceMeasure,
    p_distanceMode,
    p_gapSize,
    p_jaggedGap,
};

#define TRUE 1
#define FALSE 0


AI_SHADER_NODE_EXPORT_METHODS(WorleynoiseMethods);

enum WorleynoiseSpaceEnum
{
	NS_WORLD = 0,
	NS_OBJECT,
	NS_PREF,
};

static const char* worleySpaceNames[] =
{
	"world",
	"object",
	"Pref",
	NULL
};

enum DistmodeEnum
{
    DIST_F1 = 0, // f1
    DIST_F2_M_F1, // f2 - f1
    DIST_F1_P_F3, // (2 * f1 + f2) / 3
    DIST_F3_M_F2_M_F1, // (2 * f3 - f2 - f1) / 2
    DIST_F1_P_F2_P_F3, // (0.5 * f1 + 0.33 * f2 + (1 - 0.5 - 0.33) * f3)
};

static const char* distmodeNames[] =
{
	"F1",
	"F2-F1",
	"(2 f1 + f2) / 3",
        "(2 f3 - f2 - f1) / 2",
        "F1/2 + F2/3 + f3/6",
	NULL
};

node_parameters
{
    AiParameterEnum("space", 0, worleySpaceNames);
    AiParameterVec("scale", 1.0f, 1.0f, 1.0f);
    AiParameterRGB("innercolor", 0.0f, 0.0f, 0.0f);
    AiParameterRGB("outercolor", 1.0f, 1.0f, 1.0f);
    AiParameterRGB("gapcolor", 0.0f, 0.0f, 0.0f);
    AiParameterInt("octaves", 1);
    AiParameterFlt("lacunarity", 1.92f);
    AiParameterFlt("distancemeasure", 2.0f); // Minkowski distance measure
    AiParameterEnum("distancemode", 0, distmodeNames);
    AiParameterFlt("gapsize", 0.05f);
    AiParameterBool("jaggedgap", FALSE);
}

// stores static parameters
struct ShaderData {
    int space;
    int octaves;
    float distMeasure;
    int distMode;
    bool jaggedGap;
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
    
    // in node_update, we have to use AiNodeGet* instead of AuShaderEvalParam
    data->space = AiNodeGetInt(node, "space");
    data->octaves = AiNodeGetInt(node, "octaves");
    data->distMeasure = AiNodeGetFlt(node, "distancemeasure");
    data->distMode = AiNodeGetInt(node, "distancemode");
    data->jaggedGap = AiNodeGetBool(node, "jaggedgap");
}

node_loader
{
   if (i > 0) return FALSE;
   
   node->methods      = WorleynoiseMethods;
   node->output_type  = AI_TYPE_RGB;
   node->name         = "mrAiWorleynoise";
   node->node_type    = AI_NODE_SHADER;
   strcpy(node->version, AI_VERSION);
   return TRUE;
}


shader_evaluate
{
    ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
      
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
    AtVector scale = AiShaderEvalParamVec(p_scale);
    P *= scale;

    // eval distances
    float r = 0.0; // sic!
    {
#define PT_CNT 3
        float F[PT_CNT]; // distances to feature points
        AtVector delta[PT_CNT]; // vector difference between input point and n-th closest feature point
                                // => feature point n is located at P-delta[n]
        float lacunarity = AiShaderEvalParamFlt(p_lacunarity);
        AiCellular(P, PT_CNT, data->octaves, lacunarity, 1.0, F, delta, NULL);

        // FIXME: does this work as intended?
        float p = data->distMeasure;
        if(p != 2.0f) {
            // recompute distances based on metric
            for(int i=0; i<PT_CNT; ++i) {
                F[i] = pow(pow(abs(delta[i].x), p) +
                           pow(abs(delta[i].y), p) +
                           pow(abs(delta[i].z), p),
                           1.0f / p);
            }
        }
        
        float fw[PT_CNT];
        switch (data->distMode) {
        case DIST_F1: fw[0] = 1.0; fw[1] = 0.0; fw[2] = 0.0; break;
        case DIST_F2_M_F1: fw[0] = -1.0; fw[1] = 1.0; fw[2] = 0.0; break;
        case DIST_F1_P_F3: fw[0] = 2.0/3.0; fw[1] = 1.0/3.0; fw[2] = 0.0; break;
        case DIST_F3_M_F2_M_F1: fw[0] = -1.0/2.0; fw[1] = -1.0 / 2.0; fw[2] = 2.0 / 2.0; break;
        case DIST_F1_P_F2_P_F3: fw[0] = 0.5; fw[1] = 0.33; fw[2] = 1.0 - 0.5 - 0.33; break;
        default: break;
        }

        // weighted sum
        for(int i=0; i<PT_CNT; ++i) {
            r += F[i] * fw[i];
        }

        float gapSize = AiShaderEvalParamFlt(p_gapSize);
        if(gapSize > 0) {
            // scaling for even-sized gaps. Original idea from Advaced Renderman section 10.5 on cell noise
            // see also https://thebookofshaders.com/12/
            // and http://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm
            AtVector diff = (P - delta[1]) - (P - delta[0]); // difference between 2 nearest hit points
            
            // jagging
            if(data->jaggedGap) {
                diff += AiVNoise3(P * 3, 1, 0, 1) * 5;
            }

            // length of diff in chosen distance metric
            // for p=2 equivalent to Euclidean length of diff
            float diffLen = pow((pow(abs(diff.x), p) + pow(abs(diff.y), p) + pow(abs(diff.z),p)), 1.0f / p);

            // F[1] - F[0]: 0 exactly at the dividing line
            // F[0] + F[1]: shortest distance between hit points
            // gapScaleFactor: relative amount we are away from minimum distance
            //                 will usually be > 1
            float gapScaleFactor = diffLen / (F[0] + F[1]);
        
            // gap
            if(gapSize * gapScaleFactor > F[1] - F[0]) {
                r *= (-1.0);
            }
        }
    }

    if(r < 0) {
        AtRGB gapColor = AiShaderEvalParamRGB(p_gapColor);
        sg->out.RGB() = gapColor;
    }
    else {
        AtRGB innerColor = AiShaderEvalParamRGB(p_innerColor);
        AtRGB outerColor = AiShaderEvalParamRGB(p_outerColor);
        sg->out.RGB() = AiLerp(r, outerColor, innerColor);
    }
}

