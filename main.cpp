#include <GL/glew.h>	//must be before glfw, because most header files we need are in glew
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS


struct object_struct{
	unsigned int program;
	unsigned int vao;
	unsigned int vbo[4];
	unsigned int texture;
	glm::mat4 model;
	object_struct(): model(glm::mat4(1.0f)){}
} ;

std::vector<object_struct> objects;//vertex array object,vertex buffer object and texture(color) for objs
unsigned int program, program2;
std::vector<int> indicesCount;//Number of indice of objs

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

static unsigned int setup_shader(const char *vertex_shader, const char *fragment_shader)
{
	GLuint vs=glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const GLchar**)&vertex_shader, nullptr);

	glCompileShader(vs);	//compile vertex shader

	int status, maxLength;
	char *infoLog=nullptr;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);		//get compile status
	if(status==GL_FALSE)								//if compile error
	{
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);	//get error message length

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];
		
		glGetShaderInfoLog(vs, maxLength, &maxLength, infoLog);		//get error message

		fprintf(stderr, "Vertex Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}
	//	for fragment shader --> same as vertex shader
	GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar**)&fragment_shader, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(fs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Fragment Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	unsigned int program=glCreateProgram();
	// Attach our shaders to our program
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if(status==GL_FALSE)
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);


		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];
		glGetProgramInfoLog(program, maxLength, NULL, infoLog);

		glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Link Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}
	return program;
}

static std::string readfile(const char *filename)
{
	std::ifstream ifs(filename);
	if(!ifs)
		exit(EXIT_FAILURE);
	return std::string( (std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
}

// mini bmp loader written by HSU YOU-LUN
static unsigned char *load_bmp(const char *bmp, unsigned int *width, unsigned int *height, unsigned short int *bits)
{
	unsigned char *result=nullptr;
	FILE *fp = fopen(bmp, "rb");
	if(!fp)
		return nullptr;
	char type[2];
	unsigned int size, offset;
	// check for magic signature	
	fread(type, sizeof(type), 1, fp);
	if(type[0]==0x42 || type[1]==0x4d){
		fread(&size, sizeof(size), 1, fp);
		// ignore 2 two-byte reversed fields
		fseek(fp, 4, SEEK_CUR);
		fread(&offset, sizeof(offset), 1, fp);
		// ignore size of bmpinfoheader field
		fseek(fp, 4, SEEK_CUR);
		fread(width, sizeof(*width), 1, fp);
		fread(height, sizeof(*height), 1, fp);
		// ignore planes field
		fseek(fp, 2, SEEK_CUR);
		fread(bits, sizeof(*bits), 1, fp);
		unsigned char *pos = result = new unsigned char[size-offset];
		fseek(fp, offset, SEEK_SET);
		while(size-ftell(fp)>0)
			pos+=fread(pos, 1, size-ftell(fp), fp);
	}
	fclose(fp);
	return result;
}

static int add_obj(unsigned int program, const char *filename,const char *texbmp)
{
	object_struct new_node;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, filename);

	if (!err.empty()||shapes.size()==0)
	{
		std::cerr<<err<<std::endl;
		exit(1);
	}

	glGenVertexArrays(1, &new_node.vao);
	glGenBuffers(4, new_node.vbo);
	glGenTextures(1, &new_node.texture);

	glBindVertexArray(new_node.vao);

	// Upload postion array
	glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.positions.size(),
			shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	if(shapes[0].mesh.texcoords.size()>0)
	{

		// Upload texCoord array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.texcoords.size(),
				shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
		//glActiveTexture(GL_TEXTURE0);	//Activate texture unit before binding texture, used when having multiple texture
		glBindTexture( GL_TEXTURE_2D, new_node.texture);
		unsigned int width, height;
		unsigned short int bits;
		unsigned char *bgr=load_bmp(texbmp, &width, &height, &bits);
		GLenum format = (bits == 24? GL_BGR: GL_BGRA);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, bgr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glGenerateMipmap(GL_TEXTURE_2D);
		delete [] bgr;
	}

	if(shapes[0].mesh.normals.size()>0)
	{
		// Upload normal array
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.normals.size(),
				shapes[0].mesh.normals.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// Setup index buffer for glDrawElements
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_node.vbo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*shapes[0].mesh.indices.size(),
			shapes[0].mesh.indices.data(), GL_STATIC_DRAW);

	indicesCount.push_back(shapes[0].mesh.indices.size());

	glBindVertexArray(0);

	new_node.program = program;

	objects.push_back(new_node);
	return objects.size()-1;
}

static void releaseObjects()
{
	for(int i=0;i<objects.size();i++){
		glDeleteVertexArrays(1, &objects[i].vao);
		glDeleteTextures(1, &objects[i].texture);
		glDeleteBuffers(4, objects[i].vbo);
	}
	glDeleteProgram(program);
}

static void setUniformMat4(unsigned int program, const std::string &name, const glm::mat4 &mat)
{
	// This line can be ignore. But, if you have multiple shader program
	// You must check if currect binding is the one you want
	glUseProgram(program);
	GLint loc=glGetUniformLocation(program, name.c_str());
	if(loc==-1) return;

	// mat4 of glm is column major, same as opengl
	// we don't need to transpose it. so..GL_FALSE
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

static void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for(int i=0;i<objects.size();i++){
		//VAO VBO are binded in add_Object function
		glUseProgram(objects[i].program);
		glBindVertexArray(objects[i].vao);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture);
		//you should send some data to shader here
		GLint modelLoc = glGetUniformLocation(objects[i].program, "model");
		glm::mat4 mPosition;
		
		//mPosition = glm::translate(mPosition, modelPositions[i]);
		if (i == 1) {		//for earth
			float radiusX = 20.0f;
			float radiusZ = 30.0f;
			float X = sin(glfwGetTime()) * radiusX;
			float Z = cos(glfwGetTime()) * radiusZ;
			mPosition = glm::translate(mPosition, glm::vec3(X, 0, Z));
			mPosition = glm::rotate(mPosition, (float)glfwGetTime()*5.0f, glm::vec3(0.0f, 1.0f, 0.0f));
			//mPosition = rotation then translation
		}
		else {				//for sun
			mPosition = glm::rotate(mPosition, (float)glfwGetTime()*1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mPosition));
		//std::cout << i<< std::endl;
		glDrawElements(GL_TRIANGLES, indicesCount[i], GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);
}

int main(int argc, char *argv[])
{
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	// OpenGL 3.3, Mac OS X is reported to have some problem. However I don't have Mac to test
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);		//set hint to glfwCreateWindow, (target, hintValue)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//hint--> window not resizable,  explicit use core-profile,  opengl version 3.3
	// For Mac OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(800, 600, "Simple Example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);	//set current window as main window to focus

	// This line MUST put below glfwMakeContextCurrent
	glewExperimental = GL_TRUE;		//tell glew to use more modern technique for managing OpenGL functionality
	glewInit();

	// Enable vsync
	glfwSwapInterval(1);

	// Setup input callback
	glfwSetKeyCallback(window, key_callback);	//set key event handler

	// load shader program
	program = setup_shader(readfile("vs.txt").c_str(), readfile("fs.txt").c_str());
	program2 = setup_shader(readfile("vs.txt").c_str(), readfile("fs.txt").c_str());

	int sun = add_obj(program, "sun.obj","sun.bmp");
	int earth = add_obj(program, "earth.obj","earth.bmp");

	glEnable(GL_DEPTH_TEST);
	// prevent faces rendering to the front while they're behind other faces. 
	 glCullFace(GL_BACK);
	// discard back-facing trangle
	// Enable blend mode for billboard
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	 setUniformMat4(program, "vp", glm::perspective(glm::radians(45.0f), 640.0f / 480, 1.0f, 100.f) *
		 glm::lookAt(glm::vec3(40.0f), glm::vec3(), glm::vec3(0, 1, 0)) * glm::mat4(1.0f));
	 setUniformMat4(program2, "vp", glm::mat4(1.0));

	//setUniformMat4(program2, "vp", glm::mat4(1.0));
	glm::mat4 tl=glm::translate(glm::mat4(),glm::vec3(15.0f,0.0f,0.0));
	//glm::mat4 rot;
	//glm::mat4 rev;

	float last, start;
	last = start = glfwGetTime();
	int fps=0;
	objects[sun].model = glm::scale(glm::mat4(1.0f), glm::vec3(0.85f));
	while (!glfwWindowShouldClose(window))
	{//program will keep draw here until you close the window
		float delta = glfwGetTime() - start;
		
		render();
		glfwSwapBuffers(window);	//swap the color buffer and show it as output to the screen.
		glfwPollEvents();			//check if there is any event being triggered
		fps++;
		if(glfwGetTime() - last > 1.0)
		{
			std::cout<<(double)fps/(glfwGetTime()-last)<<std::endl;
			fps = 0;
			last = glfwGetTime();
		}
	}

	releaseObjects();
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}
