#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 6) out;

const int OBJTYPE_FLOOR = 0;
const int OBJTYPE_MODEL = 1;

const int NORMALSMODE_INTERPOLATE = 0;
const int NORMALSMODE_FACE = 1;

uniform int objType;            // 0 for floor and 1 for model
uniform int normalsMode;

smooth in vec3 geoPos[];	    // Interpolated position in world-space
smooth in vec3 geoFNorm[];	    // Interpolated normal in world-space
smooth in vec3 geoVNorm[];	    // Interpolated normal in world-space
smooth in vec3 geoColor[];	    // Interpolated color (for Gouraud shading)
smooth in vec2 geoUV[];         // Interpolated texture coordinates
smooth in vec3 tanLightPosG[];    // Light position in tangent space
smooth in vec3 tanViewerG[];      // Viewing vector in tangent space
smooth in vec3 tanGeoPos[];     // Geoment position in tangent space
smooth in vec4 lightGeoPos[];   // Geoment position in light space

smooth out vec3 fragPos;		    // Interpolated position in world-space
smooth out vec3 fragNorm;	    // Interpolated normal in world-space
smooth out vec3 fragColor;	    // Interpolated color (for Gouraud shading)
smooth out vec2 fragUV;          // Interpolated texture coordinates
smooth out vec3 tanLightPos;     // Light position in tangent space
smooth out vec3 tanViewer;       // Viewing vector in tangent space
smooth out vec3 tanFragPos;      // Fragment position in tangent space
smooth out vec4 lightFragPos;    // Fragment position in light space
out float isOutline;

uniform float outline;
uniform mat4 viewProjMat;

void main() {
    
    isOutline = 0.0;
    for (int i = 0; i < 3; i++) {
        gl_Position = gl_in[i].gl_Position;
        fragPos = geoPos[i];
        fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[i] : geoVNorm[i];
        fragColor = geoColor[i];
        fragUV = geoUV[i];
        tanLightPos = tanLightPosG[i];
        tanViewer = tanViewerG[i];
        tanFragPos = tanGeoPos[i];
        lightFragPos = lightGeoPos[i];
        EmitVertex();
    }
    EndPrimitive();
    
    if (objType == OBJTYPE_FLOOR || outline == 0.0) {
        return;
    }

    isOutline = 1.0;
    vec4 viewNorm;
    int ord[] = {0, 2, 1};
    for (int i = 0; i < 3; i++) {
        viewNorm = viewProjMat * vec4(geoVNorm[ord[i]], 0.0) * outline;
        gl_Position = gl_in[ord[i]].gl_Position + viewNorm;
        fragPos = geoPos[ord[i]] + geoVNorm[ord[i]] * outline;
        fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[ord[i]] : geoVNorm[ord[i]];
        fragColor = geoColor[ord[i]];
        fragUV = geoUV[ord[i]];
        tanLightPos = tanLightPosG[ord[i]];
        tanViewer = tanViewerG[ord[i]];
        tanFragPos = tanGeoPos[ord[i]];
        lightFragPos = lightGeoPos[ord[i]];
        EmitVertex();
    }

    EndPrimitive();
}  
