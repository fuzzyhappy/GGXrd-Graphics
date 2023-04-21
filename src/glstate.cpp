#define NOMINMAX
#include <fstream>
#include <sstream>
#include <iostream>
#include "glstate.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "util.hpp"
#include "mesh.hpp"

// Constructor
GLState::GLState() :
	shadingMode(SHADINGMODE_CEL),
	normalsMode(NORMALSMODE_INTERPOLATE),
	tintMode(TINTMODE_SSS),
	occlusionMode(OCCLUSION_ON),
	specularMode(SPECULAR_ON),
	textureMode(TEXTUREMODE_TEX),
	contourMode(CONTOUR_ON),
	outlineMode(OUTLINE_ON),
	width(1), height(1),
	fovy(45.0f),
	camCoords(0.0f, 1.0f, 4.5f),
	lookAt(0.0f, -1.0f),
	camRotating(false),
	shader(0),
	depthShader(0),
	modelMatLoc(0),
	modelMatDepthLoc(0),
	lightSpaceMatLoc(0),
	lightSpaceMatDepthLoc(0),
	objTypeLoc(0),
	viewProjMatLoc(0),
	shadingModeLoc(0),
	outlineModeLoc(0),
	camPosLoc(0),
	floorColorLoc(0),
	floorAmbStrLoc(0),
	floorDiffStrLoc(0),
	floorSpecStrLoc(0),
	floorSpecExpLoc(0),
	modelColorLoc(0),
	modelAmbStrLoc(0),
	modelDiffStrLoc(0),
	modelSpecStrLoc(0),
	modelSpecExpLoc(0),
	featureToggleLoc()
	{}

// Destructor
GLState::~GLState() {
	// Release OpenGL resources
	if (shader)	glDeleteProgram(shader);
	if (depthShader) glDeleteProgram(depthShader);
}

// Called when OpenGL context is created (some time after construction)
void GLState::initializeGL() {
	// General settings
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Initialize OpenGL state
	initShaders();

	// Set drawing state
	setShadingMode(SHADINGMODE_CEL);
	setOutlineMode(OUTLINE_ON);

	// Create lights
	lights.resize(Light::MAX_LIGHTS);

	// Set initialized state
	init = true;

	// Initialize textures
	textures.load();
	textures.prepareDepthMap();
}

// Called when window requests a screen redraw
void GLState::paintGL() {
	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// ========== Begin the first render pass to generate the depth map ==========
	glUseProgram(depthShader);

	// Render the scene from the light's perspective
	// TODO 4-1.
	// Calculate the matrix "lightSpaceMat" to do world-to-light space transform
	// This matrix contains a projection and a view matrix from the light "lights[0]"
	// Remember the light is directional so its projection should be an orthographic projection

	glm::mat4 lightSpaceMat = glm::mat4(1.0f);
	glm::mat4 lightView = glm::lookAt(lights[0].getPos(), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
	lightSpaceMat = lightProj * lightView;

	// Pass the transform matrix to the depth shader
	glUniformMatrix4fv(lightSpaceMatDepthLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMat));

	// Prepare before rendering
	int shadowWidth, shadowHeight;
	textures.getShadowWidthHeight(shadowWidth, shadowHeight);
	glViewport(0, 0, shadowWidth, shadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, textures.getdepthMapFBO());
	glClear(GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_FRONT);  // Fix peter panning
	for (auto& objPtr : objects) {
		// Pass the model matrix to the depth shader
		glm::mat4 modelMat = objPtr->getModelMat();
		glUniformMatrix4fv(modelMatDepthLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

		// Draw the mesh
		objPtr->draw();
	}
	glCullFace(GL_BACK);  // Reset
	glFrontFace(GL_CCW);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	// ========== Begin the second render pass ===================================
	glViewport(0, 0, width, height);  // Reset the viewport
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader);

	// Activate textures and pass them to the shader
	textures.activeTextures();
	textures.activeDepthMap();
	GLuint texUnitLoc0, texUnitLoc1, texUnitLoc2, texUnitLoc3, texUnitLoc4;
	texUnitLoc0 = glGetUniformLocation(shader, "texModelColor");
	texUnitLoc1 = glGetUniformLocation(shader, "texModelSss");
	texUnitLoc2 = glGetUniformLocation(shader, "texModelNrm");
	texUnitLoc3 = glGetUniformLocation(shader, "texModelIlm");
	texUnitLoc4 = glGetUniformLocation(shader, "shadowMap");
	glUniform1i(texUnitLoc0, 0);
	glUniform1i(texUnitLoc1, 1);
	glUniform1i(texUnitLoc2, 2);
	glUniform1i(texUnitLoc3, 3);
	glUniform1i(texUnitLoc4, 4);

	// Pass the transform matrix to the shader
	glUniformMatrix4fv(lightSpaceMatLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMat));

	// Construct a transformation matrix for the camera
	glm::mat4 viewProjMat(1.0f);
	// Perspective projection
	float aspect = (float)width / (float)height;
	glm::mat4 proj = glm::perspective(glm::radians(fovy), aspect, 0.1f, 100.0f);
	// Camera viewpoint
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(lookAt, -camCoords.z));
	view = glm::rotate(view, glm::radians(camCoords.y), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, glm::radians(camCoords.x), glm::vec3(0.0f, 1.0f, 0.0f));
	// Combine transformations
	viewProjMat = proj * view;
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUniform1f(glGetUniformLocation(shader, "outline"), .015f);
	for (auto& objPtr : objects) {
		glm::mat4 modelMat = objPtr->getModelMat();
		// Upload transform matrices to shader
		glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelMat));
		glUniformMatrix4fv(viewProjMatLoc, 1, GL_FALSE, glm::value_ptr(viewProjMat));

		// Get camera position and upload to shader
		glm::vec3 camPos = glm::vec3(glm::inverse(view)[3]);
		glUniform3fv(camPosLoc, 1, glm::value_ptr(camPos));

		// Pass object type to shader
		glUniform1i(objTypeLoc, (int)objPtr->getMeshType());
		// Draw the mesh
		objPtr->draw();
	}

	glUseProgram(0);

	// Draw enabled light icons (if in lighting mode)
	if (shadingMode != SHADINGMODE_NORMALS)
		for (auto& l : lights)
			if (l.getEnabled()) l.drawIcon(viewProjMat);
}

// Called when window is resized
void GLState::resizeGL(int w, int h) {
	// Tell OpenGL the new dimensions of the window
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}

// Set the shading mode (normals, cels, or Phong)
void GLState::setShadingMode(ShadingMode sm) {
	shadingMode = sm;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(shadingModeLoc, (int)shadingMode);
	glUseProgram(0);
}

// Set the shading mode (normals, cels, or Phong)
void GLState::setNormalsMode(NormalsMode nm) {
	normalsMode = nm;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(normalsModeLoc, (int)normalsMode);
	glUseProgram(0);
}

// Set the tint mode (const or SSS)
void GLState::setTintMode(TintMode tm) {
	tintMode = tm;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(tintModeLoc, (int)tintMode);
	glUseProgram(0);
}

// Set the occlusion mode (on or off)
void GLState::setOcclusionMode(OcclusionMode om) {
	occlusionMode = om;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(occlusionModeLoc, (int)occlusionMode);
	glUseProgram(0);
}

// Set the specular mode (on or off)
void GLState::setSpecularMode(SpecularMode om) {
	specularMode = om;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(specularModeLoc, (int)specularMode);
	glUseProgram(0);
}

// Set the texture mode (blank or texture)
void GLState::setTextureMode(TextureMode tm) {
	textureMode = tm;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(textureModeLoc, (int)textureMode);
	glUseProgram(0);
}

// Set the interior line mode (on or off)
void GLState::setContourMode(ContourMode tm) {
	contourMode = tm;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(contourModeLoc, (int)contourMode);
	glUseProgram(0);
}

void GLState::setOutlineMode(OutlineMode om) {
	outlineMode = om;

	// Update mode in shader
	glUseProgram(shader);
	glUniform1i(outlineModeLoc, (int)outlineMode);
	glUseProgram(0);
}

// Get object color
glm::vec3 GLState::getObjectColor() const {
	glm::vec3 objColor;
	glGetUniformfv(shader, modelColorLoc, glm::value_ptr(objColor));
	return objColor;
}

// Get ambient strength
float GLState::getAmbientStrength() const {
	float ambStr;
	glGetUniformfv(shader, modelAmbStrLoc, &ambStr);
	return ambStr;
}

// Get diffuse strength
float GLState::getDiffuseStrength() const {
	float diffStr;
	glGetUniformfv(shader, modelDiffStrLoc, &diffStr);
	return diffStr;
}

// Get specular strength
float GLState::getSpecularStrength() const {
	float specStr;
	glGetUniformfv(shader, modelSpecStrLoc, &specStr);
	return specStr;
}

// Get specular exponent
float GLState::getSpecularExponent() const {
	float specExp;
	glGetUniformfv(shader, modelSpecExpLoc, &specExp);
	return specExp;
}

// Set object color
void GLState::setObjectColor(glm::vec3 color) {
	// Update value in shader
	glUseProgram(shader);
	glUniform3fv(modelColorLoc, 1, glm::value_ptr(color));
	glUseProgram(0);
}

// Set ambient strength
void GLState::setAmbientStrength(float ambStr) {
	// Update value in shader
	glUseProgram(shader);
	glUniform1f(modelAmbStrLoc, ambStr);
	glUseProgram(0);
}

// Set diffuse strength
void GLState::setDiffuseStrength(float diffStr) {
	// Update value in shader
	glUseProgram(shader);
	glUniform1f(modelDiffStrLoc, diffStr);
	glUseProgram(0);
}

// Set specular strength
void GLState::setSpecularStrength(float specStr) {
	// Update value in shader
	glUseProgram(shader);
	glUniform1f(modelSpecStrLoc, specStr);
	glUseProgram(0);
}

// Set specular exponent
void GLState::setSpecularExponent(float specExp) {
	// Update value in shader
	glUseProgram(shader);
	glUniform1f(modelSpecExpLoc, specExp);
	glUseProgram(0);
}

void GLState::setMaterialAttrs(
	glm::vec3 floorColor, glm::vec3 modelColor,
	float floorAmbStr, float floorDiffStr, float floorSpecStr, float floorSpecExp,
	float modelAmbStr, float modelDiffStr, float modelSpecStr, float modelSpecExp) {  // set material attributes (initialization)
	// Update values in shader
	glUseProgram(shader);
	glUniform3fv(floorColorLoc, 1, glm::value_ptr(floorColor));
	glUniform3fv(modelColorLoc, 1, glm::value_ptr(modelColor));
	glUniform1f(floorAmbStrLoc, floorAmbStr);
	glUniform1f(floorDiffStrLoc, floorDiffStr);
	glUniform1f(floorSpecStrLoc, floorSpecStr);
	glUniform1f(floorSpecExpLoc, floorSpecExp);
	glUniform1f(modelAmbStrLoc, modelAmbStr);
	glUniform1f(modelDiffStrLoc, modelDiffStr);
	glUniform1f(modelSpecStrLoc, modelSpecStr);
	glUniform1f(modelSpecExpLoc, modelSpecExp);
	glUseProgram(0);
}

// Start rotating the camera (click + drag)
void GLState::beginCameraRotate(glm::vec2 mousePos) {
	camRotating = true;
	initCamRot = glm::vec2(camCoords);
	initMousePos = mousePos;
}

// Stop rotating the camera (mouse button is released)
void GLState::endCameraRotate() {
	camRotating = false;
}

// Use mouse delta to determine new camera rotation
void GLState::rotateCamera(glm::vec2 mousePos) {
	if (camRotating) {
		float rotScale = glm::min(width / 450.0f, height / 270.0f);
		glm::vec2 mouseDelta = mousePos - initMousePos;
		glm::vec2 newAngle = initCamRot + mouseDelta / rotScale;
		newAngle.y = glm::clamp(newAngle.y, -90.0f, 90.0f);
		while (newAngle.x > 180.0f) newAngle.x -= 360.0f;
		while (newAngle.x < -180.0f) newAngle.x += 360.0f;
		if (glm::length(newAngle - glm::vec2(camCoords)) > FLT_EPSILON) {
			camCoords.x = newAngle.x;
			camCoords.y = newAngle.y;
		}
	}
}

void GLState::beginCameraTranslate(glm::vec2 mousePos) {
	camTranslating = true;
	initLookAt = lookAt;
	initMousePos = mousePos;
}

void GLState::endCameraTranslate() {
	camTranslating = false;
}

void GLState::translateCamera(glm::vec2 mousePos) {
	if (camTranslating) {
		float transScale = glm::min(width / 450.0f, height / 270.0f);
		glm::vec2 mouseDelta = mousePos - initMousePos;
		glm::vec2 newPos = initLookAt + mouseDelta / transScale;
		if (glm::length(newPos - glm::vec2(newPos)) > FLT_EPSILON) {
			lookAt = newPos;
		}
	}
}

void GLState::update_time(float time) {
	cur_time = time;
}

// Moves the camera toward / away from the origin (scroll wheel)
void GLState::offsetCamera(float offset) {
	camCoords.z = glm::clamp(camCoords.z + offset, 0.1f, 10.0f);
}

// Display a given .obj file
void GLState::showObjFile(const std::string& filename, const unsigned int meshType, const glm::mat4& modelMat) {
	// Load the .obj file if it's not already loaded
	auto mesh = std::make_shared<Mesh>(filename, static_cast<Mesh::ObjType>(meshType));
	mesh->setModelMat(modelMat);
	objects.push_back(mesh);
}

// Create shaders and associated state
void GLState::initShaders() {
	// Compile and link shader files
	std::vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/v.glsl"));
	shaders.push_back(compileShader(GL_GEOMETRY_SHADER, "shaders/g.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/f.glsl"));
	shader = linkProgram(shaders);
	std::vector<GLuint> depthShaders;  // the shader to get depth map
	depthShaders.push_back(compileShader(GL_VERTEX_SHADER, "shaders/depth_v.glsl"));
	depthShaders.push_back(compileShader(GL_FRAGMENT_SHADER, "shaders/depth_f.glsl"));
	depthShader = linkProgram(depthShaders);
	// Cleanup extra state
	for (auto s : shaders)
		glDeleteShader(s);
	shaders.clear();
	for (auto s : depthShaders)
		glDeleteShader(s);
	depthShaders.clear();

	// Get uniform locations for shader
	modelMatLoc		 = glGetUniformLocation(shader, "modelMat");
	lightSpaceMatLoc = glGetUniformLocation(shader, "lightSpaceMat");
	objTypeLoc		 = glGetUniformLocation(shader, "objType");
	viewProjMatLoc	 = glGetUniformLocation(shader, "viewProjMat");

	shadingModeLoc	 = glGetUniformLocation(shader, "shadingMode");
	normalsModeLoc	 = glGetUniformLocation(shader, "normalsMode");
	tintModeLoc		 = glGetUniformLocation(shader, "tintMode");
	occlusionModeLoc = glGetUniformLocation(shader, "occlusionMode");
	specularModeLoc  = glGetUniformLocation(shader, "specularMode");
	textureModeLoc	 = glGetUniformLocation(shader, "textureMode");
	contourModeLoc	 = glGetUniformLocation(shader, "contourMode");
	outlineModeLoc   = glGetUniformLocation(shader, "outlineMode");
	
	camPosLoc		 = glGetUniformLocation(shader, "camPos");
	floorColorLoc	 = glGetUniformLocation(shader, "floorColor");
	floorAmbStrLoc	 = glGetUniformLocation(shader, "floorAmbStr");
	floorDiffStrLoc	 = glGetUniformLocation(shader, "floorDiffStr");
	floorSpecStrLoc	 = glGetUniformLocation(shader, "floorSpecStr");
	floorSpecExpLoc	 = glGetUniformLocation(shader, "floorSpecExp");
	modelColorLoc	 = glGetUniformLocation(shader, "modelColor");
	modelAmbStrLoc	 = glGetUniformLocation(shader, "modelAmbStr");
	modelDiffStrLoc	 = glGetUniformLocation(shader, "modelDiffStr");
	modelSpecStrLoc	 = glGetUniformLocation(shader, "modelSpecStr");
	modelSpecExpLoc	 = glGetUniformLocation(shader, "modelSpecExp");

	// Get uniform locations for depth shader
	modelMatDepthLoc = glGetUniformLocation(depthShader, "modelMat");
	lightSpaceMatDepthLoc = glGetUniformLocation(depthShader, "lightSpaceMat");

	// Bind lights uniform block to binding index
	glUseProgram(shader);
	GLuint lightBlockIndex = glGetUniformBlockIndex(shader, "LightBlock");
	glUniformBlockBinding(shader, lightBlockIndex, Light::BIND_PT);
	glUseProgram(0);
}


// Trim leading and trailing whitespace from a line
std::string trim(const std::string& line) {
	const std::string whitespace = " \t\r\n";
	auto first = line.find_first_not_of(whitespace);
	if (first == std::string::npos)
		return "";
	auto last = line.find_last_not_of(whitespace);
	auto range = last - first + 1;
	return line.substr(first, range);
}

// Reads lines from istream, stripping whitespace and comments,
// until it finds a line with content in it
std::string getNextLine(std::istream& istr) {
	const std::string comment = "#";
	std::string line = "";
	while (line == "") {
		std::getline(istr, line);
		// Skip comments and empty lines
		auto found = line.find(comment);
		if (found != std::string::npos)
			line = line.substr(0, found);
		line = trim(line);
	}
	return line;
}

// Preprocess the file to remove empty lines and comments
std::string preprocessFile(std::string filename) {
	std::ifstream file;
	file.exceptions(std::ios::badbit | std::ios::failbit);
	file.open(filename);

	std::stringstream ss;
	try {
		// Read each line until the end of the file
		while (true) {
			std::string line = getNextLine(file);
			ss << line << std::endl;
		}
	} catch (const std::exception& e) { e; }

	return ss.str();
}

// Read config file
void GLState::readConfig(std::string filename) {
	try {
		// Read the file contents into a string stream
		std::stringstream ss;
		ss.str(preprocessFile(filename));
		ss.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

		ss >> numObjects;  // get number of objects
		std::vector<std::pair<std::string, unsigned int>> objNamesTypes;  // model names and their types
		for (unsigned int i = 0; i < numObjects; i++) {
			// Read .obj filename
			std::string objName;
			unsigned int meshType;
			ss >> objName;
			ss >> meshType;
			objNamesTypes.push_back(std::make_pair(objName, meshType));
		}

		for (auto& objNameType : objNamesTypes) {
			std::string objName = objNameType.first;
			unsigned int meshType = objNameType.second;

			glm::mat3 rotMat;  // rotation matrix
			glm::vec3 translation;  // translation vector
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					ss >> rotMat[i][j];
				}
			}
			for (int i = 0; i < 3; i++)
				ss >> translation[i];

			glm::mat4 modelMat = calModelMat(rotMat, translation);  // model matrix
			showObjFile(objName, static_cast<Mesh::ObjType>(meshType), modelMat);  // add this object to the scene
		}

		// Objects attributes
		float modelAmbStr, modelDiffStr, modelSpecStr, modelSpecExp;
		float floorAmbStr, floorDiffStr, floorSpecStr, floorSpecExp;
		glm::vec3 modelColor, floorColor;
		// Read material properties
		ss >> modelAmbStr >> modelDiffStr >> modelSpecStr >> modelSpecExp;
		ss >> modelColor.r >> modelColor.g >> modelColor.b;
		modelColor /= 255.0f;
		ss >> floorAmbStr >> floorDiffStr >> floorSpecStr >> floorSpecExp;
		ss >> floorColor.r >> floorColor.g >> floorColor.b;
		floorColor /= 255.0f;

		// Set material properties
		setMaterialAttrs(
			floorColor, modelColor,
			floorAmbStr, floorDiffStr, floorSpecStr, floorSpecExp,
			modelAmbStr, modelDiffStr, modelSpecStr, modelSpecExp
		);

		// Read number of lights
		unsigned int numLights;
		ss >> numLights;
		if (numLights == 0)
			throw std::runtime_error("Must have at least 1 light");
		if (numLights > Light::MAX_LIGHTS)
			throw std::runtime_error("Cannot create more than "
				+ std::to_string(Light::MAX_LIGHTS) + " lights");

		for (unsigned int i = 0; i < lights.size(); i++) {
			// Read properties of each light
			if (i < numLights) {
				int enabled, type;
				glm::vec3 lightColor, lightPos;
				ss >> enabled >> type;
				ss >> lightColor.r >> lightColor.g >> lightColor.b;
				ss >> lightPos.x >> lightPos.y >> lightPos.z;
				lightColor /= 255.0f;
				lights[i].setEnabled((bool)enabled);
				lights[i].setType((Light::LightType)type);
				lights[i].setColor(lightColor);
				lights[i].setPos(lightPos);

			// Disable all other lights
			} else
				lights[i].setEnabled(false);
		}

	} catch (const std::exception& e) {
		// Construct an error message and throw again
		std::stringstream ss;
		ss << "Failed to read config file " << filename << ": " << e.what();
		throw std::runtime_error(ss.str());
	}
}

glm::mat4 GLState::calModelMat(const glm::mat3 rotMat, const glm::vec3 translation) {
	glm::mat4 modelMat = glm::mat4(1.0);  // initialize the matrix as an identity matrix
	glm::mat4 rotateMat = glm::mat4(1.0);
	glm::mat4 translateMat = glm::mat4(1.0);
	// copy
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			rotateMat[i][j] = rotMat[i][j];
		}
	}
	for (int i = 0; i < 3; i++)
		translateMat[i][3] = translation[i];
	modelMat = glm::transpose(translateMat) * glm::transpose(rotateMat);
	return modelMat;
}
