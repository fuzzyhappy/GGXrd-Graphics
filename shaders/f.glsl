#version 330

const int SHADINGMODE_NORMALS = 0;		// Show normals as colors
const int SHADINGMODE_CEL = 1;			// Cel shading + illumination
const int SHADINGMODE_PHONG = 2;

const int LIGHTTYPE_POINT = 0;			// Point light
const int LIGHTTYPE_DIRECTIONAL = 1;	// Directional light

const int SSS_SHADING = 0;
const int CONST_SHADING = 1;

const int OBJTYPE_FLOOR = 0;
const int OBJTYPE_MODEL = 1;

// Textures
uniform sampler2D texModelColor; // Model color texture
uniform sampler2D texModelSss; 	 // Model tint texture
uniform sampler2D texModelNrm; 	 // Model normal texture
uniform sampler2D texModelIlm; 	 // Special Texture
uniform sampler2D shadowMap;     // Shadow map

smooth in vec3 fragPos;		    // Interpolated position in world-space
smooth in vec3 fragNorm;	    // Interpolated normal in world-space
smooth in vec3 fragColor;	    // Interpolated color (for Gouraud shading)
smooth in vec2 fragUV;          // Interpolated texture coordinates
smooth in vec4 lightFragPos;    // Fragment position in light space
smooth in float isOutline;   

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

vec3 renderFloor() {
	vec3 objColor = vec3(.7, .7, .7);
	float ambStr, diffStr, specStr, specExp;
	ambStr = floorAmbStr;
	diffStr = floorDiffStr;
	specStr = floorSpecStr;
	specExp = floorSpecExp;
	float shadow = calculateShadow(lightFragPos);        // TODO: add more parameters if necessary
	objColor -= 0.2 * shadow;
	return objColor;
}

vec3 renderMesh() {
	return vec3(1.0);
}

void main() {
	// Decide material attributes
	vec3 objColor = floorColor;
	float ambStr = floorAmbStr, diffStr = floorDiffStr, specStr = floorSpecStr, specExp = floorSpecExp;
	if (shadingMode == SHADINGMODE_NORMALS) {
		outCol = normalize(fragNorm) * 0.5 + vec3(0.5);
		return;
	}
	if (isOutline != 0.0) {
		outCol = vec3(0.0);
		return;
	}
	else if (objType == OBJTYPE_FLOOR) {
		outCol = renderFloor();
		return;
	}
	else if (objType == OBJTYPE_MODEL) {
		objColor = vec3(1.0, 1.0, 1.0);
		objColor *= texture(texModelIlm, fragUV).a;
		objColor *= texture(texModelColor, fragUV).rgb;
		ambStr = modelAmbStr;
		diffStr = modelDiffStr;
		specStr = modelSpecStr;
		specExp = modelSpecExp;
	}

	
	outCol = vec3(1.0);
	for (int i = 0; i < MAX_LIGHTS; i++) {
		if (lights[i].enabled) {
			vec3 normal = normalize(fragNorm);

			vec3 lightDir;
			if (lights[i].type == LIGHTTYPE_POINT)
				lightDir = normalize(lights[i].pos - fragPos);
			else if (lights[i].type == LIGHTTYPE_DIRECTIONAL) {
				lightDir = normalize(lights[i].pos);
			}

			vec3 viewDir = normalize(camPos - fragPos);

			if (shadingMode == SHADINGMODE_CEL) {
				vec4 ilm = texture(texModelIlm, fragUV);

				float diffuse = dot(normal, lightDir);
				vec3 reflectDir = -lightDir - 2 * dot(-lightDir, normal) * normal;
				float specular = dot(viewDir, reflectDir);

				float diffuseThreshhold = 1-ilm.g;
				float specularThreshhold = 1-ilm.b;
				
				if (diffuse <= diffuseThreshhold) {
					outCol *= texture(texModelSss, fragUV).rgb;
				}
				if (specular >= specularThreshhold) {
					outCol += 0.2*ilm.r;
				}
			} 

			else {
				float ambient = ambStr;
				float diffuse = max(dot(normal, lightDir), 0.0) * diffStr;
				vec3 reflectDir = -lightDir - 2 * dot(-lightDir, normal) * normal;
				float specular = max(dot(viewDir, reflectDir), 0.0);
				specular = pow(dot(viewDir, reflectDir), specExp) * specStr;
				outCol += (ambient + diffuse + specular) * lights[i].color;
			}
		}
	}
	outCol *= objColor;
	
}
