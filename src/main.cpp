#define NOMINMAX
#include <iostream>
#include <memory>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include "glstate.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/freeglut.h>
namespace fs = std::filesystem;

// Menu identifiers
const int MENU_OBJBASE = 64;				// Select object to view
const int MENU_SHADING_CEL = 4;			// Shading mode
const int MENU_SHADING_NORMALS = 6;
const int MENU_EXIT = 1;					// Exit application

// OpenGL state
int width, height;
std::unique_ptr<GLState> glState;
unsigned int activeLight = 0;

// Initialization functions
void initGLUT(int* argc, char** argv);
void initMenu();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyPress(unsigned char key, int x, int y);
void keyRelease(unsigned char key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();

// Program entry point
int main(int argc, char** argv) {
	std::string configFile = "config.txt";
	if (argc > 1)
		configFile = std::string(argv[1]);

	try {
		// Create the window and menu
		initGLUT(&argc, argv);
		initMenu();
		// Initialize OpenGL (buffers, shaders, etc.)
		glState = std::unique_ptr<GLState>(new GLState());
		glState->initializeGL();
		glState->readConfig(configFile);

	} catch (const std::exception& e) {
		// Handle any errors
		std::cerr << "Fatal error: " << e.what() << std::endl;
		cleanup();
		return -1;
	}

	std::cout << "Mouse controls:" << std::endl;
	std::cout << "  Left click + drag to rotate camera" << std::endl;
	std::cout << "  Scroll wheel to zoom in/out" << std::endl;
	std::cout << "  SHIFT + left click + drag to rotate active light source" << std::endl;
	std::cout << "  SHIFT + scroll wheel to change active light distance" << std::endl;
	std::cout << "Keyboard controls:" << std::endl;
	std::cout << "  0:  Turn off all cel shading features" << std::endl;
	std::cout << "  1-9:  Toggle cel shading features" << std::endl;
	std::cout << "  h,H:  Move the object along y axis" << std::endl;
	std::cout << "  j,J:  Move the object along x axis" << std::endl;
	std::cout << "  k,K:  Move the object along z axis" << std::endl;
	std::cout << "  l,L:  Cycle through shading type (Colored normals vs. Cel vs. Phong)" << std::endl;
	std::cout << "  o,O:  Toggle outlining mode (on or off)" << std::endl;
	std::cout << std::endl;

	// Execute main loop
	glutMainLoop();

	return 0;
}

// Setup window and callbacks
void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 800; height = 600;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	// Create the window
	glutCreateWindow("FreeGLUT Window");

	// Create a menu

	// GLUT callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPress);
	glutKeyboardUpFunc(keyRelease);
	glutMouseFunc(mouseBtn);
	glutMotionFunc(mouseMove);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
}

void initMenu() {
	int shadingMenu = glutCreateMenu(menu);
	glutAddMenuEntry("Cel", MENU_SHADING_CEL);
	glutAddMenuEntry("Normals", MENU_SHADING_NORMALS);

	// Create the main menu, adding the objects menu as a submenu
	glutCreateMenu(menu);
	glutAddSubMenu("Shading", shadingMenu);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

}

// Called whenever a screen redraw is requested
void display() {
	// Tell the GLState to render the scene
	glState->paintGL();

	// Scene is rendered to the back buffer, so swap the buffers to display it
	glutSwapBuffers();
}

// Called when the window is resized
void reshape(GLint w, GLint h) {
	// Tell OpenGL the new window size
	width = w; height = h;
	glState->resizeGL(width, height);
}

// Called when a key is pressed
void keyPress(unsigned char key, int x, int y) {
	switch (key) {
	// turn off all implemented functionality
	case '0': {
		glState->setShadingMode(GLState::SHADINGMODE_NONE);
		glState->setNormalsMode(GLState::NORMALSMODE_FACE);
		glState->setTintMode(GLState::TINTMODE_CONST);
		glState->setOcclusionMode(GLState::OCCLUSION_OFF);
		glState->setSpecularMode(GLState::SPECULAR_OFF);
		glState->setTextureMode(GLState::TEXTUREMODE_CONST);
		glState->setContourMode(GLState::CONTOUR_OFF);
		glState->setOutlineMode(GLState::OUTLINE_OFF);
		break;
	}

	case '1': {
		GLState::NormalsMode nm = glState->getNormalsMode();
		if (nm == GLState::NORMALSMODE_INTERPOLATE) {
			glState->setNormalsMode(GLState::NORMALSMODE_FACE);
			std::cout << "Turned off interpolated face normals" << std::endl;
		}
		else {
			glState->setNormalsMode(GLState::NORMALSMODE_INTERPOLATE);
			std::cout << "Turned on interpolated face normals" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}

	case '2': {
		GLState::TintMode tm = glState->getTintMode();
		if (tm == GLState::TINTMODE_CONST) {
			glState->setTintMode(GLState::TINTMODE_SSS);
			std::cout << "Turned on SSS map tinting" << std::endl;
		}
		else {
			glState->setTintMode(GLState::TINTMODE_CONST);
			std::cout << "Turned on constant tinting" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}

	case '3': {
		GLState::OcclusionMode om = glState->getOcclusionMode();
		if (om == GLState::OCCLUSION_OFF) {
			glState->setOcclusionMode(GLState::OCCLUSION_ON);
			std::cout << "Turned on occlusion map" << std::endl;
		}
		else {
			glState->setOcclusionMode(GLState::OCCLUSION_OFF);
			std::cout << "Turned off occlusion map" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}

	case '4': {
		GLState::SpecularMode sm = glState->getSpecularMode();
		if (sm == GLState::SPECULAR_OFF) {
			glState->setSpecularMode(GLState::SPECULAR_ON);
			std::cout << "Turned on cel shading specular" << std::endl;
		}
		else {
			glState->setSpecularMode(GLState::SPECULAR_OFF);
			std::cout << "Turned off cel shading specular" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}

	case 't': 
	case 'T': {
		GLState::TextureMode tm = glState->getTextureMode();
		if (tm == GLState::TEXTUREMODE_CONST) {
			glState->setTextureMode(GLState::TEXTUREMODE_TEX);
			std::cout << "Turned on texture map" << std::endl;
		}
		else {
			glState->setTextureMode(GLState::TEXTUREMODE_TEX);
			std::cout << "Turned off texture map" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}

	case 'i': 
	case 'I': {
		GLState::ContourMode cm = glState->getContourMode();
		if (cm == GLState::CONTOUR_OFF) {
			glState->setContourMode(GLState::CONTOUR_ON);
			std::cout << "Turned on interior lines" << std::endl;
		}
		else {
			glState->setContourMode(GLState::CONTOUR_OFF);
			std::cout << "Turned off interior lines" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}

	// Toggle shading mode (normals vs Cel)
	case 'l': 
	case 'L': {
		GLState::ShadingMode sm = glState->getShadingMode();
		if (sm != GLState::SHADINGMODE_PHONG) {
			glState->setShadingMode(GLState::SHADINGMODE_PHONG);
			std::cout << "Turned on Phong shading" << std::endl;
		}
		else if (sm == GLState::SHADINGMODE_PHONG) {
			glState->setShadingMode(GLState::SHADINGMODE_CEL);
			std::cout << "Turned on cel shading" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}
	case 'n': 
	case 'N': {
		GLState::ShadingMode sm = glState->getShadingMode();
		if (sm != GLState::SHADINGMODE_NORMALS) {
			glState->setShadingMode(GLState::SHADINGMODE_NORMALS);
			std::cout << "Turned on normal shading" << std::endl;
		}
		else if (sm == GLState::SHADINGMODE_NORMALS) {
			glState->setShadingMode(GLState::SHADINGMODE_CEL);
			std::cout << "Turned off normal shading" << std::endl;
		}
		glutPostRedisplay();
		break; 
	}
	// Toggle outline
	case 'O':
	case 'o': {
		GLState::OutlineMode smm = glState->getOutlineMode();
		if (smm == GLState::OUTLINE_ON) {
			glState->setOutlineMode(GLState::OUTLINE_OFF);
			std::cout << "Turned off outline" << std::endl;
		}
		else if (smm == GLState::OUTLINE_OFF) {
			glState->setOutlineMode(GLState::OUTLINE_ON);
			std::cout << "Turned on outline" << std::endl;
		}
		glutPostRedisplay();
		break;
	}

	// Move the object along +y
	case 'h': {
		auto curObj = glState->getObjects()[glState->getActiveObj()];  // Currently controlled object
		auto curModelMat = curObj->getModelMat();  // Its model matrix
		glm::mat4 newModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, glState->getMoveStep(), 0.0f)) * curModelMat;
		curObj->setModelMat(newModelMat);
		glutPostRedisplay();
		break;
	}
	// Move the object along -y
	case 'H': {
		auto curObj = glState->getObjects()[glState->getActiveObj()];  // Currently controlled object
		auto curModelMat = curObj->getModelMat();  // Its model matrix
		glm::mat4 newModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -glState->getMoveStep(), 0.0f)) * curModelMat;
		curObj->setModelMat(newModelMat);
		glutPostRedisplay();
		break;
	}
	// Move the object along +x
	case 'j': {
		auto curObj = glState->getObjects()[glState->getActiveObj()];  // Currently controlled object
		auto curModelMat = curObj->getModelMat();  // Its model matrix
		glm::mat4 newModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(glState->getMoveStep(), 0.0f, 0.0f)) * curModelMat;
		curObj->setModelMat(newModelMat);
		glutPostRedisplay();
		break;
	}
	// Move the object along -x
	case 'J': {
		auto curObj = glState->getObjects()[glState->getActiveObj()];  // Currently controlled object
		auto curModelMat = curObj->getModelMat();  // Its model matrix
		glm::mat4 newModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-glState->getMoveStep(), 0.0f, 0.0f)) * curModelMat;
		curObj->setModelMat(newModelMat);
		glutPostRedisplay();
		break;
	}
	// Move the object along +z
	case 'k': {
		auto curObj = glState->getObjects()[glState->getActiveObj()];  // Currently controlled object
		auto curModelMat = curObj->getModelMat();  // Its model matrix
		glm::mat4 newModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, glState->getMoveStep())) * curModelMat;
		curObj->setModelMat(newModelMat);
		glutPostRedisplay();
		break;
	}
	// Move the object along -z
	case 'K': {
		auto curObj = glState->getObjects()[glState->getActiveObj()];  // Currently controlled object
		auto curModelMat = curObj->getModelMat();  // Its model matrix
		glm::mat4 newModelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -glState->getMoveStep())) * curModelMat;
		curObj->setModelMat(newModelMat);
		glutPostRedisplay();
		break;
	}
	default:
		break;
	}
}

// Called when a key is released
void keyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// Escape key
		menu(MENU_EXIT);
		break;
	}
}

// Called when a mouse button is pressed or released
void mouseBtn(int button, int state, int x, int y) {
	int modifiers = glutGetModifiers();

	// Press left mouse button
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		// Start rotating the active light if holding shift
		if (modifiers & GLUT_ACTIVE_SHIFT) {
			float scale = glm::min((float)width, (float)height);
			glState->getLight(activeLight).beginRotate(
				glm::vec2(x / scale, y / scale));

		} else if (modifiers & GLUT_ACTIVE_CTRL) {
			float scale = glm::min((float)width, (float)height);
			glState->beginCameraTranslate(
				glm::vec2(x / scale, y / scale));

		// Start rotating the camera otherwise
		} else
			glState->beginCameraRotate(glm::vec2(x, y));
	}
	// Release left mouse button
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
		// Stop camera and light rotation
		glState->endCameraRotate();
		glState->endCameraTranslate();
		glState->getLight(activeLight).endRotate();
	}
	// Scroll wheel up
	if (button == 3) {
		// Offset the active light if holding shift
		if (modifiers & GLUT_ACTIVE_SHIFT)
			glState->getLight(activeLight).offsetLight(-0.05f);

		// "Zoom in" otherwise
		else
			glState->offsetCamera(-0.05f);
		glutPostRedisplay();
	}
	// Scroll wheel down
	if (button == 4) {
		// Offset the active light if holding shift
		if (modifiers & GLUT_ACTIVE_SHIFT)
			glState->getLight(activeLight).offsetLight(0.05f);

		// "Zoom out" otherwise
		else
			glState->offsetCamera(0.05f);
		glutPostRedisplay();
	}
}

// Called when the mouse moves
void mouseMove(int x, int y) {
	if (glState->isCamRotating()) {
		// Rotate the camera if currently rotating
		glState->rotateCamera(glm::vec2(x, y));
		glutPostRedisplay();	// Request redraw

	} else if (glState->isCamTranslating()) {
		glState->rotateCamera(glm::vec2(x, y));
		glutPostRedisplay();

	} else if (glState->getLight(activeLight).isRotating()) {
		float scale = glm::min((float)width, (float)height);
		glState->getLight(activeLight).rotateLight(
			glm::vec2(x / scale, y / scale));
		glutPostRedisplay();
	}
}

static auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());  // record start time
// Called when there are no events to process
void idle() {
	// Anything that happens every frame (e.g. movement) should be done here
	// Be sure to call glutPostRedisplay() if the screen needs to update as well

	auto	finish	= std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());	// record end time
	auto	elapsed = static_cast<float>((finish - start).count());

	if ((int)elapsed % (int)(1000.0/60.0) == 0) {
		glState->update_time(elapsed);
		glutPostRedisplay();
	}
}

// Called when a menu button is pressed
void menu(int cmd) {
	switch (cmd) {
	// End the program
	case MENU_EXIT:
		glutLeaveMainLoop();
		break;

	// Show Cel shading & illumination
	case MENU_SHADING_CEL:
		glState->setShadingMode(GLState::SHADINGMODE_CEL);
		glutPostRedisplay();
		break;

	// Show normals as colors
	case MENU_SHADING_NORMALS:
		glState->setShadingMode(GLState::SHADINGMODE_NORMALS);
		glutPostRedisplay();
		break;

	default:
		break;
	}
}

// Called when the window is closed or the event loop is otherwise exited
void cleanup() {
	// Delete the GLState object, calling its destructor,
	// which releases the OpenGL objects
	glState.reset(nullptr);
}
