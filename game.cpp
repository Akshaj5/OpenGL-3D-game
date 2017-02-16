#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <ao/ao.h>
#include <mpg123.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define BITS 8
using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct COLOR {
    float r;
    float g;
    float b;
};
typedef struct COLOR COLOR;


struct Sprite {
    string name;
    COLOR color;
    float x,y,z;
    VAO* object;
    int status;
    float height,width,depth;
    float x_change,y_change,z_change;
    float angle; //Current Angle (Actual rotated angle of the object)
    float radius;
    int fixed;
    float flag ; //Value from 0 to 1
    int isRotating;
    int direction; //0 for clockwise and 1 for anticlockwise for animation
    float remAngle; //the remaining angle to finish animation
    int isMovingAnim;
    int dx;
    int dy;
};
typedef struct Sprite Sprite;


struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct GLMatrices Matrices1;

map <string, Sprite> tiles;
map <string, Sprite> ltiles;
map <string, Sprite> block;
map <string, Sprite> switches;
map <string, Sprite> point1;
map <string, Sprite> point2;
map <string, Sprite> point3;
map <string, Sprite> sec1;
map <string, Sprite> sec2;
map <string, Sprite> min1;
map <string, Sprite> min2;
map <string, Sprite> label;
map <string, Sprite> endlabel;


glm::mat4 rotateblock = glm::mat4(1.0f);


int seconds=0;
float zoom_camera = 1;
float game_over=0;


GLuint programID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for(int i=0; i<numVertices;i++){
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

int level =0;
int moves =0;


float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

double launch_angle=0;
int keyboard_pressed=0;

/* Executed when a regular key is pressed/released/held-down 
  Prefered for Keyboard events */
int key_pressed_up=0;
int key_pressed_down=0;
int key_pressed_left=0;
int key_pressed_right=0;
int key_pressed_right_alt=0;
int key_pressed_right_control=0;

void mousescroll(GLFWwindow* window, double xoffset, double yoffset){
    Matrices.projection = glm::perspective(glm::radians(45.0f),(float)1000/(float)800, 0.1f, 5000.0f);
}

int key_pressed_T =0;
int key_pressed_H =0;
int key_pressed_F =0;
int key_pressed_B =0;

float eye_x = -300;
float eye_y = 1000;
float eye_z = 600;
float target_x = -200; 
float target_y = -100;
float target_z = 0;


mpg123_handle *mh;
unsigned char *buffer;
size_t buffer_size;
size_t done;
int err;

int driver;
ao_device *dev;

ao_sample_format format;
int channels, encoding;
long rate;

void audio_init() {
    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = 2500;
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    /* open the file and get the decoding format */
    mpg123_open(mh, "./background.mp3");
    mpg123_getformat(mh, &rate, &channels, &encoding);

    /* set the output format and open the output device */
    format.bits = mpg123_encsize(encoding) * BITS;
    format.rate = rate;
    format.channels = channels;
    format.byte_format = AO_FMT_NATIVE;
    format.matrix = 0;
    dev = ao_open_live(driver, &format, NULL);
}

void audio_play() {
    /* decode and play */
    if (mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK)
        ao_play(dev, (char*) buffer, done);
    else mpg123_seek(mh, 0, SEEK_SET);
}

void audio_close() {
    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
}


void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods){
    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_B:
                key_pressed_B =1;
                key_pressed_T =0;
                key_pressed_F =0; 
                break;
            case GLFW_KEY_F:
                key_pressed_F =1;
                key_pressed_T =0;
                key_pressed_B =0;
                break;
            case GLFW_KEY_UP:
                key_pressed_up = 1;
                moves++;
                break;
            case GLFW_KEY_DOWN:
                key_pressed_down =1;
                moves++;
                break;
            case GLFW_KEY_RIGHT:
                key_pressed_right =1;
                moves++;
                break;
            case GLFW_KEY_LEFT:
                key_pressed_left =1;
                moves++;
                break;
            case GLFW_KEY_T:
                key_pressed_T = 1;
                key_pressed_B =0;
                key_pressed_F =0;
                break;
            case GLFW_KEY_H:
                eye_x = -300;
                eye_y = 1000;
                eye_z = 600;
                target_x = -200; 
                target_y = -100;
                target_z = 0;
                key_pressed_T =0;
                key_pressed_F =0;
                key_pressed_B =0;
                break;
            case GLFW_KEY_RIGHT_ALT:
                key_pressed_right_alt=0;
                break;
            case GLFW_KEY_RIGHT_CONTROL:
                key_pressed_right_control = 0;
                break;
            case GLFW_KEY_A:
               // initKeyboard();
                if(launch_angle<90-10)
                    launch_angle+=10;
                else
                    launch_angle=90;
                break;
            case GLFW_KEY_D:
                //initKeyboard();
                if(launch_angle>-80)
                    launch_angle-=10;
                else
                    launch_angle=-90;
                break;
            case GLFW_KEY_SPACE:
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_UP:
                //key_pressed_up = 1;
                break;
            case GLFW_KEY_F:
               // key_pressed_F = 1;
                break;
            case GLFW_KEY_RIGHT_ALT:
                key_pressed_right_alt=1;
                break;
            case GLFW_KEY_RIGHT_CONTROL:
                key_pressed_right_control = 1;
                break;
            case GLFW_KEY_RIGHT:
              //  key_pressed_right = 1;
                break;
            case GLFW_KEY_LEFT:
               // key_pressed_left = 1;
                break;    
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}
int mouse_clicked=0;
int right_mouse_clicked=0;

void mouse_click(){
    mouse_clicked=1;
    keyboard_pressed=0;
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_PRESS) {
                mouse_click();
            }
            if (action == GLFW_RELEASE) {
                //do;
            }
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_PRESS) {
                right_mouse_clicked=1;
            }
            if (action == GLFW_RELEASE) {
                right_mouse_clicked=0;
            }
            break;
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

    Matrices.projection = glm::perspective(fov,(float)fbwidth/(float)fbheight, 0.1f, 5000.0f);
}

void createCube(string name,COLOR top,COLOR bottom,COLOR right,COLOR left,COLOR far,COLOR near,float x, float y ,float z,float width,float height,float depth,string component){

    float w=width/2,h=height/2,d=depth/2;
    GLfloat vertex_buffer_data []={
        -w,-h,-d,
        -w,h,-d,
        w,h,-d,

        w,h,-d,
        w,-h,-d,
        -w,-h,-d,

        -w,-h,d,
        -w,h,d,
        w,h,d,

        w,h,d,
        w,-h,d,
        -w,-h,d,

        -w,h,d,
        -w,h,-d,
        -w,-h,d,

        -w,-h,d,
        -w,-h,-d,
        -w,h,-d,

        w,h,d,
        w,-h,d,
        w,h,-d,

        w,h,-d,
        w,-h,-d,
        w,-h,d,

        -w,h,d,
        -w,h,-d,
        w,h,d,

        w,h,d,
        w,h,-d,
        -w,h,-d,

        -w,-h,d,
        -w,-h,-d,
        w,-h,d,

        w,-h,d,
        w,-h,-d,
        -w,-h,-d        
    };


    GLfloat color_buffer_data [] = {
        far.r,far.g,far.b,
        far.r,far.g,far.b,
        far.r,far.g,far.b,
        
        far.r,far.g,far.b,
        far.r,far.g,far.b,
        far.r,far.g,far.b,

        near.r,near.g,near.b,
        near.r,near.g,near.b,
        near.r,near.g,near.b,

        near.r,near.g,near.b,
        near.r,near.g,near.b,
        near.r,near.g,near.b,

        left.r,left.g,left.b,
        left.r,left.g,left.b,
        left.r,left.g,left.b,

        left.r,left.g,left.b,
        left.r,left.g,left.b,
        left.r,left.g,left.b,

        right.r,right.g,right.b,
        right.r,right.g,right.b,
        right.r,right.g,right.b,

        right.r,right.g,right.b,
        right.r,right.g,right.b,
        right.r,right.g,right.b,

        top.r,top.g,top.b,
        top.r,top.g,top.b,
        top.r,top.g,top.b,

        top.r,top.g,top.b,
        top.r,top.g,top.b,
        top.r,top.g,top.b,

        bottom.r,bottom.g,bottom.b,
        bottom.r,bottom.g,bottom.b,
        bottom.r,bottom.g,bottom.b,

        bottom.r,bottom.g,bottom.b,
        bottom.r,bottom.g,bottom.b,
        bottom.r,bottom.g,bottom.b
    };

    VAO *cube = create3DObject(GL_TRIANGLES,36,vertex_buffer_data,color_buffer_data,GL_FILL);
    Sprite vishsprite = {};
    vishsprite.color = top;
    vishsprite.name = name;
    vishsprite.object = cube;
    vishsprite.x=x;
    vishsprite.y=y;
    vishsprite.z=z;
    vishsprite.height=height;
    vishsprite.width=width;
    vishsprite.depth=depth;
    vishsprite.status=1;
    vishsprite.x_change=x;
    vishsprite.y_change=y;
    vishsprite.z_change=z;
    vishsprite.fixed=0;
    vishsprite.flag=0;

    if(component=="tiles")
        tiles[name]=vishsprite;
    else if(component=="ltiles")
        ltiles[name]=vishsprite;
    else if(component=="block")
        block[name]=vishsprite;
}

void createRectangle1(string name, COLOR colorA, COLOR colorB, COLOR colorC, COLOR colorD, float x, float y, float height, float width, string component){
    // GL3 accepts only Triangles. Quads are not supported
    float w=width/2,h=height/2;
    GLfloat vertex_buffer_data [] = {
        -w,-h,0, // vertex 1
        -w,h,0, // vertex 2
        w,h,0, // vertex 3

        w,h,0, // vertex 3
        w,-h,0, // vertex 4
        -w,-h,0  // vertex 1
    };

    GLfloat color_buffer_data [] = {
        colorA.r,colorA.g,colorA.b, // color 1
        colorB.r,colorB.g,colorB.b, // color 2
        colorC.r,colorC.g,colorC.b, // color 3

        colorC.r,colorC.g,colorC.b, // color 4
        colorD.r,colorD.g,colorD.b, // color 5
        colorA.r,colorA.g,colorA.b // color 6
    };

    // create3DObject creates and returns a handle to a VAO that can be used later
    VAO *rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
    Sprite vishsprite = {};
    vishsprite.color = colorA;
    vishsprite.name = name;
    vishsprite.object = rectangle;
    vishsprite.x=x;
    vishsprite.y=y;
    vishsprite.height=height;
    vishsprite.width=width;
    vishsprite.status=1;
    vishsprite.fixed=0;
    vishsprite.angle=launch_angle;
    vishsprite.radius=(sqrt(height*height+width*width))/2;
    vishsprite.flag=0;

    if(component=="block")
        block[name]=vishsprite;
    else if(component=="point1")
        point1[name]=vishsprite;
    else if(component=="point2")
        point2[name]=vishsprite;
    else if(component=="point3")
        point3[name]=vishsprite;
    else if(component=="sec1")
        sec1[name]=vishsprite;
    else if(component=="sec2")
        sec2[name]=vishsprite;
    else if(component=="min1")
        min1[name]=vishsprite;
    else if(component=="min2")
        min2[name]=vishsprite;
    else if(component=="label")
        label[name]=vishsprite;
}

void createCircle (string name, COLOR color, float x,float y,float z, float r, int NoOfParts, string component, int fill){
    int parts = NoOfParts;
    float radius = r;
    GLfloat vertex_buffer_data[parts*9];
    GLfloat color_buffer_data[parts*9];
    int i,j;
    float angle=(2*M_PI/parts);
    float current_angle = 0;
    for(i=0;i<parts;i++){
        for(j=0;j<3;j++){
            color_buffer_data[i*9+j*3]=color.r;
            color_buffer_data[i*9+j*3+1]=color.g;
            color_buffer_data[i*9+j*3+2]=color.b;
        }
        vertex_buffer_data[i*9]=0;
        vertex_buffer_data[i*9+1]=1;
        vertex_buffer_data[i*9+2]=0;
        vertex_buffer_data[i*9+3]=radius*cos(current_angle);
        vertex_buffer_data[i*9+4]=1;
        vertex_buffer_data[i*9+5]=radius*sin(current_angle);
        vertex_buffer_data[i*9+6]=radius*cos(current_angle+angle);
        vertex_buffer_data[i*9+7]=1;
        vertex_buffer_data[i*9+8]=radius*sin(current_angle+angle);
        current_angle+=angle;
    }
    VAO* circle;
    if(fill==1)
        circle = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_FILL);
    else
        circle = create3DObject(GL_TRIANGLES, (parts*9)/3, vertex_buffer_data, color_buffer_data, GL_LINE);
    Sprite vishsprite = {};
    vishsprite.color = color;
    vishsprite.name = name;
    vishsprite.object = circle;
    vishsprite.x=x;
    vishsprite.y=y;
    vishsprite.z=z;
    vishsprite.height=2*r; //Height of the sprite is 2*r
    vishsprite.width=2*r; //Width of the sprite is 2*r
    vishsprite.status=1;
    vishsprite.radius=r;
    vishsprite.fixed=0;

    if(component=="switch")
        switches[name]=vishsprite;
}


/* Render the scene with openGL */
/* Edit this function according to your assignment */

double mouse_pos_x, mouse_pos_y;
double new_mouse_pos_x, new_mouse_pos_y;
float angle=0;
float xpos;
float ypos = 2720;
int num = 1;
int flag =0;
int gir1 =0;
int gir2 =0;
int score =0;
int gameover=0;
int count=0;
float downfall =.1;
float downtile = 0;
int tileflag =0;
float tileX;
float tileZ;
int switch1 =0;
int switch2 =0;
int sig=0;


void draw (GLFWwindow* window, int width, int height)
{

    if(gameover ==1)
        return;

    if(key_pressed_T == 1){
        eye_x = block["block"].x_change;
        eye_y = 1300;
        eye_z = block["block"].z_change;
        target_x = block["block"].x_change;
        target_y = 0;
        target_z = block["block"].z_change - 10;
    }
    else if(key_pressed_F ==1){
        eye_x = block["block"].x_change ;
        eye_y = block["block"].y_change +300;
        eye_z = block["block"].z_change +300;
        target_x = block["block"].x_change;
        target_y = block["block"].y_change;
        target_z = block["block"].z_change;
    }
    else if(key_pressed_B ==1){
        eye_x = block["block"].x_change;
        eye_y = block["block"].y_change + 300; 
        eye_z = block["block"].z_change + 50;
        target_x = block["block"].x_change;
        target_y = block["block"].y_change;
        target_z = block["block"].z_change - 200;   
    }


    glClearColor(1.0f,1.0f,1.0f,1.0f);//set background color
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram (programID);
      
    int fbwidth=width, fbheight=height; 
    glm::vec3 eye ( eye_x ,eye_y, eye_z );
    glm::vec3 target (target_x,target_y, target_z);
    glm::vec3 up (0, 1, 0);
    Matrices.view = glm::lookAt(eye, target, up);


    for(map<string,Sprite>::iterator it1=point1.begin();it1!=point1.end();it1++){
        point1[it1->first].status=0;
    }
    for(map<string,Sprite>::iterator it1=point2.begin();it1!=point2.end();it1++){
        point2[it1->first].status=0;
    }
    for(map<string,Sprite>::iterator it1=point3.begin();it1!=point3.end();it1++){
        point3[it1->first].status=0;
    }
    for(map<string,Sprite>::iterator it1=sec1.begin();it1!=sec1.end();it1++){
        sec1[it1->first].status=0;
    }
    for(map<string,Sprite>::iterator it1=sec2.begin();it1!=sec2.end();it1++){
        sec2[it1->first].status=0;
    }
    for(map<string,Sprite>::iterator it1=min1.begin();it1!=min1.end();it1++){
        min1[it1->first].status=0;
    }
    for(map<string,Sprite>::iterator it1=min2.begin();it1!=min2.end();it1++){
        min2[it1->first].status=0;
    }


    int time = abs(seconds % 60);

    int two;

    two = time%10;
    if(two == 0 || two == 2 ||two == 3 ||two == 5 ||two == 6 ||two == 7 ||two == 8 ||two == 9){
    sec1["seg1"].status=1;
    }
    if(two == 0 || two == 1 ||two == 2 ||two == 3 ||two == 4 ||two == 7 ||two == 8 ||two == 9){
    sec1["seg2"].status=1;
    }
    if(two == 0 || two == 1 ||two == 3 ||two == 4 ||two == 5  ||two == 6 ||two == 7 ||two == 8 ||two == 9){
    sec1["seg3"].status=1;
    }
    if(two == 0 || two == 2 ||two == 3 ||two == 5 ||two == 6 ||two == 8 ||two == 9){
    sec1["seg4"].status=1;
    }
    if(two == 0 || two == 2 ||two == 6 ||two == 8){
    sec1["seg5"].status=1;
    }
    if(two == 0 || two == 4 ||two == 5 ||two == 6 ||two == 8 ||two == 9){
    sec1["seg6"].status=1;
    }
    if(two == 2 ||two == 3  ||two == 4 ||two == 5 ||two == 6 ||two == 8 ||two == 9){
    sec1["seg7"].status=1;
    }


    time = time/10;
    two = time%10;

    if(two == 0 || two == 2 ||two == 3 ||two == 5 ||two == 6 ||two == 7 ||two == 8 ||two == 9){
    sec2["seg1"].status=1;
    }
    if(two == 0 || two == 1 ||two == 2 ||two == 3 ||two == 4 ||two == 7 ||two == 8 ||two == 9){
    sec2["seg2"].status=1;
    }
    if(two == 0 || two == 1 ||two == 3 ||two == 4 ||two == 5  ||two == 6 ||two == 7 ||two == 8 ||two == 9){
    sec2["seg3"].status=1;
    }
    if(two == 0 || two == 2 ||two == 3 ||two == 5 ||two == 6 ||two == 8 ||two == 9){
    sec2["seg4"].status=1;
    }
    if(two == 0 || two == 2 ||two == 6 ||two == 8){
    sec2["seg5"].status=1;
    }
    if(two == 0 || two == 4 ||two == 5 ||two == 6 ||two == 8 ||two == 9){
    sec2["seg6"].status=1;
    }
    if(two == 2 ||two == 3  ||two == 4 ||two == 5 ||two == 6 ||two == 8 ||two == 9){
    sec2["seg7"].status=1;
    }


    for(map<string,Sprite>::iterator it1=sec1.begin();it1!=sec1.end();it1++){
        string current = it1->first; 

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        glm::mat4 MVP;  // MVP = Projection * View * Model

        if(sec1[current].status==0)
        continue;

        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(sec1[current].x,sec1[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(sec1[current].object);
    }


    for(map<string,Sprite>::iterator it1=sec2.begin();it1!=sec2.end();it1++){
        string current = it1->first; 

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        glm::mat4 MVP;  // MVP = Projection * View * Model

        if(sec2[current].status==0)
        continue;

        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(sec2[current].x,sec2[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(sec2[current].object);
    }


    int time1 = abs(seconds/60);

    int the = time1%10;

    if(the == 0 || the == 2 ||the == 3 ||the == 5 ||the == 6 ||the == 7 ||the == 8 ||the == 9){
    
    min1["seg1"].status=1;
    }
    if(the == 0 || the == 1 ||the == 2 ||the == 3 ||the == 4 ||the == 7 ||the == 8 ||the == 9){
    
    min1["seg2"].status=1;
    }
    if(the == 0 || the == 1 ||the == 3 ||the == 4 ||the == 5  ||the == 6 ||the == 7 ||the == 8 ||the == 9){
    
    min1["seg3"].status=1;
    }
    if(the == 0 || the == 2 ||the == 3 ||the == 5 ||the == 6 ||the == 8 ||the == 9){
    
    min1["seg4"].status=1;
    }
    if(the == 0 || the == 2 ||the == 6 ||the == 8){
    
    min1["seg5"].status=1;
    }
    if(the == 0 || the == 4 ||the == 5 ||the == 6 ||the == 8 ||the == 9){
    
    min1["seg6"].status=1;
    }
    if(the == 2 ||the == 3  ||the == 4 ||the == 5 ||the == 6 ||the == 8 ||the == 9){
    
    min1["seg7"].status=1;
    }

    time1 = time1/10;
    the=time1%10;

    if(the == 0 || the == 2 ||the == 3 ||the == 5 ||the == 6 ||the == 7 ||the == 8 ||the == 9){
    min2["seg1"].status=1;
    }
    if(the == 0 || the == 1 ||the == 2 ||the == 3 ||the == 4 ||the == 7 ||the == 8 ||the == 9){
    min2["seg2"].status=1;
    }
    if(the == 0 || the == 1 ||the == 3 ||the == 4 ||the == 5  ||the == 6 ||the == 7 ||the == 8 ||the == 9){
    min2["seg3"].status=1;
    }
    if(the == 0 || the == 2 ||the == 3 ||the == 5 ||the == 6 ||the == 8 ||the == 9){
    min2["seg4"].status=1;
    }
    if(the == 0 || the == 2 ||the == 6 ||the == 8){
    min2["seg5"].status=1;
    }
    if(the == 0 || the == 4 ||the == 5 ||the == 6 ||the == 8 ||the == 9){
    min2["seg6"].status=1;
    }
    if(the == 2 ||the == 3  ||the == 4 ||the == 5 ||the == 6 ||the == 8 ||the == 9){
    min2["seg7"].status=1;
    }


    for(map<string,Sprite>::iterator it1=min1.begin();it1!=min1.end();it1++){
        string current = it1->first; 

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        glm::mat4 MVP;  // MVP = Projection * View * Model

        if(min1[current].status==0)
        continue;

        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(min1[current].x,min1[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(min1[current].object);
    }

    for(map<string,Sprite>::iterator it1=min2.begin();it1!=min2.end();it1++){
        string current = it1->first; 

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        glm::mat4 MVP;  // MVP = Projection * View * Model

        if(min2[current].status==0)
        continue;

        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(min2[current].x,min2[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(min2[current].object);
    }


    for(map<string,Sprite>::iterator it1=label.begin();it1!=label.end();it1++){
        string current = it1->first; 

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        glm::mat4 MVP;  // MVP = Projection * View * Model

        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(label[current].x,label[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(label[current].object);
    }

    int poi = abs(moves);

    int one;

    one = poi%10;
    if(one == 0 || one == 2 ||one == 3 ||one == 5 ||one == 6 ||one == 7 ||one == 8 ||one == 9){
    point3["seg1"].status=1;
    }
    if(one == 0 || one == 1 ||one == 2 ||one == 3 ||one == 4 ||one == 7 ||one == 8 ||one == 9){
    point3["seg2"].status=1;
    }
    if(one == 0 || one == 1 ||one == 3 ||one == 4 ||one == 5  ||one == 6 ||one == 7 ||one == 8 ||one == 9){
    point3["seg3"].status=1;
    }
    if(one == 0 || one == 2 ||one == 3 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point3["seg4"].status=1;
    }
    if(one == 0 || one == 2 ||one == 6 ||one == 8){
    point3["seg5"].status=1;
    }
    if(one == 0 || one == 4 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point3["seg6"].status=1;
    }
    if(one == 2 ||one == 3  ||one == 4 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point3["seg7"].status=1;
    }

    poi =poi/10;
    one =poi%10;
    if(one == 0 || one == 2 ||one == 3 ||one == 5 ||one == 6 ||one == 7 ||one == 8 ||one == 9){
    point2["seg1"].status=1;
    }
    if(one == 0 || one == 1 ||one == 2 ||one == 3 ||one == 4 ||one == 7 ||one == 8 ||one == 9){
    point2["seg2"].status=1;
    }
    if(one == 0 || one == 1 ||one == 3 ||one == 4 ||one == 5  ||one == 6 ||one == 7 ||one == 8 ||one == 9){
    point2["seg3"].status=1;
    }
    if(one == 0 || one == 2 ||one == 3 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point2["seg4"].status=1;
    }
    if(one == 0 || one == 2 ||one == 6 ||one == 8){
    point2["seg5"].status=1;
    }
    if(one == 0 || one == 4 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point2["seg6"].status=1;
    }
    if(one == 2 || one == 3 ||one == 4 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point2["seg7"].status=1;
    }


    poi =poi/10;
    one =poi%10;
    if(one == 0 || one == 2 ||one == 3 ||one == 5 ||one == 6 ||one == 7 ||one == 8 ||one == 9){
    point1["seg1"].status=1;
    }
    if(one == 0 || one == 1 ||one == 2 ||one == 3 ||one == 4 ||one == 7 ||one == 8 ||one == 9){
    point1["seg2"].status=1;
    }
    if(one == 0 || one == 1 ||one == 3 ||one == 4 ||one == 5  ||one == 6 ||one == 7 ||one == 8 ||one == 9){
    point1["seg3"].status=1;
    }
    if(one == 0 || one == 2 ||one == 3 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point1["seg4"].status=1;
    }
    if(one == 0 || one == 2 ||one == 6 ||one == 8){
    point1["seg5"].status=1;
    }
    if(one == 0 || one == 4 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point1["seg6"].status=1;
    }
    if(one == 2 || one == 3 ||one == 4 ||one == 5 ||one == 6 ||one == 8 ||one == 9){
    point1["seg7"].status=1;
    }


    for(map<string,Sprite>::iterator it1=point1.begin();it1!=point1.end();it1++){
        string current = it1->first; 

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        glm::mat4 MVP;  // MVP = Projection * View * Model

        if(point1[current].status==0)
        continue;

        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(point1[current].x,point1[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(point1[current].object);
    }

    for(map<string,Sprite>::iterator it1=point2.begin();it1!=point2.end();it1++){
        string current = it1->first; 
        glm::mat4 MVP;  // MVP = Projection * View * Model

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        if(point2[current].status==0)
        continue;
        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(point2[current].x,point2[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(point2[current].object);
    }

    for(map<string,Sprite>::iterator it1=point3.begin();it1!=point3.end();it1++){
        string current = it1->first; 
        glm::mat4 MVP;  // MVP = Projection * View * Model

        glm::vec3 eye ( 0 ,0, 5 );
        glm::vec3 target (0,0, 0);
        glm::vec3 up (0, 1, 0);
        Matrices1.view = glm::lookAt(eye, target, up);

        Matrices1.projection = glm::ortho((float)(-400.0f), (float)(400.0f), (float)(-300.0f), (float)(300.0f), 0.1f, 500.0f);

        glm::mat4 VP = Matrices1.projection * Matrices1.view;

        if(point3[current].status==0)
        continue;
        Matrices.model = glm::mat4(1.0f);

        /* Render your scene */
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(point3[current].x,point3[current].y, 0.0f)); // glTranslatef
        ObjectTransform=translateObject;
        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(point3[current].object);
    }

    GLfloat fov = M_PI/4;
    Matrices.projection = glm::perspective(fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 50000.0f);


    glm::mat4 VP = Matrices.projection * Matrices.view;
    

    glm::mat4 MVP;	// MVP = Projection * View * Model

    // Load identity to model matrix
    Matrices.model = glm::mat4(1.0f);

    if(mouse_clicked==1) {
        double mouse_x_cur;
        double mouse_y_cur;
        glfwGetCursorPos(window,&mouse_x_cur,&mouse_y_cur);
    }

    if(key_pressed_up ==1 && flag ==0){
        if(block["block"].direction ==0){
             glm::mat4 translateObject = glm::translate (glm::vec3(0,0,-(block["block"].z_change - 30)));
             glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(1,0,0));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(0,0,block["block"].z_change -30));

             rotateblock = translateObject1*rotate*translateObject*rotateblock;
             block["block"].x_change += 0;
             block["block"].z_change -= 90.0;
             block["block"].y_change -= 30.0;
             block["block"].direction =2;
        }
        else if(block["block"].direction==1){
             glm::mat4 translateObject = glm::translate (glm::vec3(0,0,-(block["block"].z_change - 30)));
             glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(1,0,0));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(0,0,block["block"].z_change -30));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].z_change -= 60.0;
        } 
        else if(block["block"].direction==2){
             glm::mat4 translateObject = glm::translate (glm::vec3(0,0,-(block["block"].z_change - 60)));
             glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(1,0,0));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(0,0,block["block"].z_change -60));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].y_change += 30.0;
             block["block"].z_change -= 90.0;
             block["block"].direction =0;
        }
        if(switches["switch1"].x == block["block"].x_change && switches["switch1"].z == block["block"].z_change){
            if(switch1==1)
                switch1=0;
            else
                switch1 =1;
        }
        if(switches["switch3"].x == block["block"].x_change && switches["switch3"].z == block["block"].z_change){
            if(switch2==1)
                switch2=0;
            else 
                switch2 =1;
        }
        key_pressed_up =0;
    }

    if(key_pressed_down ==1 && flag ==0){
        if(block["block"].direction ==0){
             glm::mat4 translateObject = glm::translate (glm::vec3(0,0,-(block["block"].z_change + 30)));
             glm::mat4 rotate = glm::rotate((float)((90.0)*M_PI/180.0f), glm::vec3(1,0,0));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(0,0,block["block"].z_change +30));

             rotateblock = translateObject1*rotate*translateObject*rotateblock;
             block["block"].x_change += 0;
             block["block"].z_change += 90.0;
             block["block"].y_change -= 30.0;
             block["block"].direction =2;
        }
        else if(block["block"].direction==1){
             glm::mat4 translateObject = glm::translate (glm::vec3(0,0,-(block["block"].z_change + 30)));
             glm::mat4 rotate = glm::rotate((float)((90.0)*M_PI/180.0f), glm::vec3(1,0,0));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(0,0,block["block"].z_change +30));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].z_change += 60.0;
        } 
        else if(block["block"].direction==2){
             glm::mat4 translateObject = glm::translate (glm::vec3(0,0,-(block["block"].z_change + 60)));
             glm::mat4 rotate = glm::rotate((float)((+90.0)*M_PI/180.0f), glm::vec3(1,0,0));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(0,0,block["block"].z_change +60));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].y_change += 30.0;
             block["block"].z_change += 90.0;
             block["block"].direction =0;
        }
     key_pressed_down =0;       
    }
    
    if(key_pressed_right ==1 && flag==0){
        if(block["block"].direction ==0){
             glm::mat4 translateObject = glm::translate (glm::vec3(-(block["block"].x_change + 30.0),0,0));
             glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(0,0,1));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(block["block"].x_change + 30.0,0,0));

             rotateblock = translateObject1*rotate*translateObject*rotateblock;
             block["block"].x_change += 90.0;
             block["block"].y_change -= 30.0;
             block["block"].direction =1;
        }
        else if(block["block"].direction==1){
             glm::mat4 translateObject = glm::translate (glm::vec3(-(block["block"].x_change + 60),0,0));
             glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(0,0,1));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(block["block"].x_change + 60,0,0));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].x_change += 90.0;
             block["block"].y_change += 30.0;
             block["block"].direction = 0;
        } 
        else if(block["block"].direction==2){
             glm::mat4 translateObject = glm::translate (glm::vec3(-(block["block"].x_change + 30.0),0,0));
             glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(0,0,1));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(block["block"].x_change + 30.0,0,0));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].x_change += 60.0;
             block["block"].direction = 2;
        }
        key_pressed_right =0;
    }

    if(key_pressed_left ==1 && flag==0){
        if(block["block"].direction ==0){
             glm::mat4 translateObject = glm::translate (glm::vec3(-(block["block"].x_change - 30.0),0,0));
             glm::mat4 rotate = glm::rotate((float)((90.0)*M_PI/180.0f), glm::vec3(0,0,1));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(block["block"].x_change - 30.0,0,0));

             rotateblock = translateObject1*rotate*translateObject*rotateblock;
             block["block"].x_change -= 90.0;
             block["block"].y_change -= 30.0;
             block["block"].direction =1;
        }
        else if(block["block"].direction==1){
             glm::mat4 translateObject = glm::translate (glm::vec3(-(block["block"].x_change - 60),0,0));
             glm::mat4 rotate = glm::rotate((float)((90.0)*M_PI/180.0f), glm::vec3(0,0,1));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(block["block"].x_change - 60,0,0));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].x_change -= 90.0;
             block["block"].y_change += 30.0;
             block["block"].direction = 0;
        } 
        else if(block["block"].direction==2){
             glm::mat4 translateObject = glm::translate (glm::vec3(-(block["block"].x_change - 30.0),0,0));
             glm::mat4 rotate = glm::rotate((float)((90.0)*M_PI/180.0f), glm::vec3(0,0,1));
             glm::mat4 translateObject1 = glm::translate (glm::vec3(block["block"].x_change - 30.0,0,0));

             rotateblock = translateObject1 * rotate * translateObject * rotateblock;
             block["block"].x_change -= 60.0;
             block["block"].direction = 2;
        }  
        key_pressed_left =0;
    }

    if(level==1){
        flag =1;
        gir1=0;
        gir2=0;
        for(map<string,Sprite>::iterator it1=tiles.begin();it1!=tiles.end();it1++){
            string current = it1->first;
            float XX = block["block"].x_change;
            float ZZ = block["block"].z_change;
            
            if(block["block"].direction == 0){
                if(tiles[current].x == XX && tiles[current].z == ZZ){
                    if(current[0] == 'o'){
                        tileflag =1;
                        tileX = tiles[current].x;
                        tileZ = tiles[current].z;
                        break;
                    }
                    else{
                        flag =0 ;
                        break;
                     }
                }
                if(XX == -380 && ZZ == -120){
                    moves=0;
                    seconds=0;
                }
            }    
            if(block["block"].direction == 1){
                if(tiles[current].z == ZZ){
                    if(tiles[current].x + 30.0 == XX || tiles[current].x - 30.0 == XX ||( XX<=-320 && XX>= -440 && ZZ ==-120)){
                        if(gir1 ==1)
                            gir2 =1;
                        else 
                            gir1 =1;
                    }
                    if(gir1 == 1 && gir2 ==1){
                        flag =0 ; 
                        break;
                    }
                }
            }
            if(block["block"].direction == 2){
                if(tiles[current].x == XX){
                    if(tiles[current].z + 30.0 == ZZ || tiles[current].z - 30.0 == ZZ || ( ZZ<= -60 && ZZ>= -180 && XX ==-380)){
                        if(gir1 ==1)
                            gir2 =1;
                        else 
                            gir1 =1;
                    }
                    if(gir1 == 1 && gir2 ==1){
                        flag =0 ; 
                        break;
                    }
                }
            }
        }
        if(switch1==0){
            float XX = block["block"].x_change;
            float ZZ = block["block"].z_change;
            if( XX >= -80 && XX <= -50 && ZZ <=210 && ZZ >= 180)
                flag =1;
        }
        if(switch2==0){
            if(block["block"].z_change ==0 && block["block"].x_change >= -50 && block["block"].x_change <= 70)
                flag =1;
        }
    }
        
    if(level==0){
        flag =1;
        gir1=0;
        gir2=0;
        for(map<string,Sprite>::iterator it1=ltiles.begin();it1!=ltiles.end();it1++){
            string current = it1->first;
            float XX = block["block"].x_change;
            float ZZ = block["block"].z_change;
            
            if(block["block"].direction == 0){
                if(ltiles[current].x == XX && ltiles[current].z == ZZ){
                        flag =0 ;
                        break;
                }
                if(XX == -380 && ZZ == -120){
                   sig=1;
                   break;
                }
            }    
            if(block["block"].direction == 1){
                if(ltiles[current].z == ZZ){
                    if(ltiles[current].x + 30.0 == XX || ltiles[current].x - 30.0 == XX ||( XX<=-320 && XX>= -440 && ZZ ==-120)){
                        if(gir1 ==1)
                            gir2 =1;
                        else 
                            gir1 =1;
                    }
                    if(gir1 == 1 && gir2 ==1){
                        flag =0 ; 
                        break;
                    }
                }
            }
            if(block["block"].direction == 2){
                if(ltiles[current].x == XX){
                    if(ltiles[current].z + 30.0 == ZZ || ltiles[current].z - 30.0 == ZZ || ( ZZ<= -60 && ZZ>= -180 && XX ==-380)){
                        if(gir1 ==1)
                            gir2 =1;
                        else 
                            gir1 =1;
                    }
                    if(gir1 == 1 && gir2 ==1){
                        flag =0 ; 
                        break;
                    }
                }
            }
        }
    }

    glm::mat4 rotatetile1 = glm::mat4(1.0f);
    glm::mat4 rotatetile2 = glm::mat4(1.0f);
    glm::mat4 rotatetile3 = glm::mat4(1.0f);

    if(switch1==0){
        glm::mat4 translatetile = glm::translate (glm::vec3(-(tiles["tile31"].x_change - 30.0),12,0));
        glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(0,0,1));
        glm::mat4 translatetile1 = glm::translate (glm::vec3(tiles["tile31"].x_change - 30.0,-12,0));
        rotatetile3 = translatetile1 * rotate * translatetile;
    }   
    if(switch2==0){
        glm::mat4 translatetile = glm::translate (glm::vec3(-(tiles["tile5"].x_change - 30.0),12,0));
        glm::mat4 rotate = glm::rotate((float)((-90.0)*M_PI/180.0f), glm::vec3(0,0,1));
        glm::mat4 translatetile1 = glm::translate (glm::vec3(tiles["tile5"].x_change - 30.0,-12,0));
        rotatetile1 = translatetile1 * rotate * translatetile;
    }   
    if(switch2==0){
        glm::mat4 translatetile = glm::translate (glm::vec3(-(tiles["tile6"].x_change + 30.0),12,0));
        glm::mat4 rotate = glm::rotate((float)((90.0)*M_PI/180.0f), glm::vec3(0,0,1));
        glm::mat4 translatetile1 = glm::translate (glm::vec3(tiles["tile6"].x_change + 30.0,-12,0));
        rotatetile2 = translatetile1 * rotate * translatetile;
    }
    /* Render your scene */
    if(level==1){
        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(block["block"].x, block["block"].y,block["block"].z)); // glTranslatef
        
        glm::mat4 translateblock = glm::translate (glm::vec3(0,-downfall,0)); // glTranslatef
        if(flag ==1){
            ObjectTransform=  translateblock * rotateblock * translateObject ;
            block["block"].y_change -= 3;
            downfall += 3.0;
        }
        else 
            ObjectTransform= rotateblock * translateObject ;

        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(block["block"].object);
        
        if(block["block"].y_change <= -200){
            block["block"].x_change =block["block"].x;
            block["block"].y_change =block["block"].y;
            block["block"].z_change =block["block"].z;
            rotateblock = glm::mat4(1.0f);
            downfall =0;
            flag =0;
            block["block"].direction =0;
            tileflag =0;
            downtile =0;
            switch1=0;
            switch2=0;  
        }

    }

    if(level==1){
        for(map<string,Sprite>::iterator it1=tiles.begin();it1!=tiles.end();it1++){
            string current = it1->first; 
            glm::mat4 MVP; 
            Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
            glm::mat4 ObjectTransform;
            glm::mat4 translateObject = glm::translate (glm::vec3(tiles[current].x, tiles[current].y,tiles[current].z)); // glTranslatef
            glm::mat4 translatetile = glm::translate (glm::vec3(0,-downtile,0)); // glTranslatef
         
            if(tileflag ==1 && tiles[current].x==tileX && tiles[current].z==tileZ){
                    ObjectTransform = translatetile * translateObject;
                    downtile += 5;
            }
            else{
                if(current=="tile5"){
                    ObjectTransform = rotatetile1 * translateObject;
                }
                else if(current=="tile6"){
                    ObjectTransform = rotatetile2 * translateObject;
                }
                else if(current =="tile31"){
                    ObjectTransform = rotatetile3 * translateObject;
                }
                else 
                    ObjectTransform=translateObject;
            }
            Matrices.model *= ObjectTransform;
            MVP = VP * Matrices.model; // MVP = p * V * M
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
            draw3DObject(tiles[current].object);
        }
          
        for(map<string,Sprite>::iterator it1=switches.begin();it1!=switches.end();it1++){
            string current = it1->first; 
            glm::mat4 MVP; 
            Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
            glm::mat4 ObjectTransform;
            glm::mat4 translateObject = glm::translate (glm::vec3(switches[current].x,switches[current].y,switches[current].z)); // glTranslatef
         
            ObjectTransform=translateObject;
            Matrices.model *= ObjectTransform;
            MVP = VP * Matrices.model; // MVP = p * V * M
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
            draw3DObject(switches[current].object);
        }
    }
   
    if(level==0){
        for(map<string,Sprite>::iterator it1=ltiles.begin();it1!=ltiles.end();it1++){
            string current = it1->first; 
            glm::mat4 MVP; 
            Matrices.model = glm::mat4(1.0f);

                /* Render your scene */
            glm::mat4 ObjectTransform;
            glm::mat4 translateObject = glm::translate (glm::vec3(ltiles[current].x, ltiles[current].y,ltiles[current].z)); // glTranslatef
         
            ObjectTransform=translateObject;
            
            Matrices.model *= ObjectTransform;
            MVP = VP * Matrices.model; // MVP = p * V * M
            glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
            draw3DObject(ltiles[current].object);
        }

    

        glm::mat4 ObjectTransform;
        glm::mat4 translateObject = glm::translate (glm::vec3(160.0,60.0,300.0)); // glTranslatef
        Matrices.model = glm::mat4(1.0f);
        glm::mat4 translateblock = glm::translate (glm::vec3(0,-downfall,0)); // glTranslatef
        if(flag ==1){
            ObjectTransform=  translateblock * rotateblock * translateObject ;
            block["block"].y_change -= 3;
            downfall += 3.0;
        }
        else 
            ObjectTransform= rotateblock * translateObject ;

        Matrices.model *= ObjectTransform;
        MVP = VP * Matrices.model; // MVP = p * V * M
        glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
        draw3DObject(block["block"].object);


        if(block["block"].y_change <= -200){
            if(sig==1)
                level=1;
            else{
                block["block"].x_change =160;
                block["block"].y_change =60;
                block["block"].z_change =300;
            }
            rotateblock = glm::mat4(1.0f);
            downfall =0;
            flag =0;
            block["block"].direction =0; 
        }
    }

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height) {
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
      if (!glfwInit()) {
          exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width ,height, "My openGL game", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
    glfwSetScrollCallback(window, mousescroll); // mouse scroll

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height){

    /* Objects should be created before any other gl function and shaders */
	// Create the models
    COLOR green1 = {46/255.0,199/255.0,0/255.0};
    COLOR green2 = {85/255.0,255/255.0,66/255.0};
    COLOR green3 = {62/255.0,148/255.0,0/255.0};
    COLOR orange = {255.0/255.0,61.0/255.0,0.0/255.0}; 
    COLOR dark = {178.0/255.0,24.0/255.0,0.0/255.0};
    COLOR grey = {168.0/255.0,168.0/255.0,168.0/255.0};
    COLOR gold = {218.0/255.0,165.0/255.0,32.0/255.0};
    COLOR coingold = {255.0/255.0,223.0/255.0,0.0/255.0};
    COLOR red = {255.0/255.0,2.0/255.0,0.0/255.0};
    COLOR lightgreen = {57/255.0,230/255.0,0/255.0};
    COLOR black = {30/255.0,30/255.0,21/255.0};
    COLOR blue = {0,0,1};
    COLOR darkbrown = {46/255.0,46/255.0,31/255.0};
    COLOR lightbrown = {95/255.0,63/255.0,32/255.0};
    COLOR darkred = {204/255.0,1/255.0,0/255.0};
    COLOR cratebrown = {153/255.0,102/255.0,0/255.0};
    COLOR cratebrown1 = {121/255.0,85/255.0,0/255.0};
    COLOR cratebrown2 = {102/255.0,68/255.0,0/255.0};
    COLOR skyblue2 = {113/255.0,185/255.0,209/255.0};
    COLOR skyblue1 = {123/255.0,201/255.0,227/255.0};
    COLOR skyblue = {0/255.0,0/255.0,127/255.0};
    COLOR cloudwhite = {229/255.0,255/255.0,255/255.0};
    COLOR cloudwhite1 = {204/255.0,255/255.0,255/255.0};
    COLOR lightnknk = {255/255.0,122/255.0,173/255.0};
    COLOR darkpink = {255/255.0,51/255.0,119/255.0};
    COLOR white = {255/255.0,255/255.0,255/255.0};
    COLOR score = {117/255.0,78/255.0,40/255.0};
	// Create and compile our GLSL program from the shaders


    createCube("block",green1,green1,green2,green2,green3,green3,-500,60,60,60.0,120.0,60.0,"block");
    block["block"].direction = 0;

    createCube("otile1",orange,gold,score,grey,coingold,black,-200,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile2",skyblue,gold,score,grey,coingold,black,-140,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile3",skyblue,gold,score,grey,coingold,black,-200,-6,60,60.0,12.0,60.0,"tiles");
    createCube("tile4",blue,gold,score,grey,coingold,black,-80,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile5",skyblue,gold,score,grey,coingold,black,-20,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile6",blue,gold,score,grey,coingold,black,40,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile7",skyblue,gold,score,grey,coingold,black,100,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile8",blue,gold,score,grey,coingold,black,160,-6,0,60.0,12.0,60.0,"tiles");
    createCube("tile10",blue,gold,score,grey,coingold,black,100,-6,-60,60.0,12.0,60.0,"tiles");
    createCube("tile11",skyblue,gold,score,grey,coingold,black,160,-6,-60,60.0,12.0,60.0,"tiles");
    createCube("tile17",blue,gold,score,grey,coingold,black,160,-6,240,60.0,12.0,60.0,"tiles");
    createCube("tile18",skyblue,gold,score,grey,coingold,black,160,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile19",skyblue,gold,score,grey,coingold,black,100,-6,240,60.0,12.0,60.0,"tiles");
    createCircle("switch3",coingold,100,0,240,25,200,"switch",1);
    createCircle("switch4",darkred,100,1,240,12,200,"switch",1);
    createCube("tile20",blue,gold,score,grey,coingold,black,100,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile21",skyblue,gold,score,grey,coingold,black,100,-6,360,60.0,12.0,60.0,"tiles");
    createCube("tile22",skyblue,gold,score,grey,coingold,black,40,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile23",blue,gold,score,grey,coingold,black,40,-6,360,60.0,12.0,60.0,"tiles");
    createCube("tile24",blue,gold,score,grey,coingold,black,-20,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile25",skyblue,gold,score,grey,coingold,black,-20,-6,360,60.0,12.0,60.0,"tiles");
    createCube("tile26",skyblue,gold,score,grey,coingold,black,-80,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile27",blue,gold,score,grey,coingold,black,-80,-6,360,60.0,12.0,60.0,"tiles");
    createCube("tile28",blue,gold,score,grey,coingold,black,-140,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile29",skyblue,gold,score,grey,coingold,black,-140,-6,360,60.0,12.0,60.0,"tiles");
	createCube("tile30",skyblue,gold,score,grey,coingold,black,-140,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile31",red,gold,score,grey,coingold,black,-80,-6,240,60.0,12.0,60.0,"tiles");
    createCube("tile31",skyblue,gold,score,grey,coingold,black,-80,-6,180,60.0,12.0,60.0,"tiles");
    createCube("otile32",orange,gold,score,grey,coingold,dark,-200,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile33",red,gold,score,grey,coingold,darkred,-200,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile34",orange,gold,score,grey,coingold,dark,-260,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile35",red,gold,score,grey,coingold,darkred,-260,-6,240,60.0,12.0,60.0,"tiles");
    createCube("tile36",skyblue,gold,score,grey,coingold,dark,-320,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile37",red,gold,score,grey,coingold,darkred,-320,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile38",orange,gold,score,grey,coingold,dark,-380,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile39",red,gold,score,grey,coingold,darkred,-380,-6,240,60.0,12.0,60.0,"tiles");
    createCube("tile40",skyblue,gold,score,grey,coingold,dark,-440,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile41",red,gold,score,grey,coingold,darkred,-440,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile42",orange,gold,score,grey,coingold,dark,-500,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile43",red,gold,score,grey,coingold,darkred,-500,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile44",orange,gold,score,grey,coingold,dark,-560,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile45",red,gold,score,grey,coingold,darkred,-560,-6,300,60.0,12.0,60.0,"tiles");
    createCube("tile46",skyblue,gold,score,grey,coingold,black,-620,-6,300,60.0,12.0,60.0,"tiles");
    createCube("otile47",red,gold,score,grey,coingold,darkred,-620,-6,240,60.0,12.0,60.0,"tiles");
    createCube("otile48",orange,gold,score,grey,coingold,dark,-560,-6,360,60.0,12.0,60.0,"tiles");
    createCube("tile49",skyblue,gold,score,grey,coingold,black,-500,-6,360,60.0,12.0,60.0,"tiles");
    createCube("otile50",orange,gold,score,grey,coingold,dark,-440,-6,360,60.0,12.0,60.0,"tiles");
    createCube("otile51",red,gold,score,grey,coingold,darkred,-380,-6,360,60.0,12.0,60.0,"tiles");
    createCube("otile52",orange,gold,score,grey,coingold,dark,-320,-6,360,60.0,12.0,60.0,"tiles");
    createCube("otile53",orange,gold,score,grey,coingold,darkred,-620,-6,180,60.0,12.0,60.0,"tiles");
    createCube("otile54",red,gold,score,grey,coingold,darkred,-560,-6,180,60.0,12.0,60.0,"tiles");
    createCube("otile55",orange,gold,score,grey,coingold,darkred,-500,-6,180,60.0,12.0,60.0,"tiles");
    createCube("otile56",red,gold,score,grey,coingold,darkred,-440,-6,180,60.0,12.0,60.0,"tiles");
    createCube("tile57",skyblue,gold,score,grey,coingold,darkred,-560,-6,120,60.0,12.0,60.0,"tiles");
    createCircle("switch1",coingold,-560,0,120,25,200,"switch",1);
    createCircle("switch2",darkred,-560,1,120,12,200,"switch",1);
    createCube("tile58",blue,gold,score,grey,coingold,darkred,-500,-6,120,60.0,12.0,60.0,"tiles");
    createCube("tile59",skyblue,gold,score,grey,coingold,darkred,-440,-6,120,60.0,12.0,60.0,"tiles");
    createCube("tile60",skyblue,gold,score,grey,coingold,darkred,-500,-6,60,60.0,12.0,60.0,"tiles");
    createCube("tile61",blue,gold,score,grey,coingold,black,-200,-6,120,60.0,12.0,60.0,"tiles");
    createCube("otile62",red,gold,score,grey,coingold,black,-200,-6,180,60.0,12.0,60.0,"tiles");
    createCube("otile63",orange,gold,score,grey,coingold,black,-140,-6,60,60.0,12.0,60.0,"tiles");
    createCube("otile64",red,gold,score,grey,coingold,black,-140,-6,120,60.0,12.0,60.0,"tiles");
    createCube("tile65",blue,gold,score,grey,coingold,black,-140,-6,180,60.0,12.0,60.0,"tiles");
    createCube("tile66",skyblue,gold,score,grey,coingold,black,100,-6,-120,60.0,12.0,60.0,"tiles");
    createCube("tile67",blue,gold,score,grey,coingold,black,160,-6,-120,60.0,12.0,60.0,"tiles");
    createCube("tile68",blue,gold,score,grey,coingold,black,100,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile69",skyblue,gold,score,grey,coingold,black,160,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile70",skyblue,gold,score,grey,coingold,black,40,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile71",blue,gold,score,grey,coingold,black,-20,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("otile72",red,gold,score,grey,coingold,black,-80,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("otile73",orange,gold,score,grey,coingold,black,-140,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile74",skyblue,gold,score,grey,coingold,black,-200,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile75",blue,gold,score,grey,coingold,black,-260,-6,-180,60.0,12.0,60.0,"tiles"); 
    createCube("tile76",skyblue,gold,score,grey,coingold,black,-260,-6,-120,60.0,12.0,60.0,"tiles");
    createCube("tile77",skyblue,gold,score,grey,coingold,black,-320,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile78",blue,gold,score,grey,coingold,black,-320,-6,-120,60.0,12.0,60.0,"tiles");
    createCube("tile79",skyblue,gold,score,grey,coingold,black,-320,-6,-60,60.0,12.0,60.0,"tiles");
    createCube("tile80",blue,gold,score,grey,coingold,black,-380,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile81",blue,gold,score,grey,coingold,black,-380,-6,-60,60.0,12.0,60.0,"tiles");
    createCube("tile82",skyblue,gold,score,grey,coingold,black,-440,-6,-180,60.0,12.0,60.0,"tiles");
    createCube("tile83",blue,gold,score,grey,coingold,black,-440,-6,-120,60.0,12.0,60.0,"tiles");
    createCube("tile84",skyblue,gold,score,grey,coingold,black,-440,-6,-60,60.0,12.0,60.0,"tiles");


    createRectangle1("seg1",score,score,score,score,325,285,2,10,"point1");
    createRectangle1("seg2",score,score,score,score,330,280,10,2,"point1");
    createRectangle1("seg3",score,score,score,score,330,270,10,2,"point1");
    createRectangle1("seg4",score,score,score,score,325,265,2,10,"point1");
    createRectangle1("seg5",score,score,score,score,320,270,10,2,"point1");
    createRectangle1("seg6",score,score,score,score,320,280,10,2,"point1");
    createRectangle1("seg7",score,score,score,score,325,275,2,10,"point1");
    point1["seg7"].status=0;

    createRectangle1("seg1",score,score,score,score,340,285,2,10,"point2");
    createRectangle1("seg2",score,score,score,score,345,280,10,2,"point2");
    createRectangle1("seg3",score,score,score,score,345,270,10,2,"point2");
    createRectangle1("seg4",score,score,score,score,340,265,2,10,"point2");
    createRectangle1("seg5",score,score,score,score,335,270,10,2,"point2");
    createRectangle1("seg6",score,score,score,score,335,280,10,2,"point2");
    createRectangle1("seg7",score,score,score,score,340,275,2,10,"point2");
    point2["seg7"].status=0;

    createRectangle1("seg1",score,score,score,score,355,285,2,10,"point3");
    createRectangle1("seg2",score,score,score,score,360,280,10,2,"point3");
    createRectangle1("seg3",score,score,score,score,360,270,10,2,"point3");
    createRectangle1("seg4",score,score,score,score,355,265,2,10,"point3");
    createRectangle1("seg5",score,score,score,score,350,270,10,2,"point3");
    createRectangle1("seg6",score,score,score,score,350,280,10,2,"point3");
    createRectangle1("seg7",score,score,score,score,355,275,2,10,"point3");
    point3["seg7"].status=0;

    createRectangle1("seg1",score,score,score,score,355,255,2,10,"sec1");
    createRectangle1("seg2",score,score,score,score,360,250,10,2,"sec1");
    createRectangle1("seg3",score,score,score,score,360,240,10,2,"sec1");
    createRectangle1("seg4",score,score,score,score,355,235,2,10,"sec1");
    createRectangle1("seg5",score,score,score,score,350,240,10,2,"sec1");
    createRectangle1("seg6",score,score,score,score,350,250,10,2,"sec1");
    createRectangle1("seg7",score,score,score,score,355,245,2,10,"sec1");
    sec1["seg7"].status=0;


    createRectangle1("seg1",score,score,score,score,340,255,2,10,"sec2");
    createRectangle1("seg2",score,score,score,score,345,250,10,2,"sec2");
    createRectangle1("seg3",score,score,score,score,345,240,10,2,"sec2");
    createRectangle1("seg4",score,score,score,score,340,235,2,10,"sec2");
    createRectangle1("seg5",score,score,score,score,335,240,10,2,"sec2");
    createRectangle1("seg6",score,score,score,score,335,250,10,2,"sec2");
    createRectangle1("seg7",score,score,score,score,340,245,2,10,"sec2");
    sec2["seg7"].status=0;
    
    createRectangle1("l1",score,score,score,score,330,250,3,3,"label");
    createRectangle1("l2",score,score,score,score,330,240,3,3,"label");

    createRectangle1("seg1",score,score,score,score,320,255,2,10,"min1");
    createRectangle1("seg2",score,score,score,score,325,250,10,2,"min1");
    createRectangle1("seg3",score,score,score,score,325,240,10,2,"min1");
    createRectangle1("seg4",score,score,score,score,320,235,2,10,"min1");
    createRectangle1("seg5",score,score,score,score,315,240,10,2,"min1");
    createRectangle1("seg6",score,score,score,score,315,250,10,2,"min1");
    createRectangle1("seg7",score,score,score,score,320,245,2,10,"min1");
    min1["seg7"].status=0;


    createRectangle1("seg1",score,score,score,score,305,255,2,10,"min2");
    createRectangle1("seg2",score,score,score,score,310,250,10,2,"min2");
    createRectangle1("seg3",score,score,score,score,310,240,10,2,"min2");
    createRectangle1("seg4",score,score,score,score,305,235,2,10,"min2");
    createRectangle1("seg5",score,score,score,score,300,240,10,2,"min2");
    createRectangle1("seg6",score,score,score,score,300,250,10,2,"min2");
    createRectangle1("seg7",score,score,score,score,305,245,2,10,"min2");
    min2["seg7"].status=0;
    
    /*level 1*/

    createCube("tile7",skyblue,gold,score,grey,coingold,black,100,-6,0,60.0,12.0,60.0,"ltiles");
    createCube("tile8",blue,gold,score,grey,coingold,black,160,-6,0,60.0,12.0,60.0,"ltiles");
    createCube("tile9",skyblue,gold,score,grey,coingold,black,220,-6,0,60.0,12.0,60.0,"ltiles");
    createCube("tile10",blue,gold,score,grey,coingold,black,100,-6,-60,60.0,12.0,60.0,"ltiles");
    createCube("tile11",skyblue,gold,score,grey,coingold,black,160,-6,-60,60.0,12.0,60.0,"ltiles");
    createCube("tile12",blue,gold,score,grey,coingold,black,220,-6,-60,60.0,12.0,60.0,"ltiles");
    createCube("tile13",blue,gold,score,grey,coingold,black,220,-6,60,60.0,12.0,60.0,"ltiles");
    createCube("tile14",skyblue,gold,score,grey,coingold,black,220,-6,120,60.0,12.0,60.0,"ltiles");
    createCube("tile15",blue,gold,score,grey,coingold,black,220,-6,180,60.0,12.0,60.0,"ltiles");
    createCube("tile16",skyblue,gold,score,grey,coingold,black,160,-6,180,60.0,12.0,60.0,"ltiles");
    createCube("tile17",blue,gold,score,grey,coingold,black,160,-6,240,60.0,12.0,60.0,"ltiles");
    createCube("tile18",skyblue,gold,score,grey,coingold,black,160,-6,300,60.0,12.0,60.0,"ltiles");
    createCube("tile19",skyblue,gold,score,grey,coingold,black,220,-6,240,60.0,12.0,60.0,"ltiles");
    createCube("tile66",skyblue,gold,score,grey,coingold,black,100,-6,-120,60.0,12.0,60.0,"ltiles");
    createCube("tile67",blue,gold,score,grey,coingold,black,160,-6,-120,60.0,12.0,60.0,"ltiles");
    createCube("tile68",blue,gold,score,grey,coingold,black,100,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile69",skyblue,gold,score,grey,coingold,black,160,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile70",skyblue,gold,score,grey,coingold,black,40,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile71",blue,gold,score,grey,coingold,black,-20,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile72",skyblue,gold,score,grey,coingold,black,-80,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile73",blue,gold,score,grey,coingold,black,-140,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile74",skyblue,gold,score,grey,coingold,black,-200,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile75",blue,gold,score,grey,coingold,black,-260,-6,-180,60.0,12.0,60.0,"ltiles"); 
    createCube("tile76",skyblue,gold,score,grey,coingold,black,-260,-6,-120,60.0,12.0,60.0,"ltiles");
    createCube("tile77",skyblue,gold,score,grey,coingold,black,-320,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile78",blue,gold,score,grey,coingold,black,-320,-6,-120,60.0,12.0,60.0,"ltiles");
    createCube("tile79",skyblue,gold,score,grey,coingold,black,-320,-6,-60,60.0,12.0,60.0,"ltiles");
    createCube("tile80",blue,gold,score,grey,coingold,black,-380,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile81",blue,gold,score,grey,coingold,black,-380,-6,-60,60.0,12.0,60.0,"ltiles");
    createCube("tile82",skyblue,gold,score,grey,coingold,black,-440,-6,-180,60.0,12.0,60.0,"ltiles");
    createCube("tile83",blue,gold,score,grey,coingold,black,-440,-6,-120,60.0,12.0,60.0,"ltiles");
    createCube("tile84",skyblue,gold,score,grey,coingold,black,-440,-6,-60,60.0,12.0,60.0,"ltiles");
    block["block"].x_change =160.0;
    block["block"].y_change= 60.0;
    block["block"].z_change= 300.0;



    programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 900;
	int height = 700;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    audio_init();
    double last_update_time = glfwGetTime(), current_time;

    glfwGetCursorPos(window, &mouse_pos_x, &mouse_pos_y);

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        /*if(flag ==1)
            gameover =1;*/
        audio_play();
        // OpenGL Draw commands
        draw(window, width, height);

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 1){ // atleast 0.5s elapsed since last frame
            seconds ++;
            last_update_time = current_time;
        }
    }

    audio_close();
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
