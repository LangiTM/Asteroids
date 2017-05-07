/*
 *	asteroids.c
 *  Tyler Langille
 */


#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG 180.0/M_PI
#define DEG2RAD M_PI/180.0

#define myTranslate2D(x,y) glTranslated(x, y, 0.0)
#define myScale2D(x,y) glScalef(x, y, 1.0)
#define myRotate2D(angle) glRotatef(RAD2DEG*angle, 0.0, 0.0, 1.0)
#define S_HEIGHT 4.0
#define S_WIDTH 3.0

#define MAX_PHOTONS	8
#define MAX_ASTEROIDS	16
#define MAX_VERTICES	16


#define drawCircle() glCallList(circle)

/* display list for drawing a circle */ 

static GLuint	circle;

void
buildCircle() {
    GLint   i;

    circle = glGenLists(1);
    glNewList(circle, GL_COMPILE);
      glBegin(GL_POLYGON);
        for(i=0; i<40; i++)
            glVertex2d(cos(i*M_PI/20.0), sin(i*M_PI/20.0));
      glEnd();
    glEndList();
}

/*  type definitions */

typedef struct Coords {
	double		x, y;
} Coords;

typedef struct {
	double	x, y, phi, dx, dy;
} Ship;

typedef struct {
	int	active;
	double	x, y, dx, dy;
} Photon;

typedef struct {
	int	active, nVertices;
  double	x, y, phi, dx, dy, dphi, size;
	Coords	coords[MAX_VERTICES];
} Asteroid;


/* -- function prototypes --------------------------------------------------- */

static void	myDisplay(void);
static void	myTimer(int value);
static void	myKey(unsigned char key, int x, int y);
static void	keyPress(int key, int x, int y);
static void	keyRelease(int key, int x, int y);
static void	myReshape(int w, int h);

static void	init();
static void	initAsteroid(Asteroid *a, double x, double y, double size);
static void	drawShip(Ship *s);
static void	drawPhoton(Photon *p);
static void	drawAsteroid(Asteroid *a);

static double	myRandom(double min, double max);
static int      pointInAst(Asteroid *a, double x, double y);
static void     displayText(double x, double y, double r, double g, double b, const char *string);

/* -- global variables ------------------------------------------------------ */

static int	up=0, down=0, left=0, right=0;	/* state of cursor keys */
static double	xMax, yMax, min=-1.5, max=1.5;
static double   wWidth = 750.0, wHeight = 450.0;
static double   maxVel; 
static Ship	ship;
static Photon	photons[MAX_PHOTONS];
static Asteroid	asteroids[MAX_ASTEROIDS];
static double   r=0.5, g=0.5, b=0.5;
static int      lives=3, score=0, level=1, asts=0, invTime;

/* -- main ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(wWidth, wHeight);
    glutCreateWindow("Asteroids");
    /* buildCircle(); */
    glutDisplayFunc(myDisplay);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(myKey);
    glutSpecialFunc(keyPress);
    glutSpecialUpFunc(keyRelease);
    glutReshapeFunc(myReshape);
    glutTimerFunc(33, myTimer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    init();
    glutMainLoop();
    return 0;
}


/* -- callback functions ---------------------------------------------------- */

void
myDisplay()
{
    /*
	display callback function
    */ 

    int	i;
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    if (lives > 0)
      drawShip(&ship);
    else {
      const char* o = "GAME OVER";
      const char* u = "Press R to play again";
      displayText(50.0*wWidth/wHeight-18.0, 50.0, 1.0, 0.0, 0.0, o);
      displayText(50.0*wWidth/wHeight-25.0, 45.0, 1.0, 1.0, 1.0, u);
    }
    for (i=0; i<MAX_PHOTONS; i++) {
    	if (photons[i].active) {
            drawPhoton(&photons[i]);
	}
    }
    for (i=0; i<MAX_ASTEROIDS; i++) {
      if (asteroids[i].active) {
	drawAsteroid(&asteroids[i]);
      }
    }
    glLoadIdentity();
    if (invTime > 0&&lives>0) {
      const char* p = "Invincible";
      displayText(50.0*wWidth/wHeight-15.0, 5.0, 0.0, 0.7, 1.0, p);
    }
    char* t;
    sprintf(t, "Level: %d\tLives: %d\tScore: %d", level, lives, score);
    const char* s = t;
    displayText(5.0, 95.0, 1.0, 1.0, 1.0, s);
    glutSwapBuffers();
}

void
myTimer(int value)
{
  if (invTime > 0) invTime--;
  int i;
  asts=0;
  for (i=0; i<MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      asts++;
    }
  }
  if (asts==0) {
    level++;
    init();
    maxVel+=maxVel/2;
  }
    /*
     *	timer callback function
     */

    /* advance the ship */
  if (up==1) {
    ship.dy += (0.75)*cos(ship.phi);
    ship.dx -= (0.75)*sin(ship.phi);
  }
  if (down==1) {
    ship.dy -= (0.75)*cos(ship.phi);
    ship.dx += (0.75)*sin(ship.phi);
  }
  if (right==1) ship.phi-=0.50;
  if (left==1) ship.phi+=0.50;

  if (ship.dy > maxVel) ship.dy = maxVel;
  if (ship.dx < -maxVel) ship.dx = -maxVel;
  if (ship.dy < -maxVel) ship.dy = -maxVel;
  if (ship.dx > maxVel) ship.dx = maxVel;

  ship.x += ship.dx * 0.03;
  ship.y += ship.dy * 0.03;

  if(ship.x < 0.0) ship.x = 100.0*wWidth/wHeight;
  if(ship.x > 100.0*wWidth/wHeight) ship.x = 0.0;
  if(ship.y < 0.0) ship.y = 100.0;
  if(ship.y > 100.0) ship.y = 0.0;

    /* advance photon laser shots, eliminating those that have gone past
      the window boundaries */
  for (i=0; i<MAX_PHOTONS; i++) {
    if (photons[i].active) {
      if ((photons[i].x < 0.0) || (photons[i].x > 100.0*wWidth/wHeight) || (photons[i].y < 0.0) || (photons[i].y > 100.0)) {
	photons[i].active = 0;
      }
      photons[i].x += photons[i].dx;
      photons[i].y += photons[i].dy;
    }
  }
    /* advance asteroids */
  for (i=0; i<MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      asteroids[i].x += asteroids[i].dx;
      if (asteroids[i].dx > maxVel) asteroids[i].dx = maxVel;
      if (asteroids[i].dx < -maxVel) asteroids[i].dx = -maxVel; 
      asteroids[i].y += asteroids[i].dy;
      if (asteroids[i].dy > maxVel) asteroids[i].dy = maxVel;
      if (asteroids[i].dy < -maxVel) asteroids[i].dy = -maxVel; 
      asteroids[i].phi += asteroids[i].dphi;
  
      if (asteroids[i].x < -10.0) {
	asteroids[i].dx = -asteroids[i].dx;
      }
      else if (asteroids[i].x > 100.0*wWidth/wHeight+10.0) {
	asteroids[i].dx = -asteroids[i].dx;
      }
      if (asteroids[i].y < -10.0) {
	asteroids[i].dy = -asteroids[i].dy;
      }
      else if (asteroids[i].y > 110.0) {
	asteroids[i].dy = -asteroids[i].dy;
      }
    }
  }
    /* test for and handle collisions */
  
  int j = 0;
  for (i=0; i<MAX_ASTEROIDS; i++) {
    if (asteroids[i].active) {
      for (j=0; j<MAX_PHOTONS; j++) {
	if (photons[j].active) {
	  /*  Check if photon inside bounding box  */	  
	  if ((photons[j].x > asteroids[i].x-10.0) && (photons[j].x < asteroids[i].x+10.0) && (photons[j].y > asteroids[i].y-10.0) && (photons[j].y < asteroids[i].y+10.0)) {
	    /* Check if photon inside asteroid */
	    if (pointInAst(&asteroids[i], photons[j].x, photons[j].y)) {
	      score++;
	      asteroids[i].active = 0;
	      photons[j].active = 0;
	      /* if asteroid large enough to have smaller pieces */
	      if (asteroids[i].size >= 2.0) {
		double x=asteroids[i].x, y=asteroids[i].y, s=asteroids[i].size-1.2;
		initAsteroid(&asteroids[i], x, y, s);
		int m;
		for (m=0; m<MAX_ASTEROIDS; m++) {
		  if (asteroids[m].active == 0) {
		    initAsteroid(&asteroids[m], x, y, s);
		    break;
		  }
		}
 	      }
	    }
	  }
	}
	/* Check if ship inside bounding box */
	if ((ship.x>asteroids[i].x-10.0) && (ship.x<asteroids[i].x+10.0) && (ship.y>asteroids[i].y-10.0) && (ship.y<asteroids[i].y+10.0) && invTime == 0) {
	  /* Check if ship points inside asteroid */
	  if (pointInAst(&asteroids[i], ship.x, ship.y+S_HEIGHT) || pointInAst(&asteroids[i], ship.x+S_WIDTH, ship.y-S_HEIGHT) || pointInAst(&asteroids[i], ship.x-S_WIDTH, ship.y-S_HEIGHT) || pointInAst(&asteroids[i], ship.x-0.5*S_WIDTH, ship.y) || pointInAst(&asteroids[i], ship.x+0.5*S_WIDTH, ship.y) || pointInAst(&asteroids[i], ship.x, ship.y-S_HEIGHT)) {
	    asteroids[i].active = 0;
	    /* if asteroid large enough to have smaller pieces */
	    if (asteroids[i].size >= 2.0) {
	      double x=asteroids[i].x, y=asteroids[i].y, s=asteroids[i].size-1.2;
	      initAsteroid(&asteroids[i], x, y, s);
	      int m;
	      for (m=0; m<MAX_ASTEROIDS; m++) {
		if (asteroids[m].active == 0) {
		  initAsteroid(&asteroids[m], x, y, s);
		  break;
		}
	      }
 	    }
	    /* reset ship to initial position */
	    ship.x=50.0*(wWidth/wHeight);
	    ship.y=50.0;
	    ship.dx = 0.0;
	    ship.dy = 0.0;
	    ship.phi = 0.0;
	    /* put up the ship's respawn shield */
	    invTime = 90;
	    if (lives>0) lives--;
	  }
	} 
      }
    }
  }
    glutPostRedisplay();
    glutTimerFunc(33, myTimer, value);		/* 30 frames per second */
}

void
myKey(unsigned char key, int x, int y)
{
    /*
     *	keyboard callback function; add code here for firing the laser,
     *	starting and/or pausing the game, etc.
     */
  int i = 0;
  /* Primary fire key */
  if (key==' '&&lives>0) {
    while (i < MAX_PHOTONS) { 
      if (!photons[i].active) {
	photons[i].active = 1;
	photons[i].x = ship.x;
	photons[i].y = ship.y;
	photons[i].dx = -1.5*sin(ship.phi);
	photons[i].dy = 1.5*cos(ship.phi);
	drawPhoton(&photons[i]);
	break;
      }
      i++;
    } 
  }
  /* Restart key */
  if (key=='r'||key=='R') {
    level = 1;
    lives = 3;
    score = 0;
    init();
  }
  /* 'Godmode' key for dev testing */
  if ((key=='g'||key=='G') && lives>0) {
    if (lives > 3)
      lives = 3;
    else lives = 1000;
  }
}

void
keyPress(int key, int x, int y)
{
    /*
     *	this function is called when a special key is pressed; we are
     *	interested in the cursor keys only
     */
  if (lives>0) {
    switch (key)
    {
        case 100:
            left = 1; break;
        case 101:
            up = 1; break;
	case 102:
            right = 1; break;
        case 103:
            down = 1; break;
    }
  }
}

void
keyRelease(int key, int x, int y)
{
    /*
     *	this function is called when a special key is released; we are
     *	interested in the cursor keys only
     */

    switch (key)
    {
        case 100:
            left = 0; break;
        case 101:
            up = 0; break;
	case 102:
            right = 0; break;
        case 103:
            down = 0; break;
    }
}

void
myReshape(int w, int h)
{
    /*
     *	reshape callback function; the upper and lower boundaries of the
     *	window are at 100.0 and 0.0, respectively; the aspect ratio is
     *  determined by the aspect ratio of the viewport
     */

    xMax = 100.0*w/h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glutPostRedisplay();
}


/* -- other functions ------------------------------------------------------- */

void
init()
{
    /*
     * set parameters including the numbers of asteroids and photons present,
     * the maximum velocity of the ship, the velocity of the laser shots, the
     * ship's coordinates and velocity, etc.
     */
  int i;
  min=-1.5;
  max=1.5;
  /* Make the max speed faster each level */
  if (level > 1) {
    min=min+level*(min/4);
    max=max+level*(max/4);
  }
  ship.x=50.0*(wWidth/wHeight);
  ship.y=50.0;
  ship.phi=0.0;
  ship.dx=0.0;
  ship.dy=0.0;
  maxVel = 25.0;
  /* Clear any asteroids that have been partially destroyed */
  for (i=0; i<MAX_ASTEROIDS; i++) {
    asteroids[i].active = 0;
  }
  for (i=0; i<MAX_ASTEROIDS/2; i++) {
    double ran = myRandom(0.0, 4.0);
    double  x, y;
    if (ran < 1.0) {
      x = 0.0;
      y = myRandom(0.0, 100.0);
    }
    else if (ran < 2.0) {
      x = 100.0*wWidth/wHeight;
      y = myRandom(0.0, 100.0);
    }
    else if (ran < 3.0) {
      x = myRandom(0.0, 100.0*wWidth/wHeight);
      y = 0.0;
    }
    else {
      x = myRandom(0.0, 100.0*wWidth/wHeight);
      y = 100.0;
    }
    initAsteroid(&asteroids[i], x, y, 3.0); 
  }  
}

void
initAsteroid(Asteroid *a, double x, double y, double size)
{
    /*
     *	generate an asteroid at the given position; velocity, rotational
     *	velocity, and shape are generated randomly; size serves as a scale
     *	parameter that allows generating asteroids of different sizes; feel
     *	free to adjust the parameters according to your needs
     */

    double	theta, r;
    int		i;
        
    a->x = x;
    a->y = y;
    a->phi = 0.0;
    a->dx = myRandom(min, max);
    a->dy = myRandom(min, max);
    a->dphi = myRandom(-0.015, 0.015);
    
    a->size = size;
    a->nVertices = 6+rand()%(MAX_VERTICES-6);
    for (i=0; i<a->nVertices; i++)
    {
	theta = 2.0*M_PI*i/a->nVertices;
	r = size*myRandom(2.0, 3.0);
	a->coords[i].x = -r*sin(theta);
	a->coords[i].y = r*cos(theta);
    }
    a->active = 1;
}


void
drawShip(Ship *s)
{
  int i;
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  myTranslate2D(s->x, s->y);
  myRotate2D(s->phi);
  myTranslate2D(-s->x, -s->y);
  if (invTime > 0) glColor3f(0.0, 0.7, 1.0);
  else glColor3f(0.5, 0.5, 0.5);
  glBegin(GL_TRIANGLES);
    glVertex2f(ship.x, ship.y+S_HEIGHT);
    if (up == 1 && invTime == 0) glColor3f(1.0, 0.5, 0.0);
    else if (invTime == 0) glColor3f(0.2, 0.7, 0.8);
    else glColor3f(0.0, 0.0, 1.0);
    glVertex2f(ship.x-S_WIDTH, ship.y-S_HEIGHT);
    glVertex2f(ship.x+S_WIDTH, ship.y-S_HEIGHT);
    glColor3f(1.0, 0.5, 0.0);
  glEnd();
  glPopMatrix();
}

void
drawPhoton(Photon *p)
{
  glColor3f(0.0, 0.2, 1.0);
  glBegin(GL_LINE_LOOP);
    glVertex2f(p->x, p->y);
    glVertex2f(p->x+0.3, p->y+0.3);
    glVertex2f(p->x-3*p->dx, p->y-3*p->dy);  
  glEnd();
}

void
drawAsteroid(Asteroid *a)
{
  int i;
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  myTranslate2D(a->x, a->y);
  myRotate2D(a->phi);
  glColor3f(r, g, b);
  glBegin(GL_LINE_LOOP);
    for(i=0; i<a->nVertices; i++) {
      glVertex2f(a->coords[i].x, a->coords[i].y);
    } 
  glEnd();
  glFlush();
}


/* -- helper function ------------------------------------------------------- */

double
myRandom(double min, double max)
{
	double	d;
	
	/* return a random number uniformly draw from [min,max] */
	d = min+(max-min)*(rand()%0x7fff)/32767.0;	
	return d;
}

int
pointInAst(Asteroid *a, double x, double y)
{
  int i, j, c=0;
  for (i=0, j=1; i<a->nVertices; i++, j=(j+1)%a->nVertices) {
    double axi = a->x+a->coords[i].x;
    double axj = a->x+a->coords[j].x;
    double ayi = a->y+a->coords[i].y;
    double ayj = a->y+a->coords[j].y;
    if (( ((axi<x) && (x<=axj)) || ((axj<x) && (x<=axi))) && (y>ayi + (ayj-ayi)/(axj-axi)*(x-axi))) {
      c++;
    }
  }
  /* If ray from point passes odd number of lines, point is in polygon */
  return c%2==1;
}

void
displayText(double x, double y, double r, double g, double b, const char *string) {
  int k = strlen(string), i;
  glColor3f(r,g,b);
  glRasterPos2f(x,y);
  for (i=0; i<k; i++) {
    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
  }
}
