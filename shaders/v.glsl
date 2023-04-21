#version 330

const int NORMALMODE_FACE = 0;			// Flat normals
const int NORMALMODE_SMOOTH = 1;		// Smooth normals

const int LIGHTTYPE_POINT = 0;
const int LIGHTTYPE_DIRECTIONAL = 1;

const int OBJTYPE_FLOOR = 0;
const int OBJTYPE_CUBE = 1;

layout(location = 0) in vec3 pos;			// Model-space position
layout(location = 1) in vec3 fnorm;		    // Model-space face normal
layout(location = 2) in vec3 vnorm;		    // Model-space face normal
layout(location = 3) in vec2 uv;	        // Texture coordinates
layout(location = 4) in vec3 tangent;	    // Right vector in tangent space
layout(location = 5) in vec3 bitangent;	    // Forward vector in tangent space

smooth out vec3 geoPos;	    // Interpolated position in world-space
smooth out vec3 geoFNorm;	    // Interpolated normal in world-space
smooth out vec3 geoVNorm;	    // Interpolated normal in world-space
smooth out vec3 geoColor;	    // Interpolated color (for Gouraud shading)
smooth out vec2 geoUV;         // Interpolated texture coordinates
smooth out vec3 tanLightPosG;    // Light position in tangent space
smooth out vec3 tanViewerG;      // Viewing vector in tangent space
smooth out vec3 tanGeoPos;     // Geoment position in tangent space
smooth out vec4 lightGeoPos;   // Geoment position in light space

// Light information
struct LightData {
	bool enabled;
	int type;
	vec3 pos;
	vec3 color;
};

// Array of lights
const int MAX_LIGHTS = 1;
layout (std140) uniform LightBlock {
	LightData lights [MAX_LIGHTS];
};

uniform mat4 modelMat;		 // Model-to-world transform matrix
uniform mat4 lightSpaceMat;  // World-to-light matrix location
uniform int objType;         // 0 for floor and 1 for cube
uniform mat4 viewProjMat;	 // World-to-clip transform matrix
uniform int shadingMode;     // Cel vs. colored normals
uniform vec3 camPos;         // Camera position

uniform vec3 floorColor;
uniform float floorAmbStr;
uniform float floorDiffStr;
uniform float floorSpecStr;
uniform float floorSpecExp;
uniform vec3 cubeColor;
uniform float cubeAmbStr;
uniform float cubeDiffStr;
uniform float cubeSpecStr;
uniform float cubeSpecExp;

uniform float outline;

void main() {
	// Get world-space position and normal
	geoPos = vec3(modelMat * vec4(pos, 1.0));
	geoFNorm = vec3(modelMat * vec4(fnorm, 0.0));
	geoVNorm = normalize(vec3(modelMat * vec4(vnorm, 0.0)));

	// Get light-space position, pass to geoment shader
	lightGeoPos = lightSpaceMat * vec4(geoPos, 1.0);

	// TODO 2-1 TBN system construction

	// TODO 2-2 Convert lighting related parameters based on TBN system
	//          Hint: "lights[0].pos", "camPos", and "geoPos" to tangent space: light position ("tanLightPosG"), camera position ("tanViewerG") and geoment position ("tanGeoPos") to tangent space
	vec3 n = normalize(fnorm);
	vec3 t = normalize(tangent);
	vec3 b = normalize(bitangent);
	mat3 TBN = mat3(t, b, n);
	TBN = transpose(TBN);

	tanLightPosG = TBN * lights[0].pos;
	tanViewerG = TBN * camPos;
	tanGeoPos = TBN * geoPos;

	// Pass the interpolated texture coordinates to the geometry shader
	geoUV = uv;

	// Output clip-space position
	gl_Position = viewProjMat * vec4(geoPos, 1.0);
}
