// SphereWorld.cpp
// OpenGL SuperBible
// New and improved (performance) sphere world
// Program by Richard S. Wright Jr.

#include <GLTools.h>
#include <GLShaderManager.h>
#include <GLFrustum.h>
#include <GLBatch.h>
#include <GLFrame.h>
#include <GLMatrixStack.h>
#include <GLGeometryTransform.h>
#include <StopWatch.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#ifdef ANGLE
#include "esGLUT/esGLUT.h"
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif
#endif

#include "browser/big_browser.h"
#include "renderer/big_renderer_gles20.h"

#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];


GLShaderManager		shaderManager;			// Shader Manager
GLMatrixStack		modelViewMatrix;		// Modelview Matrix
GLMatrixStack		projectionMatrix;		// Projection Matrix
GLFrustum			viewFrustum;			// View Frustum
GLGeometryTransform	transformPipeline;		// Geometry Transform Pipeline

GLTriangleBatch		torusBatch;
GLBatch				floorBatch;
GLTriangleBatch     sphereBatch;
GLFrame             cameraFrame;

BigBrowser			*browser = NULL;
BigRendererGLES20	*renderer = NULL;
        
//////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering
// context. 
void SetupRC()
    {
	// Initialze Shader Manager
	shaderManager.InitializeStockShaders();
	
	glEnable(GL_DEPTH_TEST);
    
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	// This makes a torus
	gltMakeTorus(torusBatch, 0.4f, 0.15f, 30, 30);
	
    // This make a sphere
    gltMakeSphere(sphereBatch, 0.1f, 26, 13);
    	
	floorBatch.Begin(GL_LINES, 324);
    for(GLfloat x = -20.0; x <= 20.0f; x+= 0.5) {
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
        }
    floorBatch.End();    

    // Randomly place the spheres
    for(int i = 0; i < NUM_SPHERES; i++) {
        GLfloat x = ((GLfloat)((rand() % 400) - 200) * 0.1f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200) * 0.1f);
        spheres[i].SetOrigin(x, 0.0f, z);
        }
    }


///////////////////////////////////////////////////
// Screen changes size or is initialized
void ChangeSize(int nWidth, int nHeight)
    {
	glViewport(0, 0, nWidth, nHeight);
	
    // Create the projection matrix, and load it on the projection matrix stack
	viewFrustum.SetPerspective(35.0f, float(nWidth)/float(nHeight), 1.0f, 100.0f);
	projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    
    // Set the transformation pipeline to use the two matrix stacks 
	transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
    }

        
// Called to draw scene
void RenderScene(void)
	{
    // Color values
    static GLfloat vFloorColor[] = { 0.0f, 1.0f, 0.0f, 1.0f};
    static GLfloat vTorusColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    static GLfloat vSphereColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

    // Time Based animation
	static CStopWatch	rotTimer;
	float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
	
	// Clear the color and depth buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    
    // Save the current modelview matrix (the identity matrix)
	modelViewMatrix.PushMatrix();	
    
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.PushMatrix(mCamera);

    // Transform the light position into eye coordinates
    M3DVector4f vLightPos = { 0.0f, 10.0f, 5.0f, 1.0f };
    M3DVector4f vLightEyePos;
    m3dTransformVector4(vLightEyePos, vLightPos, mCamera);
		
	// Draw the ground
	shaderManager.UseStockShader(GLT_SHADER_FLAT,
								 transformPipeline.GetModelViewProjectionMatrix(),
								 vFloorColor);	
	floorBatch.Draw();    
    
    for(int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), 
                                transformPipeline.GetProjectionMatrix(), vLightEyePos, vSphereColor);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
        }

    // Draw the spinning Torus
    modelViewMatrix.Translate(0.0f, 0.0f, -2.5f);
    
    // Save the Translation
    modelViewMatrix.PushMatrix();
    
        // Apply a rotation and draw the torus
        modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), 
                                     transformPipeline.GetProjectionMatrix(), vLightEyePos, vTorusColor);
        torusBatch.Draw();
    modelViewMatrix.PopMatrix(); // "Erase" the Rotation from before

    // Apply another rotation, followed by a translation, then draw the sphere
    modelViewMatrix.Rotate(yRot * -2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), 
                                transformPipeline.GetProjectionMatrix(), vLightEyePos, vSphereColor);
    sphereBatch.Draw();

	if(browser) {
		browser->Paint();
	}

	// Restore the previous modleview matrix (the identity matrix)
	modelViewMatrix.PopMatrix();
    modelViewMatrix.PopMatrix();    
    // Do the buffer Swap
    glutSwapBuffers();
        
    // Tell GLUT to do it again
    glutPostRedisplay();
    }


// Respond to arrow keys by moving the camera frame of reference
void SpecialKeys(int key, int x, int y)
    {
	float linear = 0.1f;
	float angular = float(m3dDegToRad(5.0f));
	
	if(key == GLUT_KEY_UP)
		cameraFrame.MoveForward(linear);
	
	if(key == GLUT_KEY_DOWN)
		cameraFrame.MoveForward(-linear);
	
	if(key == GLUT_KEY_LEFT)
		cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
	
	if(key == GLUT_KEY_RIGHT)
		cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);		

	if(key == GLUT_KEY_F11 && browser)
		browser->OpenDevTools();
    }

// Subclass procedure 
WNDPROC wpOrigProc = NULL;
LRESULT APIENTRY SubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	switch(uMsg) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		if(browser && !browser->IsTransparent(LOWORD(lParam), HIWORD(lParam))) {
			browser->SendMouseClickEvent(LOWORD(lParam), HIWORD(lParam),
				(uMsg == WM_LBUTTONDOWN ? MBT_LEFT : MBT_RIGHT), false, 1);
		}
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		if (browser && !browser->IsTransparent(LOWORD(lParam), HIWORD(lParam))) {
			browser->SendMouseClickEvent(LOWORD(lParam), HIWORD(lParam),
				(uMsg == WM_LBUTTONUP ? MBT_LEFT : MBT_RIGHT), true, 1);
		}
		break;
	case WM_MOUSEMOVE:
		if(browser) {
			browser->SendMouseMoveEvent(LOWORD(lParam), HIWORD(lParam), false);
		}
		break;
	case WM_MOUSEWHEEL:
		if(browser) {
			browser->SendMouseWheelEvent(LOWORD(lParam), HIWORD(lParam),
				0, GET_WHEEL_DELTA_WPARAM(wParam));
		}
		break;
	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		if(browser) {
			browser->SendFocusEvent(uMsg == WM_SETFOCUS);
		}
		break;
	case WM_CAPTURECHANGED:
	case WM_CANCELMODE:
		if (browser) {
			browser->SendCaptureLostEvent();
		}
		break;
	}
	return CallWindowProc(wpOrigProc, hwnd, uMsg, 
		wParam, lParam); 
} 

int main(int argc, char* argv[])
    {
	gltSetWorkingDirectory(argv[0]);
		
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800,600);
  
    glutCreateWindow("OpenGL SphereWorld");

	HWND hwnd = FindWindow(L"esGLUT", L"OpenGL SphereWorld");
	wpOrigProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)SubclassProc);
 
    glutSpecialFunc(SpecialKeys);
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RenderScene);
    
#ifndef ANGLE
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fprintf(stderr, "GLEW Error: %s\n", glewGetErrorString(err));
        return 1;
        }
#endif 

	std::string url;
	if(argc >= 2) {
		url = argv[1];
	} else {
		const char *pLastSlash = strrchr(argv[0], '\\');   
		std::string strTemp;
		strTemp.assign(argv[0], pLastSlash);
		pLastSlash = strrchr(strTemp.c_str(), '\\');
		url.assign(strTemp.c_str(), pLastSlash);
		url += "\\res\\pages\\wheel_menu\\wheel_menu.html";
	}

	BigInit();

	browser = new BigBrowser(800, 600, url.c_str(), hwnd);
	renderer = new BigRendererGLES20();
	browser->SetRenderer(renderer);

    SetupRC();
    glutMainLoop();    

	delete browser;
	browser = NULL;

	// Must shut down BIG before exit.
	BigShutdown();
    return 0;
    }
