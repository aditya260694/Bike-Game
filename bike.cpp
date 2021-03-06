#include <stdlib.h> 
#include <math.h> 
#include <cmath> 
#include <stdio.h> 
#include<iostream>
#include<time.h>
#include<string.h>

// The following works for both linux and MacOS
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#define PI 3.141
// escape key (for exit)
#define ESC 27
//#include <OpenGL/OpenGL.h>



#include "imageloader.h"
#include "vec3f.h"
using namespace std;

// DK starts1
GLuint loadTexture(Image* image) {
	GLuint textureId;
	glGenTextures(1, &textureId); //Make room for our texture
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
				 0,                            //0 for now
				 GL_RGB,                       //Format OpenGL uses for image
				 image->width, image->height,  //Width and height
				 0,                            //The border of the image
				 GL_RGB, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
				                   //as unsigned numbers
				 image->pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}
GLuint _textureId;
//DK ends1

// Camera position
float x = 10.0, y = 10.0; // initially 5 units south of origin
float deltaMove = 0.0; // initially camera doesn't move
int cam=0;

// Camera direction
float lx = 0.0, ly = 1.0; // camera points initially along y-axis
float angle = 0.0; // angle of rotation for the camera direction
float deltaAngle = 0.0; // additional angle change when dragging
float bike_angle=0.0;
float turn=0.0;
// Mouse drag control
int isDragging = 0; // true when dragging
int xDragStart = 0; // records the x-coordinate when dragging starts
float dir=0.0;
float accel=0;
int score=0;
float cam_height;
float xcopter=0.0,ycopter=0.0;
float gravity=0.2f;
float xnear=1.0,ynear=1.0;
int gameover=0;
clock_t t1,t2;

typedef struct fossil
{
	float x;
	float y;
}fossil;

fossil f[100];

class Terrain {
	private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		float** color;
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) {
			w = w2;
			l = l2;
			
			hs = new float*[l];
			color=new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
				color[i] = new float[w];
			}
			
			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}
			
			computedNormals = false;
		}
		
		~Terrain() {
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
				delete[] color[i];
			}
			delete[] hs;
			delete[] color;
			
			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}
		
		int width() {
			return w;
		}
		
		int length() {
			return l;
		}
		
		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			color[z][x]=0;
			if(hs[z][x] < -2)
			{
				hs[z][x]=-2;
				color[z][x]=1;
			}			
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}
		
		float getColor(int x, int z) {
			return color[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];
					
					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
};



Terrain* _terrain;

void changeSize(int w, int h) 
{
	float ratio =  ((float) w) / ((float) h); // window aspect ratio
	glMatrixMode(GL_PROJECTION); // projection matrix is active
	glLoadIdentity(); // reset the projection
	gluPerspective(45.0, ratio, 0.1, 100.0); // perspective transformation
	glMatrixMode(GL_MODELVIEW); // return to modelview mode
	glViewport(0, 0, w, h); // set viewport (drawing area) to entire window
}

Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for(int y = 0; y < image->height; y++) {
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}
	
	delete image;
	t->computeNormals();
	return t;
}


void cleanup() {
	delete _terrain;
}


void update(int value) 
{
	t2=clock();
	if((t2-t1)/CLOCKS_PER_SEC>120)
	{
		cout<<"GAME OVER"<<endl;
		gameover=1;
				
	}

	if(gameover==1)
		return;

	// update camera position
		if((deltaMove>0 && deltaMove+accel<0) || (deltaMove<0 && deltaMove+accel>0)) 		
		{
			deltaMove=0;
			accel=0;
		}
			
		else if(deltaMove+accel<=5 && deltaMove+accel>=-1)
		{
			deltaMove+=accel;
		}

		if(_terrain->getColor(x+deltaMove*lx*0.1,y+deltaMove*ly*0.1)==0 && x+deltaMove*lx*0.1>0 && y+deltaMove*ly*0.1>0 && x+deltaMove*lx*0.1 < _terrain->width() && y+deltaMove*ly*0.1 < _terrain->length()  )
		{		
		float f=cam_height;

		x += deltaMove * lx * 0.1;
		y += deltaMove * ly * 0.1;
		

		cam_height=_terrain->getHeight(x,y);
	
		if(f-gravity > cam_height)
		{
			cam_height=f-gravity;
			gravity+=0.1;
		}		
			
		else
			gravity=0.2;		

		}

				
	
		else
		{
			deltaMove=0;
			accel=0;
		}
	
	if(deltaMove<0)
		bike_angle-=turn;
	
	else
		bike_angle+=turn;
	
	int i;

	for(i=0;i<100;i++)			
	{
		if(x>=f[i].x-0.5 && x<=f[i].x+0.5 && y>=f[i].y-0.5 && y<=f[i].y+0.5)
		{
			
			do
			{	
			f[i].x=rand()%(_terrain->width()-5)+3;
			f[i].y=rand()%(_terrain->length()-5)+3;
			}while(_terrain->getColor(f[i].x,f[i].y)==1);

			score++;

			break;	
		}
			
	}
	glutPostRedisplay(); // redisplay everything
	lx=sin(bike_angle*PI/180);
	ly=cos(bike_angle*PI/180);
	glutTimerFunc(5, update,0);
}


void drawBike()
{
	dir=atan(float(-ly)/lx)*180/PI;
	if(lx<0 && -ly<0)
		dir+=180;
	if(lx<0 && -ly>0)
		dir+=180;
	//cout<<"#"<<" "<<x<<" "<<y<<" "<<_terrain->getHeight(x, y)+6<<endl;
	float max=cam_height;

	float i,j;
//	float x_h=x,y_h=y;	
	float x_h=x+0.3*lx,y_h=y+0.3*ly;	
/*	j=y;
	for(i=x;i<=x+0.3*lx;i+=0.01*lx)
	{
				
				if(max <= _terrain->getHeight(i,j))
				{	
					max=_terrain->getHeight(i,j);
					x_h=i;
					y_h=j;
				}

		j+=0.01*ly;
		if(j>0.3*ly+y)
			break;


	}
*/
//	max=_terrain->getHeight(x+0.3*lx,y+0.3*ly);
	

	glPushMatrix();

		glTranslatef(x, y, cam_height+0.2f+6);
		
		glRotatef(90,0,1,0);
		glRotatef(dir,1,0,0);

		if(turn==0)
		glRotatef(0,0,1,0.0);
		
		else if(turn<0)
		glRotatef(-45,0,0,1.0);		
		
		else		
		glRotatef(45,0,0,1.0);				

		glColor3f(1.0, 0.0, 0.0);
		glRotatef(90,1,0,0);				

		glutSolidTorus(0.01,0.03,10,10);

	glPopMatrix();




	glPushMatrix();
		glTranslatef(x, y,cam_height+6+0.28f);
		
		glRotatef(90,0,1,0);
		glRotatef(dir,1,0,0);

		if(turn==0)
		glRotatef(0,x,y,-2.73f);
		
		else if(turn<0)
		glTranslatef(0,0.035,0.001);		
		
		else		
		glTranslatef(0,-0.035,0.001);				

	
		glPushMatrix();		
			
			//glRotatef(asin((_terrain->getHeight(x_h,y_h)-_terrain->getHeight(x,y))/0.3f)*180/PI,0,-1,0);	
			GLUquadricObj *quadratic;
			quadratic=gluNewQuadric();
			glColor3f(0.0, 0.0, 0.0);
			gluCylinder(quadratic,0.05,0.05f,0.3,10,10);
		glPopMatrix();		
	glPopMatrix();





	glPushMatrix();
		//glTranslatef(x+lx, y+ly,1.0f);
		glTranslatef(x_h, y_h,max+6+0.2f);

		
		glRotatef(90,0,1,0);
		glRotatef(dir,1,0,0);
		
		if(turn==0)
		glRotatef(0,0,0,1);
		
		else if(turn<0)
		glRotatef(-45,0,0,1);		
		
		else		
		glRotatef(45,1,0,1);				
		
			
	glPushMatrix();		
		
		glColor3f(1.0, 1.0, 0.0);
		glRotatef(90,1,0,0);				
		glutSolidTorus(0.01,0.03,50,50);
				
	

	glPopMatrix();

	glPopMatrix();

	
}



void renderScene(void) 
{
	if(gameover==1)
		return;
GLfloat _spotlight_position[3];
GLfloat _light_position[4];
GLfloat light_ambient[] = { 5.0, 0.0, 5.0, 0.0 };
GLfloat light_diffuse[] = { 5.0, 0.0, 5.0, 0.0 };
GLfloat light_specular[] = { 1.0, 1.0, 1.0, 0.0 };

_light_position[0] =  x;
_light_position[1] = y;
_light_position[2] = 8.0;
_light_position[3] = 1.0;

_spotlight_position[0] = 0.0;
_spotlight_position[1] = 0.0;
_spotlight_position[2] = 1.0;


glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);

   glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION, _light_position);
    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 5.0);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 2.0);
    glLightfv(GL_LIGHT1,GL_SPOT_DIRECTION,_spotlight_position);

glTranslatef(x,y,cam_height+6+0.3f);
glPushMatrix();
GLfloat light_ambient1[] = { 255.0, 255.0, 0.0, 0.0 };
GLfloat light_diffuse1[] = { 1.0, 1.0, 1.0, 1.0 };

_light_position[0] =  0;
_light_position[1] = 0;
_light_position[2] = 1.0;
_light_position[3] = 1.0;

_spotlight_position[0] = lx;
_spotlight_position[1] = ly;
_spotlight_position[2] = -0.2f;


glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient1);
glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse1);
glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);

   glEnable(GL_LIGHT2);
    glLightfv(GL_LIGHT2, GL_POSITION, _light_position);
    glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 5.0);
    glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 2.0);
    glLightfv(GL_LIGHT2,GL_SPOT_DIRECTION,_spotlight_position);
glPopMatrix();
	int i;
	// Clear color and depth buffers
	glClearColor(0.0, 1.0, 1.0, 0.0); // sky color is light blue
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reset transformations
	glLoadIdentity();
	
	

	// Set the camera centered at (x,y,1) and looking along directional
	// vector (lx, ly, 0), with the z-axis pointing up
	if(cam==0)	
	gluLookAt(
			x,  y , cam_height+7  ,
			x+lx, y+ly, cam_height+7,
			0,    0,  1.0);
	
	if(cam==1)
	gluLookAt(
			x,  y , cam_height+12  ,
			x, y, cam_height+7,
			0.0,    1.0,    0.0);
		

	if(cam==2)
	gluLookAt(
			x+0.07*lx,  y+0.07*ly , cam_height+6.5  ,
			x+lx, y+ly, cam_height+6.5,
			0.0,    0.0,    1.0);
		
	
	if(cam==3)
	gluLookAt(
			x-3*lx,  y-3*ly , cam_height+7  ,
			x, y, cam_height+7,
			0.0,    0.0,    1.0);

	if(cam==4)
	gluLookAt(
			x+xnear*xcopter,  y+ynear*ycopter , cam_height+7  ,
			x, y, cam_height+7,
			0.0,    0.0,    1.0);
	//cout<<x<<" "<<y<<" "<<_terrain->getHeight(x, y)+6<<endl;	

	//gluLookAt(
	//		30,      30,    0,
	//		x, y, 0.0,
	//		0.0,    0.0,    1.0);
	
//glColor3f(3.0, 0.1, 0.0);

	
		// Draw ground - 200x200 square colored green
	glColor3f(0.0, 1.0, 0.0);

	
	
		

	//glBegin(GL_QUADS);
	//	glVertex3f(-100.0, -100.0, 0.0);
	//	glVertex3f(-100.0,  100.0, 0.0);
	//	glVertex3f( 100.0,  100.0, 0.0);
	//	glVertex3f( 100.0, -100.0, 0.0);
	//glEnd();

	
	


	

	
	glColor3f(3.0, 0.7, 4.0);
	glTranslatef(10, 10,1);
	glutSolidSphere(2.05, 20, 20);
	glTranslatef(-10, -10, -1);
	
	glColor3f(10.0, 0.0, 0.0);
	glTranslatef(11, 11,1);
	glutSolidSphere(2.05, 20, 20);
	glTranslatef(-11, -11, -1);

	glColor3f(0.0, 0.0, 10.0);
	glTranslatef(12, 10,1);
	glutSolidSphere(2.05, 20, 20);
	glTranslatef(-12, -10, -1);

	glColor3f(0.0, 10.0, 0.0);
	glTranslatef(11, 9,1);
	glutSolidSphere(2.05, 20, 20);
	glTranslatef(-11, -9, -1);

	drawBike();




// dk start 3
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//	glScalef(scale, scale, scale);
//	glTranslatef(-(float)(_terrain->width() - 1) / 2,
//		-(float)(_terrain->length() - 1) / 2,
//		0.0f);
//	printf("%d\n", _terrain->width());
	
	glColor3f(0.3f, 0.9f, 0.0f);
	for(int z = 0; z < _terrain->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		
		for(int x = 0; x < _terrain->width()-1; x++) {
			if(_terrain->getColor(x, z) == 1)
			{
				glBegin(GL_TRIANGLE_STRIP);
				glColor4f(0.0f, 0.0f, 0.2f,0.5f);
				
				Vec3f normal = _terrain->getNormal(x, z);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(0.0f,0.0f);
				glVertex3f(x, z, _terrain->getHeight(x, z)+6);

				normal = _terrain->getNormal(x+1, z);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(1.0f,0.0f);
				glVertex3f(x+1, z, _terrain->getHeight(x+1, z)+6);
				
				
				normal = _terrain->getNormal(x, z+1);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(0.0f,1.0f);
				glVertex3f(x, z+1, _terrain->getHeight(x, z+1)+6);

				normal = _terrain->getNormal(x+1, z + 1);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(x+1, z + 1, _terrain->getHeight(x+1, z + 1)+6);
				glEnd();
			}
			else
			{
				glBegin(GL_TRIANGLE_STRIP);
				glColor3f(0.3f, 0.9f, 0.0f);

				Vec3f normal = _terrain->getNormal(x, z);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(0.0f,0.0f);
				glVertex3f(x, z, _terrain->getHeight(x, z)+6);

				normal = _terrain->getNormal(x+1, z);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(1.0f,0.0f);
				glVertex3f(x+1, z, _terrain->getHeight(x+1, z)+6);
				
				
				normal = _terrain->getNormal(x, z+1);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(0.0f,1.0f);
				glVertex3f(x, z+1, _terrain->getHeight(x, z+1)+6);

				normal = _terrain->getNormal(x+1, z + 1);
				glNormal3f(normal[0], normal[2], normal[1]);
				glTexCoord2f(1.0f, 1.0f);
				glVertex3f(x+1, z + 1, _terrain->getHeight(x+1, z + 1)+6);

				glEnd();
			}


//			if(x==0.0 && z==0)
//				printf("terr:%f\n", _terrain->getHeight(x, z) );
		}
		
	}
	glDisable(GL_TEXTURE_2D);


	glColor3f(0.3f, 0.9f, 0.0f);
	glColor4f(0.0f, 0.0f, 1.0f, 0.8f);
//water
	
	glColor3f(0.3f, 0.9f, 0.0f);



	glColor3f(0.5f, 0.0f, 0.5f);
// dk end 3
	for(i = 0; i < 100; i++)
		{
			glPushMatrix();
				glTranslatef(f[i].x,f[i].y, _terrain->getHeight(f[i].x,f[i].y)+6+0.5);
			

				glutSolidSphere(0.05, 20, 20); // head sphere
			glPopMatrix();
		}

	if(cam==0)
	{
	glRasterPos3f(x+lx,y+ly,cam_height+7.35);	
	}

	if(cam==1)
	{
	glRasterPos3f(x+1.7,y+1,cam_height+9.35);	
	}

	if(cam==2)
	{
	glRasterPos3f(x+lx,y+ly,cam_height+6.85);	
	}

if(cam==3)
	{
	glRasterPos3f(x-2*lx,y-2*ly,cam_height+7.35);	
	}

if(cam==4)
	{
	glRasterPos3f(x+xnear*xcopter,y+ynear*ycopter,cam_height+6.5);	
	}

	char scre[30]="score :";
	char num[30];
	char timer[30]="Timer :";
	int time=120 - (t2-t1)/CLOCKS_PER_SEC;
	
	snprintf(num,100,"%d",score);	
	strcat(scre,num);
	strcat(scre,"  ");
	strcat(scre,timer);
	snprintf(num,100,"%d",time);	
	strcat(scre,num);
	


	int len=strlen(scre);

	for(i=0;i<len;i++)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, scre[i]);


	/*// dk start3
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, _textureId);
	
	//Bottom
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glColor3f(0.2f, 1.0f, 0.2f);
	glBegin(GL_QUADS);
	
	glNormal3f(0.0, 1.0f, 0.0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(0.0f,0.0f, 2.5f);//_terrain->width(),_terrain->length()
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(_terrain->width(),0, 2.5f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(_terrain->width(),_terrain->length(), -2.5f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(0,_terrain->length(), -2.5f);
	
	glEnd();
	glDisable(GL_TEXTURE_2D);
	// dk ends 3*/

	glutSwapBuffers(); // Make it all visible


} 










void processNormalKeys(unsigned char key, int xx, int yy)
{
	if (key == ESC || key == 'q' || key == 'Q') {exit(0);}
	
	if(key=='c' || key=='C')
		cam=(cam+1)%5;
} 

int flg_left=0;

void pressSpecialKey(int key, int xx, int yy)
{
	switch (key) {
		case GLUT_KEY_UP :if(deltaMove<0)accel=+0.05; else accel=0.01; break;
		case GLUT_KEY_DOWN : if(deltaMove>=0)accel=-0.05; else accel=-0.01; break;
		case GLUT_KEY_LEFT : turn=-2;  break;
		case  GLUT_KEY_RIGHT : turn=2;break;
	}
} 

void releaseSpecialKey(int key, int x, int y) 
{
	switch (key) {
		case GLUT_KEY_UP : if(deltaMove>0)accel=-0.015; else if(deltaMove<0)accel=0.015; break;
		case GLUT_KEY_DOWN : if(deltaMove<0)accel=0.015; else if(deltaMove>0)accel=-0.015;break;
		case GLUT_KEY_LEFT :turn=0; break;
		case GLUT_KEY_RIGHT :turn=0; break;
	}
} 





void mouseMove(int x, int y) 
{ 	
	if (isDragging) { // only when dragging
		// update the change in angle
		deltaAngle = (x - xDragStart) * 0.005;
		
		// camera's direction is set to angle + deltaAngle
		xcopter = +sin(angle + deltaAngle);
		ycopter = -cos(angle + deltaAngle);
		
	}
}

void mouseButton(int button, int state, int x, int y) 
{
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) { // left mouse button pressed
			isDragging = 1; // start dragging
			xDragStart = x; // save x where button first pressed
		}
		else  { /* (state = GLUT_UP) */
			angle += deltaAngle; // update camera turning angle
			isDragging = 0; // no longer dragging
		}
	}
    if (button == 3 && state == GLUT_DOWN)
    {
	angle=atan((y-ynear)/(x-xnear));

	
       
	xnear-=0.1*sin(angle);
	ynear-=0.1*cos(angle);
    

	}
   if (button == 4 && state == GLUT_DOWN)
    {
        angle=atan((y-ynear)/(x-xnear));
	
        xnear+=0.1*sin(angle);
	ynear+=0.1*cos(angle);
    }


	
	

}

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glShadeModel(GL_SMOOTH);
	// DK statrs2
	Image* image1 = loadBMP("grass.bmp");
	_textureId = loadTexture(image1);
	delete image1;
	// DK ends2
}
int main(int argc, char **argv) 
{
	int i;
	printf("\n\
-----------------------------------------------------------------------\n\
  OpenGL Sample Program:\n\
  - Drag mouse left-right to rotate camera\n\
  - Hold up-arrow/down-arrow to move camera forward/backward\n\
  - q or ESC to quit\n\
-----------------------------------------------------------------------\n");
	_terrain = loadTerrain("firstheightmap2.bmp", 20);
	
	//srand(time(NULL));
	
//	cout<<_terrain->width()<<" "<<_terrain->length()<<endl;	

	for(i=0;i<100;i++)
	{
		do
		{
		f[i].x=rand()%(_terrain->width()-5)+3;
		f[i].y=rand()%(_terrain->length()-5)+3;
		}while(_terrain->getColor(f[i].x,f[i].y)==1);
	}
	cam_height=_terrain->getHeight(x,y);
	t1=t2=clock();	
	// general initializations
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(800, 400);
	glutCreateWindow("OpenGL/GLUT Sample Program");
	initRendering();
	// register callbacks
	glutReshapeFunc(changeSize); // window reshape callback
	glutDisplayFunc(renderScene); // (re)display callback
	glutIdleFunc(renderScene); // incremental update 
	
	glutIgnoreKeyRepeat(1); // ignore key repeat when holding key down
	glutMouseFunc(mouseButton); // process mouse button push/release
	glutTimerFunc(5, update, 0);
	glutMotionFunc(mouseMove); // process mouse dragging motion
	glutKeyboardFunc(processNormalKeys); // process standard key clicks
	glutSpecialFunc(pressSpecialKey); // process special key pressed
						// Warning: Nonstandard function! Delete if desired.
	glutSpecialUpFunc(releaseSpecialKey); // process special key release

	// OpenGL init
	glEnable(GL_DEPTH_TEST);

	// enter GLUT event processing cycle
	glutMainLoop();

	return 0; // this is just to keep the compiler happy
}




