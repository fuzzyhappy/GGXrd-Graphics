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

void main() {
    isOutline = 0.0;

    gl_Position = gl_in[0].gl_Position;
    fragPos = geoPos[0];
    fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[0] : geoVNorm[0];    
    fragColor = geoColor[0];
    fragUV = geoUV[0];
    tanLightPos = tanLightPosG[0];
    tanViewer = tanViewerG[0];
    tanFragPos = tanGeoPos[0];
    lightFragPos = lightGeoPos[0];
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    fragPos = geoPos[1];
    fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[1] : geoVNorm[1];    
    fragColor = geoColor[1];
    fragUV = geoUV[1];
    tanLightPos = tanLightPosG[1];
    tanViewer = tanViewerG[1];
    tanFragPos = tanGeoPos[1];
    lightFragPos = lightGeoPos[1];
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    fragPos = geoPos[2];
    fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[2] : geoVNorm[2];    
    fragColor = geoColor[2];
    fragUV = geoUV[2];
    tanLightPos = tanLightPosG[2];
    tanViewer = tanViewerG[2];
    tanFragPos = tanGeoPos[2];
    lightFragPos = lightGeoPos[2];
    EmitVertex();

    EndPrimitive();
    
    if (objType == OBJTYPE_FLOOR) {
        return;
    }

    isOutline = 1.0;

    gl_Position = gl_in[0].gl_Position + vec4(geoVNorm[0] * outline, 0);
    fragPos = geoPos[0];
    fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[0] : geoVNorm[0];    
    fragColor = geoColor[0];
    fragUV = geoUV[0];
    tanLightPos = tanLightPosG[0];
    tanViewer = tanViewerG[0];
    tanFragPos = tanGeoPos[0];
    lightFragPos = lightGeoPos[0];
    EmitVertex();
      

    gl_Position = gl_in[1].gl_Position + vec4(geoVNorm[1] * outline, 0);
    fragPos = geoPos[1];
    fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[1] : geoVNorm[1];    
    fragColor = geoColor[1];
    fragUV = geoUV[1];
    tanLightPos = tanLightPosG[1];
    tanViewer = tanViewerG[1];
    tanFragPos = tanGeoPos[1];
    lightFragPos = lightGeoPos[1];
    EmitVertex();

    gl_Position = gl_in[2].gl_Position + vec4(geoVNorm[2] * outline, 0);
    fragPos = geoPos[2];
    fragNorm = normalsMode == NORMALSMODE_FACE ? geoFNorm[2] : geoVNorm[2];
    fragColor = geoColor[2];
    fragUV = geoUV[2];
    tanLightPos = tanLightPosG[2];
    tanViewer = tanViewerG[2];
    tanFragPos = tanGeoPos[2];
    lightFragPos = lightGeoPos[2];
    EmitVertex();

    EndPrimitive();
}  
