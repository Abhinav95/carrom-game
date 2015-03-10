#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <sys/time.h>
#include <bits/stdc++.h>
#include <GL/glut.h>

using namespace std;

#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)
#define RAD2DEG(rad) (rad * 180 / PI)
#define AIMING 0
#define MOVING 1
#define LOADING 2
#define ENDED 3
#define INVALID 10
#define NUMBER_OF_COINS 18

class Coin
{
   public:
      float x;
      float y;
      float velx;
      float vely;
    Coin(float x, float y)
    {
    	this->x = x;
    	this->y = y;
    	this->velx = 0.0f;
    	this->vely = 0.0f;
    }
    Coin()
    {
    	this->x = 0.0f;
    	this->y = 0.0f;
    	this->velx = 0.0f;
    	this->vely = 0.0f;
    }
};

class Pocket
{
	public:
		float x;
		float y;
	Pocket(float x, float y)
	{
		this->x = x;
		this->y = y;
	}
	Pocket()
	{
		this->x = 0.0f;
		this->y = 0.0f;
	}
};

class Image {
    public:
        Image(char* ps, int w, int h);
        ~Image();
        
        /* An array of the form (R1, G1, B1, R2, G2, B2, ...) indicating the
         * color of each pixel in image.  Color components range from 0 to 255.
         * The array starts the bottom-left pixel, then moves right to the end
         * of the row, then moves up to the next column, and so on.  This is the
         * format in which OpenGL likes images.
         */
        char* pixels;
        int width;
        int height;
};

Image::Image(char* ps, int w, int h) : pixels(ps), width(w), height(h) {
}

Image::~Image() {
    delete[] pixels;
}

namespace {
    //Converts a four-character array to an integer, using little-endian form
    int toInt(const char* bytes) {
        return (int)(((unsigned char)bytes[3] << 24) |
                     ((unsigned char)bytes[2] << 16) |
                     ((unsigned char)bytes[1] << 8) |
                     (unsigned char)bytes[0]);
    }
    
    //Converts a two-character array to a short, using little-endian form
    short toShort(const char* bytes) {
        return (short)(((unsigned char)bytes[1] << 8) |
                       (unsigned char)bytes[0]);
    }
    
    //Reads the next four bytes as an integer, using little-endian form
    int readInt(ifstream &input) {
        char buffer[4];
        input.read(buffer, 4);
        return toInt(buffer);
    }
    
    //Reads the next two bytes as a short, using little-endian form
    short readShort(ifstream &input) {
        char buffer[2];
        input.read(buffer, 2);
        return toShort(buffer);
    }
    
    //Just like auto_ptr, but for arrays
    template<class T>
    class auto_array {
        private:
            T* array;
            mutable bool isReleased;
        public:
            explicit auto_array(T* array_ = NULL) :
                array(array_), isReleased(false) {
            }
            
            auto_array(const auto_array<T> &aarray) {
                array = aarray.array;
                isReleased = aarray.isReleased;
                aarray.isReleased = true;
            }
            
            ~auto_array() {
                if (!isReleased && array != NULL) {
                    delete[] array;
                }
            }
            
            T* get() const {
                return array;
            }
            
            T &operator*() const {
                return *array;
            }
            
            void operator=(const auto_array<T> &aarray) {
                if (!isReleased && array != NULL) {
                    delete[] array;
                }
                array = aarray.array;
                isReleased = aarray.isReleased;
                aarray.isReleased = true;
            }
            
            T* operator->() const {
                return array;
            }
            
            T* release() {
                isReleased = true;
                return array;
            }
            
            void reset(T* array_ = NULL) {
                if (!isReleased && array != NULL) {
                    delete[] array;
                }
                array = array_;
            }
            
            T* operator+(int i) {
                return array + i;
            }
            
            T &operator[](int i) {
                return array[i];
            }
    };
}

//Reads a bitmap image from file.
Image* loadBMP(const char* filename);

// Function Declarations
void drawScene();
void update(int value);
void drawBox(float len);
void drawDock(float w, float h);
void drawBall(float rad);
void drawTriangle();
void initRendering();
void handleResize(int w, int h);
void handleKeypress1(unsigned char key, int x, int y);
void handleKeypress2(int key, int x, int y);
void handleMouseclick(int button, int state, int x, int y);
void drawLine(float x1, float y1, float x2, float y2);
void pocketCoin(int coin_index);
void pocketStriker();
void pocketQueen();
void UpdateTimeCounter();
void scoredisplay (float posx, float posy, float posz, float space_char);
void scoreboard();
void endGame(int success);
GLuint loadTexture(Image* image);

GLuint _textureId; //The id of the texture
GLuint _textureId2;
GLuint _textureId3;

// Global Variables

int state;
int success = 0;
int game_ended = 0;
int score = 30;
int last_time = 0;
int mouseleftdown = 0;
int mouserightdown = 0;
int mousex = 0;
int mousey = 0;
float blackx,blacky,whitex,whitey,queenx,queeny,strikerx,strikery;
float scalefactor = 0.008f;
float strike_possible = 0.5f;
float ball_rad = 0.1f;
float pocket_rad = 0.125f;
float box_len = 4.0f;
float friction = 0.975f;
float strike_power = 0.0f;
float max_strike_power = 0.3f;
float min_vel = 0.00001f;
float theta = 0.0f;
float smallbox_len = 1.3f;
Coin Queen = Coin(0.0,0.0);
Coin striker = Coin(0.0f, (smallbox_len/2)-(box_len/2)-ball_rad);
vector<Coin> coinList(NUMBER_OF_COINS);
vector<Pocket> pocketList(4);
vector<int> isPocketed(NUMBER_OF_COINS);
int striker_pocketed = 0;
int queen_pocketed = 0;
float TimeCounter, LastFrameTimeCounter, DT, prevTime = 0.0;
struct timeval tv, tv0;

int main(int argc, char **argv) {
    gettimeofday(&tv0, NULL);

    state = LOADING;

    //Pockets
    pocketList[0] = Pocket(((box_len/2)-pocket_rad),((box_len/2)-pocket_rad));
    pocketList[1] = Pocket(-((box_len/2)-pocket_rad),((box_len/2)-pocket_rad));
    pocketList[2] = Pocket(-((box_len/2)-pocket_rad),-((box_len/2)-pocket_rad));
    pocketList[3] = Pocket(((box_len/2)-pocket_rad),-((box_len/2)-pocket_rad));

    //Coins?
    int i;
    /*coinList[0] = Coin(0.0f,2*ball_rad);
    coinList[2] = Coin(-2*ball_rad*sin(DEG2RAD(60)),0.2f+2*ball_rad*cos(DEG2RAD(60)));
    coinList[4] = Coin(2*ball_rad*sin(DEG2RAD(60)),0.2f+2*ball_rad*cos(DEG2RAD(60)));
    coinList[6] = Coin(-2*ball_rad*sin(DEG2RAD(60)),-2*ball_rad*cos(DEG2RAD(60)));
    coinList[8] = Coin(2*ball_rad*sin(DEG2RAD(60)),-2*ball_rad*cos(DEG2RAD(60)));
    coinList[10] = Coin(-2*ball_rad*sin(DEG2RAD(60)),-2*ball_rad-2*ball_rad*cos(DEG2RAD(60)));
    coinList[12] = Coin(2*ball_rad*sin(DEG2RAD(60)),-2*ball_rad-2*ball_rad*cos(DEG2RAD(60)));
    coinList[14] = Coin(-4*ball_rad,0.0f);
    coinList[16] = Coin(4*ball_rad,0.0f);

    coinList[1] = Coin(0.0f,-2*ball_rad);
    coinList[3] = Coin(0.0f,-4*ball_rad);
    coinList[5] = Coin(-2*ball_rad*sin(DEG2RAD(60)),2*ball_rad*cos(DEG2RAD(60)));
    coinList[7] = Coin(-4*ball_rad*sin(DEG2RAD(60)),4*ball_rad*cos(DEG2RAD(60)));
    coinList[9] = Coin(2*ball_rad*sin(DEG2RAD(60)),2*ball_rad*cos(DEG2RAD(60)));
    coinList[11] = Coin(4*ball_rad*sin(DEG2RAD(60)),4*ball_rad*cos(DEG2RAD(60)));
    coinList[13] = Coin(4*ball_rad*sin(DEG2RAD(60)),-4*ball_rad*cos(DEG2RAD(60)));
    coinList[15] = Coin(-4*ball_rad*sin(DEG2RAD(60)),-4*ball_rad*cos(DEG2RAD(60)));
    coinList[17] = Coin(0.0f,4*ball_rad);*/

    coinList[0] = Coin(0.0f,2*(ball_rad+0.1f));
    coinList[2] = Coin(-2*(ball_rad+0.1f)*sin(DEG2RAD(60)),0.1+0.2f+2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[4] = Coin(2*(ball_rad+0.1f)*sin(DEG2RAD(60)),0.1+0.2f+2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[6] = Coin(-2*(ball_rad+0.1f)*sin(DEG2RAD(60)),-2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[8] = Coin(2*(ball_rad+0.1f)*sin(DEG2RAD(60)),-2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[10] = Coin(-2*(ball_rad+0.1f)*sin(DEG2RAD(60)),-2*(ball_rad+0.1f)-2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[12] = Coin(2*(ball_rad+0.1f)*sin(DEG2RAD(60)),-2*(ball_rad+0.1f)-2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[14] = Coin(-4*(ball_rad+0.1f),0.0f);
    coinList[16] = Coin(4*(ball_rad+0.1f),0.0f);

    coinList[1] = Coin(0.0f,-2*(ball_rad+0.1f));
    coinList[3] = Coin(0.0f,-4*(ball_rad+0.1f));
    coinList[5] = Coin(-2*(ball_rad+0.1f)*sin(DEG2RAD(60)),2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[7] = Coin(-4*(ball_rad+0.1f)*sin(DEG2RAD(60)),4*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[9] = Coin(2*(ball_rad+0.1f)*sin(DEG2RAD(60)),2*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[11] = Coin(4*(ball_rad+0.1f)*sin(DEG2RAD(60)),4*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[13] = Coin(4*(ball_rad+0.1f)*sin(DEG2RAD(60)),-4*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[15] = Coin(-4*(ball_rad+0.1f)*sin(DEG2RAD(60)),-4*(ball_rad+0.1f)*cos(DEG2RAD(60)));
    coinList[17] = Coin(0.0f,4*(ball_rad+0.1f));

    for(i=0;i<isPocketed.size();i++)
        isPocketed[i]=0;
    blackx = -(box_len/2) - smallbox_len/2 -2*ball_rad;
    blacky = -9*ball_rad;
    whitex = -(box_len/2) - smallbox_len/2 +2*ball_rad;
    whitey = -9*ball_rad;
    queenx = whitex;
    queeny = 11*ball_rad;
    strikerx = blackx;
    strikery = 11*ball_rad;

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);

    int w = glutGet(GLUT_SCREEN_WIDTH);
    int h = glutGet(GLUT_SCREEN_HEIGHT);
    int windowWidth = w * 2 / 3;
    int windowHeight = h * 2 / 3;

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition((w - windowWidth) / 2, (h - windowHeight) / 2);

    glutCreateWindow("Carrom - Graphics Assignment 1");  // Setup the window
    initRendering();

    // Register callbacks
    glutDisplayFunc(drawScene);
    glutIdleFunc(drawScene);
    glutKeyboardFunc(handleKeypress1);
    glutSpecialFunc(handleKeypress2);
    glutMouseFunc(handleMouseclick);
    glutReshapeFunc(handleResize);
    glutTimerFunc(10, update, 0);

    glutMainLoop();
    return 0;
}

// Function to draw objects on the screen
void drawScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();

    // Draw Board
    glTranslatef(0.0f, 0.0f, -5.0f);

    //Draw wood
    glBegin(GL_QUADS);
        glColor3f(0.95f, 0.82f, 0.61f);
        glVertex3f(-(box_len/2),-(box_len/2),0.0f);
        glColor3f(0.95f, 0.82f, 0.61f);
        glVertex3f((box_len/2),-(box_len/2),0.0f);
        glColor3f(0.95f, 0.82f, 0.61f);
        glVertex3f((box_len/2),(box_len/2),0.0f);
        glColor3f(0.95f, 0.82f, 0.61f);
        glVertex3f(-(box_len/2),(box_len/2),0.0f);
    glEnd();

    glColor3f(1.0f, 0.0f, 0.0f);
    drawBox(box_len);
    glTranslatef(0.0f, 0.0f, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawBox(box_len-smallbox_len);

    //Circle
    glBegin(GL_LINE_LOOP);
    glColor3f(0,0,0);
    for(int i=0 ; i<360 ; i++) {
       glVertex3f(cos(DEG2RAD(i)), sin(DEG2RAD(i)),0.0f);
    }
    glEnd();

    //8 small circles :P
    glPushMatrix();
    glTranslatef((((box_len-smallbox_len)/2)-ball_rad),(((box_len-smallbox_len)/2)+ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef((((box_len-smallbox_len)/2)+ball_rad),(((box_len-smallbox_len)/2)-ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-(((box_len-smallbox_len)/2)+ball_rad),(((box_len-smallbox_len)/2)-ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-(((box_len-smallbox_len)/2)-ball_rad),(((box_len-smallbox_len)/2)+ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef((((box_len-smallbox_len)/2)-ball_rad),-(((box_len-smallbox_len)/2)+ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef((((box_len-smallbox_len)/2)+ball_rad),-(((box_len-smallbox_len)/2)-ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-(((box_len-smallbox_len)/2)-ball_rad),-(((box_len-smallbox_len)/2)+ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-(((box_len-smallbox_len)/2)+ball_rad),-(((box_len-smallbox_len)/2)-ball_rad),0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    //More Designs
    glPushMatrix();
    drawLine((box_len-smallbox_len)/2 + 2*ball_rad, (box_len-smallbox_len)/2 -ball_rad, (box_len-smallbox_len)/2 + 2*ball_rad, -(box_len-smallbox_len)/2 +ball_rad);
    glPopMatrix();
    glPushMatrix();
    drawLine(-(box_len-smallbox_len)/2 - 2*ball_rad, (box_len-smallbox_len)/2 -ball_rad, -(box_len-smallbox_len)/2 - 2*ball_rad, -(box_len-smallbox_len)/2 +ball_rad);
    glPopMatrix();
    glPushMatrix();
    drawLine(-(box_len-smallbox_len)/2 + ball_rad, (box_len-smallbox_len)/2 +2*ball_rad, (box_len-smallbox_len)/2 - ball_rad, (box_len-smallbox_len)/2 +2*ball_rad);
    glPopMatrix();
    glPushMatrix();
    drawLine(-(box_len-smallbox_len)/2 + ball_rad, -(box_len-smallbox_len)/2 -2*ball_rad, (box_len-smallbox_len)/2 - ball_rad, -(box_len-smallbox_len)/2 -2*ball_rad);
    glPopMatrix();
    
    // Draw Ball
    glPushMatrix();
    glTranslatef(striker.x, striker.y, 0.0f);
    glColor3f(strike_possible, 0.5f, 0.5f);
    drawBall(ball_rad);
    glPopMatrix();


    //Draw Coins
    glPushMatrix();
    glTranslatef(Queen.x, Queen.y, 0.0f);
    glColor3f(0.58f, 0.0f, 0.0f);
    drawBall(ball_rad);
    glPopMatrix();
    int i;
    for(i=0;i<coinList.size();i++)
    {
        glPushMatrix();
        glTranslatef(coinList[i].x , coinList[i].y , 0.0f);
        if(i%2==0)
            glColor3f(0.0f, 0.0f, 0.0f);
        else
            glColor3f(1.0f, 1.0f, 1.0f);
        drawBall(ball_rad);
        glPopMatrix();
    }

    //Draw Pockets
	for(i=0;i<pocketList.size();i++)
    {
        glPushMatrix();
    	glTranslatef(pocketList[i].x, pocketList[i].y, 0.0f);
    	glColor3f(0.65f, 0.65f, 0.42f);
    	drawBall(pocket_rad);
    	glPopMatrix();
    }

    //Draw docks
    glPushMatrix();
    glTranslatef(blackx, -0.1f, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawDock(2*ball_rad,18*ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(whitex, -0.1f, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawDock(2*ball_rad,18*ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(strikerx, strikery, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawDock(2*ball_rad,2*ball_rad);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(queenx, queeny, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawDock(2*ball_rad,2*ball_rad);
    glPopMatrix();

    scoreboard();

    //Draw Aiming help for keyboard
    if(state == AIMING)
    {
        glPushMatrix();
        glTranslatef(striker.x, striker.y, 0.0f);
        drawLine(0.0f,0.0f,10*strike_power*sin(DEG2RAD(theta)),10*strike_power*cos(DEG2RAD(theta)));
        glPopMatrix();
    }

    //Draw Board borders    
    glBegin(GL_QUADS);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2)-ball_rad,-(box_len/2)-ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2),-(box_len/2)-ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2),(box_len/2)+ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2)-ball_rad,(box_len/2)+ball_rad,0.0f);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0,0,0);
        glVertex3f((box_len/2),-(box_len/2)-ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2)+ball_rad,-(box_len/2)-ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2)+ball_rad,(box_len/2)+ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2),(box_len/2)+ball_rad,0.0f);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2),(box_len/2),0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2),(box_len/2),0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2),(box_len/2)+ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2),(box_len/2)+ball_rad,0.0f);
    glEnd();
    glBegin(GL_QUADS);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2),-(box_len/2)-ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2),-(box_len/2)-ball_rad,0.0f);
        glColor3f(0,0,0);
        glVertex3f((box_len/2),-(box_len/2),0.0f);
        glColor3f(0,0,0);
        glVertex3f(-(box_len/2),-(box_len/2),0.0f);
    glEnd();

    if(state == LOADING)
    {
        int w = glutGet(GLUT_WINDOW_WIDTH);
        int h = glutGet(GLUT_WINDOW_HEIGHT);
        glBegin(GL_QUADS);
        glColor3f(1,1,1);
        glVertex3f(-w/2,-h/2,0.0f);
        glColor3f(1,1,1);
        glVertex3f(w/2,-h/2,0.0f);
        glColor3f(1,1,1);
        glVertex3f(w/2,h/2,0.0f);
        glColor3f(1,1,1);
        glVertex3f(-w/2,h/2,0.0f);
        glEnd();

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, _textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        
        glNormal3f(0.0, 0.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-(box_len/2), -(box_len/2), 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(box_len/2, -(box_len/2), 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(box_len/2, box_len/2, 0.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-(box_len/2), box_len/2, 0.0f);
            
        glEnd();
        glDisable(GL_TEXTURE_2D);

    }
    if(state == ENDED)
    {
        if(success)
        {

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, _textureId2);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            
            glNormal3f(0.0, 0.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(-(box_len/2), -(box_len/2), 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(box_len/2, -(box_len/2), 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(box_len/2, box_len/2, 0.0f);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(-(box_len/2), box_len/2, 0.0f);
                
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }
        else
        {

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, _textureId3);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glColor3f(1.0f, 1.0f, 1.0f);
            glBegin(GL_QUADS);
            
            glNormal3f(0.0, 0.0f, 1.0f);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(-(box_len/2), -(box_len/2), 0.0f);
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(box_len/2, -(box_len/2), 0.0f);
            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(box_len/2, box_len/2, 0.0f);
            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(-(box_len/2), box_len/2, 0.0f);
                
            glEnd();
            glDisable(GL_TEXTURE_2D);
        }
    }

    glPopMatrix();
    glutSwapBuffers();
}


// Function to handle all calculations in the scene
// updated evry 10 milliseconds
void update(int value) {
    if(state != LOADING && state != ENDED)
    {
        UpdateTimeCounter();
        if(int(TimeCounter) > last_time)
        {
            last_time = int(TimeCounter);
            score -= 1;
        }
        if(score < 0)
        {
            score = 0;
            endGame(0);
        }
    }
    int i,j;
    if(state == AIMING)
    {
        if(striker_pocketed)
        {
            striker_pocketed=0;
            striker.x = 0;
            striker.y = -(box_len/2) + smallbox_len/2 - ball_rad;
        }
        //Control aiming the shot
        //Movement restrictions on aiming
        if(striker.x < -(box_len/2) + smallbox_len/2 + ball_rad)
        {
            striker.x = -(box_len/2) + smallbox_len/2 + ball_rad;
        }
        if(striker.x > box_len/2 - smallbox_len/2 -ball_rad)
        {
            striker.x = box_len/2 - smallbox_len/2 -ball_rad;
        }
        if(striker.y != -(box_len/2) + smallbox_len/2 + ball_rad)
        {
            striker.y = -(box_len/2) + smallbox_len/2 - ball_rad;
        }
        //Necessary Conditions
        if(strike_power < 0.0f)
        {
            strike_power = 0.0f;
        }
    }
    if(state==MOVING)
    {
        //Reset Aiming variables
        strike_power = 0;
        theta = 0;
    	//Pocketing conditions
        for(i=0;i<pocketList.size();i++)
        {
            //For all the pockets
            if(fabs(striker.x - pocketList[i].x) < ball_rad && fabs(striker.y - pocketList[i].y) < ball_rad)
            {
                pocketStriker();
            }
            if(fabs(Queen.x - pocketList[i].x) < ball_rad && fabs(Queen.y - pocketList[i].y) < ball_rad)
            {
                pocketQueen();
            }
            for(j=0;j<coinList.size();j++)
            {
                if(fabs(coinList[j].x - pocketList[i].x) < ball_rad && fabs(coinList[j].y - pocketList[i].y) < ball_rad)
                {
                    pocketCoin(j);
                }
            }
        }
        
		/*
        proc collide (var b1, b2 : BallData)
            var nx := (b1.x - b2.x) / (b1.r + b2.r)     - the normalised vector in the x direction
            var ny := (b1.y - b2.y) / (b1.r + b2.r)     - the normalised vector in the y direction
            var a1 := b1.vx * nx + b1.vy * ny          - 1st ball impulse
            var a2 := b2.vx * nx + b2.vy * ny          - 2nd ball impulse
            var p := 2 * (a1 - a2) / (b1.m + b2.m)    - resultant impulse

            b1.vxp := b1.vx - p * nx * b2.m              - ball1 resultant Vx value
            b1.vyp := b1.vy - p * ny * b2.m              - ball1 resultant Vy value

            b2.vxp := b2.vx + p * nx * b1.m              - ball2 resultant Vx value
            b2.vyp := b2.vy + p * ny * b1.m              - ball2 resultant Vy value
        end collide 
        */
        
        //New collision code
        //if collision will happen in next update
        if((Queen.x+Queen.velx - striker.x-striker.velx)*(Queen.x+Queen.velx - striker.x-striker.velx) + (Queen.y+Queen.vely - striker.y-striker.vely)*(Queen.y+Queen.vely - striker.y-striker.vely) < 4*ball_rad*ball_rad)
        {
            float tempvelx1 = Queen.velx/100.0f;
            float tempvely1 = Queen.vely/100.0f;
            float tempvelx2 = striker.velx/100.0f;  
            float tempvely2 = striker.vely/100.0f;
            while((Queen.x - striker.x)*(Queen.x - striker.x) + (Queen.y - striker.y)*(Queen.y - striker.y) > 4*ball_rad*ball_rad)
            {
                Queen.x += tempvelx1;
                Queen.y += tempvely1;
                striker.x += tempvelx2;
                striker.y += tempvely2;
            }
            float nx = (Queen.x - striker.x)/(2*ball_rad);
            float ny = (Queen.y - striker.y)/(2*ball_rad);
            float a1 = Queen.velx*nx + Queen.vely*ny;
            float a2 = striker.velx*nx + striker.vely*ny;
            float p = 2*(a1-a2)/(2);
            Queen.velx = Queen.velx - p*nx*1;
            Queen.vely = Queen.vely - p*ny*1;
            striker.velx = striker.velx + p*nx*1;
            striker.vely = striker.vely +p*ny*1;
        }
        for(i=0;i<coinList.size();i++)
        {
            if((coinList[i].x+coinList[i].velx - striker.x-striker.velx)*(coinList[i].x+coinList[i].velx - striker.x-striker.velx) + (coinList[i].y+coinList[i].vely - striker.y-striker.vely)*(coinList[i].y+coinList[i].vely - striker.y-striker.vely) < 4*ball_rad*ball_rad)
            {
                float tempvelx1 = coinList[i].velx/100.0f;
                float tempvely1 = coinList[i].vely/100.0f;
                float tempvelx2 = striker.velx/100.0f;  
                float tempvely2 = striker.vely/100.0f;
                while((coinList[i].x - striker.x)*(coinList[i].x - striker.x) + (coinList[i].y - striker.y)*(coinList[i].y - striker.y) > 4*ball_rad*ball_rad)
                {
                    coinList[i].x += tempvelx1;
                    coinList[i].y += tempvely1;
                    striker.x += tempvelx2;
                    striker.y += tempvely2;
                }
                float nx = (coinList[i].x - striker.x)/(2*ball_rad);
                float ny = (coinList[i].y - striker.y)/(2*ball_rad);
                float a1 = coinList[i].velx*nx + coinList[i].vely*ny;
                float a2 = striker.velx*nx + striker.vely*ny;
                float p = 2*(a1-a2)/(2);
                coinList[i].velx = coinList[i].velx - p*nx*1;
                coinList[i].vely = coinList[i].vely - p*ny*1;
                striker.velx = striker.velx + p*nx*1;
                striker.vely = striker.vely +p*ny*1;
            }
            if((coinList[i].x+coinList[i].velx - Queen.x-Queen.velx)*(coinList[i].x+coinList[i].velx - Queen.x-Queen.velx) + (coinList[i].y+coinList[i].vely - Queen.y-Queen.vely)*(coinList[i].y+coinList[i].vely - Queen.y-Queen.vely) < 4*ball_rad*ball_rad)
            {
                float tempvelx1 = coinList[i].velx/100.0f;
                float tempvely1 = coinList[i].vely/100.0f;
                float tempvelx2 = Queen.velx/100.0f;  
                float tempvely2 = Queen.vely/100.0f;
                while((coinList[i].x - Queen.x)*(coinList[i].x - Queen.x) + (coinList[i].y - Queen.y)*(coinList[i].y - Queen.y) > 4*ball_rad*ball_rad)
                {
                    coinList[i].x += tempvelx1;
                    coinList[i].y += tempvely1;
                    Queen.x += tempvelx2;
                    Queen.y += tempvely2;
                }
                float nx = (coinList[i].x - Queen.x)/(2*ball_rad);
                float ny = (coinList[i].y - Queen.y)/(2*ball_rad);
                float a1 = coinList[i].velx*nx + coinList[i].vely*ny;
                float a2 = Queen.velx*nx + Queen.vely*ny;
                float p = 2*(a1-a2)/(2);
                coinList[i].velx = coinList[i].velx - p*nx*1;
                coinList[i].vely = coinList[i].vely - p*ny*1;
                Queen.velx = Queen.velx + p*nx*1;
                Queen.vely = Queen.vely +p*ny*1;
            }
            for(j=0;j<coinList.size();j++)
            {
                if(i!=j && (coinList[i].x+coinList[i].velx - coinList[j].x-coinList[j].velx)*(coinList[i].x+coinList[i].velx - coinList[j].x-coinList[j].velx) + (coinList[i].y+coinList[i].vely - coinList[j].y-coinList[j].vely)*(coinList[i].y+coinList[i].vely - coinList[j].y-coinList[j].vely) < 4*ball_rad*ball_rad)
                {
                    float tempvelx1 = coinList[i].velx/100.0f;
                    float tempvely1 = coinList[i].vely/100.0f;
                    float tempvelx2 = coinList[j].velx/100.0f;  
                    float tempvely2 = coinList[j].vely/100.0f;
                    while((coinList[i].x - coinList[j].x)*(coinList[i].x - coinList[j].x) + (coinList[i].y - coinList[j].y)*(coinList[i].y - coinList[j].y) > 4*ball_rad*ball_rad)
                    {
                        coinList[i].x += tempvelx1;
                        coinList[i].y += tempvely1;
                        coinList[j].x += tempvelx2;
                        coinList[j].y += tempvely2;
                    }
                    float cx = (coinList[i].x + coinList[j].x)/2;
                    float cy = (coinList[i].y + coinList[j].y)/2;
                    float nx = (coinList[i].x - coinList[j].x)/(2*ball_rad);
                    float ny = (coinList[i].y - coinList[j].y)/(2*ball_rad);
                    float a1 = coinList[i].velx*nx + coinList[i].vely*ny;
                    float a2 = coinList[j].velx*nx + coinList[j].vely*ny;
                    float p = 2*(a1-a2)/(2);
                    coinList[i].velx = coinList[i].velx - p*nx*1;
                    coinList[i].vely = coinList[i].vely - p*ny*1;
                    coinList[j].velx = coinList[j].velx + p*nx*1;
                    coinList[j].vely = coinList[j].vely +p*ny*1;
                }
            }
        }

        // Handle all ball collisions with board border
        if(striker.x + ball_rad > box_len/2 || striker.x - ball_rad < -box_len/2)
            striker.velx *= -1;
        if(striker.y + ball_rad > box_len/2 || striker.y - ball_rad < -box_len/2)
            striker.vely *= -1;
        if(Queen.x + ball_rad > box_len/2 || Queen.x - ball_rad < -box_len/2)
            Queen.velx *= -1;
        if(Queen.y + ball_rad > box_len/2 || Queen.y - ball_rad < -box_len/2)
            Queen.vely *= -1;
        for(i=0;i<coinList.size();i++)
        {
            if(coinList[i].x + ball_rad > box_len/2 || coinList[i].x - ball_rad < -box_len/2)
                coinList[i].velx *= -1;
            if(coinList[i].y + ball_rad > box_len/2 || coinList[i].y - ball_rad < -box_len/2)
                coinList[i].vely *= -1;
        }
        
        //Handle collision errors
        //Make sure coins stay on the board
        if(!striker_pocketed)
        {
            if(striker.x + ball_rad > box_len/2)
                striker.x = box_len/2 - ball_rad;
            if(striker.x - ball_rad < -box_len/2)
                striker.x = -box_len/2 + ball_rad;
            if(striker.y + ball_rad > box_len/2)
                striker.y = box_len/2 - ball_rad;
            if(striker.y - ball_rad < -box_len/2)
                striker.y = -box_len/2 + ball_rad;
        }
        if(!queen_pocketed)
        {
            if(Queen.x + ball_rad > box_len/2)
                Queen.x = box_len/2 - ball_rad;
            if(Queen.x - ball_rad < -box_len/2)
                Queen.x = -box_len/2 + ball_rad;
            if(Queen.y + ball_rad > box_len/2)
                Queen.y = box_len/2 - ball_rad;
            if(Queen.y - ball_rad < -box_len/2)
                Queen.y = -box_len/2 + ball_rad;
        }
        for(i=0;i<coinList.size();i++)
        {
            if(!isPocketed[i])
            {
                if(coinList[i].x + ball_rad > box_len/2)
                    coinList[i].x = box_len/2 - ball_rad;
                if(coinList[i].x - ball_rad < -box_len/2)
                    coinList[i].x = -box_len/2 + ball_rad;
                if(coinList[i].y + ball_rad > box_len/2)
                    coinList[i].y = box_len/2 - ball_rad;
                if(coinList[i].y - ball_rad < -box_len/2)
                    coinList[i].y = -box_len/2 + ball_rad;
            }
        }

        //Apply Friction on all objects
    	striker.velx *= friction;
        striker.vely *= friction;
        Queen.velx *= friction;
        Queen.vely *= friction;
        for(i=0;i<coinList.size();i++)
        {
            coinList[i].velx *= friction;
            coinList[i].vely *= friction;
        }
        // Update position of all objects
        striker.x += striker.velx;
        striker.y += striker.vely;
        Queen.x += Queen.velx;
        Queen.y += Queen.vely;
        for(i=0;i<coinList.size();i++)
        {
            coinList[i].x += coinList[i].velx;
            coinList[i].y += coinList[i].vely;
        }

        //Stop all objects below minimum velocity
        state = AIMING;
        if(fabs(striker.velx) > min_vel || fabs(striker.vely) > min_vel)
            state = MOVING;
        else
            striker.velx = striker.vely = 0;
        if(fabs(Queen.velx) > min_vel || fabs(Queen.vely) > min_vel)
            state = MOVING;
        else
            Queen.velx = Queen.vely = 0;
        for(i=0;i<coinList.size();i++)
        {
            if(fabs(coinList[i].velx) > min_vel || fabs(coinList[i].vely) > min_vel)
                state = MOVING;
            else
                coinList[i].velx = coinList[i].vely = 0;
        }
    }

    game_ended = 1;
    if(!queen_pocketed)
        game_ended = 0;
    for(i=0;i<coinList.size();i++)
    {
        if(i%2==0)
        {
            if(!isPocketed[i])
            {
                game_ended = 0;
            }
        }
    }
    if(game_ended)
        endGame(1);

    glutTimerFunc(10, update, 0);
}

void drawBox(float len) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    glVertex2f(-len / 2, -len / 2);
    glVertex2f(len / 2, -len / 2);
    glVertex2f(len / 2, len / 2);
    glVertex2f(-len / 2, len / 2);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void drawDock(float w, float h) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    glVertex2f(-w / 2, -h / 2);
    glVertex2f(w / 2, -h / 2);
    glVertex2f(w / 2, h / 2);
    glVertex2f(-w / 2, h / 2);
    glEnd();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void drawBall(float rad) {
   glBegin(GL_TRIANGLE_FAN);
   for(int i=0 ; i<360 ; i++) {
       glVertex2f(rad * cos(DEG2RAD(i)), rad * sin(DEG2RAD(i)));
   }
   glEnd();
}

void drawLine(float x1, float y1, float x2, float y2){
    glBegin(GL_LINES);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex3f(x1,y1,0.0f);
    glVertex3f(x2,y2,0.0f);
    glEnd();
}

void drawTriangle() {
    glBegin(GL_TRIANGLES);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-1.0f, -1.0f, 0.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, -1.0f, 0.0f);
    glEnd();
}

// Initializing some openGL 3D rendering options
void initRendering() {
    glEnable(GL_DEPTH_TEST);        // Enable objects to be drawn ahead/behind one another
    glEnable(GL_COLOR_MATERIAL);    // Enable coloring
    //glClearColor(0.95f, 0.82f, 0.61f, 1.0f);   // Setting a background color
    glClearColor(0.7f, 1.0f, 0.7f, 1.0f);

    Image* image = loadBMP("carom.bmp");
    _textureId = loadTexture(image);
    delete image;
    Image* image2 = loadBMP("win.bmp");
    _textureId2 = loadTexture(image2);
    delete image2;
    Image* image3 = loadBMP("lose.bmp");
    _textureId3 = loadTexture(image3);
    delete image3;
}

// Function called when the window is resized
void handleResize(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / (float)h, 0.1f, 200.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void handleKeypress1(unsigned char key, int x, int y) {
    if (key == 27) {
        exit(0);     // escape key is pressed
    }
    if(key == 97)
    {
        theta -= 1;
    }
    if(key == 100)
    {
        theta += 1;
    }
    //Execute the strike
    strike_possible = 0.5f;
    int i;
    if((Queen.x - striker.x)*(Queen.x - striker.x)+(Queen.y - striker.y)*(Queen.y - striker.y) < 4*ball_rad*ball_rad)
        {
            strike_possible = 1.0f;
        }
    for(i=0;i<coinList.size();i++)
    {
        if((coinList[i].x - striker.x)*(coinList[i].x - striker.x)+(coinList[i].y - striker.y)*(coinList[i].y - striker.y) < 4*ball_rad*ball_rad)
        {
            strike_possible = 1.0f;
        }
    }
    if (key == 32 && state == AIMING && strike_possible==0.5f)
    {
        state = MOVING;
        striker.velx = strike_power * sin(DEG2RAD(theta));
        striker.vely = strike_power * cos(DEG2RAD(theta));
    }
    if(state == LOADING)
    {
        state = AIMING;
    }
}

void handleKeypress2(int key, int x, int y) {
    //Aiming mechanics
    if(state == AIMING)
    {
        strike_possible = 0.5f;
        int i;
        if((Queen.x - striker.x)*(Queen.x - striker.x)+(Queen.y - striker.y)*(Queen.y - striker.y) < 4*ball_rad*ball_rad)
            {
                strike_possible = 1.0f;
            }
        for(i=0;i<coinList.size();i++)
        {
            if((coinList[i].x - striker.x)*(coinList[i].x - striker.x)+(coinList[i].y - striker.y)*(coinList[i].y - striker.y) < 4*ball_rad*ball_rad)
            {
                strike_possible = 1.0f;
            }
        }
        if(key == GLUT_KEY_LEFT)
        {
            striker.x -= 0.01;
        }
        if(key == GLUT_KEY_RIGHT)
        {
            striker.x += 0.01;
        }
        if(key == GLUT_KEY_UP)
        {
            strike_power += 0.01;
            if(strike_power > max_strike_power)
                strike_power = max_strike_power;
        }
        if(key == GLUT_KEY_DOWN)
        {
            strike_power -= 0.01;
            if(strike_power < 0)
                strike_power = 0.0f;
        }
    }
    if(state == LOADING)
    {
        state = AIMING;
    }
}

void handleMouseclick(int button, int mouse_state, int x, int y) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);
    scalefactor = (box_len+(1.5*ball_rad))/h;
    //cout<< scalefactor <<endl;
    float cartesianx = scalefactor*(x - w / 2);
    float cartesiany = -scalefactor*(y - h / 2);
    if(state==AIMING)
    {   
        strike_possible = 0.5f;
        int i;
        if((Queen.x - striker.x)*(Queen.x - striker.x)+(Queen.y - striker.y)*(Queen.y - striker.y) < 4*ball_rad*ball_rad)
            {
                strike_possible = 1.0f;
            }
        for(i=0;i<coinList.size();i++)
        {
            if((coinList[i].x - striker.x)*(coinList[i].x - striker.x)+(coinList[i].y - striker.y)*(coinList[i].y - striker.y) < 4*ball_rad*ball_rad)
            {
                strike_possible = 1.0f;
            }
        }
        if(button == GLUT_LEFT_BUTTON)
        {
            mouseleftdown = (mouse_state == GLUT_DOWN);
            //Initiate the strike
            if(strike_possible != 1.0f)
            {
                theta = atan2(cartesianx-striker.x,cartesiany-striker.y);
                strike_power = sqrt((cartesianx-striker.x)*(cartesianx-striker.x)+(cartesiany-striker.y)*(cartesiany-striker.y));
                striker.velx = 0.1*strike_power*sin(theta);
                striker.vely = 0.1*strike_power*cos(theta);
                state = MOVING;
            }
        }
        if(button == GLUT_RIGHT_BUTTON)
        {
            mouserightdown = (mouse_state == GLUT_DOWN);
            //Move the striker
            striker.x = cartesianx;
        }
        mousex = x;
        mousey = y;
    }
}

void pocketCoin(int coin_index)
{
    //Handle what happens when a coin is pocketed
    isPocketed[coin_index] = 1;
    if(coin_index%2==0)
    {
        score += 10;
        coinList[coin_index].x = blackx;
        coinList[coin_index].y = blacky;
        blacky += 2*ball_rad;
    }
    else
    {
        score -= 5;
        coinList[coin_index].x = whitex;
        coinList[coin_index].y = whitey;
        whitey += 2*ball_rad;
    }
    if(score < 0)
        score = 0;
    coinList[coin_index].velx = coinList[coin_index].vely = 0;
}

void pocketStriker()
{
    //Handle what happens when the striker is pocketed
    striker_pocketed = 1;
    striker.x = strikerx;
    striker.y = strikery;
    striker.velx = striker.vely = 0;
}

void pocketQueen()
{
    //Handle what happens when the queen is pocketed
    queen_pocketed = 1;
    score += 50;
    Queen.x = queenx;
    Queen.y = queeny;
    Queen.velx = Queen.vely = 0;
}

void UpdateTimeCounter() {
 LastFrameTimeCounter = TimeCounter;
 gettimeofday(&tv, NULL);
 TimeCounter = (float)(tv.tv_sec-tv0.tv_sec) + 0.000001*((float)(tv.tv_usec-tv0.tv_usec));
 DT = TimeCounter - LastFrameTimeCounter;
}

void scoreboard()
{
    int i;
    //scoreboard box
    /*glPushMatrix();
    glTranslatef(5.5f, 0.0f, -5.0f);
    glColor3f(1.0f, 0.0f, 0.0f);
    drawBox(4.0f,0.0,0.0,0.0);
    glPopMatrix();*/

    //displaying text - Scoreboard
    char strin[] = "SCOREBOARD";
    int len = strlen(strin);
    glColor3f(0.0f,0.0f,0.0f);
    glRasterPos3f(4.3f,1.2f,-5.0f);
    for(i=0;i<len;i++)
    {
        //printf("(here)\n");
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, (int)strin[i]);
    }

    scoredisplay(5.5f,0.5f,-5.0f,0.15f);
}


void scoredisplay (float posx, float posy, float posz, float space_char)
{
        int j,p,k;
        GLvoid *font_style1 = GLUT_BITMAP_TIMES_ROMAN_24;
        p = score;
        j = 0;
        k = 0;
        while(p > 9)
        {
            k = p % 10;
            glRasterPos3f((posx - (j*space_char)),posy, posz);   
            glutBitmapCharacter(font_style1,48+k);
            j++;
            p /= 10;
        }
            glPushMatrix();
            //glTranslatef(1.0f,0.0f,0.0f);
            glRasterPos3f(posx -(j*space_char), posy, posz);   
            glutBitmapCharacter(font_style1,48+p);
            glPopMatrix();
}

void endGame(int successful)
{
    if(successful)
        success=1;
    state = ENDED;
}

Image* loadBMP(const char* filename) {
    ifstream input;
    input.open(filename, ifstream::binary);
    assert(!input.fail() || !"Could not find file");
    char buffer[2];
    input.read(buffer, 2);
    assert(buffer[0] == 'B' && buffer[1] == 'M' || !"Not a bitmap file");
    input.ignore(8);
    int dataOffset = readInt(input);
    
    //Read the header
    int headerSize = readInt(input);
    int width;
    int height;
    switch(headerSize) {
        case 40:
            //V3
            width = readInt(input);
            height = readInt(input);
            input.ignore(2);
            assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
            assert(readShort(input) == 0 || !"Image is compressed");
            break;
        case 12:
            //OS/2 V1
            width = readShort(input);
            height = readShort(input);
            input.ignore(2);
            assert(readShort(input) == 24 || !"Image is not 24 bits per pixel");
            break;
        case 64:
            //OS/2 V2
            assert(!"Can't load OS/2 V2 bitmaps");
            break;
        case 108:
            //Windows V4
            assert(!"Can't load Windows V4 bitmaps");
            break;
        case 124:
            //Windows V5
            assert(!"Can't load Windows V5 bitmaps");
            break;
        default:
            assert(!"Unknown bitmap format");
    }
    
    //Read the data
    int bytesPerRow = ((width * 3 + 3) / 4) * 4 - (width * 3 % 4);
    int size = bytesPerRow * height;
    auto_array<char> pixels(new char[size]);
    input.seekg(dataOffset, ios_base::beg);
    input.read(pixels.get(), size);
    
    //Get the data into the right format
    auto_array<char> pixels2(new char[width * height * 3]);
    for(int y = 0; y < height; y++) {
        for(int x = 0; x < width; x++) {
            for(int c = 0; c < 3; c++) {
                pixels2[3 * (width * y + x) + c] =
                    pixels[bytesPerRow * y + 3 * x + (2 - c)];
            }
        }
    }
    
    input.close();
    return new Image(pixels2.release(), width, height);
}

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