#version 330

layout(location = 0) in vec3 pos;			// Model-space position
layout(location = 1) in vec3 fnorm;		    // Model-space face normal
layout(location = 2) in vec3 vnorm;		    // Model-space face normal
layout(location = 3) in vec2 uv;	        // Texture coordinates

smooth out vec3 geoPos;	    // Interpolated position in world-space
smooth out vec3 geoFNorm;	    // Interpolated normal in world-space
smooth out vec3 geoVNorm;	    // Interpolated normal in world-space
smooth out vec2 geoUV;         // Interpolated texture coordinates
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

void main() {
	// Get world-space position and normal
	geoPos = vec3(modelMat * vec4(pos, 1.0));
	geoFNorm = vec3(modelMat * vec4(fnorm, 0.0));
	geoVNorm = normalize(vec3(modelMat * vec4(vnorm, 0.0)));

	// Get light-space position, pass to geoment shader
	lightGeoPos = lightSpaceMat * vec4(geoPos, 1.0);

	// Pass the interpolated texture coordinates to the geometry shader
	geoUV = uv;

	// Output clip-space position
	gl_Position = viewProjMat * vec4(geoPos, 1.0);
}
