/*******************************************************************
		   Multi-Part Model Construction and Manipulation
********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gl/glut.h>
#include "Vector3D.h"
#include "QuadMesh.h"

#pragma warning(disable:4996)

typedef unsigned char  byte;
typedef unsigned short word;
typedef unsigned long  dword;
typedef unsigned short ushort;
typedef unsigned long  ulong;



typedef struct RGB
{
	byte r, g, b;
} RGB;

typedef struct RGBpixmap
{
	int nRows, nCols;
	RGB *pixel;
} RGBpixmap;



RGBpixmap pix[8]; // make six empty pixmaps, one for each side of cube
const GLfloat PI = 3.1415;
const int meshSize = 16;    // Default Mesh Size
const int vWidth = 1200;     // Viewport width in pixels
const int vHeight = 800;    // Viewport height in pixels

static int currentButton;
static unsigned char currentKey;

// Lighting/shading and material properties for drone - upcoming lecture - just copy for now

// Light properties
static GLfloat light_position0[] = { -6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_position1[] = { 6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_ambient[] = { 0.2F, 0.2F, 0.2F, 1.0F };

// Material properties
static GLfloat drone_mat_ambient[] = { 0.4F, 0.2F, 0.0F, 1.0F };
static GLfloat drone_mat_specular[] = { 0.1F, 0.1F, 0.0F, 1.0F };
static GLfloat drone_mat_diffuse[] = { 0.9F, 0.5F, 0.0F, 1.0F };
static GLfloat drone_mat_shininess[] = { 0.0F };

//user drone settings!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
BOOLEAN dronemode = false;
BOOLEAN spawnDrone = true;
//rotate 
int rotate = 0;
//translation forward backward
static GLfloat transx = 0.0;
static GLfloat transy = 0.0;
static GLfloat transz = 0.0;
//altitude
static GLfloat altitude = 0.0;
// spin properties
static GLfloat spin = 0.0;
//torpedo
BOOLEAN torpedo = false;
static GLfloat Tortransx = 0.0;
static GLfloat Tortransz = 0.0;
int Torrotate = 0;

static int torpedoSpeed = 25;

static int deaths;
static int hits;


//ai drone settings!!!!!!!!!!!!
BOOLEAN spawnAI = true;
static GLfloat AItransx = 0.0;
static GLfloat AItransy = 4.0;
static GLfloat AItransz = 0.0;
int AIrotate = 0;
static GLfloat AIspin = 0.0;

//explode
BOOLEAN explode = false;
static GLfloat explodeX = 0.0;
static GLfloat explodeY = 0.0;
static GLfloat explodeZ = 0.0;


// A quad mesh representing the ground
static QuadMesh groundMesh;

//default camera
double dimension = 10.0;
double cameraX = 0.0;
double cameraY = 25.0;
double cameraZ = 20.0;
double zoom = 60.0;
double tilt = 0.0;

//********************************************************************************************************************************************

// Structure defining a bounding box, currently unused
//struct BoundingBox {
//    Vector3D min;
//    Vector3D max;
//} BBox;

// Prototypes for functions in this module
void initOpenGL(int w, int h);
void display(void);
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void mouseMotionHandler(int xMouse, int yMouse);
void keyboard(unsigned char key, int x, int y);
void functionKeys(int key, int x, int y);
Vector3D ScreenToWorld(int x, int y);

//Texture
void fskip(FILE *fp, int num_bytes)
{
	int i;
	for (i = 0; i < num_bytes; i++)
		fgetc(fp);
}


/**************************************************************************
 *                                                                        *
 *    Loads a bitmap file into memory.                                    *
 **************************************************************************/

ushort getShort(FILE *fp) //helper function
{ //BMP format uses little-endian integer types
  // get a 2-byte integer stored in little-endian form
	char ic;
	ushort ip;
	ic = fgetc(fp); ip = ic;  //first byte is little one 
	ic = fgetc(fp);  ip |= ((ushort)ic << 8); // or in high order byte
	return ip;
}
//<<<<<<<<<<<<<<<<<<<< getLong >>>>>>>>>>>>>>>>>>>
ulong getLong(FILE *fp) //helper function
{  //BMP format uses little-endian integer types
   // get a 4-byte integer stored in little-endian form
	ulong ip = 0;
	char ic = 0;
	unsigned char uc = ic;
	ic = fgetc(fp); uc = ic; ip = uc;
	ic = fgetc(fp); uc = ic; ip |= ((ulong)uc << 8);
	ic = fgetc(fp); uc = ic; ip |= ((ulong)uc << 16);
	ic = fgetc(fp); uc = ic; ip |= ((ulong)uc << 24);
	return ip;
}



void readBMPFile(RGBpixmap *pm, char *file)
{
	FILE *fp;
	long index;
	int k, row, col, numPadBytes, nBytesInRow;
	char ch1, ch2;
	ulong fileSize;
	ushort reserved1;    // always 0
	ushort reserved2;     // always 0 
	ulong offBits; // offset to image - unreliable
	ulong headerSize;     // always 40
	ulong numCols; // number of columns in image
	ulong numRows; // number of rows in image
	ushort planes;      // always 1 
	ushort bitsPerPixel;    //8 or 24; allow 24 here
	ulong compression;      // must be 0 for uncompressed 
	ulong imageSize;       // total bytes in image 
	ulong xPels;    // always 0 
	ulong yPels;    // always 0 
	ulong numLUTentries;    // 256 for 8 bit, otherwise 0 
	ulong impColors;       // always 0 
	long count;
	char dum;

	/* open the file */
	if ((fp = fopen(file, "rb")) == NULL)
	{
		printf("Error opening file %s.\n", file);
		exit(1);
	}

	/* check to see if it is a valid bitmap file */
	if (fgetc(fp) != 'B' || fgetc(fp) != 'M')
	{
		fclose(fp);
		printf("%s is not a bitmap file.\n", file);
		exit(1);
	}

	fileSize = getLong(fp);
	reserved1 = getShort(fp);    // always 0
	reserved2 = getShort(fp);     // always 0 
	offBits = getLong(fp); // offset to image - unreliable
	headerSize = getLong(fp);     // always 40
	numCols = getLong(fp); // number of columns in image
	numRows = getLong(fp); // number of rows in image
	planes = getShort(fp);      // always 1 
	bitsPerPixel = getShort(fp);    //8 or 24; allow 24 here
	compression = getLong(fp);      // must be 0 for uncompressed 
	imageSize = getLong(fp);       // total bytes in image 
	xPels = getLong(fp);    // always 0 
	yPels = getLong(fp);    // always 0 
	numLUTentries = getLong(fp);    // 256 for 8 bit, otherwise 0 
	impColors = getLong(fp);       // always 0 

	if (bitsPerPixel != 24)
	{ // error - must be a 24 bit uncompressed image
		printf("Error bitsperpixel not 24\n");
		exit(1);
	}
	//add bytes at end of each row so total # is a multiple of 4
	// round up 3*numCols to next mult. of 4
	nBytesInRow = ((3 * numCols + 3) / 4) * 4;
	numPadBytes = nBytesInRow - 3 * numCols; // need this many
	pm->nRows = numRows; // set class's data members
	pm->nCols = numCols;
	pm->pixel = (RGB *)malloc(3 * numRows * numCols);//make space for array
	if (!pm->pixel) return; // out of memory!
	count = 0;
	dum;
	for (row = 0; row < numRows; row++) // read pixel values
	{
		for (col = 0; col < numCols; col++)
		{
			int r, g, b;
			b = fgetc(fp); g = fgetc(fp); r = fgetc(fp); //read bytes
			pm->pixel[count].r = r; //place them in colors
			pm->pixel[count].g = g;
			pm->pixel[count++].b = b;
		}
		fskip(fp, numPadBytes);
	}
	fclose(fp);
}





void setTexture(RGBpixmap *p, GLuint textureID)
{
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, p->nCols, p->nRows, 0, GL_RGB,
		GL_UNSIGNED_BYTE, p->pixel);
}






// Set up OpenGL. For viewport and projection setup see reshape(). */
void initOpenGL(int w, int h)
{
	// Set up and enable lighting
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);

	glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
	glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//glEnable(GL_LIGHT1);   // This light is currently off

	// Other OpenGL setup
	glEnable(GL_DEPTH_TEST);   // Remove hidded surfaces
	glShadeModel(GL_SMOOTH);   // Use smooth shading, makes boundaries between polygons harder to see 
	glClearColor(0.6F, 0.6F, 0.6F, 0.0F);  // Color and depth for glClear
	glClearDepth(1.0f);
	glEnable(GL_NORMALIZE);    // Renormalize normal vectors 
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);   // Nicer perspective

	// Set up ground quad mesh
	Vector3D origin = NewVector3D(-16.0f, 0.0f, 8.0f);
	Vector3D dir1v = NewVector3D(1.0f, 0.0f, 0.0f);
	Vector3D dir2v = NewVector3D(0.0f, 0.0f, -1.0f);
	groundMesh = NewQuadMesh(meshSize);
	InitMeshQM(&groundMesh, meshSize, origin, 30.0, 25.0, dir1v, dir2v);

	Vector3D ambient = NewVector3D(0.0f, 0.05f, 0.0f);
	Vector3D diffuse = NewVector3D(0.4f, 0.8f, 0.4f);
	Vector3D specular = NewVector3D(0.04f, 0.04f, 0.04f);
	SetMaterialQM(&groundMesh, ambient, diffuse, specular, 0.2);

	// Set up the bounding box of the scene
	// Currently unused. You could set up bounding boxes for your objects eventually.
	//Set(&BBox.min, -8.0f, 0.0, -8.0);
	//Set(&BBox.max, 8.0f, 6.0,  8.0);

	//Texture
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	readBMPFile(&pix[0], "tiles01.bmp");
	setTexture(&pix[0], 2000);
	readBMPFile(&pix[1], "skin01.bmp");
	setTexture(&pix[1], 2001);
	readBMPFile(&pix[2], "clover01.bmp");
	setTexture(&pix[2], 2002);
	readBMPFile(&pix[3], "plank01.bmp");
	setTexture(&pix[3], 2003);
	readBMPFile(&pix[4], "road.bmp");
	setTexture(&pix[4], 2004);
	readBMPFile(&pix[5], "building.bmp");
	setTexture(&pix[5], 2005);
	readBMPFile(&pix[6], "darkerwood.bmp");
	setTexture(&pix[6], 2006);
	readBMPFile(&pix[7], "gravel.bmp");
	setTexture(&pix[7], 2007);
	readBMPFile(&pix[8], "neon.bmp");
	setTexture(&pix[8], 2008);
	readBMPFile(&pix[9], "face.bmp");
	setTexture(&pix[9], 2009);
	readBMPFile(&pix[10], "fire.bmp");
	setTexture(&pix[10], 2010);

	glViewport(0, 0, 640, 480);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 640.0 / 480.0, 1.0, 30.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslated(0.0, 0.0, -4.0); // move camera back from default position
	glShadeModel(GL_SMOOTH);

	// Set up texture mapping assuming no lighting/shading 
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

}

void dronecamera(void) {

	GLfloat zView = (-sin(((rotate * PI) / 180)) * 200);
	GLfloat xView = (-cos(((rotate * PI) / 180)) * 200);
	gluLookAt(transx, altitude +0.50, transz, zView, altitude + tilt, xView , 0, 1.0, 0.0);

}





// Callback, called whenever GLUT determines that the window should be redisplayed
// or glutPostRedisplay() has been called.
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	if (!dronemode) {
		gluLookAt(cameraX, cameraY, cameraZ, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	}
	else {
		dronecamera();
	}


	// Draw Drone

	//currently unused
	//Vector3D mov = NewVector3D(trans, altitude, 0.0f);
	//Vector3D angle = NewVector3D(rotate, 0.0f, 0.0f);

	// Set drone material properties
	glMaterialfv(GL_FRONT, GL_AMBIENT, drone_mat_ambient);
	glMaterialfv(GL_FRONT, GL_SPECULAR, drone_mat_specular);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, drone_mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SHININESS, drone_mat_shininess);

	// Apply transformations to move drone
	// ...

   // Set up the modeling transformation (M matrix)
   // The current transformation matrix CTM = VM
   // Each object of the drone is multiplied by the CTM
   // p' = VMp
   // p' =  I * V * R(rotate) * T(trans,altitude,0.0) * p
   // global transformations
   // We will surround the drawing  of the drone with glPushMatrix()/glPopMatrix()

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	// floor
	glPushMatrix();
	glScalef(20.0, 0.0, 17.0);
	glBindTexture(GL_TEXTURE_2D, 2007);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();


	//roads
	glPushMatrix();
	glScalef(2.0, 0.07, 17.0);
	glBindTexture(GL_TEXTURE_2D, 2004);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(10.0, 0.0, 0.0);
	glScalef(2.0, 0.07, 17.0);
	glBindTexture(GL_TEXTURE_2D, 2004);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-10.0, 0.0, 0.0);
	glScalef(2.0, 0.07, 17.0);
	glBindTexture(GL_TEXTURE_2D, 2004);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();
	//horizontal roads



	glPushMatrix();
	glTranslatef(0.0, 0.0, -12.0);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	glScalef(2.0, 0.05, 20.0);
	glBindTexture(GL_TEXTURE_2D, 2004);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();



	glPushMatrix();
	glTranslatef(0.0, 0.0, 5);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	glScalef(2.0, 0.05, 20.0);
	glBindTexture(GL_TEXTURE_2D, 2004);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0, 0.0, 13.0);
	glRotatef(90.0, 0.0, 1.0, 0.0);
	glScalef(2.0, 0.05, 20.0);
	glBindTexture(GL_TEXTURE_2D, 2004);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	glPopMatrix();


	if (spawnAI ==true)
	{

	//ai drone start here Drone!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//ai translations
	glPushMatrix();
	glTranslatef(AItransx, AItransy, AItransz);
	glRotatef(AIrotate, 0.0, 1.0, 0.0);
	glScalef(0.4, 0.4, 0.4);

	//drone body
	glPushMatrix();
	glTranslatef(0.0, 3.0, 0.0);
	glScalef(1.0, 1.0, 1.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2006);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBindTexture(GL_TEXTURE_2D, 2009);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//armssssss

	glPushMatrix();
	glTranslatef(0.0, 4.0, 0.0);
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glScalef(3.0, 0.2, 0.2);
	//Cube
// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2000);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	// Rotate the arm such that it is perpendicular to next arm 
	glPushMatrix();
	glTranslatef(0.0, 4.0, 0.0);
	glRotatef(135.0, 0.0, 1.0, 0.0);
	glScalef(3.0, 0.2, 0.2);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2000);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//Crete spheres at the end of each arm to join the drone arms to the propellers

	glPushMatrix();
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glTranslatef(-2.7, 4.5, 0.0);
	glScalef(0.3, 0.1, 0.3);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2002);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPushMatrix();
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glTranslatef(2.7, 4.5, 0.0);
	glScalef(0.3, 0.1, 0.3);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2002);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPushMatrix();
	glRotatef(135.0, 0.0, 1.0, 0.0);
	glTranslatef(2.7, 4.5, 0.0);
	glScalef(0.3, 0.1, 0.3);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2002);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPushMatrix();
	glRotatef(135.0, 0.0, 1.0, 0.0);
	glTranslatef(-2.7, 4.5, 0.0);
	glScalef(0.3, 0.1, 0.3);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2002);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();



	//Create propellers with position on the spheres
	// Propellers rotate on the spheres
	// this is to spin with respect to the base spheres position
	glPushMatrix();
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glTranslatef(-2.7, 4.7, 0.0);
	glRotatef(AIspin, 0.0, 1.0, 0.0);
	glScalef(1.0, 0.1, 0.2);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2003);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPushMatrix();
	glRotatef(45.0, 0.0, 1.0, 0.0);
	glTranslatef(2.7, 4.7, 0.0);
	glRotatef(AIspin, 0.0, 1.0, 0.0);
	glScalef(1.0, 0.1, 0.2);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2003);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPushMatrix();
	glRotatef(135.0, 0.0, 1.0, 0.0);
	glTranslatef(2.7, 4.7, 0.0);
	glRotatef(AIspin, 0.0, 1.0, 0.0);
	glScalef(1.0, 0.1, 0.2);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2003);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPushMatrix();
	glRotatef(135.0, 0.0, 1.0, 0.0);
	glTranslatef(-2.7, 4.7, 0.0);
	glRotatef(AIspin, 0.0, 1.0, 0.0);
	glScalef(1.0, 0.1, 0.2);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2003);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	glPopMatrix();

	//ai drone end here
}
	

//explode



if (explode == true) {
	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, 2010);
	glTranslatef(explodeX + 0.5, explodeY +0.2, explodeZ+0.5);
	glScalef(3.0, 3.0, 3.0);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.5f, 0.5f, -0.5f);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.5f, -0.5f, -0.5f);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-0.5f, 0.5f, 0.5f);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.5f, -0.5f, 0.5f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.5f, -0.5f, -0.5f);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.5f, -0.5f, -0.5f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.5f, 0.5f, -0.5f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.5f, -0.5f, -0.5f);
	glEnd();
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-0.5f, -0.5f, 0.5f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-0.5f, 0.5f, 0.5f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(0.5f, 0.5f, 0.5f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(0.5f, -0.5f, 0.5f);
	glEnd();
	glPopMatrix();
}


	//torpedo!!!!!
	if (torpedo == true && spawnDrone == true) {
		glPushMatrix();
		glTranslatef(Tortransx, altitude, Tortransz);
		glRotatef(rotate, 0.0, 1.0, 0.0);
		glScalef(0.1, 0.1, 1.0);
		//Cube
	// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2000);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

	}









	// user drone start here
	// Apply transformations to construct drone, modify this!
	if (spawnDrone == true) {

		glPushMatrix();
		glTranslatef(transx, altitude, transz);
		glRotatef(rotate, 0.0, 1.0, 0.0);
		glScalef(0.4, 0.4, 0.4);



		//Drone!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		glPushMatrix();
		glTranslatef(0.0, 3.0, 0.0);
		glScalef(1.0, 1.0, 1.0);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2001);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBindTexture(GL_TEXTURE_2D, 2008);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBindTexture(GL_TEXTURE_2D, 2001);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		//armssssss

		glPushMatrix();
		glTranslatef(0.0, 4.0, 0.0);
		glRotatef(45.0, 0.0, 1.0, 0.0);
		glScalef(3.0, 0.2, 0.2);
		//Cube
	// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2000);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		// Rotate the arm such that it is perpendicular to next arm 
		glPushMatrix();
		glTranslatef(0.0, 4.0, 0.0);
		glRotatef(135.0, 0.0, 1.0, 0.0);
		glScalef(3.0, 0.2, 0.2);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2000);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		//Crete spheres at the end of each arm to join the drone arms to the propellers

		glPushMatrix();
		glRotatef(45.0, 0.0, 1.0, 0.0);
		glTranslatef(-2.7, 4.5, 0.0);
		glScalef(0.3, 0.1, 0.3);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2002);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPushMatrix();
		glRotatef(45.0, 0.0, 1.0, 0.0);
		glTranslatef(2.7, 4.5, 0.0);
		glScalef(0.3, 0.1, 0.3);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2002);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPushMatrix();
		glRotatef(135.0, 0.0, 1.0, 0.0);
		glTranslatef(2.7, 4.5, 0.0);
		glScalef(0.3, 0.1, 0.3);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2002);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPushMatrix();
		glRotatef(135.0, 0.0, 1.0, 0.0);
		glTranslatef(-2.7, 4.5, 0.0);
		glScalef(0.3, 0.1, 0.3);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2002);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();



		//Create propellers with position on the spheres
		// Propellers rotate on the spheres
		// this is to spin with respect to the base spheres position
		glPushMatrix();
		glRotatef(45.0, 0.0, 1.0, 0.0);
		glTranslatef(-2.7, 4.7, 0.0);
		glRotatef(spin, 0.0, 1.0, 0.0);
		glScalef(1.0, 0.1, 0.2);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2003);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPushMatrix();
		glRotatef(45.0, 0.0, 1.0, 0.0);
		glTranslatef(2.7, 4.7, 0.0);
		glRotatef(spin, 0.0, 1.0, 0.0);
		glScalef(1.0, 0.1, 0.2);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2003);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPushMatrix();
		glRotatef(135.0, 0.0, 1.0, 0.0);
		glTranslatef(2.7, 4.7, 0.0);
		glRotatef(spin, 0.0, 1.0, 0.0);
		glScalef(1.0, 0.1, 0.2);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2003);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPushMatrix();
		glRotatef(135.0, 0.0, 1.0, 0.0);
		glTranslatef(-2.7, 4.7, 0.0);
		glRotatef(spin, 0.0, 1.0, 0.0);
		glScalef(1.0, 0.1, 0.2);
		//Cube
		// top face of cube
		glBindTexture(GL_TEXTURE_2D, 2003);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glEnd();
		// right face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// left face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glEnd();
		// bottom face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// back face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();
		// front face of cube
		glBegin(GL_QUADS);
		glTexCoord2f(0.0, 0.0);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(0.0, 1.0);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 1.0);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(1.0, 0.0);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glEnd();
		glFlush();
		//cube end
		glPopMatrix();

		glPopMatrix(); //pop the drone off the matrix to prevent translations from interfering with other objects
	}

	
	//Create a building outside of drone transformations
	// We will surround the drawing  of the building parts with glPushMatrix()/glPopMatrix() 
	
	glPushMatrix();
	glTranslatef(-6.0, 4.0, -4.0);
	glScalef(2.0, 4.0, 2.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//building 2
	glPushMatrix();
	glTranslatef(5.0, 5.0, -4.0);
	glScalef(2.0, 5.0, 2.5);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//building 3
	glPushMatrix();
	glTranslatef(-6.0, 3.0, 10.0);
	glScalef(2.0, 3.0, 1.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//building 4
	glPushMatrix();
	glTranslatef(6.0, 4.0, 16.0);
	glScalef(2.0, 4.0, 1.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//building 5
	glPushMatrix();
	glTranslatef(4.0, 7.0, 8.0);
	glScalef(2.0, 7.0, 1.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();


	//building 6
	glPushMatrix();
	glTranslatef(15.0, 6.0, -4.0);
	glScalef(2.0, 6.0, 3.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//building 7
	glPushMatrix();
	glTranslatef(-15.0, 6.0, -4.0);
	glScalef(2.0, 6.0, 3.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	//building 8
	glPushMatrix();
	glTranslatef(-6.0, 4.0, 16.0);
	glScalef(2.0, 4.0, 1.0);
	//Cube
	// top face of cube
	glBindTexture(GL_TEXTURE_2D, 2005);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glEnd();
	// right face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// left face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	// bottom face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// back face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, -1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, -1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, -1.0f);
	glEnd();
	// front face of cube
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex3f(-1.0f, -1.0f, 1.0f);
	glTexCoord2f(0.0, 1.0);
	glVertex3f(-1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 1.0);
	glVertex3f(1.0f, 1.0f, 1.0f);
	glTexCoord2f(1.0, 0.0);
	glVertex3f(1.0f, -1.0f, 1.0f);
	glEnd();
	glFlush();
	//cube end
	glPopMatrix();

	
	glutSwapBuffers();   // Double buffering, swap buffers
}

//Spining fuction
void spinDisplay(void)
{
	spin += 1.0;
	if (spin > 360.0)
		spin -= 360.0;

	glutPostRedisplay();
}
void spinFastDisplay(void)
{

	spin += 3.0;
	if (spin > 360.0)
		spin -= 360.0;
	glutPostRedisplay();
}
void AIspinFastDisplay(void)
{
	AIspin += 20.0;
	if (AIspin > 360.0)
		AIspin -= 360.0;
	glutPostRedisplay();

}

// help printed in console
void help()
{
	printf("How to navigate the drone:  \n");
	printf("Use 's' key to start the drone’s propellers spinning \n");
	printf("Use 'f' the keys to control the forward movement of the drone\n");
	printf("Use 'b' the keys to control the forward movement of the drone\n");
	printf("Use 'left key' the keys to control the left movement of the drone\n");
	printf("Use 'right key' the keys to control the right movement of the drone\n");
	printf("Use 'up key' the keys to control the up movement of the drone\n");
	printf("Use 'down key' the keys to control the down movement of the drone\n");
	printf("Use 't' the key to stop all movement of the drone\n");
	printf("Use 'space_bar' key to fire a torpedo\n");
	printf("How to use the cameras: \n");
	printf("Use 'c' the key to switch to drone camera\n");
	printf("Use '1' key and '2' key while in drone camera to tilt down and up respectively \n");
	printf("Use 'v' key to switch back to world camera\n");
	printf("Use 'z' key to camera zoom in\n");
	printf("Use 'x' key to camera zoom out\n");
	printf("Other features: \n");
	printf("Use 'r' key to respawn your dead drone \n");
	printf("Use 'i' key to respawn the second drone to an inital start point. (this can be used to put them in line for a torpedo easier) \n");
	printf("Use the mouse left click and drag to move world camera around. \n");
	printf("Torpedo will create hit effect on impact with second drone \n");
	printf("Notes: \n");
	printf("If your drone is too close to the ground or buildings or the second drone it will crash and be destroyed. \n");

	return 0;
}


// Callback, called at initialization and whenever user resizes the window.
void reshape(int w, int h)
{
	// Set up viewport, projection, then change to modelview matrix mode - 
	// display function will then set up camera and do modeling transforms.
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(zoom, (GLdouble)w / h, 0.2, 40.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set up the camera at position (0, 6, 22) looking at the origin, up along positive y axis
	//gluLookAt(0.0, 6.0, 22.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

void explosioning() {
	if (explode) {
		explode = false;
		glutPostRedisplay();
		glutTimerFunc(2000, explosioning, 0);
	}
}

void moveAI(int value)
{
	if (spawnAI == true) {
		AIspinFastDisplay();


		//torpedo hit
		if ((AItransy + 1 >= altitude - 0.7) && (AItransy + 1 <= altitude + 0.5) && (AItransx >= Tortransx - 1.5) && (AItransx <= Tortransx + 1.5) && (AItransz > Tortransz - 1.5) && (AItransz < Tortransz + 1.5))
		{
		
			torpedo = false;
			explode = true;
			explodeX = AItransx;
			explodeY = AItransy;
			explodeZ = AItransz;
			glutPostRedisplay();
			glutTimerFunc(1000, explosioning, 0);

			AItransx = rand() % 15 - 7;
			AItransz = rand() % 15 - 7;
			hits += 1;
			printf("YOU GOT A HIT!! Total Hits: %i \n", hits);
		}
		//drones collide
		if ((AItransy + 1 >= altitude - 0.7) && (AItransy + 1 <= altitude + 0.5) && (AItransx >= transx - 1.5) && (AItransx <= transx + 1.5) && (AItransz > transz - 1.5) && (AItransz < transz + 1.5))
		{

			spawnDrone = false;
			AItransx = rand() % 15 - 7;
			AItransz = rand() % 15 - 7;
			deaths += 1;
			printf("Drone Collision! You both crashed! Press 'r' to respawn. Total deaths: %i \n", deaths);
		}


		//boundary box in city and buildings
		if ((AItransx > 20 || AItransz > 17 || AItransx < -20 || AItransz > 17 || AItransx < -20 || AItransz < -13 || AItransx > 20 || AItransz < -13) ||
			((AItransx > -8.0) && (AItransx < -4.0) && (AItransz > -6.0) && (AItransz < -2.0)) ||
			((AItransx > 3.0) && (AItransx < 7.0) && (AItransz > -6.5) && (AItransz < -1.5)) ||
			((AItransx > -8.0) && (AItransx < -4.0) && (AItransz > 9.0) && (AItransz < 11.0)) ||
			((AItransx > 4.0) && (AItransx < 8.0) && (AItransz > 15.0) && (AItransz < 17.0)) ||
			((AItransx > 2.0) && (AItransx < 6.0) && (AItransz > 7.0) && (AItransz < 9.0)) ||
			((AItransx > 13.0) && (AItransx < 17.0) && (AItransz > -7.0) && (AItransz < -1.0)) ||
			((AItransx > -17.0) && (AItransx < -13.0) && (AItransz > -7.0) && (AItransz < -1.0)) ||
			((AItransx > -8.0) && (AItransx < -4.0) && (AItransz > 15.0) && (AItransz < 17.0)))
		{
			AIrotate += rand() % 60 + 240;
			AItransy = rand() % 6 + 1;
			AItransx += 0.6 * sinf(AIrotate * 3.14159 / 180);
			AItransz += 0.6 * cosf(AIrotate * 3.14159 / 180);
		}
		else {
			AItransx += 0.1 * sinf(AIrotate * 3.14159 / 180);
			AItransz += 0.1 * cosf(AIrotate * 3.14159 / 180);
		}
		glutPostRedisplay();
		glutTimerFunc(25, moveAI, 0);
	}
}

void fireTorpedo(int value)
{
	Tortransx -= 0.9 * sinf(rotate * 3.14159 / 180);
	Tortransz -= 0.9 * cosf(rotate * 3.14159 / 180);
	glutPostRedisplay();
	glutTimerFunc(50, fireTorpedo, 0);
}


// Callback, handles input from the keyboard, non-arrow keys
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 's':
		glutIdleFunc(spinDisplay);
		break;

	case 't':
		glutIdleFunc(NULL);
		break;

	case 'h':
	help();
		break;

	case 'H':
		help();
		break;

	}

	if ((key == 'f' || key == 'F') && (spawnDrone == true)) {
		glutIdleFunc(spinFastDisplay);
		if (transx > 20 || transz > 17 || transx < -20 || transz > 17 || transx < -20 || transz < -13 || transx > 20 || transz < -13)
		{
			transx += 0.5 * sinf(rotate * 3.14159 / 180);
			transz += 0.5 * cosf(rotate * 3.14159 / 180);
		}
		else {
			transx -= 0.5 * sinf(rotate * 3.14159 / 180);
			transz -= 0.5 * cosf(rotate * 3.14159 / 180);
		
		}

		if (((altitude < 8.0) && (transx > -8.0) && (transx < -4.0) && (transz > -6.0) && (transz < -2.0)) ||
			((altitude < 10.0) && (transx > 3.0) && (transx < 7.0) && (transz > -6.5) && (transz < -1.5)) ||
			((altitude < 6.0) && (transx > -8.0) && (transx < -4.0) && (transz > 9.0) && (transz < 11.0)) ||
			((altitude < 8.0) && (transx > 4.0) && (transx < 8.0) && (transz > 15.0) && (transz < 17.0)) ||
			((altitude < 14.0) && (transx > 2.0) && (transx < 6.0) && (transz > 7.0) && (transz < 9.0)) ||
			((altitude < 12.0) && (transx > 13.0) && (transx < 17.0) && (transz > -7.0) && (transz < -1.0)) ||
			((altitude < 12.0) && (transx > -17.0) && (transx < -13.0) && (transz > -7.0) && (transz < -1.0)) ||
			((altitude < 8.0) && (transx > -8.0) && (transx < -4.0) && (transz > 15.0) && (transz < 17.0)))
		{
			spawnDrone = false;
			deaths += 1;
			printf("YOU CRASHED INTO A BUILDING! Death count: %i. Press 'r' to reset. \n", deaths);

		}

	}
	else if ((key == 'b' || key == 'B') && (spawnDrone == true)) {
	
		glutIdleFunc(spinFastDisplay);
		if (transx > 20 || transz > 17 || transx < -20 || transz > 17 || transx < -20 || transz < -13 || transx > 20 || transz < -13) {
			transx -= 0.5 * sinf(rotate * 3.14159 / 180);
			transz -= 0.5 * cosf(rotate * 3.14159 / 180);
		}
		else {
			transx += 0.5 * sinf(rotate * 3.14159 / 180);
			transz += 0.5 * cosf(rotate * 3.14159 / 180);
		}

		if (((altitude < 8.0) && (transx > -8.0) && (transx < -4.0) && (transz > -6.0) && (transz < -2.0)) ||
			((altitude < 10.0) && (transx > 3.0) && (transx < 7.0) && (transz > -6.5) && (transz < -1.5)) ||
			((altitude < 6.0) && (transx > -8.0) && (transx < -4.0) && (transz > 9.0) && (transz < 11.0)) ||
			((altitude < 8.0) && (transx > 4.0) && (transx < 8.0) && (transz > 15.0) && (transz < 17.0)) ||
			((altitude < 14.0) && (transx > 2.0) && (transx < 6.0) && (transz > 7.0) && (transz < 9.0)) ||
			((altitude < 12.0) && (transx > 13.0) && (transx < 17.0) && (transz > -7.0) && (transz < -1.0)) ||
			((altitude < 12.0) && (transx > -17.0) && (transx < -13.0) && (transz > -7.0) && (transz < -1.0)) ||
			((altitude < 8.0) && (transx > -8.0) && (transx < -4.0) && (transz > 15.0) && (transz < 17.0)))
		{
			spawnDrone = false;
			deaths += 1;
			printf("YOU CRASHED INTO A BUILDING! Death count: %i. Press 'r' to reset. \n", deaths);
		}

	}



	if (key == '1' && tilt > -900.00) {
		tilt -= 50.0;
		
	}

	else if (key == '2' && tilt < -50.0) {
		tilt += 50.0;
		
	}

	if (key == ' ') {
		torpedo = true;
		Torrotate = rotate;
		Tortransx = transx;
		Tortransz = transz;
		
	}


	if (key == 'x' || key == 'X') {

		zoom += 5;
		reshape(vWidth, vHeight);
	}

	else if (key == 'z' || key == 'Z') {

		zoom -= 5;
		reshape(vWidth, vHeight);
	}

	else if (key == 'c' || key == 'C') {
		dronemode = true;

	}
	else if (key == 'v' || key == 'V') {
		dronemode = false;
	}
//reset drone
	else if (key == 'r' || key == 'R') {
		dronemode = false;
		spawnDrone = true;
	//rotate 
	rotate = 0;
	//translation forward backward
	transx = 0.0;
	transy = 0.0;
	transz = 0.0;
	//altitude
	altitude = 0.0;
	}
	//reset AI drone
	else if (key == 'i' || key == 'I') {
		spawnAI = true;
		//rotate 
		AIrotate = 0;
		//translation forward backward
		AItransx = 0.0;
		AItransy = 5.0;
		AItransz = 0.0;
		
	}


	glutPostRedisplay();   // Trigger a window redisplay
}

// Callback, handles input from the keyboard, function and arrow keys
void functionKeys(int key, int x, int y)
{
	// Help key
	if (key == GLUT_KEY_F1)
	{
		help();
	}
	// Do transformations with arrow keys
	//else if (...)   // GLUT_KEY_DOWN, GLUT_KEY_UP, GLUT_KEY_RIGHT, GLUT_KEY_LEFT
	//{
	//}
	else if (key == GLUT_KEY_RIGHT)
	{
		rotate -= 10;
		rotate %= 360;
	}
	else if (key == GLUT_KEY_LEFT)
	{
		rotate += 10;
		rotate %= 360;
	}

	else if (key == GLUT_KEY_UP)
	{
		altitude += 0.2;
	}
	else if ((key == GLUT_KEY_DOWN) && (spawnDrone == true))
	{
		if (altitude < -0.4) {
			spawnDrone = false;
			deaths += 1;
			printf("YOU CRASHED INTO A THE FLOOR! Death count: %i. Press 'r' to reset. \n", deaths);

		}
		else {
	altitude -= 0.2;
		}
		
	}
	glutPostRedisplay();   // Trigger a window redisplay

}


// Mouse button callback - use only if you want to 
void mouse(int button, int state, int x, int y)
{
	currentButton = button;

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN)
		{
			;

		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (state == GLUT_DOWN)
		{
			;
		}
		break;
	default:
		break;
	}

	glutPostRedisplay();   // Trigger a window redisplay
}


// Mouse motion callback - use only if you want to 
void mouseMotionHandler(int xMouse, int yMouse)
{
	if (currentButton == GLUT_LEFT_BUTTON)
	{
		cameraX = -3 * dimension*sin(3.14 / 180 * xMouse) * cos(3.14 / 180 * yMouse);
		cameraY = 3 * dimension*sin(3.14 / 180 * yMouse);
		cameraZ = 3 * dimension*cos(3.14 / 180 * xMouse) * cos(3.14 / 180 * yMouse);
		
	}

	glutPostRedisplay();   // Trigger a window redisplay
}



int main(int argc, char **argv)
{
	// Initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(vWidth, vHeight);
	glutInitWindowPosition(250, 30);
	glutCreateWindow("Assignment 3");

	// Initialize GL
	initOpenGL(vWidth, vHeight);

	// Register callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotionHandler);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(functionKeys);

	//explode
	glutTimerFunc(100, explosioning, 0);
	//ai stuff
	glutTimerFunc(50, fireTorpedo, 0);
	glutTimerFunc(25, moveAI, 0);
	// Start event loop, never returns
	glutMainLoop();

	return 0;
}


Vector3D ScreenToWorld(int x, int y)
{
	// you will need to finish this if you use the mouse
	return NewVector3D(0, 0, 0);
}



