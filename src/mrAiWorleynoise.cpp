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
    DIST_NORMALIZED,
};

static const char* distmodeNames[] =
{
	"F1",
	"F2-F1",
	"(2 f1 + f2) / 3",
        "(2 f3 - f2 - f1) / 2",
        "F1/2 + F2/3 + f3/6",
        "normalized",
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
}

// stores static parameters
struct ShaderData {
    int space;
    int octaves;
    float distMeasure;
    int distMode;
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
    bool isBorder = false;
    {
#define PT_CNT 5
        float F[PT_CNT]; // distances to feature points
        AtVector delta[PT_CNT]; // vector difference between input point and n-th closest feature point
                                // => feature point n is located at P-delta[n]
        float lacunarity = AiShaderEvalParamFlt(p_lacunarity);

        // AiCellular(P, 1, data->octaves, lacunarity, 1.0, F, delta, NULL);
        // AtVector closest = (P - delta[0]);
        
        AiCellular(P, PT_CNT, data->octaves, lacunarity, 1.0, F, delta, NULL);

        float p = data->distMeasure;
        if(p != 2.0f)
        {
            // recompute distances based on metric
            for(int i=0; i<PT_CNT; ++i) {
                F[i] = pow(pow(fabs(delta[i].x), p) +
                           pow(fabs(delta[i].y), p) +
                           pow(fabs(delta[i].z), p),
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

        float gapSize = AiShaderEvalParamFlt(p_gapSize);
        
        // normalized distance. needed for normalized dist measure and gap computation
        float normalizedDist = 0.0f;
        if(gapSize > 0 || data->distMode == DIST_NORMALIZED) {
            normalizedDist = AiV3Dot(0.5 * (delta[0] + delta[1]), // difference between shading point and halfway point (which is on shortest line between closest and 2nd closest pt)
                                     AiV3Normalize(delta[1] - delta[0]) // from closest to 2nd closest pt
                );

            // check that we calculate relative dist for closest point
            // this is still not really perfect.
            for(size_t i = 2; i < PT_CNT; ++i) {
                float d = AiV3Dot(0.5 * (delta[0] + delta[i]),
                                  AiV3Normalize(delta[i] - delta[0])
                    );
                normalizedDist = AiMin(normalizedDist, d);
            }
            
            // TODO: find a way to get a function that always goes from 0 to 1
            //       until the border
        }
        
        if(data->distMode == DIST_NORMALIZED) {
            r = normalizedDist;
        }
        else {
            // weighted sum
            for(int i=0; i<PT_CNT; ++i) {
                r += F[i] * fw[i];
            }
        }

        if(gapSize > 0 && 1.0 - AiSmoothStep(0.0f, gapSize, normalizedDist) > 0) {
            isBorder = true;
        }
    }

    if(isBorder) {
        // scaling for even-sized gaps. Originally an idea from Advaced Renderman section 10.5 on cell noise            
        // see also https://thebookofshaders.com/12/
        // and most importantly http://www.iquilezles.org/www/articles/voronoilines/voronoilines.htm
        
        // as soon as we have a normalized distance for each cell, we can just smoothstep that.
        AtRGB gapColor = AiShaderEvalParamRGB(p_gapColor);
        sg->out.RGB() = gapColor;
    }
    else {
        AtRGB innerColor = AiShaderEvalParamRGB(p_innerColor);
        AtRGB outerColor = AiShaderEvalParamRGB(p_outerColor);
        sg->out.RGB() = AiLerp(r, outerColor, innerColor);
    }
}

