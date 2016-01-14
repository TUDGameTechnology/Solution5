#include "pch.h"

#include <Kore/Application.h>
#include <Kore/IO/FileReader.h>
#include <Kore/Math/Core.h>
#include <Kore/System.h>
#include <Kore/Input/Keyboard.h>
#include <Kore/Input/Mouse.h>1
#include <Kore/Audio/Mixer.h>
#include <Kore/Graphics/Image.h>
#include <Kore/Graphics/Graphics.h>
#include <Kore/Log.h>
#include "ObjLoader.h"

using namespace Kore;

#define CAMERA_ROTATION_SPEED_X 0.001
#define CAMERA_ROTATION_SPEED_Y 0.001

namespace {
	const int width = 1024;
	const int height = 768;
	Shader* vertexShader;
	Shader* fragmentShader;
	Program* program;
	VertexBuffer* vertexBuffer;
	IndexBuffer* indexBuffer;
	Mesh* mesh;
	Texture* image;
	TextureUnit tex;

	Kore::ConstantLocation cl_modelViewMatrix;
	Kore::ConstantLocation cl_normalMatrix;

	Kore::ConstantLocation cl_lightPos;

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

		Graphics::setMatrix(cl_modelViewMatrix, modelView);

		// prepare normal matrix and pass it to shaders 
		Kore::mat4 normalMatrix = modelView;
		normalMatrix.Invert();
		normalMatrix = normalMatrix.Transpose();
		Graphics::setMatrix(cl_normalMatrix, normalMatrix);


		// update light pos
		lightPosX = 20 * Kore::sin(2 * t);
		lightPosY = 10;
		lightPosZ = 20 * Kore::cos(2 * t);
		Graphics::setFloat3(cl_lightPos, lightPosX, lightPosY, lightPosZ);


		Kore::Audio::update();

		Graphics::begin();
		Graphics::clear(Graphics::ClearColorFlag | Graphics::ClearDepthFlag, 0xff000000);

		program->set();
		Graphics::setTexture(tex, image);
		Graphics::setVertexBuffer(*vertexBuffer);
		Graphics::setIndexBuffer(*indexBuffer);
		Graphics::drawIndexedVertices();

		Graphics::end();
		Graphics::swapBuffers();
	}

	void keyDown(KeyCode code, wchar_t character) {
		switch (code)
		{
		case Key_Left:
		case Key_A:
			moveLeft = true;
			break;
		case Key_Right:
		case Key_D:
			moveRight = true;
			break;
		case Key_Up:
			moveUp = true;
			break;
		case Key_Down:
			moveDown = true;
			break;
		case Key_W:
			moveForward = true;
			break;
		case Key_S:
			moveBackward = true;
			break;
		case Key_R:
			initCamera();
			break;
		case Key_L:
			Kore::log(Kore::LogLevel::Info, "Position: (%.2f, %.2f, %.2f) - Rotation: (%.2f, %.2f, %.2f)\n", cameraX, cameraY, cameraZ, cameraRotX, cameraRotY, cameraRotZ);
			break;
		default:
			break;
		}
	}

	void keyUp(KeyCode code, wchar_t character) {
		switch (code)
		{
		case Key_Left:
		case Key_A:
			moveLeft = false;
			break;
		case Key_Right:
		case Key_D:
			moveRight = false;
			break;
		case Key_Up:
			moveUp = false;
			break;
		case Key_Down:
			moveDown = false;
			break;
		case Key_W:
			moveForward = false;
			break;
		case Key_S:
			moveBackward = false;
			break;
		default:
			break;
		}
	}

	void mouseMove(int x, int y, int movementX, int movementY) {
		if (rotate) {
			cameraRotX += (float)((mousePressY - y) * CAMERA_ROTATION_SPEED_X);
			cameraRotY += (float)((mousePressX - x) * CAMERA_ROTATION_SPEED_Y);
			mousePressX = x;
			mousePressY = y;
		}
	}

	void mousePress(int button, int x, int y) {
		rotate = true;
		mousePressX = x;
		mousePressY = y;
	}

	void mouseRelease(int button, int x, int y) {
		rotate = false;
	}

	void init() {
		mesh = loadObj("tiger.obj");
		image = new Texture("tiger-atlas.jpg", true);

		FileReader vs("shader.vert");
		FileReader fs("shader.frag");
		vertexShader = new Shader(vs.readAll(), vs.size(), VertexShader);
		fragmentShader = new Shader(fs.readAll(), fs.size(), FragmentShader);

		// This defines the structure of your Vertex Buffer
		VertexStructure structure;
		structure.add("pos", Float3VertexData);
		structure.add("tex", Float2VertexData);
		structure.add("nor", Float3VertexData);

		program = new Program;
		program->setVertexShader(vertexShader);
		program->setFragmentShader(fragmentShader);
		program->link(structure);

		tex = program->getTextureUnit("tex");

		/************************************************************************/
		/* Exercise 5                                                           */
		/************************************************************************/
		/* Get constant locations from your shader here, e.g.
		* program->getConstantLocation("bla");
		*/

		cl_modelViewMatrix = program->getConstantLocation("modelViewMatrix");
		cl_normalMatrix = program->getConstantLocation("normalMatrix");
		cl_lightPos = program->getConstantLocation("lightPos");


		// Set this to 1.0f when you do your transformations in the vertex shader
		float scale = 3.0f;

		vertexBuffer = new VertexBuffer(mesh->numVertices, structure, 0);
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
			indexBuffer = new IndexBuffer(mesh->numFaces * 3);
			int* indices = indexBuffer->lock();
			for (int i = 0; i < mesh->numFaces * 3; ++i) {
				indices[i] = mesh->indices[i];
			}
			indexBuffer->unlock();
		}

		Graphics::setRenderState(DepthTest, true);
		Graphics::setRenderState(DepthTestCompare, ZCompareLess);
	}
}

int kore(int argc, char** argv) {
	Application* app = new Application(argc, argv, width, height, 0, false, "Exercise5");

	init();

	app->setCallback(update);

	startTime = System::time();
	Kore::Mixer::init();
	Kore::Audio::init();
	//Kore::Mixer::play(new SoundStream("back.ogg", true));

	Keyboard::the()->KeyDown = keyDown;
	Keyboard::the()->KeyUp = keyUp;
	Mouse::the()->Move = mouseMove;
	Mouse::the()->Press = mousePress;
	Mouse::the()->Release = mouseRelease;

	initCamera();

	app->start();

	delete app;

	return 0;
}