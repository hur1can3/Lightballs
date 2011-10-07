// lightballs.c
// Authors: Matthew Levandowski
//          Paul Levandoski
//          Kyle Johnson

//CHANGELOG:
// ADDED REFLECTIONS, SHADOWS, TEXTURE MAPS, MENU, FULLSCREEN GAME //MODE
#include <GL/glut.h>
#include <GL/glext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <stdarg.h>

// Some <math.h> files do not define M_PI...
#ifndef M_PI
#define M_PI 3.14159265
#endif

//macros
#define TRUE 1
#define FALSE !TRUE
#define MIN(a,b) ((a)>(b)?(b):(a))
#define FSIZE 32
#define WIDTH 600
#define HEIGHT 800
#define FRAME_RATE_SAMPLES 50

// Number of spheres in the game
#define sphere_count 20
#define respawn_time 5000

//-----------------------------------------------------------------------------
// 3D vector
//-----------------------------------------------------------------------------
struct Vector3 {
    float x, y, z;
};

//-----------------------------------------------------------------------------
// Third Person Camera structure
//-----------------------------------------------------------------------------
struct ThirdPersonCamera_t {
    struct Vector3 vecPos;
    struct Vector3 vecRot;
    float fRadius;          // Distance between the camera and the object.
    float fLastX;
    float fLastY;
};

//-----------------------------------------------------------------------------
// Sphere structure
//-----------------------------------------------------------------------------
struct Sphere_t {
    struct Vector3 position;    // Position in 3D space
    int selected;               // Did OpenGL select this one?
    int dead;                   // Is this sphere dead?
    unsigned int death_time;    // How long has this sphere been dead?
    // Respawn after X milliseconds
    float distance;             // Distance between you and the sphere.
    float size;                 // The size of the sphere. Decreases during
    // death phase.
};


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------

//spheres
struct Sphere_t spheres[sphere_count];

// our camera
struct ThirdPersonCamera_t camera;

// for selection of spheres
GLuint select_buffer[sphere_count*4] = {0};
GLint hits = 0;
int score = 0;

// angle
static float lightAngle = 0.0, lightHeight = 20;
int moving, startx, starty;
int lightMoving = 0, lightStartX, lightStartY;
 
// width of player body
static GLdouble bodyWidth = 3.0;
 
//bike variables
GLfloat bikeTireAngle = 0.0, bikeHandlAngle = 0.0, bikeAngle = 0.0;
float bikex = 0.0, bikey = 0.0, bikez = 0.0;
 
//distance between camera and player
float dist_cam_player;
 
//ANGLES OF THE player in space
float movement_angle;
 
// position of the light
static GLfloat lightPosition[4];
static GLfloat lightColor[] = {0.8, 1.0, 0.8, 1.0}; // green-tinted
 
// for the player
static GLfloat playerColor[] = {0.1, 1.0, 0.1, 1.0};
 
// for floor and shadow
static GLfloat floorPlane[4];
static GLfloat floorShadow[4][4];
 
 
//enums for vector coordinates
enum {
    X, Y, Z, W
};
 
enum {
    A, B, C, D
};

 
// floor tiling pattern in ascii art
static char *circles[] = {
    "......xxxx......",
    "....xxxxxxxx....",
    "...xxx....xxx...",
    "..xxx......xxx..",
    ".xxxx......xxxx.",
    ".xxxx......xxxx.",
    ".xxxx......xxxx.",
    ".xxxx......xxxx.",
    "..xxx......xxx..",
    "...xxx....xxx...",
    "....xxxxxxxx....",
    "......xxxx......",
    "................",
    "................",
    "................",
    "................",
};

// timer function
unsigned GetTickCount() {
    struct timeval tv;
    if(gettimeofday(&tv, NULL) != 0)
        return 0;
 
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// Frames per second (FPS) statistic variables and routine.


int FrameCount=0;
float FrameRate=0;

static void getFPS(
   void
)
{
   static clock_t last=0;
   clock_t now;
   float delta;

   if (++FrameCount >= FRAME_RATE_SAMPLES) {
      now  = clock();
      delta= (now - last) / (float) CLOCKS_PER_SEC;
      last = now;

      FrameRate = FRAME_RATE_SAMPLES / delta;
      FrameCount = 0;
   }
}

 
//-----------------------------------------------------------------------------
// render floor texture
//-----------------------------------------------------------------------------
static void makeFloorTexture(void) {
    GLubyte floorTexture[16][16][3];
    GLubyte *loc;
    int s, t;
 
    // Setup RGB image for the texture.
    loc = (GLubyte*) floorTexture;
    for (t = 0; t < 16; t++) {
        for (s = 0; s < 16; s++) {
            if (circles[t][s] == 'x') {
                // light blue color for circles
                loc[0] = 0x1f;
                loc[1] = 0x1f;
                loc[2] = 0xcf;
            } else {
                // dark blue for rest
                loc[0] = 0x1a;
                loc[1] = 0x1a;
                loc[2] = 0x3a;
            }
            loc += 3;
        }
    }
 
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 16, 16, GL_RGB, GL_UNSIGNED_BYTE, floorTexture);
}
 
// simple vertices for floor
static GLfloat floorVertices[4][3] = {
    { -20.0, 0.0, 20.0 },
    { 20.0, 0.0, 20.0 },
    { 20.0, 0.0, -20.0 },
    { -20.0, 0.0, -20.0 },
};
 
//-----------------------------------------------------------------------------
// draw a texturedfloor
//-----------------------------------------------------------------------------
static void drawFloor(float size, float y) {
    glDisable(GL_LIGHTING);
 
    glEnable(GL_TEXTURE_2D);
 
    glTranslatef( 0.0f, y, 0.0f );
 
    glBegin(GL_QUADS);
    glTexCoord2f( 0.0f, 0.0f );
    glVertex3f( -size, y, -size );
    glTexCoord2f( 0.0f, FSIZE );
    glVertex3f( -size, y, +size );
    glTexCoord2f( FSIZE, FSIZE );
    glVertex3f( +size, y, +size );
    glTexCoord2f( FSIZE, 0.0f );
    glVertex3f( +size, y, -size );
    glEnd();
 
    glDisable(GL_TEXTURE_2D);
 
    glEnable(GL_LIGHTING);
}
 
 
//-----------------------------------------------------------------------------
// create a shadow matrix
//-----------------------------------------------------------------------------
void shadowMatrix(GLfloat shadowMat[4][4], GLfloat groundplane[4], GLfloat lightpos[4]) {
 
    GLfloat dot;
 
    // Find dot product between light position vector and ground plane normal.
    dot = groundplane[X] * lightpos[X] +
          groundplane[Y] * lightpos[Y] +
          groundplane[Z] * lightpos[Z] +
          groundplane[W] * lightpos[W];
 
    shadowMat[0][0] = dot - lightpos[X] * groundplane[X];
    shadowMat[1][0] = 0.f - lightpos[X] * groundplane[Y];
    shadowMat[2][0] = 0.f - lightpos[X] * groundplane[Z];
    shadowMat[3][0] = 0.f - lightpos[X] * groundplane[W];
 
    shadowMat[X][1] = 0.f - lightpos[Y] * groundplane[X];
    shadowMat[1][1] = dot - lightpos[Y] * groundplane[Y];
    shadowMat[2][1] = 0.f - lightpos[Y] * groundplane[Z];
    shadowMat[3][1] = 0.f - lightpos[Y] * groundplane[W];
 
    shadowMat[X][2] = 0.f - lightpos[Z] * groundplane[X];
    shadowMat[1][2] = 0.f - lightpos[Z] * groundplane[Y];
    shadowMat[2][2] = dot - lightpos[Z] * groundplane[Z];
    shadowMat[3][2] = 0.f - lightpos[Z] * groundplane[W];
 
    shadowMat[X][3] = 0.f - lightpos[W] * groundplane[X];
    shadowMat[1][3] = 0.f - lightpos[W] * groundplane[Y];
    shadowMat[2][3] = 0.f - lightpos[W] * groundplane[Z];
    shadowMat[3][3] = dot - lightpos[W] * groundplane[W];
 
}
 
//-----------------------------------------------------------------------------
// Finds the plane between three vectors
//-----------------------------------------------------------------------------
void findPlane(GLfloat plane[4], GLfloat v0[3], GLfloat v1[3], GLfloat v2[3]) {
    GLfloat vec0[3], vec1[3];
 
    // Need 2 vectors to find cross product.
    vec0[X] = v1[X] - v0[X];
    vec0[Y] = v1[Y] - v0[Y];
    vec0[Z] = v1[Z] - v0[Z];
 
    vec1[X] = v2[X] - v0[X];
    vec1[Y] = v2[Y] - v0[Y];
    vec1[Z] = v2[Z] - v0[Z];
 
    // find cross product to get A, B, and C of plane equation
    plane[A] = vec0[Y] * vec1[Z] - vec0[Z] * vec1[Y];
    plane[B] = -(vec0[X] * vec1[Z] - vec0[Z] * vec1[X]);
    plane[C] = vec0[X] * vec1[Y] - vec0[Y] * vec1[X];
 
    plane[D] = -(plane[A] * v0[X] + plane[B] * v0[Y] + plane[C] * v0[Z]);
}
 
 
//-----------------------------------------------------------------------------
// Enabled 2D primitive rendering by setting up the appropriate orthographic
// perspectives and matrices.
//-----------------------------------------------------------------------------
void glEnable2D( void ) {
    GLint iViewport[4];
 
    // Get a copy of the viewport
    glGetIntegerv( GL_VIEWPORT, iViewport );
 
    // Save a copy of the projection matrix so that we can restore it
    // when it's time to do 3D rendering again.
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
 
    // Set up the orthographic projection
    glOrtho( iViewport[0], iViewport[0]+iViewport[2],
             iViewport[1]+iViewport[3], iViewport[1], -1, 1 );
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
 
    // Make sure depth testing and lighting are disabled for 2D rendering until
    // we are finished rendering in 2D
    glPushAttrib( GL_DEPTH_BUFFER_BIT | GL_LIGHTING_BIT );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_LIGHTING );
}
 
 
 
//-----------------------------------------------------------------------------
// Disables 2D rendering and restores the previous matrix and render states
// before they were modified.
//-----------------------------------------------------------------------------
void glDisable2D( void ) {
    glPopAttrib();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();
}
 
//-----------------------------------------------------------------------------
//Prints a string of chars using GLUT's pre-defined char blitting API.
//-----------------------------------------------------------------------------
void glPrintf( int x, int y, void* font, char* string, ... ) {
    char *c;
    char temp[256];
    va_list ap;
 
    va_start( ap, string );
    vsprintf( temp, string, ap );
    va_end( ap );
 
    glEnable2D();
 
    glRasterPos2i(x,y);
 
    for (c=temp; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
 
    glDisable2D();
}
 
//-----------------------------------------------------------------------------
// Calculates the distance between two points.
//-----------------------------------------------------------------------------
float distance( const struct Vector3* v1, const struct Vector3* v2 ) {
    float d = 0.0f;
    float x = v2->x - v1->x;
    float y = v2->y - v1->y;
    float z = v2->z - v1->z;
 
    x *= x;
    y *= y;
    z *= z;
 
    // normalize vector
    d = sqrt(x+y+z);
 
    return d;
}
 
 
//-----------------------------------------------------------------------------
// Desc: Calulates the distance between each sphere and the camera.
//-----------------------------------------------------------------------------
void calculate_distances( void ) {
    int i;
    struct Vector3 neg_camera;
 
    // In order to accurately calculate the distance between the spheres and
    // the camera, the x and z values should be converted to negatives, and
    // the y coordinate should be ignored.  The camera updates the y value
    // still, but in a 3rd person shooter/adventure style game, the is calc-
    // lations are done seperately from the camera to compensate for jumping
    // and gravity, etc.
    neg_camera.x = camera.vecPos.x * -1.0f;
    neg_camera.y = 0.0f;
    neg_camera.z = camera.vecPos.z * -1.0f;
 
    for( i = 0; i < sphere_count; i++ ) {
        spheres[i].distance = distance( &neg_camera, &spheres[i].position );
    }
}
 
 
//-----------------------------------------------------------------------------
// Desc: Gives each sphere a random position in 3D space.
//-----------------------------------------------------------------------------
void spheres_init( void ) {
    int i;
 
    // Give each sphere a random position
    for( i = 0; i < sphere_count; i++ ) {
        spheres[i].position.x = (float) ( ( rand() % 100 ) + 1 ) - 50;
        spheres[i].position.y = 2.0f;
        spheres[i].position.z = (float) ( ( rand() % 100 ) + 1 ) - 50;
        spheres[i].selected = 0;
        spheres[i].dead = 0;
        spheres[i].death_time = 0;
        spheres[i].distance = 0.0f;
        spheres[i].size = 2.0f;
    }
}

//-----------------------------------------------------------------------------
// draws the spheres reflections
//-----------------------------------------------------------------------------
void sphere_reflection() {
      int i;
  
    for( i = 0; i < sphere_count; i++ ) {
            glPushMatrix();
            glTranslatef( -spheres[i].position.x, spheres[i].position.y + 1.0f, -spheres[i].position.z );
            glutSolidSphere( spheres[i].size, 20, 20 );
            glPopMatrix();
        }
}
 
//-----------------------------------------------------------------------------
// Renders each sphere in it's random position
//-----------------------------------------------------------------------------
void spheres_render() {
    int i;
 
    // Render each sphere with a solid green colour
    //glColor3f( 0.0f, 1.0f, 0.0f );
 
    for( i = 0; i < sphere_count; i++ ) {
        // Is this sphere dead?
        if( spheres[i].dead ) {
            unsigned int current_time;
 
            // Slowly decrease the size of the sphere when it's dying
            if( spheres[i].size > 0.0f )
                spheres[i].size -= 0.1f;
 
            // When time expires, bring the sphere back into play (respawn).
            // The sphere will fall from the sky after the respawn time expires.
            current_time = GetTickCount();
            if( (current_time - spheres[i].death_time) > respawn_time ) {
                spheres[i].position.y = 50.0f;
                spheres[i].dead = 0;
                spheres[i].size = 2.0f;
            }
        }
 
        //if( !spheres[i].dead )
        if( spheres[i].size != 0.0f ) {
            glPushMatrix();
            glTranslatef( -spheres[i].position.x, spheres[i].position.y, -spheres[i].position.z );
            glutSolidSphere( spheres[i].size, 20, 20 );
 
            // Render a slightly larger purple transparent sphere around
            // selected objects.
            if( spheres[i].selected ) {
                glPushAttrib( GL_ALL_ATTRIB_BITS );
 
                glColor4ub( 128, 0, 255, 64 );
 
                glDisable( GL_COLOR_MATERIAL );
                glDisable( GL_LIGHTING );
                // glDisable( GL_DEPTH_TEST );
                glDepthMask( GL_FALSE );
                glDisable( GL_TEXTURE_2D );
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
 
                glutSolidSphere( spheres[i].size * 1.5f, 20, 20 );
 
                glPopAttrib();
            }
 
            glPopMatrix();
        }
 
        // If the sphere is in the sky, let it fall back down to the ground
        if( spheres[i].position.y > 2.0f ) {
            spheres[i].position.y -= 0.5f;
        }
    }
}
 
 
//-----------------------------------------------------------------------------
// Draws an aiming crosshair
//-----------------------------------------------------------------------------
void draw_crosshair( void ) {
    float fViewport[4];
    float w = 32.0f, h = 32.0f;
 
    // Get the current viewport. We want to make sure the crosshair
    // is in the center of the screen, even if the window is resized.
    glGetFloatv( GL_VIEWPORT, fViewport );
 
    // Enable 2D rendering
    glEnable2D();
 
    glBegin( GL_LINES );
    glColor3f( 0.2f, 1.0f, 0.5f );
    glVertex2f( (fViewport[2]/2.0f)-(w/2.0f), fViewport[3]/2.0f );
    glVertex2f( (fViewport[2]/2.0f)+(w/2.0f), fViewport[3]/2.0f );
    glVertex2f( fViewport[2]/2.0f, (fViewport[3]/2.0f)-(h/2.0f) );
    glVertex2f( fViewport[2]/2.0f, (fViewport[3]/2.0f)+(h/2.0f) );
    glEnd();
 
    // Disable 2D rendering
    glDisable2D();
}
 
//-----------------------------------------------------------------------------
// Show the players stats
//-----------------------------------------------------------------------------
void show_player_stats( void ) {
    char string[128];
    char buf[128];
    sprintf(buf,"FPS: %f F: %2d", FrameRate, FrameCount);
    sprintf( string, "Player pos:<%f,%f,%f> score: <%d>", camera.vecPos.x, camera.vecPos.y, camera.vecPos.z, score );
    glPrintf( 30, 30, GLUT_BITMAP_9_BY_15, string );
    glPrintf( 30, 530, GLUT_BITMAP_9_BY_15, buf );
}
  
//-----------------------------------------------------------------------------
// Marks selected object as dead.
//-----------------------------------------------------------------------------
void kill_selected_object( void ) {
    int i = 0;
    char string[64];

    // kill all spheres that are selected
    for( i = 0; i < sphere_count; i++ ) {      
          // the sphere was selected kill it
          if(spheres[i].selected) {
            score += 100;
            spheres[i].dead = 1;
            spheres[i].death_time = GetTickCount();
           }
        }
}

//-----------------------------------------------------------------------------
// Uses OpenGL's slection buffer to select objects in viewport
//-----------------------------------------------------------------------------
void do_selection( int select_x, int select_y, int preselect ) {
    GLint iViewport[4];
    float fViewport[4];
    int i;
 
    // Set the selection buffer and specify it's size
    glSelectBuffer( sphere_count*4, select_buffer );
 
    // Get a copy of the current viewport
    glGetIntegerv( GL_VIEWPORT, iViewport );
    glGetFloatv( GL_VIEWPORT, fViewport );
 
    // Switch into selection mode
    glRenderMode( GL_SELECT );
 
    // Clear the name stack
    glInitNames();
    // Give the stack at least one name or else it will generate an error
    glPushName(0);
 
    // Save the projection matrix
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
 
    // Restrict the rendering area to the given point using gluPickMatrix
    gluPickMatrix( select_x, select_y, 1.0f, 1.0f, iViewport );
    gluPerspective( 45.0f, (GLdouble) fViewport[2]/fViewport[3], 0.1f, 500.0f );
 
    // Now render selectable objects to the screen
    glMatrixMode( GL_MODELVIEW );

    for( i = 0; i < sphere_count; i++ ) {
        //  if( !spheres[i].dead )
        if( spheres[i].size != 0.0f ) {
            glPushMatrix();
            glLoadName(i);
            glTranslatef( -spheres[i].position.x, spheres[i].position.y, -spheres[i].position.z );
            glutSolidSphere( spheres[i].size, 20, 20 );
            glPopMatrix();
        }
    }

    // Restore the projection matrix
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
 
    // Get the number of hits (the number of objects drawn within that
    // specified area) and return to render mode
    hits = glRenderMode( GL_RENDER );

    // Don't do anything except mark objects as selected in preselect mode
    if( preselect ) {
        // Go through the list of objects and deselect them all
        for( i = 0; i < sphere_count; i++ ) {
           spheres[i].selected = 0;
        }
 
        // Now mark all selected objects
        for( i = 0; i < hits; i++ ) {
            GLubyte name = select_buffer[i*4+3];
            spheres[name].selected = 1;
        }
    } else {
        kill_selected_object();
    }
 
    // Return to the modelview matrix
    glMatrixMode( GL_MODELVIEW );
}
 
 
//-----------------------------------------------------------------------------
// Draws the lightcycle bike
//----------------------------------------------------------------------------- 
static void drawbike(void) {
    float bikeMaterialAmbient0[] = {0.0, 0.0, 0.0, 1.0};
    float bikeMaterialDiffuse0[] = {0.0, 0.2, 0.0, 1.0};
    float bikeMaterialSpecular0[] = {0.0, 0.8, 0.0, 1.0 };
    float bikeMaterialShininess0[] = {100.0};
    float bikeMaterialEmission0[] = {0.0, 0.02, 0.0, 1.0};
 
    // scale our model
    glScalef(2.0F, 2.0F, 2.0F);
 
    glMaterialfv(GL_FRONT, GL_AMBIENT, bikeMaterialAmbient0);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, bikeMaterialDiffuse0);
    glMaterialfv(GL_FRONT, GL_SPECULAR, bikeMaterialSpecular0);
    glMaterialfv(GL_FRONT, GL_SHININESS, bikeMaterialShininess0);
    glMaterialfv(GL_FRONT, GL_EMISSION, bikeMaterialEmission0);
    //Draws a bike
    glPushMatrix();
    glTranslatef(bikex,bikey,bikez-1.8);
    glRotatef(bikeAngle, 0.0,1.0,0.0);
    glPushMatrix();
    glTranslatef(bikex,bikey,bikez);
    glScalef(1.0, 1.0, 3.0);
    glRotatef(180.0, 0.0, 0.0, 1.0);
    glutSolidSphere(0.5, 5.0, 20.0);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bikex,bikey-0.3,bikez+1.2);
    glScalef(2.4, 1.0, 1.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glRotatef(bikeTireAngle, 0.0, 0.0, 1.0);
    glutSolidTorus(0.25, 0.25, 10.0, 10.0);
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.0, 0.0, 0.0);
    glutWireTorus(0.25, 0.25, 10.0, 10.0);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bikex,bikey-0.3,bikez-1.2);
    glScalef(2.4, 1.0, 1.0);
    glRotatef(90.0, 0.0, 1.0, 0.0);
    glRotatef(bikeTireAngle, 0.0, 0.0, 1.0);
    glutSolidTorus(0.25, 0.25, 10.0, 10.0);
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.0, 0.0, 0.0);
    glutWireTorus(0.25, 0.25, 10.0, 10.0);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(bikex,bikey+0.4,bikez-0.8);
    if(bikeHandlAngle < 0) {
        glRotatef(40.0, 0.0, 1.0, 0.0);
    } else if(bikeHandlAngle > 0) {
        glRotatef(-40.0, 0.0, 1.0, 0.0);
    }
    glScalef(1.2, 0.1, 0.1);
    glutSolidCube(1.0);
    glPopMatrix();
 
    glPopMatrix();
 
}
 
 
//-----------------------------------------------------------------------------
// Positions and draws the player
//-----------------------------------------------------------------------------
static void drawplayer(void) {
 
    glPushMatrix();
 
    // Translate the player position
    glTranslatef(-8, 1.5, -bodyWidth / 2);
 
    //setup player color material
    glMaterialfv(GL_FRONT, GL_DIFFUSE, playerColor);
 
    // draw player model
    drawbike();
 
    glPopMatrix();
}
 
//-----------------------------------------------------------------------------
// Handles all rendering
//-----------------------------------------------------------------------------
static void render(void) {
    int start, end;
    int iViewport[4];
 
    // Clear; default stencil clears to zero.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
 
    // reposition the light here
    lightPosition[0] = 40*cos(lightAngle);
    lightPosition[1] = lightHeight;
    lightPosition[2] = 40*sin(lightAngle);
    lightPosition[3] = 0.0;

    shadowMatrix(floorShadow, floorPlane, lightPosition);
 
    glPushMatrix();
    // Position the camera behind our character and render it
    glTranslatef( 0.0f, -2.0f, -camera.fRadius );
    glRotatef( camera.vecRot.x, 1.0f, 0.0f, 0.0f );
 
    // draw bike here
    glTranslatef( 8.0f, 0.0, 0.0);
    drawplayer();
 
    // Rotate the camera as necessary
    glRotatef( camera.vecRot.y, 0.0f, 1.0f, 0.0f );
    glTranslatef( -camera.vecPos.x, 2.2f, -camera.vecPos.z );
 
    // Calculate distances
    calculate_distances();
 
    // Do pre-selection
    glGetIntegerv( GL_VIEWPORT, iViewport );
    do_selection( iViewport[2]/2, iViewport[3]/2, TRUE );
 
    // Tell GL new light source position.
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
 
    // Don't update color or depth.
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
 
    // Draw 1 into the stencil buffer.
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xffffffff);
 
    // Now render floor; floor pixels just get their stencil set to 1.
    drawFloor(50.0f, -0.75f);
  
    // Re-enable update of color and depth.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_DEPTH_TEST);
 
    // Now, only render where stencil is set to 1.
    glStencilFunc(GL_EQUAL, 1, 0xffffffff);  // draw if ==1
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
 
    glPushMatrix();
 
    // move reflection beneath floor
    glTranslatef(0.0, -1.3, 0.0);
    // reflect the objects through the floor (y-plane)
    glScalef(1.0, -1.0, 1.0);
 
    // reflect light position
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
 
    // normalize light
    glEnable(GL_NORMALIZE);
    glCullFace(GL_FRONT);
 
    // Draw the reflected objects.
    // Render spheres reflections
    sphere_reflection();
 
    // Disable noramlize again and re-enable back face culling.
    glDisable(GL_NORMALIZE);
    glCullFace(GL_BACK);
 
    glPopMatrix();
 
    // Switch back to the unreflected light position.
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
 
 
    // disable stenciling
    glDisable(GL_STENCIL_TEST);
 
    // Draw "bottom" of floor in blue.
    glFrontFace(GL_CW);  // Switch face orientation.
    glColor4f(0.1, 0.1, 0.7, 1.0);
    drawFloor(50.0f, -0.75f);
    glFrontFace(GL_CCW);
 
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 3, 0xffffffff);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
 
    // Draw "top" of floor.  Use blending to blend in reflection.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.7, 0.0, 0.0, 0.3);
    glColor4f(1.0, 1.0, 1.0, 0.3);
    drawFloor(50.0f, -0.75f);
    glDisable(GL_BLEND);
 
    // Draw "actual" objects not their reflection
    // Render spheres
    spheres_render();
 
    glStencilFunc(GL_LESS, 2, 0xffffffff);  // draw if ==1
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
 
    // one of these will work
    // glEnable(GL_POLYGON_OFFSET_EXT);
    glEnable(GL_POLYGON_OFFSET_FILL);
 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_LIGHTING);  // Force the 50% black.
    glColor4f(0.0, 0.0, 0.0, 0.5);
 
    glPushMatrix();
 
    // Project the shadow.
    glMultMatrixf((GLfloat *) floorShadow);
 
    //draw object shadows
    sphere_reflection();
    
    glPopMatrix();
 
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
 
    //glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_STENCIL_TEST);
 
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 0.0);
 
    // Draw an arrowhead for light source
    glDisable(GL_CULL_FACE);
    glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
    glRotatef(lightAngle * -180.0 / M_PI, 0, 1, 0);
    glRotatef(atan(lightHeight/12) * 180.0 / M_PI, 0, 0, 1);
    glutSolidSphere(2.0, 10, 10);
    // glBegin(GL_TRIANGLE_FAN);
    // glVertex3f(0, 0, 0);
    // glVertex3f(2, 1, 1);
    // glVertex3f(2, -1, 1);
    // glVertex3f(2, -1, -1);
    // glVertex3f(2, 1, -1);
    // glVertex3f(2, 1, 1);
    // glEnd();
    // // Draw a white line from light direction.
    // glColor3f(1.0, 1.0, 1.0);
    // glBegin(GL_LINES);
    // glVertex3f(0, 0, 0);
    // glVertex3f(5, 0, 0);
    // glEnd();
     glEnable(GL_CULL_FACE);
 
    glEnable(GL_LIGHTING);
    glPopMatrix();
 
    glPopMatrix();

 
    // Draw the crosshair
    draw_crosshair();

    getFPS();

    // Show player's statistics
    glColor3f( 0.0f, 0.0f, 1.0f );
    show_player_stats();
 
    glutSwapBuffers();
}
 
//-----------------------------------------------------------------------------
// Handles mouse clicks
//-----------------------------------------------------------------------------
static void mouse(int button, int state, int x, int y) {
    int iViewport[4];
    int center_x;
    int center_y;
 
    if( ( button == GLUT_LEFT_BUTTON ) && ( state == GLUT_DOWN ) ) {
        glGetIntegerv( GL_VIEWPORT, iViewport );
 
        center_x = iViewport[2]/2;
        center_y = iViewport[3]/2;
        do_selection( center_x, center_y, FALSE );
    }
}
 
//-----------------------------------------------------------------------------
// Handles mouse movement
//-----------------------------------------------------------------------------
static void motion(int x, int y) {
    int diffx = x - camera.fLastX;
    int diffy = y - camera.fLastY;
 
    camera.fLastX = x;
    camera.fLastY = y;
 
    camera.vecRot.x += (float) diffy;
    camera.vecRot.y += (float) diffx;
 
    if( diffy > 0) {
        bikeHandlAngle = 1;
    } else if( diffy < 0 ) {
        bikeHandlAngle = -1;
    } else {
        bikeHandlAngle = 0;
    }
 
    // reset camera if underneath floor or above it
    if( camera.vecRot.x < -30.0f ) {
        camera.vecRot.x = -30.0f;
    }
 
    if( camera.vecRot.x > 90.0f ) {
        camera.vecRot.x = 90.0f;
    }
}
 
//-----------------------------------------------------------------------------
// A timer for animations used in code
//-----------------------------------------------------------------------------
static void idle(void) {
    static float time = 0.0;
 
    time = glutGet(GLUT_ELAPSED_TIME) / 500.0;
 
    //jump = 4.0 * fabs(sin(time)*0.5);
 
    if (!lightMoving) {
        lightAngle += 0.03;
    }
 
    glutPostRedisplay();
}
 
 
//-----------------------------------------------------------------------------
// A simple idle visibility routine
//-----------------------------------------------------------------------------
static void visible(int vis) {
    glutIdleFunc(idle);
}
 
//-----------------------------------------------------------------------------
// Handles keyboard input
//-----------------------------------------------------------------------------
static void key(GLubyte k, int x, int y) {
 
    static float fRotSpeed = 1.0f;
 
    if (k=='q') {
        camera.vecRot.x += fRotSpeed;
        if (camera.vecRot.x > 360) camera.vecRot.x -= 360;
    }
    if (k=='z') {
        camera.vecRot.x -= 1;
        if (camera.vecRot.x < -360) camera.vecRot.x += 360;
    }
    if (k=='w') {
        float xrotrad, yrotrad;
        yrotrad = (camera.vecRot.y / 180.0f * 3.141592654f);
        xrotrad = (camera.vecRot.x / 180.0f * 3.141592654f);
        camera.vecPos.x += (float)(sin(yrotrad));
        camera.vecPos.z -= (float)(cos(yrotrad));
        camera.vecPos.y -= (float)(sin(xrotrad));
        bikeTireAngle -= 10.0;
        bikeHandlAngle = 0.0;
    }
    if (k=='s') {
        float xrotrad, yrotrad;
        yrotrad = (camera.vecRot.y / 180.0f * 3.141592654f);
        xrotrad = (camera.vecRot.x / 180.0f * 3.141592654f);
        camera.vecPos.x -= (float)(sin(yrotrad));
        camera.vecPos.z += (float)(cos(yrotrad));
        camera.vecPos.y += (float)(sin(xrotrad));
        bikeTireAngle += 10.0;
        bikeHandlAngle = 0.0;
    }
    if (k=='d') {
        float yrotrad;
        yrotrad = (camera.vecRot.y / 180.0f * 3.141592654f);
        camera.vecPos.x += (float)(cos(yrotrad)) * 0.5f;
        camera.vecPos.z += (float)(sin(yrotrad)) * 0.5f;
        //bikeAngle -= 2.0;
        bikeHandlAngle = 1;
    }
    if (k =='a') {
        float yrotrad;
        yrotrad = (camera.vecRot.y / 180.0f * 3.141592654f);
        camera.vecPos.x -= (float)(cos(yrotrad)) * 0.5f;
        camera.vecPos.z -= (float)(sin(yrotrad)) * 0.5f;
        //  bikeAngle += 2.0;
        bikeHandlAngle = -1;
    }
 
    // Has escape been pressed?
    if( k == 27 ) {
        exit(0);
    }
    glutPostRedisplay();
}

//-----------------------------------------------------------------------------
// Special keyboard functions not implemented
//-----------------------------------------------------------------------------
static void special(int k, int x, int y) {
    glutPostRedisplay();
}

//-----------------------------------------------------------------------------
// Initialize opengl settings
//-----------------------------------------------------------------------------
static void init() {

    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
 
   glutGameModeString("1440x900:32");
    // enter full screen
    if (glutGameModeGet(GLUT_GAME_MODE_POSSIBLE))
        glutEnterGameMode();
    else {
        glutInitWindowSize(HEIGHT,WIDTH);
        glutCreateWindow("TRON Game");
    }
 
    if (glutGet(GLUT_WINDOW_STENCIL_SIZE) <= 1) {
        printf("tron: Sorry, I need at least 2 bits of stencil.\n");
        exit(1);
    }

   glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glLineWidth(3.0);
 
    glMatrixMode(GL_PROJECTION);
    gluPerspective( 40.0, 1.0, 20.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    gluLookAt(0.0, 8.0, 60.0,  /* eye is at (0,0,30) */
              0.0, 8.0, 0.0,      /* center is at (0,0,0) */
              0.0, 1.0, 0.);      /* up is in postivie Y direction */
 
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
 
    //enable scene
    spheres_init();
 
    // setup camera
    memset( &camera, 0, sizeof( struct ThirdPersonCamera_t ) );
    camera.fRadius = 10.0f;
 
    //timer
    srand( time( NULL ) );

    // make floor
    makeFloorTexture();
 
    // Setup floor plane for projected shadow calculations.
    findPlane(floorPlane, floorVertices[1], floorVertices[2], floorVertices[3]);

}

//-----------------------------------------------------------------------------
// Main Function
//----------------------------------------------------------------------------- 
int main(int argc, char **argv) {
    int i;
 
    glutInit( &argc, argv );
 
    init();

    // Register GLUT callbacks.
    glutDisplayFunc(render);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(motion);
    glutVisibilityFunc(visible);
    glutKeyboardFunc(key);
    glutSpecialFunc(special);
 
    glutMainLoop();
    return 0;
}
