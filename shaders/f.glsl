#version 330

const int SHADINGMODE_NORMALS = 0;		// Show normals as colors
const int SHADINGMODE_CEL = 1;			// Cel shading + illumination

const int NORMAL_MAPPING_ON = 0;        // Turn on normal mapping
const int NORMAL_MAPPING_OFF = 1;       // Turn off

const int SHADOW_MAPPING_ON = 0;        // Turn on shadow mapping
const int SHADOW_MAPPING_OFF = 1;       // Turn off

const int LIGHTTYPE_POINT = 0;			// Point light
const int LIGHTTYPE_DIRECTIONAL = 1;	// Directional light

const int OBJTYPE_FLOOR = 0;
const int OBJTYPE_MODEL = 1;

// Textures
uniform sampler2D texModelColor; // Model color texture
uniform sampler2D texModelSss; 	 // Model tint texture
uniform sampler2D texModelNrm; 	 // Model normal texture
uniform sampler2D texModelLgt; 	 // Model inner line Texture
uniform sampler2D shadowMap;     // Shadow map

smooth in vec3 fragPos;		    // Interpolated position in world-space
smooth in vec3 fragNorm;	    // Interpolated normal in world-space
smooth in vec3 fragColor;	    // Interpolated color (for Gouraud shading)
smooth in vec2 fragUV;          // Interpolated texture coordinates
// Normal mapping
smooth in vec3 tanLightPos;     // Light position in tangent space
smooth in vec3 tanViewer;       // Viewing vector in tangent space
smooth in vec3 tanFragPos;      // Fragment position in tangent space
// Shadow mapping
smooth in vec4 lightFragPos;    // Fragment position in light space

out vec3 outCol;	         // Final pixel color

// Light information
struct LightData {
	bool enabled;	// Whether the light is on
	int type;		// Type of light (0 = point, 1 = directional)
	vec3 pos;		// World-space position/direction of light source
	vec3 color;		// Color of light
};

// Array of lights
const int MAX_LIGHTS = 1;
layout (std140) uniform LightBlock {
	LightData lights [MAX_LIGHTS];
};

uniform int shadingMode;		// Which shading mode
uniform int normalMapMode;      // Whether turn on normal mapping
uniform int shadowMapMode;      // Whether turn on shadow mapping
uniform int objType;            // 0 for floor and 1 for model
uniform vec3 camPos;			// World-space camera position

uniform vec3 floorColor;			// Object color
uniform float floorAmbStr;			// Ambient strength
uniform float floorDiffStr;			// Diffuse strength
uniform float floorSpecStr;			// Specular strength
uniform float floorSpecExp;			// Specular exponent
uniform vec3  modelColor;			    // Object color
uniform float modelAmbStr;			// Ambient strength
uniform float modelDiffStr;			// Diffuse strength
uniform float modelSpecStr;			// Specular strength
uniform float modelSpecExp;			// Specular exponent

float calculateShadow(vec4 light_frag_pos) {  // TODO: add other parameters if needed
	// Perspective divide
	vec3 projCoords = light_frag_pos.xyz / light_frag_pos.w;
	// Remap to [0.0, 1.0]
	projCoords = projCoords * 0.5 + 0.5;

	// TODO 4-3
	// Decide whether a fragment is in shadow
	// Read the depth from depth map, then compare it with the depth of the current fragment
	float closestDepth = texture(shadowMap, projCoords.xy).x;

	float depth = projCoords.z;

	// return a binary representing whether there is shadow (0.0 or 1.0)
	return depth > closestDepth ? 1.0 : 0.0;
}

void main() {
	// Decide material attributes
	vec3 objColor = floorColor;
	float ambStr = floorAmbStr, diffStr = floorDiffStr, specStr = floorSpecStr, specExp = floorSpecExp;
	if (objType == OBJTYPE_FLOOR) {
		objColor = vec3(.7, .7, .7);
		ambStr = floorAmbStr;
		diffStr = floorDiffStr;
		specStr = floorSpecStr;
		specExp = floorSpecExp;
	}
	else if (objType == OBJTYPE_MODEL) {
		objColor = vec3(1.0, 1.0, 1.0);
		vec4 lineData = texture(texModelLgt, fragUV).rgba;
		objColor *= lineData.rgb;
		objColor *= texture(texModelColor, fragUV).rgb;
		ambStr = modelAmbStr;
		diffStr = modelDiffStr;
		specStr = modelSpecStr;
		specExp = modelSpecExp;
	}

	if (shadingMode == SHADINGMODE_NORMALS) {
		if (normalMapMode == NORMAL_MAPPING_ON && objType == OBJTYPE_MODEL)
			outCol = texture(texModelNrm, fragUV).xyz;  // Get normal from normal map
		else
			outCol = normalize(fragNorm) * 0.5 + vec3(0.5);
	}
	else if (shadingMode == SHADINGMODE_CEL) {
		outCol = vec3(1.0);
		for (int i = 0; i < MAX_LIGHTS; i++) {
			if (lights[i].enabled) {
				vec3 normal;

				if (normalMapMode == NORMAL_MAPPING_ON && objType == OBJTYPE_MODEL) {
					// TODO 3-1
					// Get the fragment normal, store it in "normal" SEE LINE 111
					//normal = normalize(fragNorm);
					normal = vec3(texture(texModelNrm, fragUV));
					normal = 2 * normal;
					normal = normal - 1.0;
					normal = normalize(normal);
				}
				else
					normal = normalize(fragNorm);

				vec3 lightDir;
				if (lights[i].type == LIGHTTYPE_POINT)
					lightDir = normalize(lights[i].pos - fragPos);
				else if (lights[i].type == LIGHTTYPE_DIRECTIONAL) {
					if (normalMapMode == NORMAL_MAPPING_ON && objType == OBJTYPE_MODEL) {
						// TODO 3-2
						lightDir = normalize(tanLightPos);
						// Get vector to the light in tangent space, store it in "lightDir", SEE LINE 120
					}
					else
						lightDir = normalize(lights[i].pos);
				}

				float ambient = ambStr;

				//float diffuse = max(dot(normal, lightDir), 0.0) * diffStr;
				float diffuse = dot(normal, lightDir);

				vec3 viewDir;
				if (normalMapMode == NORMAL_MAPPING_ON && objType == OBJTYPE_MODEL) {
					// TODO 3-3
					// Get vector to the viewer in tangent space, store it in "viewDir", SEE LINE 136
				}
				else
					viewDir = normalize(camPos - fragPos);

				//vec3 reflectDir = -lightDir - 2 * dot(-lightDir, normal) * normal;
				//float specular = max(dot(viewDir, reflectDir), 0.0);
				//specular = pow(specular, specExp) * specStr;

				// TODO 4-2
				// Calculate shadow using the function calculateShadow, and modify the line (calculating the final color) below
				float shadow;
				if (shadowMapMode == SHADOW_MAPPING_ON && objType == OBJTYPE_FLOOR)
					shadow = calculateShadow(lightFragPos);        // TODO: add more parameters if necessary
				else if (shadowMapMode == SHADOW_MAPPING_OFF)
					shadow = 0.0;

				float threshhold = 0;
				if (diffuse < threshhold) {
					outCol *= texture(texModelSss, fragUV).rgb;
				}
			}
		}
		outCol *= objColor;
	}
}
