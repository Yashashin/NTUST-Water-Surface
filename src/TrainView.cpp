/************************************************************************
     File:        TrainView.cpp

     Author:     
                  Michael Gleicher, gleicher@cs.wisc.edu

     Modifier
                  Yu-Chi Lai, yu-chi@cs.wisc.edu
     
     Comment:     
						The TrainView is the window that actually shows the 
						train. Its a
						GL display canvas (Fl_Gl_Window).  It is held within 
						a TrainWindow
						that is the outer window with all the widgets. 
						The TrainView needs 
						to be aware of the window - since it might need to 
						check the widgets to see how to draw

	  Note:        we need to have pointers to this, but maybe not know 
						about it (beware circular references)

     Platform:    Visio Studio.Net 2003/2005

*************************************************************************/

#include <iostream>
#include<string>
#include <Fl/fl.h>

// we will need OpenGL, and OpenGL needs windows.h
#include <windows.h>
//#include "GL/gl.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <GL/glu.h>

#include "TrainView.H"
#include "TrainWindow.H"
#include "Utilities/3DUtils.H"



#ifdef EXAMPLE_SOLUTION
#	include "TrainExample/TrainExample.H"
#endif


//************************************************************************
//
// * Constructor to set up the GL window
//========================================================================
TrainView::
TrainView(int x, int y, int w, int h, const char* l) 
	: Fl_Gl_Window(x,y,w,h,l)
//========================================================================
{
	mode( FL_RGB|FL_ALPHA|FL_DOUBLE | FL_STENCIL );

	resetArcball();
}

//************************************************************************
//
// * Reset the camera to look at the world
//========================================================================
void TrainView::
resetArcball()
//========================================================================
{
	// Set up the camera to look at the world
	// these parameters might seem magical, and they kindof are
	// a little trial and error goes a long way
	arcball.setup(this, 40, 250, .2f, .4f, 0);
}

//************************************************************************
//
// * FlTk Event handler for the window
//########################################################################
// TODO: 
//       if you want to make the train respond to other events 
//       (like key presses), you might want to hack this.
//########################################################################
//========================================================================
int TrainView::handle(int event)
{
	// see if the ArcBall will handle the event - if it does, 
	// then we're done
	// note: the arcball only gets the event if we're in world view
	if (tw->worldCam->value())
		if (arcball.handle(event)) 
			return 1;

	// remember what button was used
	static int last_push;

	switch(event) {
		// Mouse button being pushed event
		case FL_PUSH:
			last_push = Fl::event_button();
			// if the left button be pushed is left mouse button
			if (last_push == FL_LEFT_MOUSE  ) {
				doPick();
				//addDrop();
				damage(1);
				return 1;
			};
			break;

	   // Mouse button release event
		case FL_RELEASE: // button release
			damage(1);
			last_push = 0;
			return 1;

		// Mouse button drag event
		case FL_DRAG:

			// Compute the new control point position
			if ((last_push == FL_LEFT_MOUSE) && (selectedCube >= 0)) {
				ControlPoint* cp = &m_pTrack->points[selectedCube];

				double r1x, r1y, r1z, r2x, r2y, r2z;
				getMouseLine(r1x, r1y, r1z, r2x, r2y, r2z);

				double rx, ry, rz;
				mousePoleGo(r1x, r1y, r1z, r2x, r2y, r2z, 
								static_cast<double>(cp->pos.x), 
								static_cast<double>(cp->pos.y),
								static_cast<double>(cp->pos.z),
								rx, ry, rz,
								(Fl::event_state() & FL_CTRL) != 0);

				cp->pos.x = (float) rx;
				cp->pos.y = (float) ry;
				cp->pos.z = (float) rz;
				damage(1);
			}
			break;

		// in order to get keyboard events, we need to accept focus
		case FL_FOCUS:
			return 1;

		// every time the mouse enters this window, aggressively take focus
		case FL_ENTER:	
			focus(this);
			break;

		case FL_KEYBOARD:
		 		int k = Fl::event_key();
				int ks = Fl::event_state();
				if (k == 'p') {
					// Print out the selected control point information
					if (selectedCube >= 0) 
						printf("Selected(%d) (%g %g %g) (%g %g %g)\n",
								 selectedCube,
								 m_pTrack->points[selectedCube].pos.x,
								 m_pTrack->points[selectedCube].pos.y,
								 m_pTrack->points[selectedCube].pos.z,
								 m_pTrack->points[selectedCube].orient.x,
								 m_pTrack->points[selectedCube].orient.y,
								 m_pTrack->points[selectedCube].orient.z);
					else
						printf("Nothing Selected\n");

					return 1;
				};
				break;
	}

	return Fl_Gl_Window::handle(event);
}

void TrainView::addDrop() {
	this->drop_point = glm::vec2(0.5, 0.5);
}

unsigned int loadCubemap(vector<const GLchar*> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		cv::Mat img;
		img = cv::imread(faces[i], cv::IMREAD_COLOR);

		 width=img.cols;
		 height=img.rows;
		if (!img.empty())
		{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB ,width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
		}
		img.release();
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

unsigned int loadTexture(const GLchar* tex)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	cv::Mat img;
	img = cv::imread(tex, cv::IMREAD_COLOR);
	int width = img.cols;
	int height = img.rows;

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	if (img.type() == CV_8UC3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, img.cols, img.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, img.data);
	else if (img.type() == CV_8UC4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.cols, img.rows, 0, GL_BGRA, GL_UNSIGNED_BYTE, img.data);
	glBindTexture(GL_TEXTURE_2D, 0);

	img.release();
	return textureID;
}

void readObj(
	std::string filepath,
	std::vector<glm::vec3>& points,
	std::vector<glm::vec2>& texcoords,
	std::vector<glm::vec3>& normals
)
{


	// 頂點屬性
	std::vector<glm::vec3> vectexPosition;
	std::vector<glm::vec2> vertexTexcoord;
	std::vector<glm::vec3> vectexNormal;

	// 面片索引資訊
	std::vector<glm::ivec3> positionIndex;
	std::vector<glm::ivec3> texcoordIndex;
	std::vector<glm::ivec3> normalIndex;

	// 開啟檔案流
	std::ifstream fin(filepath);
	std::string line;
	if (!fin.is_open())
	{


		std::cout << "檔案 " << filepath << " 開啟失敗" << std::endl;
		exit(-1);
	}

	// 按行讀取
	while (std::getline(fin, line))
	{


		std::istringstream sin(line);   // 以一行的資料作為 string stream 解析並且讀取
		std::string type;
		GLfloat x, y, z;
		int v0, vt0, vn0;   // 面片第 1 個頂點的【位置，紋理座標，法線】索引
		int v1, vt1, vn1;   // 2
		int v2, vt2, vn2;   // 3
		int v3, vt3, vn3;   // 4
		char slash;

		// 讀取obj檔案
		sin >> type;
		if (type == "v") {


			sin >> x >> y >> z;
			vectexPosition.push_back(glm::vec3(x, y, z));
		}
		if (type == "vt") {


			sin >> x >> y;
			vertexTexcoord.push_back(glm::vec2(x, y));
		}
		if (type == "vn") {


			sin >> x >> y >> z;
			vectexNormal.push_back(glm::vec3(x, y, z));
		}
		if (type == "f") {


			sin >> v0 >> slash >> vt0 >> slash >> vn0;
			sin >> v1 >> slash >> vt1 >> slash >> vn1;
			sin >> v2 >> slash >> vt2 >> slash >> vn2;
			if (sin.rdbuf()->in_avail() > 6)
			{
				sin >> v3 >> slash >> vt3 >> slash >> vn3;
				positionIndex.push_back(glm::ivec3(v0 - 1, v1 - 1, v2 - 1));
				texcoordIndex.push_back(glm::ivec3(vt0 - 1, vt1 - 1, vt2 - 1));
				normalIndex.push_back(glm::ivec3(vn0 - 1, vn1 - 1, vn2 - 1));
				positionIndex.push_back(glm::ivec3(v0 - 1, v2 - 1, v3 - 1));
				texcoordIndex.push_back(glm::ivec3(vt0 - 1, vt2 - 1, vt3 - 1));
				normalIndex.push_back(glm::ivec3(vn0 - 1, vn2 - 1, vn3 - 1));
			}
			else
			{
				positionIndex.push_back(glm::ivec3(v0 - 1, v1 - 1, v2 - 1));
				texcoordIndex.push_back(glm::ivec3(vt0 - 1, vt1 - 1, vt2 - 1));
				normalIndex.push_back(glm::ivec3(vn0 - 1, vn1 - 1, vn2 - 1));
			}


		}
	}

	// 根據面片資訊生成最終傳入頂點著色器的頂點資料
	for (int i = 0; i < positionIndex.size(); i++)
	{

		// 頂點位置
		points.push_back(vectexPosition[positionIndex[i].x]);
		points.push_back(vectexPosition[positionIndex[i].y]);
		points.push_back(vectexPosition[positionIndex[i].z]);

		// 頂點紋理座標
		texcoords.push_back(vertexTexcoord[texcoordIndex[i].x]);
		texcoords.push_back(vertexTexcoord[texcoordIndex[i].y]);
		texcoords.push_back(vertexTexcoord[texcoordIndex[i].z]);

		// 頂點法線
		normals.push_back(glm::vec3(0,1,0));
		normals.push_back(glm::vec3(0, 1, 0));
		normals.push_back(glm::vec3(0, 1, 0));
	}
}
//************************************************************************
//
// * this is the code that actually draws the window
//   it puts a lot of the work into other routines to simplify things
//========================================================================
void TrainView::draw()
{

	//*********************************************************************
	//
	// * Set up basic opengl informaiton
	//
	//**********************************************************************
	//initialized gladk
	if (gladLoadGL())
	{
		//initiailize VAO, VBO, Shader...
		if (!this->drop)
		{
			this->drop=new Shader( "src/shaders/drop.vert", nullptr, nullptr, nullptr, "src/shaders/drop.frag");
			ripple_tex = loadTexture("Images/ripple.jpg");
			GLfloat quadVertice[] = {
				-1.0f,1.0f,0.0f,
				-1.0f,-1.0f,0.0f,
				1.0f,1.0f,0.0f,
				1.0f,-1.0f,0.0f
			};
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertice), &quadVertice, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		
			glGenFramebuffers(1, &FFrameBuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, FFrameBuffer);

			glGenTextures(1, &empty_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, empty_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 400,400, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, empty_textureColorbuffer, 0);
			//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ripple_tex, 0);

			GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
			glDrawBuffers(1, DrawBuffers);

			
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 400, 400);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			
		}
		if (!this->update)
		{
			this->drop = new Shader( "src/shaders/drop.vert", nullptr, nullptr, nullptr,  "src/shaders/drop.frag");
		}
		if (!this->frame_buffer)
		{
			this->frame_buffer = new Shader( "src/shaders/framebuffer.vert", nullptr, nullptr, nullptr,  "src/shaders/framebuffer.frag");

			float quadVertices[] = { 
			  -1.0f,  1.0f,  0.0f, 1.0f,
			  -1.0f, -1.0f,  0.0f, 0.0f,
			   1.0f, -1.0f,  1.0f, 0.0f,

			  -1.0f,  1.0f,  0.0f, 1.0f,
			   1.0f, -1.0f,  1.0f, 0.0f,
			   1.0f,  1.0f,  1.0f, 1.0f
			};
	
			glGenVertexArrays(1, &frameBuffer_quadVAO);
			glGenBuffers(1, &frameBuffer_quadVBO);
			glBindVertexArray(frameBuffer_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, frameBuffer_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
			// framebuffer configuration
			glGenFramebuffers(1, &frameBuffer_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_framebuffer);

			glGenTextures(1, &frameBuffer_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, frameBuffer_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBuffer_textureColorbuffer, 0);

			glGenRenderbuffers(1, &frameBuffer_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); 
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frameBuffer_rbo); 
		
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		if (!this->screen)
		{
			this->screen = new Shader( "src/shaders/framebuffer_screen.vert", nullptr, nullptr, nullptr, "src/shaders/framebuffer_screen.frag");
			float screenVertices[] = {
			  -1.0f,  1.0f,  0.0f,
			  1.0f, -1.0f, -1.0f,
			  0.0f, 0.0f, 1.0f,
			  -1.0f,  1.0f, 0.0f,
			  -1.0f,  1.0f,  0.0f,
			  1.0f, 1.0f, -1.0f,
			  1.0f, 0.0f, 1.0f,
			  1.0f, 1.0f, 1.0f
			};
			glGenVertexArrays(1, &screen_quadVAO);
			glGenBuffers(1, &screen_quadVBO);
			glBindVertexArray(screen_quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, screen_quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(screenVertices), &screenVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			screen->Use();
			glUniform1i(glGetUniformLocation(screen->Program, "screenTexture"), 0);

			glGenFramebuffers(1, &screen_framebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);

			glGenTextures(1, &screen_textureColorbuffer);
			glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_textureColorbuffer, 0);

			glGenRenderbuffers(1, &screen_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		}
		if (!this->water)
		{
			this->water = new Shader( "src/shaders/water.vert", nullptr, nullptr, nullptr,  "src/shaders/water.frag");
		}
		if (!this->height_map)
		{
			this->height_map = new Shader( "src/shaders/heightMap.vert", nullptr, nullptr, nullptr,  "src/shaders/heightMap.frag");
			for (int i = 0; i < 200; i++)
			{
				std::string str = "Images/waves/";
				if (i < 10)
				{
					str += ("00" + std::to_string(i) + ".png");
				}
				else if (i < 100)
				{
					str += ("0" + std::to_string(i) + ".png");
				}
				else
				{
					str += (std::to_string(i) + ".png");
				}
				this->height_map_tex[i] = loadTexture(str.c_str());
			}
			
		}
		if (!this->skybox)
		{
			this->skybox = new Shader(  "src/shaders/skybox.vert", nullptr, nullptr, nullptr,   "src/shaders/skybox.frag");
			float skybox_vertice[] = {
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f
			};
			glGenVertexArrays(1, &skybox_vao);
			glGenBuffers(1, &skybox_vbo);
			glBindVertexArray(skybox_vao);
			glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertice), &skybox_vertice, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			vector<const GLchar*> skybox_faces = {
			"Images/skybox/right.jpg",
			"Images/skybox/left.jpg",
			"Images/skybox/top.jpg",
			"Images/skybox/bottom.jpg",
			"Images/skybox/back.jpg",
			"Images/skybox/front.jpg",
			};
			this->skybox_cubemap_tex = loadCubemap(skybox_faces);
		}
		if (!this->tile)
		{
			this->tile = new Shader( "src/shaders/tile.vert", nullptr, nullptr, nullptr,  "src/shaders/tile.frag");
			GLfloat tile_vertice[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f

			};
			GLfloat tile_normals[] = {

				0,0,-1,
				0,0,-1,
				0,0,-1,
				0,0,-1,
				0,0,-1,
				0,0,-1,

				-1,0,0,
				-1,0,0,
				-1,0,0,
				-1,0,0,
				-1,0,0,
				-1,0,0,

			    1,0,0,
				1,0,0,
				1,0,0,
				1,0,0,
				1,0,0,
				1,0,0,
			
				0,0,1,
				0,0,1,
				0,0,1,
				0,0,1,
				0,0,1,
				0,0,1,

				0,-1,0,
				0,-1,0,
				0,-1,0,
				0,-1,0,
				0,-1,0,
				0,-1,0
			};
			glGenVertexArrays(1, &tile_vao);
			glGenBuffers(2, tile_vbo);
			glBindVertexArray(tile_vao);

			glBindBuffer(GL_ARRAY_BUFFER, tile_vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(tile_vertice), &tile_vertice, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

			glBindBuffer(GL_ARRAY_BUFFER,tile_vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, sizeof(tile_normals), &tile_normals[0], GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);
			vector<const GLchar*> tile_faces = {
			"Images/tile.jpg",
			"Images/tile.jpg",
			"Images/tile.jpg",
			"Images/tile.jpg",
			"Images/tile.jpg",
			"Images/tile.jpg"
			};
			this->tile_cubemap_tex = loadCubemap(tile_faces);
		}

		if (!this->commom_matrices)
			this->commom_matrices = new UBO();
		this->commom_matrices->size = 2 * sizeof(glm::mat4);
		glGenBuffers(1, &this->commom_matrices->ubo);
		glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
		glBufferData(GL_UNIFORM_BUFFER, this->commom_matrices->size, NULL, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		if (!this->plane) {
			std::vector<glm::vec3> points;
			std::vector<glm::vec2> texcoords;
			std::vector<glm::vec3> normals;
			readObj("water.obj", points, texcoords, normals);

			this->plane = new VAO;
			this->plane->element_amount = points.size();
			glGenVertexArrays(1, &this->plane->vao);
			glGenBuffers(3, this->plane->vbo);
			//glGenBuffers(1, &this->plane->ebo);

			glBindVertexArray(this->plane->vao);

			// Position attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[0]);
			glBufferData(GL_ARRAY_BUFFER, points.size() * 3 * 4, &points[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(0);

			// Normal attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[1]);
			glBufferData(GL_ARRAY_BUFFER, normals.size() * 3 * 4, &normals[0], GL_STATIC_DRAW);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(1);

			// Texture Coordinate attribute
			glBindBuffer(GL_ARRAY_BUFFER, this->plane->vbo[2]);
			glBufferData(GL_ARRAY_BUFFER, texcoords.size() * 2 * 4, &texcoords[0], GL_STATIC_DRAW);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
			glEnableVertexAttribArray(2);

			// Unbind VAO
			glBindVertexArray(0);

		}

		if (!this->texture)
			this->texture = new Texture2D( "Images/water_top.jpg");


		static int pre_w = w(), pre_h = h();
		// Set up the view port
		glViewport(0, 0, w(), h());
		if (pre_w != w() || pre_h != h()) {
			glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);

			glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screen_textureColorbuffer, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, screen_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, screen_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			////=======================================================
			// framebuffer configuration
			glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer_framebuffer);
			glBindTexture(GL_TEXTURE_2D, frameBuffer_textureColorbuffer);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w(), h(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBuffer_textureColorbuffer, 0);

			glBindRenderbuffer(GL_RENDERBUFFER, frameBuffer_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w(), h()); // use a single renderbuffer object for both a depth AND stencil buffer.
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, frameBuffer_rbo); // now actually attach it
			// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		// clear the window, be sure to clear the Z-Buffer too
		glClearColor(0, 0, .3f, 0);		// background should be blue

		// we need to clear out the stencil buffer since we'll use
		// it for shadows
		glClearStencil(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glEnable(GL_DEPTH);

		// Blayne prefers GL_DIFFUSE
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

		// prepare for projection
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		setProjection();		// put the code to set up matrices here

		//######################################################################
		// TODO: 
		// you might want to set the lighting up differently. if you do, 
		// we need to set up the lights AFTER setting up the projection
		//######################################################################
		// enable the lighting
		glEnable(GL_COLOR_MATERIAL);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		// top view only needs one light
		if (tw->topCam->value()) {
			glDisable(GL_LIGHT1);
			glDisable(GL_LIGHT2);
		}
		else {
			glEnable(GL_LIGHT1);
			glEnable(GL_LIGHT2);
		}

		//*********************************************************************
		//
		// * set the light parameters
		//
		//**********************************************************************
		GLfloat lightPosition1[] = { 0,1,1,0 }; // {50, 200.0, 50, 1.0};
		GLfloat lightPosition2[] = { 1, 0, 0, 0 };
		GLfloat lightPosition3[] = { 0, -1, 0, 0 };
		GLfloat yellowLight[] = { 0.5f, 0.5f, .1f, 1.0 };
		GLfloat whiteLight[] = { 1.0f, 1.0f, 1.0f, 1.0 };
		GLfloat blueLight[] = { .1f,.1f,.3f,1.0 };
		GLfloat grayLight[] = { .3f, .3f, .3f, 1.0 };

		glLightfv(GL_LIGHT0, GL_POSITION, lightPosition1);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
		glLightfv(GL_LIGHT0, GL_AMBIENT, grayLight);

		glLightfv(GL_LIGHT1, GL_POSITION, lightPosition2);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, yellowLight);

		glLightfv(GL_LIGHT2, GL_POSITION, lightPosition3);
		glLightfv(GL_LIGHT2, GL_DIFFUSE, blueLight);

		//*********************************************************************
		// now draw the ground plane
		//*********************************************************************
		// set to opengl fixed pipeline(use opengl 1.x draw function)

		// setup FBO for ripple texture 

		//std::cout << drop_point.x << " " << drop_point.y << std::endl;
		//drop buffer
		glBindFramebuffer(GL_FRAMEBUFFER, FFrameBuffer);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // we're not using the stencil buffer now
			drop->Use();
			glBindVertexArray(quadVAO);
			drop->setInt("u_water", 0);
			glUniform1f(glGetUniformLocation(this->drop->Program, "u_radius"), 0.09f);
			glUniform1f(glGetUniformLocation(this->drop->Program, "u_strength"), 0.5f);
			glUniform2f(glGetUniformLocation(this->drop->Program, "u_center"), drop_point.x,drop_point.y);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, ripple_tex);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, FFrameBuffer);
		glBindTexture(GL_TEXTURE_2D, ripple_tex);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 400,400);

		glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		 //cleanup
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &FFrameBuffer);
		drop_point = glm::vec2(0, 0);
		glUseProgram(0);

		//screen framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, screen_framebuffer);
		glEnable(GL_DEPTH_TEST); 
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		setupFloor();
		glDisable(GL_LIGHTING);
		//drawFloor(200, 10);


		//*********************************************************************
		// now draw the object and we need to do it twice
		// once for real, and then once for shadows
		//*********************************************************************
		glEnable(GL_LIGHTING);
		setupObjects();

		drawStuff();

		// this time drawing is for shadows (except for top view)
		if (!tw->topCam->value()) {
			setupShadows();
			drawStuff(true);
			unsetupShadows();
		}

		setUBO();
		glBindBufferRange(
			GL_UNIFORM_BUFFER, /*binding point*/0, this->commom_matrices->ubo, 0, this->commom_matrices->size);

		glm::mat4 view;
		glGetFloatv(GL_MODELVIEW_MATRIX, &view[0][0]);
		glm::mat4 projection;
		glGetFloatv(GL_PROJECTION_MATRIX, &projection[0][0]);
		glm::mat4 inversion = glm::inverse(view);
		glm::vec3 viewerPos(inversion[3][0], inversion[3][1], inversion[3][2]);
		
	
		//skybox
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default
		glDisable(GL_CULL_FACE);
		glm::mat4 skybox_matrix = glm::mat4();
		skybox_matrix = glm::translate(skybox_matrix, viewerPos);
		skybox_matrix = glm::scale(skybox_matrix, glm::vec3(600.0f, 600.0f, 600.0f));
		glDepthFunc(GL_LEQUAL);
		skybox->Use();
		//glUniform1f(glGetUniformLocation(skybox->Program, "skybox"), skybox_cubemap_tex);
		glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "u_projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "u_view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(skybox->Program, "s_model"), 1, GL_FALSE, &skybox_matrix[0][0]);
		glBindVertexArray(skybox_vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_cubemap_tex);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default


	
		
		//tile
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default
		glEnable(GL_CULL_FACE);
		glFrontFace(GL_CW);
		glCullFace(GL_FRONT);
		glm::mat4 tile_matrix = glm::mat4();
		tile_matrix = glm::scale(tile_matrix, glm::vec3(100.0f, 100.0f, 100.0f));
		glDepthFunc(GL_LEQUAL);

		tile->Use();
		tile->setInt("tile", 0);
		tile->setInt("skybox", 1);
		tile->setInt("heightmap", 2);
		//glUniform1f(glGetUniformLocation(tile->Program, "tile"), tile_cubemap_tex);
		//glUniform1f(glGetUniformLocation(skybox->Program, "skybox"), skybox_cubemap_tex);
		glUniform3f(glGetUniformLocation(tile->Program, "cameraPos"), viewerPos.x,viewerPos.y,viewerPos.z);
		glUniform1f(glGetUniformLocation(tile->Program, "amplitude"), tw->amplitude->value());
		glUniformMatrix4fv(glGetUniformLocation(tile->Program, "u_projection"), 1, GL_FALSE, &projection[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(tile->Program, "u_view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(tile->Program, "s_model"), 1, GL_FALSE, &tile_matrix[0][0]);
		glBindVertexArray(tile_vao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tile_cubemap_tex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_cubemap_tex);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, height_map_tex[count_height_map]);
		glDrawArrays(GL_TRIANGLES, 0,30);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS); // set depth function back to default
		

		
		
		//water

		glm::vec3 pointLightPositions[] = {
		glm::vec3(0.0f,  10.0f,  0.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
		};

		glm::mat4 model_matrix = glm::mat4();
		model_matrix = glm::translate(model_matrix, this->source_pos);
		model_matrix = glm::scale(model_matrix, glm::vec3(100.0f, 100.0f, 100.0f));

		if (tw->waveBrowser->value() == 1) //sin wave
		{
			this->water->Use();
			this->texture->bind(0);
		
			glUniform1f(glGetUniformLocation(this->water->Program, "time"), this->time);
			glUniform1f(glGetUniformLocation(this->water->Program, "speed"), tw->speed->value());
			glUniform1f(glGetUniformLocation(this->water->Program, "amplitude"), tw->amplitude->value());
			glUniform1f(glGetUniformLocation(this->water->Program, "waveLength"), tw->waveLength->value());

			GLint viewPosLoc = glGetUniformLocation(this->water->Program, "viewPos");
			glUniform3f(viewPosLoc, viewerPos.x, viewerPos.y, viewerPos.z);
			glUniform1f(glGetUniformLocation(this->water->Program, "material.shininess"), 32.0f);

			glUniform3f(glGetUniformLocation(this->water->Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
			glUniform3f(glGetUniformLocation(this->water->Program, "dirLight.ambient"), 0.0f, 0.0f, 0.0f);
			glUniform3f(glGetUniformLocation(this->water->Program, "dirLight.diffuse"), 0.1f, 0.1f, 0.1f);
			glUniform3f(glGetUniformLocation(this->water->Program, "dirLight.specular"), 0.5f, 0.5f, 0.5f);

			glUniform3f(glGetUniformLocation(this->water->Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
			glUniform3f(glGetUniformLocation(this->water->Program, "pointLights[0].ambient"), 0.0f, 0.0f, 0.0f);
			glUniform3f(glGetUniformLocation(this->water->Program, "pointLights[0].diff"), 0.8f, 0.8f, 0.8f);
			glUniform3f(glGetUniformLocation(this->water->Program, "pointLights[0].specular"), 1.0f, 1.0f, 1.0f);
			glUniform1f(glGetUniformLocation(this->water->Program, "pointLights[0].constant"), 1.0f);
			glUniform1f(glGetUniformLocation(this->water->Program, "pointLights[0].linear"), 0.09);
			glUniform1f(glGetUniformLocation(this->water->Program, "pointLights[0].quadratic"), 0.032);

			glUniformMatrix4fv(
				glGetUniformLocation(this->water->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);

		}
		else if (tw->waveBrowser->value() == 2) //height map
		{
			this->height_map->Use();
			height_map->setInt("u_texture", 0);
			height_map->setInt("heightMap", 1);
			height_map->setInt("u_heightMap", 2);
			height_map->setInt("tile", 3);
			height_map->setInt("ripple", 4);
			height_map->setInt("u_ripple", 5);
			height_map->setInt("skybox", 6);

			
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D,height_map_tex[count_height_map]);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, height_map_tex[count_height_map]);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_CUBE_MAP, tile_cubemap_tex);
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_2D, ripple_tex);
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, ripple_tex);
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_CUBE_MAP, skybox_cubemap_tex);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture->id);

			glUniform1f(glGetUniformLocation(this->height_map->Program, "amplitude"), tw->amplitude->value());
			glUniform1f(glGetUniformLocation(this->height_map->Program, "f_amplitude"), tw->amplitude->value());
			GLint viewPosLoc = glGetUniformLocation(this->height_map->Program, "viewPos");
			glUniform3f(viewPosLoc, viewerPos.x, viewerPos.y, viewerPos.z);
			glUniform1f(glGetUniformLocation(this->height_map->Program, "material.shininess"),100.0f);

			glUniform3f(glGetUniformLocation(this->height_map->Program, "dirLight.direction"), 0, -20.0f, 0);
			glUniform3f(glGetUniformLocation(this->height_map->Program, "dirLight.ambient"), 0, 0, 0);
			glUniform3f(glGetUniformLocation(this->height_map->Program, "dirLight.diffuse"), 0.5, 0.5, 0.5);
			glUniform3f(glGetUniformLocation(this->height_map->Program, "dirLight.specular"), 1.0, 1.0, 1.0);

			glUniform3f(glGetUniformLocation(this->height_map->Program, "pointLights[0].position"), pointLightPositions[0].x, pointLightPositions[0].y, pointLightPositions[0].z);
			glUniform3f(glGetUniformLocation(this->height_map->Program, "pointLights[0].ambient"), 0.05f, 0.05f, 0.05f);
			glUniform3f(glGetUniformLocation(this->height_map->Program, "pointLights[0].diffuse"), 0.8f, 0.8f, 0.8f);
			glUniform3f(glGetUniformLocation(this->height_map->Program, "pointLights[0].specular"), 1.0f, 1.0f, 1.0f);
			glUniform1f(glGetUniformLocation(this->height_map->Program, "pointLights[0].constant"), 1.0f);
			glUniform1f(glGetUniformLocation(this->height_map->Program, "pointLights[0].linear"), 0.09);
			glUniform1f(glGetUniformLocation(this->height_map->Program, "pointLights[0].quadratic"), 0.032);

			glUniformMatrix4fv(
				glGetUniformLocation(this->height_map->Program, "u_model"), 1, GL_FALSE, &model_matrix[0][0]);
		}
		//bind VAO
		glBindVertexArray(this->plane->vao);

		glDrawArrays(GL_TRIANGLES, 0, this->plane->element_amount);

		//unbind VAO
		glBindVertexArray(0);

		//unbind shader(switch to fixed pipeline)
		glUseProgram(0);
	

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST); 
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
		glClear(GL_COLOR_BUFFER_BIT);
		this->screen->Use();
		//glUniform1i(glGetUniformLocation(screen->Program, "frame_buffer_type"), tw->frame_buffer_type->value());
		glUniform1f(glGetUniformLocation(screen->Program, "screen_w"), w());
		glUniform1f(glGetUniformLocation(screen->Program, "screen_h"), h());
		glUniform1f(glGetUniformLocation(screen->Program, "isPixel"),tw->pixel->value());
		//glUniform1f(glGetUniformLocation(screen->Program, "t"), tw->time * 20);
		glBindVertexArray(screen_quadVAO);
		glBindTexture(GL_TEXTURE_2D, screen_textureColorbuffer);	// use the color attachment texture as the texture of the quad plane
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//unbind shader(switch to fixed pipeline)
		glUseProgram(0);
	}
}

//************************************************************************
//
// * This sets up both the Projection and the ModelView matrices
//   HOWEVER: it doesn't clear the projection first (the caller handles
//   that) - its important for picking
//========================================================================
void TrainView::
setProjection()
//========================================================================
{
	// Compute the aspect ratio (we'll need it)
	float aspect = static_cast<float>(w()) / static_cast<float>(h());

	// Check whether we use the world camp
	if (tw->worldCam->value())
		arcball.setProjection(false);
	// Or we use the top cam
	else if (tw->topCam->value()) {
		float wi, he;
		if (aspect >= 1) {
			wi = 110;
			he = wi / aspect;
		} 
		else {
			he = 110;
			wi = he * aspect;
		}

		// Set up the top camera drop mode to be orthogonal and set
		// up proper projection matrix
		glMatrixMode(GL_PROJECTION);
		glOrtho(-wi, wi, -he, he, 200, -200);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(-90,1,0,0);
	} 
	// Or do the train view or other view here
	//####################################################################
	// TODO: 
	// put code for train view projection here!	
	//####################################################################
	else {
#ifdef EXAMPLE_SOLUTION
		trainCamView(this,aspect);
#endif
	}
}

//************************************************************************
//
// * this draws all of the stuff in the world
//
//	NOTE: if you're drawing shadows, DO NOT set colors (otherwise, you get 
//       colored shadows). this gets called twice per draw 
//       -- once for the objects, once for the shadows
//########################################################################
// TODO: 
// if you have other objects in the world, make sure to draw them
//########################################################################
//========================================================================
void TrainView::drawStuff(bool doingShadows)
{
	// Draw the control points
	// don't draw the control points if you're driving 
	// (otherwise you get sea-sick as you drive through them)
	if (!tw->trainCam->value()) {
		for(size_t i=0; i<m_pTrack->points.size(); ++i) {
			if (!doingShadows) {
				if ( ((int) i) != selectedCube)
					glColor3ub(240, 60, 60);
				else
					glColor3ub(240, 240, 30);
			}
			m_pTrack->points[i].draw();
		}
	}
	//std::cout << m_pTrack->points[0].pos.x<<std::endl;
	// draw the track
	//####################################################################
	// TODO: 
	// call your own track drawing code
	//####################################################################

#ifdef EXAMPLE_SOLUTION
	drawTrack(this, doingShadows);
#endif

	// draw the train
	//####################################################################
	// TODO: 
	//	call your own train drawing code
	//####################################################################
#ifdef EXAMPLE_SOLUTION
	// don't draw the train if you're looking out the front window
	if (!tw->trainCam->value())
		drawTrain(this, doingShadows);
#endif
}

// 
//************************************************************************
//
// * this tries to see which control point is under the mouse
//	  (for when the mouse is clicked)
//		it uses OpenGL picking - which is always a trick
//########################################################################
// TODO: 
//		if you want to pick things other than control points, or you
//		changed how control points are drawn, you might need to change this
//########################################################################
//========================================================================
void TrainView::
doPick()
//========================================================================
{
	// since we'll need to do some GL stuff so we make this window as 
	// active window
	make_current();		

	// where is the mouse?
	int mx = Fl::event_x(); 
	int my = Fl::event_y();

	// get the viewport - most reliable way to turn mouse coords into GL coords
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Set up the pick matrix on the stack - remember, FlTk is
	// upside down!
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity ();

	gluPickMatrix((double)mx, (double)(viewport[3]-my), 
						5, 5, viewport);

	// now set up the projection
	setProjection();

	// now draw the objects - but really only see what we hit
	GLuint buf[100];
	glSelectBuffer(100,buf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(0);

	// draw the cubes, loading the names as we go
	for(size_t i=0; i<m_pTrack->points.size(); ++i) {
		glLoadName((GLuint) (i+1));
		m_pTrack->points[i].draw();
	}

	// go back to drawing mode, and see how picking did
	int hits = glRenderMode(GL_RENDER);
	if (hits) {
		// warning; this just grabs the first object hit - if there
		// are multiple objects, you really want to pick the closest
		// one - see the OpenGL manual 
		// remember: we load names that are one more than the index
		selectedCube = buf[3]-1;
	} else // nothing hit, nothing selected
		selectedCube = -1;

	printf("Selected Cube %d\n",selectedCube);
}

void TrainView::setUBO()
{
	float wdt = this->pixel_w();
	float hgt = this->pixel_h();

	glm::mat4 view_matrix;
	glGetFloatv(GL_MODELVIEW_MATRIX, &view_matrix[0][0]);
	/*HMatrix view_matrix; 
	this->arcball.getMatrix(view_matrix);*/

	glm::mat4 projection_matrix;
	glGetFloatv(GL_PROJECTION_MATRIX, &projection_matrix[0][0]);
	//projection_matrix = glm::perspective(glm::radians(this->arcball.getFoV()), (GLfloat)wdt / (GLfloat)hgt, 0.01f, 1000.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, this->commom_matrices->ubo);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &projection_matrix[0][0]);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &view_matrix[0][0]);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}