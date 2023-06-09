#ifndef GLSTATE_HPP
#define GLSTATE_HPP

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "gl_core_3_3.h"
#include "mesh.hpp"
#include "light.hpp"
#include "texture.hpp"

// Manages OpenGL state, e.g. camera transform, objects, shaders
class GLState {
public:
	GLState();
	~GLState();
	// Disallow copy, move, & assignment
	GLState(const GLState& other) = delete;
	GLState& operator=(const GLState& other) = delete;
	GLState(GLState&& other) = delete;
	GLState& operator=(GLState&& other) = delete;

	// Callbacks
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

	// Enums
	enum ShadingMode {
		SHADINGMODE_NORMALS = 0,	// View normals as colors
		SHADINGMODE_CEL = 1,		// Use Cel shading and illumination
		SHADINGMODE_PHONG = 2,
		SHADINGMODE_NONE = 3
	};
	enum NormalsMode {
		NORMALSMODE_INTERPOLATE = 0,
		NORMALSMODE_FACE = 1,
	};
	enum TintMode {
		TINTMODE_SSS = 0,
		TINTMODE_CONST = 1,
	};
	enum OcclusionMode {
		OCCLUSION_ON = 0,
		OCCLUSION_OFF = 1,
	};
	enum SpecularMode {
		SPECULAR_ON = 0,
		SPECULAR_OFF = 1,
	};
	enum TextureMode {
		TEXTUREMODE_TEX = 0,
		TEXTUREMODE_CONST = 1,
	};
	enum ContourMode {
		CONTOUR_ON = 0,
		CONTOUR_OFF = 1,
	};
	enum OutlineMode {
		OUTLINE_ON = 0,      // Toggle outline
		OUTLINE_OFF = 1,     // Turn off
	};

	bool isInit() const { return init; }
	void readConfig(std::string filename);	// Read from a config file

	// Drawing modes
	ShadingMode getShadingMode() const { return shadingMode; }
	NormalsMode getNormalsMode() const { return normalsMode; }
	TintMode getTintMode() const { return tintMode; }
	OcclusionMode getOcclusionMode() const { return occlusionMode; }
	SpecularMode getSpecularMode() const { return specularMode; }
	TextureMode getTextureMode() const { return textureMode; }
	ContourMode getContourMode() const { return contourMode; }
	OutlineMode getOutlineMode() const { return outlineMode; }
	void setShadingMode(ShadingMode sm);
	void setNormalsMode(NormalsMode nm);
	void setTintMode(TintMode tm);
	void setOcclusionMode(OcclusionMode om);
	void setSpecularMode(SpecularMode om);
	void setTextureMode(TextureMode tm);
	void setContourMode(ContourMode tm);
	void setOutlineMode(OutlineMode om);

	// Object properties
	float getAmbientStrength() const;
	float getDiffuseStrength() const;
	float getSpecularStrength() const;
	float getSpecularExponent() const;
	glm::vec3 getObjectColor() const;
	void setAmbientStrength(float ambStr);
	void setDiffuseStrength(float diffStr);
	void setSpecularStrength(float specStr);
	void setSpecularExponent(float specExp);
	void setObjectColor(glm::vec3 color);

	void setMaterialAttrs(
		glm::vec3 floorColor, glm::vec3 modelColor,
		float floorAmbStr, float floorDiffStr, float floorSpecStr, float floorSpecExp,
		float modelAmbStr, float modelDiffStr, float modelSpecStr, float modelSpecExp
	);

	// Set the currently active model (controlled by keyboard)
	inline void setActiveObj(const int objIndex) { activeObj = objIndex; }
	inline int getActiveObj() const { return activeObj; }

	// Mesh & Light access
	unsigned int getNumLights() const { return (unsigned int)lights.size(); }
	Light& getLight(int index) { return lights.at(index); }
	const Light& getLight(int index) const { return lights[index]; }
	// Get the list of objects
	inline std::vector<std::shared_ptr<Mesh>>& getObjects() { return objects; }

	// Camera control
	bool isCamRotating() const { return camRotating; }
	void beginCameraRotate(glm::vec2 mousePos);
	void endCameraRotate();
	void rotateCamera(glm::vec2 mousePos);
	bool isCamTranslating() const { return camTranslating; }
	void beginCameraTranslate(glm::vec2 mousePos);
	void endCameraTranslate();
	void translateCamera(glm::vec2 mousePos);
	void offsetCamera(float offset);
	inline float getMoveStep() { return moveStep; }
	inline float getRotStep() { return rotStep; }
	void update_time(float time);

	// Set object to display
	void showObjFile(const std::string& filename, const unsigned int meshType, const glm::mat4& modelMat);

protected:
	bool init;  // Whether we've been initialized yet

	// Initialization
	void initShaders();

	// Calculate model matrix from rotation and translation
	static glm::mat4 calModelMat(const glm::mat3 rotMat, const glm::vec3 translation);

	// Drawing modes
	ShadingMode 	shadingMode;
	NormalsMode 	normalsMode;
	TintMode 		tintMode;
	OcclusionMode 	occlusionMode;
	SpecularMode 	specularMode;
	TextureMode 	textureMode;
	ContourMode 	contourMode;
	OutlineMode 	outlineMode;

	// Camera state
	int width, height;		// Width and height of the window
	float fovy;				// Vertical field of view in degrees
	glm::vec3 camCoords;	// Camera spherical coordinates
	glm::vec2 lookAt;		// Camera center coordinates
	bool camRotating;		// Whether camera is currently rotating
	bool camTranslating;	// Whether camera is currently translating
	glm::vec2 initCamRot;	// Initial camera rotation on click
	glm::vec2 initLookAt;	// Initial lookat translation on click
	glm::vec2 initMousePos;	// Initial mouse position on click

	// Mesh and lights
	std::vector<std::shared_ptr<Mesh>> objects;		// Pointer to mesh object
	std::vector<Light> lights;		// Lights

	unsigned int numObjects;  // Number of objects in the scene
	unsigned int activeObj = 1;   // The current active model (1-3)
	float moveStep = 0.1f;  // Translation step
	float rotStep = 3.14159265 / 24;
	float outlineFactor = 0.003f;

	// Textures
	Texture textures;

	// Shader state
	GLuint shader;			       // GPU shader program
	GLuint depthShader;	           // Depth shader program
	GLuint modelMatLoc;	           // Model-to-world matrix location, used in shader
	GLuint modelMatDepthLoc;	   // Model-to-world matrix location, used in depth shader
	GLuint lightSpaceMatLoc;       // World-to-light matrix location, used in shader
	GLuint lightSpaceMatDepthLoc;  // World-to-light matrix location, used in depth 
	GLuint objTypeLoc;             // Object type location (decide which material attributes to use and which texture to map)
	GLuint viewProjMatLoc;	       // World-to-clip matrix location
	GLuint shadingModeLoc;	       // Shading mode location
	GLuint normalsModeLoc;
	GLuint tintModeLoc;
	GLuint occlusionModeLoc;
	GLuint specularModeLoc;
	GLuint textureModeLoc;
	GLuint contourModeLoc;
	GLuint outlineModeLoc;		   // Outline mode location
	GLuint camPosLoc;		       // Camera position location
	GLuint floorColorLoc, 	modelColorLoc;		    // Object color
	GLuint floorAmbStrLoc, 	modelAmbStrLoc;		// Ambient strength location
	GLuint floorDiffStrLoc, modelDiffStrLoc;		// Diffuse strength location
	GLuint floorSpecStrLoc, modelSpecStrLoc;		// Specular strength location
	GLuint floorSpecExpLoc, modelSpecExpLoc;		// Specular exponent location
	GLuint featureToggleLoc[10];
	float cur_time;
};

#endif
