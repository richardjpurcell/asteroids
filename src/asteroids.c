/*
 *	asteroids.c
 *  's' starts/restarts the game
 *  '->' rotates left
 *  '<-' rotates right
 *  'up arrow' moves ship forward
 *  'down arrow' moves ship backwards
 *  'space bar' fires photons
 *  'c' changes asteroids to circles
 *  'a' changes circles to asteroids
 *  'p' slows the game for debugging
 *  'r' resumes game speed
 *  'q' quit
 *   An asteroids game for CSCI3161 based on provided skeleton code.
 *	 original author: Dirk Arnold
 *   additions by: Richard Purcell B00647567
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>
#include <stdio.h>
#include <GL/gl.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG 180.0 / M_PI
#define DEG2RAD M_PI / 180.0

#define myTranslate2D(x, y) glTranslated(x, y, 0.0)
#define myScale2D(x, y) glScalef(x, y, 1.0)
#define myRotate2D(angle) glRotatef(RAD2DEG *angle, 0.0, 0.0, 1.0)

#define MAX_PHOTONS 8
#define MAX_ASTEROIDS 8
#define MAX_VERTICES 16
#define CIRCLE_MULTIPLIER 2.0
#define SHIP_POINTS 3
#define BLAST_POINTS 100
#define MAX_STARS 100

#define drawCircle() glCallList(circle)

/* -- display list for drawing a circle ------------------------------------- */

static GLuint circle;

void buildCircle()
{
    GLint i;

    circle = glGenLists(1);
    glNewList(circle, GL_COMPILE);
    glBegin(GL_POLYGON);
    for (i = 0; i < 40; i++)
        glVertex2d(CIRCLE_MULTIPLIER * cos(i * M_PI / 20.0),
                   CIRCLE_MULTIPLIER * sin(i * M_PI / 20.0));
    glEnd();
    glEndList();
}

/* -- type definitions ------------------------------------------------------ */

typedef struct Coords
{
    double x, y;
} Coords;

typedef struct
{
    double x, y, phi, dx, dy;
} Ship;

typedef struct
{
    int active;
    double x, y, dx, dy;
} Photon;

typedef struct
{
    int active, nVertices;
    double x, y, phi, dx, dy, dphi, viz;
    Coords coords[MAX_VERTICES];
} Asteroid;

typedef struct
{
    Coords location;
    int size;
    double intensity;
} Star;

/* -- function prototypes --------------------------------------------------- */

static void myDisplay(void);
static void myTimer(int value);
static void myKey(unsigned char key, int x, int y);
static void keyPress(int key, int x, int y);
static void keyRelease(int key, int x, int y);
static void myReshape(int w, int h);

static void init(void);
static void initAsteroid(Asteroid *a, double x, double y, double size);
static void drawCounter();
static void drawStar(Star *s);
static void drawShip(Ship *s);
static void drawPhoton(Photon *p);
static void drawAsteroid(Asteroid *a);
static void drawBitmapText(char *string, float x, float y);
static void drawBitmapInt(int i, float x, float y);

static double myRandom(double min, double max);

/* -- global variables ------------------------------------------------------ */

static int up = 0, down = 0, left = 0, right = 0; /* state of cursor keys */
int photonCounter, fps, asteroidType, shipDestroyed, usePointPolyTest, killCount;
static double width = 500.0, height = 300.0, accel, velMax;
static double xMax, yMax;
static Asteroid asteroids[MAX_ASTEROIDS];
static Photon photons[MAX_PHOTONS];
static Star stars[MAX_STARS];
static Ship ship;
static Coords shipP[SHIP_POINTS];
double blastColour, flameX, flameY;

/* -- main ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    srand((unsigned int)time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(width, height);
    glutCreateWindow("Asteroids");
    buildCircle();
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

void myDisplay()
{
    /*
     *	display callback function
     */

    int i, j;

    glClear(GL_COLOR_BUFFER_BIT);

    for (i = 0; i < MAX_STARS; i++)
    {
        drawStar(&stars[i]);
    }

    drawShip(&ship);

    for (i = 0; i < MAX_PHOTONS; i++)
        if (photons[i].active == 1)
        {
            drawPhoton(&photons[i]);
        }

    for (j = 0; j < MAX_ASTEROIDS; j++)
    {
        //if (asteroids[j].active)
        drawAsteroid(&asteroids[j]);
    }

    if ((killCount % 8) == 0 && killCount > 0)
    {
        glColor3f(0.0, 1.0, 0.0);
        glLoadIdentity();
        drawBitmapText("To Continue Press s.", 60, 55);
        glColor3f(1.0, 0.0, 0.0);
        drawBitmapText("To Quit Press q.", 65, 45);
    }

    drawCounter();
    glutSwapBuffers();
}

void myTimer(int value)
{
    /*
     *	timer callback function
     */

    /* rotate the ship */
    if (left == 1)
        ship.phi = ship.phi + 0.1;
    if (right == 1)
        ship.phi = ship.phi - 0.1;

    /* calculate velocity */
    if (up == 1 &&
        (ship.dx <= velMax && ship.dx >= -velMax) &&
        (ship.dy <= velMax && ship.dy >= -velMax))
    {
        ship.dx = ship.dx - (accel * sin(ship.phi));
        ship.dy = ship.dy + (accel * cos(ship.phi));
    }
    if (down == 1 &&
        (ship.dx <= velMax && ship.dx >= -velMax) &&
        (ship.dy <= velMax && ship.dy >= -velMax))
    {
        ship.dx = ship.dx + (accel * sin(ship.phi));
        ship.dy = ship.dy - (accel * cos(ship.phi));
    }

    /* drag */
    if (ship.dx > 0 || ship.dx < 0)
        ship.dx = ship.dx - ship.dx * 0.01;
    if (ship.dy > 0 || ship.dy < 0)
        ship.dy = ship.dy - ship.dy * 0.01;

    /* advance the ship */
    if (ship.x > xMax)
        ship.x = 1;
    else if (ship.x < 0)
        ship.x = xMax;
    else
        ship.x = ship.x + ship.dx;

    if (ship.y > yMax)
        ship.y = 1;
    else if (ship.y < 0)
        ship.y = yMax;
    else
        ship.y = ship.y + ship.dy;

    /* advance photon laser shots, eliminating those that have gone past
      the window boundaries */
    int i, j, k, m;

    for (i = 0; i < MAX_PHOTONS; i++)
    {
        if (photons[i].active == 1)
        {
            photons[i].x = photons[i].x + photons[i].dx;
            photons[i].y = photons[i].y + photons[i].dy;

            if (photons[i].x > xMax || photons[i].x < 0 ||
                photons[i].y > yMax || photons[i].y < 0)
                photons[i].active = 0;
        }
    }

    /* advance asteroids */
    for (j = 0; j < MAX_ASTEROIDS; j++)
    {
        asteroids[j].phi = asteroids[j].phi + asteroids[j].dphi;
        if (asteroids[j].active == 1)
        {
            if (asteroids[j].x > xMax)
                asteroids[j].x = 1;
            else if (asteroids[j].x < 0)
                asteroids[j].x = xMax;
            else
                asteroids[j].x = asteroids[j].x + asteroids[j].dx;

            if (asteroids[j].y > yMax)
                asteroids[j].y = 1;
            else if (asteroids[j].y < 0)
                asteroids[j].y = yMax;
            else
                asteroids[j].y = asteroids[j].y + asteroids[j].dy;
        }
    }
    double x, y, x0, y0, x1, y1, x2, y2, dist, lambda;
    int counter;
    /* test for and handle collisions */
    /* photons and asteroids */
    /* point-polygon test */
    if (asteroidType)
    {
        for (i = 0; i < MAX_PHOTONS; i++)
        {
            if (photons[i].active)
            {
                x = photons[i].x;
                y = photons[i].y;

                for (j = 0; j < MAX_ASTEROIDS; j++)
                {
                    counter = 0;
                    if (asteroids[j].active)
                    {

                        for (k = 0; k < MAX_VERTICES; k++)
                        {
                            x1 = asteroids[j].coords[k].x + asteroids[j].x;
                            y1 = asteroids[j].coords[k].y + asteroids[j].y;
                            x2 = asteroids[j].coords[(k + MAX_VERTICES + 1) % MAX_VERTICES].x + asteroids[j].x;
                            y2 = asteroids[j].coords[(k + MAX_VERTICES + 1) % MAX_VERTICES].y + asteroids[j].y;

                            if ((y1 < y && y < y2) || (y2 < y && y < y1))
                            {
                                if ((((y - y1) / (y2 - y1)) * x2 + ((y2 - y) / (y2 - y1)) * x1) > x)
                                {
                                    counter++;
                                }
                            }
                        }
                    }
                    if ((counter + 2) % 2 != 0)
                    {
                        asteroids[j].active = 0;
                        photons[i].active = 0;
                        killCount++;
                        
                        printf("killCount is: %0.d\n", killCount);
                    }
                }
            }
        }
        for (i = 0; i < SHIP_POINTS; i++)
        {
            if (shipDestroyed != 1)
            {
                x = ship.x + shipP[(i + 4) % 3].x;
                y = ship.y + shipP[(i + 4) % 3].y;

                for (j = 0; j < MAX_ASTEROIDS; j++)
                {
                    counter = 0;
                    if (asteroids[j].active)
                    {

                        for (k = 0; k < MAX_VERTICES; k++)
                        {
                            x1 = asteroids[j].coords[k].x + asteroids[j].x;
                            y1 = asteroids[j].coords[k].y + asteroids[j].y;
                            x2 = asteroids[j].coords[(k + MAX_VERTICES + 1) % MAX_VERTICES].x + asteroids[j].x;
                            y2 = asteroids[j].coords[(k + MAX_VERTICES + 1) % MAX_VERTICES].y + asteroids[j].y;

                            if ((y1 < y && y < y2) || (y2 < y && y < y1))
                            {
                                if ((((y - y1) / (y2 - y1)) * x2 + ((y2 - y) / (y2 - y1)) * x1) > x)
                                {
                                    counter++;
                                }
                            }
                        }
                    }
                    if ((counter + 2) % 2 != 0)
                    {
                        asteroids[j].active = 0;
                        shipDestroyed = 1;
                    }
                }
            }
            else /* point-circle test for photons inside of circle*/
            {
                for (i = 0; i < MAX_PHOTONS; i++)
                {
                    for (j = 0; j < MAX_ASTEROIDS; j++)
                    {
                        if (asteroids[j].active == 1 && photons[i].active == 1)
                        {
                            if ((pow((photons[i].x - asteroids[j].x), 2) +
                                 pow((photons[i].y - asteroids[j].y), 2)) <=
                                pow(CIRCLE_MULTIPLIER, 2))
                            {
                                asteroids[j].active = 0;
                                photons[i].active = 0;
                                killCount++;
                                printf("killCount is: %0.d\n", killCount);
                            }
                        }
                    }
                }
                /* ship and asteroids */
                /* point circle test  for ship vertices and circle*/
                for (i = 0; i < SHIP_POINTS; i++)
                {
                    for (j = 0; j < MAX_ASTEROIDS; j++)
                    {
                        if (asteroids[j].active == 1)
                        {
                            if ((pow((ship.x + shipP[i].x - asteroids[j].x), 2) +
                                 pow((ship.y + shipP[i].y - asteroids[j].y), 2)) <=
                                pow(CIRCLE_MULTIPLIER, 2))
                            {
                                asteroids[j].active = 0;
                                shipDestroyed = 1;
                            }
                        }
                    }
                }
                /* line circle test for ship and circle */
                for (k = 0; k < SHIP_POINTS; k++)
                {
                    x1 = ship.x + shipP[k].x;
                    y1 = ship.y + shipP[k].y;
                    x2 = ship.x + shipP[(k + 4) % 3].x;
                    y2 = ship.y + shipP[(k + 4) % 3].y;

                    for (m = 0; m < MAX_ASTEROIDS; m++)
                    {
                        if (asteroids[m].active == 1)
                        {
                            x0 = asteroids[m].x;
                            y0 = asteroids[m].y;

                            lambda = ((x0 - x1) * (x2 - x1) + (y0 - y1) * (y2 - y1)) /
                                     (pow(x2 - x1, 2) + pow(y2 - y1, 2));

                            dist = pow(x1 - x0 + lambda * (x2 - x1), 2) +
                                   pow(y1 - y0 + lambda * (y2 - y1), 2);

                            if (dist <= pow(CIRCLE_MULTIPLIER, 2) &&
                                lambda >= 0 && lambda <= 1)
                            {
                                asteroids[j].active = 0;
                                shipDestroyed = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    glutPostRedisplay();

    glutTimerFunc(fps, myTimer, value); /* 30 frames per second */
}

void myKey(unsigned char key, int x, int y)
{
    /*
     *	keyboard callback function; add code here for firing the laser,
     *	starting and/or pausing the game, etc.
     */
    int temp;

    switch (key)
    {
    case 32:
        if (photonCounter >= MAX_PHOTONS)
        {
            photonCounter = 0;
        }
        photons[photonCounter].active = 1;
        photons[photonCounter].x = ship.x;
        photons[photonCounter].y = ship.y;
        photons[photonCounter].dx = -(velMax + 0.1) * sin(ship.phi);
        photons[photonCounter].dy = (velMax + 0.1) * cos(ship.phi);
        photonCounter++;
        break;

    //'a' sets asteroid type to jagged
    case 97:
        asteroidType = 1;
        break;
    //'c' sets asteroid type to circle
    case 99:
        asteroidType = 0;
        break;
    //'p' slows down playback for testing
    case 112:
        fps = 1000;
        break;
    //'q' resumes play
    case 113:
        exit(0);
        break;
    //'r' resumes play
    case 114:
        fps = 33;
        break;
    //'s' start
    case 115:
        temp = killCount;
        init();
        killCount = temp;
        break;
    default:
        printf("No command associated with that key.");
    }
}

void keyPress(int key, int x, int y)
{
    /*
     *	this function is called when a special key is pressed; we are
     *	interested in the cursor keys only
     */

    switch (key)
    {
    case 100:
        left = 1;
        break;
    case 101:
        up = 1;
        break;
    case 102:
        right = 1;
        break;
    case 103:
        down = 1;
        break;
    }
}

void keyRelease(int key, int x, int y)
{
    /*
     *	this function is called when a special key is released; we are
     *	interested in the cursor keys only
     */

    switch (key)
    {
    case 100:
        left = 0;
        break;
    case 101:
        up = 0;
        break;
    case 102:
        right = 0;
        break;
    case 103:
        down = 0;
        break;
    }
}

void myReshape(int w, int h)
{
    /*
     *	reshape callback function; the upper and lower boundaries of the
     *	window are at 100.0 and 0.0, respectively; the aspect ratio is
     *  determined by the aspect ratio of the viewport
     */

    xMax = 100.0 * w / h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
}

/* -- other functions ------------------------------------------------------- */

void init()
{
    /*
     * set parameters including the numbers of asteroids and photons present,
     * the maximum velocity of the ship, the velocity of the laser shots, the
     * ship's coordinates and velocity, etc.
     */
    xMax = 100.0 * width / height;
    yMax = 100.0;
    killCount = 0;
    ship.x = xMax / 2.0;
    ship.y = yMax / 2.0;
    velMax = 3.0;
    accel = 0.1;
    photonCounter = 0;
    fps = 33;
    asteroidType = 1;
    shipDestroyed = 0;
    blastColour = 1;
    usePointPolyTest = 1;
    int i;
    double x, y, size;
    //starfield
    for (i = 0; i < MAX_STARS; i++)
    {
        stars[i].location.x = myRandom(0, xMax);
        stars[i].location.y = myRandom(0, yMax);
        stars[i].intensity = myRandom(.1, 0.6);
        stars[i].size = myRandom(1, 5);
    }
    //asteroids
    for (i = 0; i < MAX_ASTEROIDS; i++)
    {
        x = myRandom(1, 100);
        y = myRandom(1, 100);
        size = myRandom(1, 3);

        if (i % 2 > 0)
            initAsteroid(&asteroids[i], 0, y, size);
        else
            initAsteroid(&asteroids[i], x, 0, size);
    }
    //ship
    shipP[0].x = 0;
    shipP[0].y = 4;
    shipP[1].x = -2;
    shipP[1].y = -4;
    shipP[2].x = 2;
    shipP[2].y = -4;
}

void initAsteroid(
    Asteroid *a,
    double x, double y, double size)
{
    /*
     *	generate an asteroid at the given position; velocity, rotational
     *	velocity, and shape are generated randomly; size serves as a scale
     *	parameter that allows generating asteroids of different sizes; feel
     *	free to adjust the parameters according to your needs
     */

    double theta, r;
    int i;

    a->x = x;
    a->y = y;
    a->phi = 0.0;
    a->dx = myRandom(-0.8, 0.8);
    a->dy = myRandom(-0.8, 0.8);
    a->dphi = myRandom(-0.1, 0.1);
    a->viz = 1.0;

    a->nVertices = 6 + rand() % (MAX_VERTICES - 6);
    for (i = 0; i < a->nVertices; i++)
    {
        theta = 2.0 * M_PI * i / a->nVertices;
        r = size * myRandom(1.0, 4.0);
        a->coords[i].x = -r * sin(theta);
        a->coords[i].y = r * cos(theta);
    }

    a->active = 1;
}

void drawCounter(in)
{
    glLoadIdentity();
    glColor3f(1.0, 0.0, 1.0);
    drawBitmapText("SCORE.", 5, yMax-10);

    glRasterPos2f(27, yMax-10);
    char  tempB; 
    tempB = (char)((killCount%10 + 48));
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, tempB);

    glRasterPos2f(23, yMax-10);
    char  tempC; 
    tempC = (char)((killCount/10)%10 + 48);
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, tempC);

}

void drawStar(Star *s)
{
    glLoadIdentity();
    glColor3f(s->intensity, s->intensity, s->intensity);
    glPointSize(s->size);
    glBegin(GL_POINTS);
    glVertex2f(s->location.x, s->location.y);
    glEnd();
}

void drawShip(Ship *s)
{
    glLoadIdentity();
    myTranslate2D(s->x, s->y);
    myRotate2D(s->phi);
    if (!shipDestroyed)
    {
        glColor3f(1.0, 1.0, 1.0);
        glBegin(GL_TRIANGLES);
        glVertex2f(shipP[0].x, shipP[0].y);
        glVertex2f(shipP[1].x, shipP[1].y);
        glVertex2f(shipP[2].x, shipP[2].y);
        glEnd();

        if (up)
        {
            glColor3f(myRandom(0.7, 1.0), myRandom(0.0, 0.5), 0.0);
            flameX = myRandom(-1, 1);
            flameY = myRandom(-5, -12);
            glBegin(GL_TRIANGLES);
            glVertex2f(-2, -4);
            glVertex2f(2, -4);
            glVertex2f(flameX, flameY);
            glEnd();
        }
    }
    else
    {
        double theta = 0;
        double r = 0;

        blastColour = blastColour - 0.01;

        r = myRandom(1.0, 20.0);
        for (int i = 0; i < BLAST_POINTS; i++)
        {
            theta = 2.0 * M_PI * i / 20;
            glPointSize(3);
            glBegin(GL_POINTS);
            glColor3f(myRandom(0.7, 1.0) * blastColour, myRandom(0.0, 0.4) * blastColour, 0.0);
            glVertex2f(-r * sin(theta), r * cos(theta));
            glEnd();
            glColor3f(myRandom(0.7, 1.0) * blastColour, myRandom(0.0, 0.4) * blastColour, 0.0);
            glPointSize(2);
            glBegin(GL_POINTS);
            glVertex2f(-r / 2 * sin(theta + myRandom(0, 4)), r / 2 * cos(theta + myRandom(0, 8)));
            glEnd();
            glColor3f(myRandom(0.7, 1.0) * blastColour, myRandom(0.0, 0.4) * blastColour, myRandom(0.0, 0.6) * blastColour);
            glPointSize(myRandom(0, 5));
            glBegin(GL_POINTS);
            glVertex2f(-r / 5 * sin(theta + 10), r / 5 * cos(theta + 5));
            glEnd();
        }
        glColor3f(0.0, 1.0, 0.0);
        glLoadIdentity();
        drawBitmapText("To Continue Press s.", 60, 55);
        glColor3f(1.0, 0.0, 0.0);
        drawBitmapText("To Quit Press q.", 65, 45);
        killCount = 0;
    }
    glColor3f(1.0, 1.0, 1.0);
}

void drawPhoton(Photon *p)
{
    glLoadIdentity();
    glPointSize(3);
    glColor3f(0.0, 1.0, 1.0);
    myTranslate2D(p->x, p->y);
    if (!shipDestroyed)
    {
        glBegin(GL_POINTS);
        glVertex2f(0, 0);
        glEnd();
    }
}

void drawAsteroid(Asteroid *a)
{
    glLoadIdentity();
    glColor3f(1.0, 1.0, 1.0);
    myTranslate2D(a->x, a->y);
    myRotate2D(a->phi);
    if (!asteroidType)
    {
        drawCircle();
    }
    else
    {
        if (a->active == 1)
        {
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < MAX_VERTICES; i++)
            {
                glVertex2f(a->coords[i].x, a->coords[i].y);
            }
            glEnd();
        }
        else
        {
            if (a->viz > 0)
                a->viz = a->viz - 0.02;
            glColor3f(a->viz, a->viz, a->viz);
            glPointSize(1);
            glBegin(GL_POINTS);
            glVertex2f(myRandom(0, 3) * sin(myRandom(0, 4)), myRandom(0, 3) * cos(myRandom(0, 8)));
            glVertex2f(myRandom(0, 5) * sin(myRandom(0, 4)), myRandom(0, 5) * cos(myRandom(0, 8)));
            glVertex2f(myRandom(0, 8) * sin(myRandom(0, 4)), myRandom(0, 8) * cos(myRandom(0, 8)));
            glEnd();
        }
    }
}

/* -- helper function ------------------------------------------------------- */

double
myRandom(double min, double max)
{
    double d;

    /* return a random number uniformly draw from [min,max] */
    d = min + (max - min) * (rand() % 0x7fff) / 32767.0;

    return d;
}

void drawBitmapText(char *string, float x, float y)
{
    char *c;
    glRasterPos2f(x, y);

    for (c = string; *c != '.'; c++)
    {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *c);
    }
}
