#include "pch.h"

#include <Kore/System.h>
#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>
#include <Kore/Audio1/Audio.h>
#include <Kore/Graphics1/Image.h>
#include <Kore/Graphics4/Graphics.h>
#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Log.h>
#include "Memory.h"
#include "ObjLoader.h"

using namespace Kore;

#define CAMERA_ROTATION_SPEED_X 0.001
#define CAMERA_ROTATION_SPEED_Y 0.001

namespace {
	const int width = 1024;
	const int height = 768;
	
	Graphics4::Shader* vertexShader;
	Graphics4::Shader* fragmentShader;
	Graphics4::PipelineState* pipeline;
	Graphics4::VertexBuffer* vertexBuffer;
	Graphics4::IndexBuffer* indexBuffer;
	Mesh* mesh;
	Graphics4::Texture* image;
	Graphics4::TextureUnit tex;

	Graphics4::ConstantLocation cl_modelViewMatrix;
	Graphics4::ConstantLocation cl_normalMatrix;

	Graphics4::ConstantLocation cl_lightPos;

	double startTime;
	float lastT = 0;

	const float movementSpeed = 10;

	float cameraX;
	float cameraY;
	float cameraZ;

	float cameraRotX = 0;
	float cameraRotY = 0;
	float cameraRotZ = 0;

	float lightPosX;
	float lightPosY;
	float lightPosZ;

	bool moveUp = false;
	bool moveDown = false;
	bool moveRight = false;
	bool moveLeft = false;
	bool moveForward = false;
	bool moveBackward = false;

	bool rotate = false;
	int mousePressX, mousePressY;

	void initCamera() {
		cameraX = 0;
		cameraY = 0;
		cameraZ = 20;

		cameraRotX = 0;
		cameraRotY = Kore::pi;
		cameraRotZ = 0;
	}

	void rotate3d(float &x, float &y, float &z, float rx, float ry, float rz) {
		float d1x = Kore::cos(ry) * x + Kore::sin(ry) * z;
		float d1y = y;
		float d1z = Kore::cos(ry) * z - Kore::sin(ry) * x;
		float d2x = d1x;
		float d2y = Kore::cos(rx) * d1y - Kore::sin(rx) * d1z;
		float d2z = Kore::cos(rx) * d1z + Kore::sin(rx) * d1y;
		float d3x = Kore::cos(rz) * d2x - Kore::sin(rz) * d2y;
		float d3y = Kore::cos(rz) * d2y + Kore::sin(rz) * d2x;
		float d3z = d2z;

		x = d3x;
		y = d3y;
		z = d3z;
	}

	void update() {
		Graphics4::begin();
		Graphics4::clear(Graphics4::ClearColorFlag | Graphics4::ClearDepthFlag, 0xff000000);
		
		// Needs to be set before any other values can be set
		Graphics4::setPipeline(pipeline);

		float t = (float)(System::time() - startTime);

		float timeSinceLastFrame = t - lastT;
		lastT = t;

		//update camera:
		float cameraMovementX = 0;
		float cameraMovementY = 0;
		float cameraMovementZ = 0;

		if (moveUp)
			cameraMovementY += timeSinceLastFrame * movementSpeed;
		if (moveDown)
			cameraMovementY -= timeSinceLastFrame * movementSpeed;
		if (moveLeft)
			cameraMovementX -= timeSinceLastFrame * movementSpeed;
		if (moveRight)
			cameraMovementX += timeSinceLastFrame * movementSpeed;
		if (moveForward)
			cameraMovementZ += timeSinceLastFrame * movementSpeed;
		if (moveBackward)
			cameraMovementZ -= timeSinceLastFrame * movementSpeed;

		// rotate direction according to current rotation
		rotate3d(cameraMovementX, cameraMovementY, cameraMovementZ, -cameraRotX, 0, -cameraRotZ);
		rotate3d(cameraMovementX, cameraMovementY, cameraMovementZ, 0, -cameraRotY, -cameraRotZ);

		cameraX += cameraMovementX;
		cameraY += cameraMovementY;
		cameraZ += cameraMovementZ;

		// prepare model view matrix and pass it to shaders
		Kore::mat4 modelView = Kore::mat4::RotationZ(cameraRotZ)
			* Kore::mat4::RotationX(cameraRotX)
			* Kore::mat4::RotationY(cameraRotY)
			* Kore::mat4::Translation(-cameraX, -cameraY, -cameraZ);

		Graphics4::setMatrix(cl_modelViewMatrix, modelView);

		// prepare normal matrix and pass it to shaders 
		Kore::mat4 normalMatrix = modelView;
		normalMatrix.Invert();
		normalMatrix = normalMatrix.Transpose();
		Graphics4::setMatrix(cl_normalMatrix, normalMatrix);


		// update light pos
		lightPosX = 20 * Kore::sin(2 * t);
		lightPosY = 10;
		lightPosZ = 20 * Kore::cos(2 * t);
		Graphics4::setFloat3(cl_lightPos, lightPosX, lightPosY, lightPosZ);


		Kore::Audio2::update();


		Graphics4::setTexture(tex, image);
		Graphics4::setVertexBuffer(*vertexBuffer);
		Graphics4::setIndexBuffer(*indexBuffer);
		Graphics4::drawIndexedVertices();

		/************************************************************************/
		/* Exercise 5                                                           */
		/************************************************************************/
		/* Set values in your shader using the constant locations you defined, e.g.
		* Graphics::setMatrix(ConstantLocation, Value);
		*/


		Graphics4::end();
		Graphics4::swapBuffers();
	}

	void keyDown(KeyCode code) {
		switch (code)
		{
		case KeyLeft:
		case KeyA:
			moveLeft = true;
			break;
		case KeyRight:
		case KeyD:
			moveRight = true;
			break;
		case KeyUp:
			moveUp = true;
			break;
		case KeyDown:
			moveDown = true;
			break;
		case KeyW:
			moveForward = true;
			break;
		case KeyS:
			moveBackward = true;
			break;
		case KeyR:
			initCamera();
			break;
		case KeyL:
			Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f) - Rotation: (%.2f, %.2f, %.2f)\n", cameraX, cameraY, cameraZ, cameraRotX, cameraRotY, cameraRotZ);
			break;
		default:
			break;
		}
	}

	void keyUp(KeyCode code) {
		switch (code)
		{
		case KeyLeft:
		case KeyA:
			moveLeft = false;
			break;
		case KeyRight:
		case KeyD:
			moveRight = false;
			break;
		case KeyUp:
			moveUp = false;
			break;
		case KeyDown:
			moveDown = false;
			break;
		case KeyW:
			moveForward = false;
			break;
		case KeyS:
			moveBackward = false;
			break;
		default:
			break;
		}
	}

	void mouseMove(int window, int x, int y, int movementX, int movementY) {
		if (rotate) {
			cameraRotX += (float)((mousePressY - y) * CAMERA_ROTATION_SPEED_X);
			cameraRotY += (float)((mousePressX - x) * CAMERA_ROTATION_SPEED_Y);
			mousePressX = x;
			mousePressY = y;
		}
	}

	void mousePress(int window, int button, int x, int y) {
		rotate = true;
		mousePressX = x;
		mousePressY = y;
	}

	void mouseRelease(int window, int button, int x, int y) {
		rotate = false;
	}

	void init() {
		Memory::init();

		mesh = loadObj("tiger.obj");
		image = new Graphics4::Texture("tiger-atlas.jpg", true);

		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Graphics4::Shader(vs.readAll(), vs.size(), Graphics4::VertexShader);
		fragmentShader = new Graphics4::Shader(fs.readAll(), fs.size(), Graphics4::FragmentShader);

		// This defines the structure of your Vertex Buffer
		Graphics4::VertexStructure structure;
		structure.add("pos", Graphics4::Float3VertexData);
		structure.add("tex", Graphics4::Float2VertexData);
		structure.add("nor", Graphics4::Float3VertexData);

		pipeline = new Graphics4::PipelineState;
		pipeline->inputLayout[0] = &structure;
		pipeline->inputLayout[1] = nullptr;
		pipeline->vertexShader = vertexShader;
		pipeline->fragmentShader = fragmentShader;
		pipeline->depthMode = Graphics4::ZCompareLess;
		pipeline->depthWrite = true;
		pipeline->compile();

		tex = pipeline->getTextureUnit("tex");

		/************************************************************************/
		/* Exercise 5                                                           */
		/************************************************************************/
		/* Get constant locations from your shader here, e.g.
		* program->getConstantLocation("bla");
		*/

		cl_modelViewMatrix = pipeline->getConstantLocation("modelViewMatrix");
		cl_normalMatrix = pipeline->getConstantLocation("normalMatrix");
		cl_lightPos = pipeline->getConstantLocation("lightPos");

		// Set this to 1.0f when you do your transformations in the vertex shader
		float scale = 3.0f;

		vertexBuffer = new Graphics4::VertexBuffer(mesh->numVertices, structure, 0);
		{
			float* vertices = vertexBuffer->lock();
			for (int i = 0; i < mesh->numVertices; ++i) {
				vertices[i * 8 + 0] = mesh->vertices[i * 8 + 0] * scale;
				vertices[i * 8 + 1] = mesh->vertices[i * 8 + 1] * scale;
				vertices[i * 8 + 2] = mesh->vertices[i * 8 + 2] * scale;
				vertices[i * 8 + 3] = mesh->vertices[i * 8 + 3];
				vertices[i * 8 + 4] = 1.0f - mesh->vertices[i * 8 + 4];
				vertices[i * 8 + 5] = mesh->vertices[i * 8 + 5];
				vertices[i * 8 + 6] = mesh->vertices[i * 8 + 6];
				vertices[i * 8 + 7] = mesh->vertices[i * 8 + 7];
			}
			vertexBuffer->unlock();
		}

		{
			indexBuffer = new Graphics4::IndexBuffer(mesh->numFaces * 3);
			int* indices = indexBuffer->lock();
			for (int i = 0; i < mesh->numFaces * 3; ++i) {
				indices[i] = mesh->indices[i];
			}
			indexBuffer->unlock();
		}
	}
}

int kore(int argc, char** argv) {
	Kore::System::init("Solution 5", width, height);

	init();

	Kore::System::setCallback(update);

	startTime = System::time();
	Kore::Audio1::init();
	Kore::Audio2::init();
	//Kore::Mixer::play(new SoundStream("back.ogg", true));

	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;

	initCamera();

	Kore::System::start();

	return 0;
}
