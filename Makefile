#	Makefile for Lightballs opengl glut

OS = $(shell uname -s)
APPS = lightballs
OBJ = $(APPS).o
SRC = $(APPS).c

CFLAGS = $(C_OPTS) -I/usr/include
ifeq ($(OS), Darwin)
	LIBS = -framework GLUT -framework OpenGL -framework Cocoa 
else
	LIBS = -L/usr/X11R6/lib -lX11 -lXi -lglut -lGL -lGLU -lm -lpthread
endif
  
application:$(APPS)

clean:
	rm -f $(APPS) *.raw *.o core a.out

realclean:	clean
	rm -f *~ *.bak *.BAK

.SUFFIXES: c o
.c.o:
	$(CC) -c $(CFLAGS) $<

$(APPS): $(OBJ) 
	$(CC) -o $(APPS) $(CFLAGS) $(OBJ) $(LIBS)

depend:
	makedepend -- $(CFLAGS) $(SRC)