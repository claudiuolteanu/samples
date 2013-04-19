// Olteanu Vasilica - Claudiu
// 332CA
//-------------------------------------------------
// I K    - miscare nava fata/spate
// J L    - miscare nava stanga/dreapta (strafe)

// I K    - miscare camera fata/spate
// J L    - miscare camera stanga/dreapta (strafe)
// U O    - miscare camera sus/jos

// P	  - creste dificultatea jocului
// M	  - pornire meteoriti

// 1	  - setare camera nava
// 2	  - setare camera dinamica
// 3	  - setare camera meteorit


// ESC    - iesire din program
//-------------------------------------------------

// INCLUDES
//-------------------------------------------------
#include <stdlib.h>
#include <freeglut.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "Camera.h"
#include "Object3D.h"
#include "Plane.h"
#include "Light.h"
#include "Vector3D.h"
#include "Vector4D.h"
#include "Meteorit.h"
#include "SpaceShip.h"
#include "Shield.h"
#include "Bonus.h"

// tasta escape
#define ESC	27

//limitele jocului
#define LEFT_LIMIT -10
#define UP_LIMIT 6
#define DOWN_LIMIT -6
#define RIGHT_LIMIT 10

//numarul maxim si minim de meteoriti
#define MAX_METEORITS 1000
#define MIN_METEORITS 40

//numarul de culori pentru meteoriti
#define COLORS_NUMBER 10


float *Vector3D::arr;
float *Vector4D::arr;

// VARIABILE
//-------------------------------------------------
int meteoritCount;		// numarul de meteoriti	
int meteoritRemains;	//numarul de meteoriti ramasi
// obiectul selectat
int selectedIndex = -1;
int cameraMeteoritIndex;	//index-ul meteoritului pe care se va pozitiona camera

// camerele
Camera *camera_front,*camera_top,*camera_meteorit;
// vector de meteoriti
Meteorit *objects;
// planul de baza
Plane *ground;
// luminile omni
Light *light_o;
Light *light_o2;
// luminile spot
Light *spot1, *spot2;

// variabila pentru animatie
GLfloat spin=0.0;

//nava
SpaceShip *ship;

//scut
Shield *shield;

// variabile necesare pentru listele de afisare
#define GROUND	1
int drawLists = 1;

// variabila folosita pentru a determina daca listele de afisare trebuiesc recalculate
int recomputeLists = 0;
int moveMeteorits = 1;

// identificator fereastra
int mainWindow;

// pe ce obiect a fost executat pick
int obiect = 0;

bool gameOver = false;		//retine daca jocul s-a terminat
bool shipCamera = false;	//retine daca este camera nava
bool meteoritCamera = false;	//retine daca este camera meteorit
bool selectCamera = false;		//retine daca se alege meteoritul pe care sa se pozitioneze camera
bool showLaser = false;			//retine daca afiseaza laser-ul
bool difficulty = false;		//verifica daca se creste dificultatea
int health;					//viata jucatorului
float difficultyValue;		//viteza ce se adauga pentru a mari dificultatea
bool keyUp = false, keyDown = false, keyLeft = false, keyRight = false;		//verifica daca au fost apasate tastele pt miscarea navei
// FUNCTII
//-------------------------------------------------
Vector3D pos;
Vector3D *colors[COLORS_NUMBER];	//vector de culori pentru meteoriti
Vector3D shipPos;					//pozitia navei

Bonus *bonus;						//bonusul
float bonusVelocity;				//viteza bonusului
float bonusExists = false;			//existenta bonusului

// functie de initializare a setarilor ce tin de contextul OpenGL asociat ferestrei
void init(void)
{
	
	// Construieste listele de display
	glNewList(GROUND,GL_COMPILE);
	ground->Draw();
	glEndList();

	// pregatim o scena noua in opengl
	glClearColor(0.0, 0.0, 0.0, 0.0);	// stergem tot
	glEnable(GL_DEPTH_TEST);			// activam verificarea distantei fata de camera (a adancimii)
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_LEQUAL,1);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glShadeModel(GL_SMOOTH);			// mod de desenare SMOOTH
	glEnable(GL_LIGHTING);				// activam iluminarea
	glEnable(GL_NORMALIZE);				// activam normalizarea normalelor
}


//functie ce deseneaza laser-ul de la nava catre meteorit-ul distrus
void shoot(Vector3D v)
{
	glLineWidth(2.5); 
	glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	Vector3D startPos;
	if(!shipCamera) 
		startPos = Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f , shipPos.z + 0.3f);
	else
		startPos = Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f , shipPos.z - 1.0f);
	glVertex3f(startPos.x, startPos.y , startPos.z);
	glVertex3f(v.x, v.y, v.z);
	glEnd();

}


//functie ce construieste bonusul
void createBonus() {
	float x, y, z, a;
	if(bonus != NULL)
		delete bonus;
	bonus = new Bonus();
	
	//determin pozitia bonusului
	x = 10.0f - 1.0 / (float) (rand() % 10);
	y = -8.0f + (float)(rand() % 16);
	z = -5.0f + (float)(rand() % 10);
	if(x > 10 || x < 0)
		x = 10.0f;
	bonus->SetPosition(new Vector3D(x, y, z));

	//determin culoarea bonusului
	x = 1.0f / (float) (rand() % 10) + 0.3f;
	y = 1.0f / (float) (rand() % 10) + 0.3f;
	z = 1.0f / (float) (rand() % 10) + 0.3f;
	a = 0.2f + 0.5f / (float) (rand() % 10 + 0.1f);
	if(x < 0 || x > 1)
		x = 1.0f;
	if(y < 0 || y > 1)
		x = 1.0f;
	if(z < 0 || z > 1)
		x = 1.0f;
	bonus->SetColor(new Vector3D(x, y, z));
	bonus->SetDiffuse(new Vector4D(x, y ,z, a));
	
	//determin dimensiunea bonusului
	x = 0.3f / (float) (rand() % 10 ) + 0.2f;
	if(x < 0 || x > 0.5f)
		x = 0.3f;
	bonus->SetScale(new Vector3D(x, x, x));

	bonusExists = true;
}


//functie ce adauga meteoriti atunci cand se atingea limita inferioara de meteoriti
void addMeteorits() {
	int count = rand() % ((int)(100 * difficultyValue) + 10);	//determin numarul de meteoriti ce vor fi adaugati
	int lastMeteorit = meteoritCount;
	meteoritRemains += count;

	//verific daca am umplut tot vectorul de meteoriti
	if((meteoritCount + count) < MAX_METEORITS) {
		meteoritCount += count;
		float x, y ,z, scale;

		//adaug meteoritii pe o pozitie random, de culoare si viteza random
		for(int i = lastMeteorit; i < meteoritCount; i++) {			
			x = 10.0f - 1.0 / (float) (rand() % 100);
			y = -10.0f + (float)(rand() % 20);
			z = -6.0f + (float)(rand() % 12);
			objects[i].SetPosition(new Vector3D(x , y, z));
			scale = 0.3 - 0.3 / (float)(rand() % 10);
			if(scale < 0 || scale >= 0.3)
				scale = 0.2;
			objects[i].SetScale(new Vector3D(scale, scale, scale));
			objects[i].setId(i + 1);
			objects[i].SetColor(colors[rand() % COLORS_NUMBER]);
			if(difficultyValue > 0.0f) {
				objects[i].setVelocity(objects[i].getVelocity() + difficultyValue);
			}
		}
		
		//verificam daca trebuie sa crestem dificultatea jocului
		if(difficulty) {
			difficultyValue += 0.01f;
			difficulty = false;
		}

	} else {
		//verificam daca s-a umplut buffer-ul de meteoriti si au fost distrusi majoritatea
		if(meteoritRemains < 5) {
			gameOver = true;
			printf("FELICITARI! Ai iesit din ploaia de meteoriti\n");
		} 
	}
}


//functie ce construieste vectorul de culori
void initColors() {
	Vector3D *color = new Vector3D(0.5, 0.5, 0.5);
	float x, y, z;
	
	for(int i = 0; i < COLORS_NUMBER; i++) {
		x = -0.3f + 0.3f / (float) (rand() % 5 + 1.0f);
		y = -0.3f + 0.3f / (float) (rand() % 5 + 1.0f);
		z = -0.3f + 0.3f / (float) (rand() % 5 + 1.0f);
		
		if(x < -0.2f || x > 0.2f)
			x = 0.15f;
		if(y < -0.2f || y > 0.2f)
			y = 0.15f;
		if(z < -0.2f || z > 0.2f)
			z = 0.15f;
		colors[i] = new Vector3D(color->x + x, color->y + y, color->z + z);
	}

}


//functie ce misca nava in functie de tastele apasate si actualizeaza camera si scutul
void shipMoves() {
	float speed;
	speed = 0.05f;
	
	//misca nava in sus
	if(keyUp) {
		pos = ship->GetPosition();
		if(pos.z + speed < UP_LIMIT)
			ship->SetPosition(new Vector3D(pos.x, pos.y, pos.z + speed));
		shipPos = ship->GetPosition();
		shield->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f, shipPos.z + 0.3f));
	}

	//misca nava in jos
	if(keyDown) {
		pos = ship->GetPosition();
		if(pos.z - speed > DOWN_LIMIT)
			ship->SetPosition(new Vector3D(pos.x, pos.y, pos.z - speed));
		shipPos = ship->GetPosition();
		shield->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f, shipPos.z + 0.3f));
	}
	
	//misca nava la dreapta
	if(keyRight) {
		pos = ship->GetPosition();
		if(shipCamera) {
			if(pos.y - speed > LEFT_LIMIT)
				ship->SetPosition(new Vector3D(pos.x, pos.y - speed, pos.z));			
		} else if(meteoritCamera) {
			if(pos.y + speed < RIGHT_LIMIT)
				ship->SetPosition(new Vector3D(pos.x, pos.y + speed, pos.z ));
		} else {
			if(pos.x + speed > LEFT_LIMIT)
				ship->SetPosition(new Vector3D(pos.x - speed, pos.y, pos.z));
		}
		shipPos = ship->GetPosition();
		shield->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f, shipPos.z + 0.3f));
	}
	
	//misca nava la stanga
	if(keyLeft) {
		pos = ship->GetPosition();
		if(shipCamera) {
			if(pos.y + speed < RIGHT_LIMIT)
				ship->SetPosition(new Vector3D(pos.x, pos.y + speed, pos.z));			
		} else if(meteoritCamera) {
			if(pos.y - speed > LEFT_LIMIT)
				ship->SetPosition(new Vector3D(pos.x, pos.y - speed, pos.z ));
		} else {
			if(pos.x + speed < RIGHT_LIMIT)
				ship->SetPosition(new Vector3D(pos.x + speed, pos.y, pos.z));
		}
		shipPos = ship->GetPosition();
		shield->SetPosition(new Vector3D(shipPos.x + 1, shipPos.y - 1.2f, shipPos.z + 0.3f));
	}
}

//determina daca are loc o coliziune
bool collisions(Vector3D meteoritPos, float meteoritLength){
	bool collision = false;
	Vector3D pos = shield->GetPosition();
	Vector3D relPos = pos - meteoritPos;
	float dist = relPos.x * relPos.x + relPos.y * relPos.y + relPos.z * relPos.z;
	float distMin = 2.0 + meteoritLength;
	collision = (dist <= distMin * distMin);
	return collision;
}


// functie de initializare a scenei 3D
void initScene(void)
{
	//glDisable(GL_BLEND);
	srand ( time(NULL) );
	initColors();
	health = 100;
	difficultyValue = 0.0f;
	gameOver = false;
	// initialize vector arrays
	Vector3D::arr = new float[3];
	Vector4D::arr = new float[4];

	// initializam camera pentru vedere top (cea default)
	camera_top = new Camera();

	camera_top->SetPosition(new Vector3D(0,15,0));
	camera_top->SetForwardVector(new Vector3D(0,-1,0));
	camera_top->SetRightVector(new Vector3D(-1,0,0));
	camera_top->SetUpVector(new Vector3D(0,0,1));


	// initializam numaurl de meteoriti
	meteoritCount = 50 + rand() % 20;
	meteoritRemains = meteoritCount;
	// initializam vectorul de meteoriti	
	objects = new Meteorit[MAX_METEORITS];

	// pentru fiecare meteorit
	float scale;
	float x, y ,z;
	for( int index = 0; index < meteoritCount; index++ )
	{
		objects[index].SetColor(colors[rand() % COLORS_NUMBER]);
		// determinam o pozitie, o culoare  si o dimensiune random
		x = 10.0f - 1.0 / (float) (rand() % 100);
		y = -10.0f + (float)(rand() % 20);
		z = -6.0f + (float)(rand() % 12);
		objects[index].SetPosition(new Vector3D(x , y, z));
		scale = 0.3 - 0.3 / (float)(rand() % 10);
		if(scale < 0 || scale >= 0.3)
			scale = 0.2;
		objects[index].SetScale(new Vector3D(scale, scale, scale));
		objects[index].setId(index + 1);	//setam id-ul meteoritului
	}

	// initializam un plan de latura 5.0f
	ground = new Plane(100.0f);
	// culoare
	ground->SetColor(new Vector3D(0.0,0.0,0.0));
	// setam o grila de 1
	ground->SetLevelOfDetail(5);
	// sub nivelul obiectelor
	ground->SetPosition(new Vector3D(0,-20,0));
	// si wireframe
	ground->Wireframe = false;
	ground->Type = Custom;

	// initializam o 2 lumini omnidirectionale
	light_o = new Light();
	// setam pozitia
	light_o->SetPosition(new Vector3D(0, 5, 0));
	light_o->Diffuse(Vector4D(1.0f, 0.0f, 0.0f, 1.0f)); 
	
	light_o2 = new Light();
	// setam pozitia
	light_o2->SetPosition(new Vector3D(0, 0, 0));
	light_o2->Diffuse(Vector4D(0.0f, 1.0f, 0.0f, 1.0f)); 
	
	//initializam nava
	ship = new SpaceShip();
	ship->SetColor(new Vector3D(1, 0, 0));
	ship->SetPosition(new Vector3D(-5.0, 0.0, 0.0));
	ship->SetRotation(new Vector3D(90, 0, 0));

	//initializam 2 lumini spot pozitionate pe nava
	shipPos = ship->GetPosition();
	spot1 = new Light();
	spot1->SetLightType(IlluminationType::Spot);
	// setam pozitia
	spot1->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.4f, shipPos.z + 0.3f));
	
	spot2 = new Light();
	spot2->SetLightType(IlluminationType::Spot);
	// setam pozitia
	spot2->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 0.8f, shipPos.z + 0.3f));


	//initializam camera front(nava)
	camera_front = new Camera();
	camera_front->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f, shipPos.z + 0.3f));
	camera_front->SetForwardVector(new Vector3D(1.0f, 0.0f, 0.0f));
	camera_front->SetRightVector(new Vector3D(0.0f, -1.0f, 0.0f));
	camera_front->SetUpVector(new Vector3D(0.0f, 0.0f, 1.0f));
	
	//initializam camera meteorit
	camera_meteorit = new Camera();
	camera_meteorit->SetForwardVector(new Vector3D(-1.0f, 0.0f, 0.0f));
	camera_meteorit->SetRightVector(new Vector3D(0.0f, 1.0f, 0.0f));
	camera_meteorit->SetUpVector(new Vector3D(0.0f, 0.0f, 1.0f));

	//initializam scutul
	shield = new Shield();
	shield->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f, shipPos.z + 0.3f));
	shield->SetColor(new Vector3D(0.6, 0.8, 0.3));
	shield->Wireframe = false;
}

// functie pentru output text
void output(GLfloat x, GLfloat y, char *format,...)
{
	va_list args;

	char buffer[1024],*p;

	va_start(args,format);

	vsprintf(buffer, format, args);

	va_end(args);

	glPushMatrix();
	
	glTranslatef(x,y,-15);

	//glRotatef(180,0,1,0);

	glScalef(0.0035, 0.0035, 0.0); /* 0.1 to 0.001 as required */

	for (p = buffer; *p; p++)
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *p);

	glPopMatrix();
}

// AFISARE SCENA
//-------------------------------------------------

// functie de desenare a scenei 3D
void drawScene(void)
{
	//verificam daca s-a terminat jocul
	if(!gameOver) {
		
		shipMoves();	//actualizam pozitia navei
		// desenam pe ecran viata jucatorului
		glColor3f(1.0f, 0.1f, 0.0f);
		char msg[14];
		if(health != 0)
			sprintf(msg, "HEALTH : %d", health);
		else
			sprintf(msg, "LAST CHANCE");
		if(meteoritCamera) {
			Vector3D position = objects[cameraMeteoritIndex].GetPosition();
			glRasterPos3f(position.x - 10, position.y + 3, position.z + 3);
		} else if(shipCamera) {
			Vector3D position = ship->GetPosition();
			glRasterPos3f(position.x + 10, position.y - 4, position.z + 3);
		} else {
			glRasterPos3f(-2, 0 , 5);
		}
		glutBitmapString(GLUT_BITMAP_HELVETICA_18, (unsigned char*)msg);
		
		//verificam daca a fost selectat un meteorit
		for(int i = 0; i < meteoritCount; i++) {
			if( obiect == i + 1) {
				if(!selectCamera) {		//verificam daca a fost selectat pentru camera meteorit sau pt distrugere
					//daca nu distrugem meteoritul
					objects[i].SetColor(new Vector3D(1.0, 0.0, 0.0));
					objects[i].destroy();
					meteoritRemains--;
					showLaser = true;
				} else {
					//daca da salvam indicele meteoritului si trecem pe camera meteorit
					selectCamera = false;
					cameraMeteoritIndex = i;
					shipCamera = false;
					meteoritCamera = true;
				}
				obiect = 0;
				break;
			}
		}

		glEnable(GL_LIGHTING);

		// pozitionam camera in functie de tipul ei
		
		if(shipCamera ) {
			camera_front->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.2f, shipPos.z + 0.3f));
			camera_front->Render();
		} else if(meteoritCamera) {
			camera_meteorit->SetPosition(&objects[cameraMeteoritIndex].GetPosition());
			camera_meteorit->Render();
			obiect = 0;
		} else {
			camera_top->Render();
		}

		
		//actualizez pozitiile spoturilor
		spot1->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 1.4f, shipPos.z + 0.3f));
		spot2->SetPosition(new Vector3D(shipPos.x + 1.0f, shipPos.y - 0.8f, shipPos.z + 0.3f));

		// activare lumina omnidirectionala
		light_o->Render();
		light_o2->Render();

		// activare lumina spot
		spot1->Render();
		spot2->Render();

			
		// desenare meteoriti
		for ( int i = 0 ; i < meteoritCount ; i ++ )
		{
			// rotatie OX
			if(i % 3 == 0)
				objects[i].SetRotation(new Vector3D(spin,0.0,0.0));

			// rotatie OY
			if(i % 3 == 1)
				objects[i].SetRotation(new Vector3D(0.0,spin,0.0));

			// rotatie OZ
			if(i % 3 == 2)
				objects[i].SetRotation(new Vector3D(0.0,0.0,spin));
			
			//actualizam pozitia meteoritilor in functie de viteza fiecaruia
			float newX, newY, newZ, scale, velocity;
			if(moveMeteorits) {
				Vector3D lastPos = objects[i].GetPosition();
				
				newX = lastPos.x - objects[i].getVelocity();
				newY = lastPos.y;
				newZ = lastPos.z;

				//verificam daca meteoritul a iesit din ecran si construim unul nou la inceputul ecranului
				if(newX <= (float)LEFT_LIMIT) {
					newX = 13.0f ;
					newY = -10.0f + (float)(rand() % 20);
					newZ = -6.0f + (float)(rand() % 12);
					scale = 0.3 - 0.3 / (float)(rand() % 10);
					if(scale < 0 || scale >= 0.3)
						scale = 0.2;
					objects[i].SetScale(new Vector3D(scale, scale, scale));
					velocity = 0.2 / (float)(rand() % 20);
					if(velocity > 0.2)
						velocity = 0.1;
					objects[i].setVelocity(velocity + difficultyValue);
				}
				Vector3D *newPos = new Vector3D(newX, newY, newZ );
				objects[i].SetPosition(newPos);
			}

			// verificam daca meteoritul trebuie distrus
			bool destroyed = objects[i].isDestroyed();
			if( !destroyed) {
				
				//adaugam meteoritul in lista pt pick
				glPushName(i + 1);
				objects[i].Draw();
				glPopName();
				
				//verificam daca are loc o coliziune cu nava
				Vector3D meteoritPos = objects[i].GetPosition();
				float dim = objects[i].getScale();
				if(collisions(meteoritPos, dim)) {
					health -= 10;
					objects[i].SetColor(new Vector3D(1.0, 0.0, 0.0));
					objects[i].destroy();	//activam distrugerea meteoritului
					meteoritRemains--;		//scadem numarul meteoreitilor existenti
					shield->decrementOpacity();	//scadem opacitatea scutului
					showLaser = false;
					//daca meteoritul ce s-a ciocnit de nava era cel pe care se afla camera meteorit trecem pe camera dinamica
					if(i == cameraMeteoritIndex) {	
						meteoritCamera = false;
					}
				}
				objects[i].Draw();	//desenam meteoritul
			} else {

				//pastram meteoritul cateva frameuri pentru a se vedea culoarea, apoi il distrugem
				if(objects[i].getCount() >= 0) {
					objects[i].Draw();
					Vector3D v = objects[i].GetPosition();
					if(showLaser) {
						shoot(v);
					}
					objects[i].setCount(objects[i].getCount() - 1);
				}
			}
			
			//verificam daca a fost atinsa limita inferioara de meteoriti
			if(meteoritRemains < MIN_METEORITS) {
				addMeteorits();
			}
		}

		// desenare plan
		if(drawLists)
		{
			glCallList(GROUND);
		}
		else
		{
			ground->Draw();
		}


		//deseneaza nava
		if(!shipCamera && health >= 0)
			ship->Draw();
		
		//verificam daca jucatorul mai are viata
		if(health < 0) {
			printf("=======GAME OVER=======\n");
			glColor3f(1, 0, 0);
			glRasterPos3f(2, 0 , 0);
			glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (unsigned char*)"GAME OVER");
			gameOver = true;
		}
		

		//daca jucatorul are viata mai mica de 50 adaugam un obiect bonus cu viata
		if(health < 50) {
			//verificam daca exista deja unul. Daca da ii actualizam pozitia
			if(bonusExists) {
				float newX, newY, newZ; 
				Vector3D lastPos = bonus->GetPosition();		
				newX = lastPos.x - bonus->getVelocity();
				newY = lastPos.y;
				newZ = lastPos.z;
				if(newX <= (float)LEFT_LIMIT - 10) {
					bonusExists = false;
				}
				Vector3D *newPos = new Vector3D(newX, newY, newZ );
				bonus->SetPosition(newPos);
				glPushName(9999);	//adaugam bonusul in lista de pick
				bonus->Draw();			
				glPopName();
				if(obiect != 9999) {
					//daca bonusul nu a fost ales il desenam
					glEnable(GL_BLEND);
					glDepthMask(GL_FALSE);
					glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);
					bonus->Draw();
					glDisable(GL_BLEND);
					glDepthMask(GL_TRUE);
				} else {
					//daca bonusul a fost ales crestem viata jucatorului
					health = health + bonus->getValue();
					shield->incrementOpacity(bonus->getValue() / 10);
					bonusExists = false;
					obiect = 0;
				}
			} else {
				createBonus(); //daca bonusul nu exista construim unul 
			}
		}
	} else {
		//daca jocul a luat sfarsit afisam pe ecran un mesaj
		camera_top->Render();
		glColor3f(1, 0, 0);
		glRasterPos3f(2, 0 , 0);
		glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (unsigned char*)"GAME OVER");
	}
}

// functie de desenare (se apeleaza cat de repede poate placa video)
// se va folosi aceeasi functie de display pentru toate subferestrele ( se puteau folosi si functii distincte 
// pentru fiecare subfereastra )
void display(void)
{
	// stergere ecran
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Reconstruieste listele de display pentru ambele subferestre
	if(recomputeLists)
	{
		glNewList(GROUND,GL_COMPILE);
		ground->Draw();
		glEndList();

		recomputeLists--;
	}

	// Render Pass - deseneaza scena
	drawScene();
	glDepthMask(GL_FALSE);

	// desenam scutul
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);
	if(!gameOver)
		shield->Draw();
	glDisable(GL_BLEND);

	// double buffering
	glutSwapBuffers();

	// redeseneaza scena
	glutPostRedisplay();
	glDepthMask(GL_TRUE);
		
}

// PICKING
//-------------------------------------------------

// functia care proceseaza hitrecordurile pentru a vedea daca s-a click pe un obiect din scena
void processhits (GLint hits, GLuint buffer[])
{
   int i;
   GLuint names, *ptr, minZ,*ptrNames, numberOfNames;

   // pointer la inceputul bufferului ce contine hit recordurile
   ptr = (GLuint *) buffer;
   // se doreste selectarea obiectului cel mai aproape de observator
   minZ = 0xffffffff;
   for (i = 0; i < hits; i++) 
   {
      // numarul de nume numele asociate din stiva de nume
      names = *ptr;
	  ptr++;
	  // Z-ul asociat hitului - se retine 
	  if (*ptr < minZ) {
		  numberOfNames = names;
		  minZ = *ptr;
		  // primul nume asociat obiectului
		  ptrNames = ptr+2;
	  }
	  
	  // salt la urmatorul hitrecord
	  ptr += names+2;
  }

  // identificatorul asociat obiectului
  ptr = ptrNames;
  
  obiect = *ptr;
     
}

// functie ce realizeaza picking la pozitia la care s-a dat click cu mouse-ul
void pick(int x, int y)
{
	// buffer de selectie
	GLuint buffer[1024];

	// numar hituri
	GLint nhits;

	// coordonate viewport curent
	GLint	viewport[4];

	// se obtin coordonatele viewportului curent
	glGetIntegerv(GL_VIEWPORT, viewport);
	// se initializeaza si se seteaza bufferul de selectie
	memset(buffer,0x0,1024);
	glSelectBuffer(1024, buffer);
	
	// intrarea in modul de selectie
	glRenderMode(GL_SELECT);

	// salvare matrice de proiectie curenta
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	// se va randa doar intr-o zona din jurul cursorului mouseului de [1,1]
	glGetIntegerv(GL_VIEWPORT,viewport);
	gluPickMatrix(x,viewport[3]-y,1.0f,1.0f,viewport);

	gluPerspective(45,(viewport[2]-viewport[0])/(GLfloat) (viewport[3]-viewport[1]),0.1,1000);
	glMatrixMode(GL_MODELVIEW);

	// se "deseneaza" scena : de fapt nu se va desena nimic in framebuffer ci se va folosi bufferul de selectie
	drawScene();

	// restaurare matrice de proiectie initiala
	glMatrixMode(GL_PROJECTION);						
	glPopMatrix();				

	glMatrixMode(GL_MODELVIEW);
	// restaurarea modului de randare uzual si obtinerea numarului de hituri
	nhits=glRenderMode(GL_RENDER);	
	
	// procesare hituri
	if(nhits != 0)
		processhits(nhits,buffer);
	else
		obiect=0;

				
}

// handler pentru tastatura
void keyboardPressed(unsigned char key , int x, int y)
{
	switch (key)
	{
		// la escape se iese din program
	case ESC : exit(0);

		//pornim meteoritii
	case 'm': case 'M':
		if(moveMeteorits)
			moveMeteorits = 0;
		else
			moveMeteorits = 1;
		break;

		// cu ,si . se modifica nivelul de detaliu al terenului
	case ',' : ground->SetLevelOfDetail(ground->GetLevelOfDetail()*2); 
			if(drawLists)
				recomputeLists = 2;			
			break;
	case '.' : ground->SetLevelOfDetail(ground->GetLevelOfDetail()/2); 
			if(drawLists)
				recomputeLists = 2;		
			break;
	
		// cu I K J L U O observatorul se misca prin scena
	case 'i' : case 'I' : 
		camera_top->MoveForward(0.1);break;
	case 'k' : case 'K' : 
		camera_top->MoveBackward(0.1);break;
	case 'j' : case 'J' : 
		camera_top->MoveRight(0.1);break;
	case 'l' : case 'L' : 
		camera_top->MoveLeft(0.1);break;
	case 'u' : case 'U' :
		camera_top->MoveDownward(-0.1); break;
	case 'o' : case 'O' :
		camera_top->MoveDownward(0.1);break;
		
		// cu W A S D nava se misca prin scena
	case 'w' : case 'W' : 
		keyUp = true;
		break;
	case 's' : case 'S' :
		keyDown = true;
		break;
	case 'a' : case 'A' :
		keyLeft = true;
		break;
	case 'd' : case 'D' :
		keyRight = true;
		break;
		
		//reseteaza jocul
	case 'r' : case 'R' :
		initScene();
		break;

		//creste dificutatea jocului
	case 'p' : case 'P' :
		addMeteorits();
		difficulty = true;
		break;

		// Activare/Dezactivare liste de display
	case 'x': case 'X':
			drawLists=(drawLists-1)*(-1);
			recomputeLists = 2;	
			break;

		// activeaza camera nava
	case '1':
		shipCamera = true;
		meteoritCamera = false;
		selectCamera = false;
		break;

		// activeaza camera dinamica
	case '2':
		shipCamera = false;
		meteoritCamera = false;
		selectCamera = false;
		break;

		// activeaza camera meteorit
	case '3':
		selectCamera = true;
		break;
	default: break;
	}
}

//verifica daca a fost ridicata mana de pe o tasta
void keyReleased(unsigned char key, int x, int y) {
	if(key == 'a')
		keyLeft = false;
	
	if(key == 'd')
		keyRight = false;

	if(key == 'w')
		keyUp = false;

	if(key == 's') {
		keyDown = false;
	}
}

// handler taste speciale
void keyboard(int key , int x, int y)
{
	// incercam sa obtinem un pointer la obiectul selectat
	Object3D *selected;
	// daca nu exista un astfel de obiect
	if( selectedIndex >= 0 && selectedIndex < meteoritCount )
		selected = &objects[selectedIndex];
	else
		// se iese din functie
		return;

	// cu stanga/dreapta/sus/jos se misca obiectul curent
	switch (key)
	{
	case GLUT_KEY_RIGHT : 
		selected->SetPosition(&(selected->GetPosition() + Vector3D(0.2,0,0))); break;
	case GLUT_KEY_LEFT : 
		selected->SetPosition(&(selected->GetPosition() + Vector3D(-0.2,0,0))); break;
	case GLUT_KEY_DOWN : 
		selected->SetPosition(&(selected->GetPosition() + Vector3D(0,-0.2,0))); break;
	case GLUT_KEY_UP : 
		selected->SetPosition(&(selected->GetPosition() + Vector3D(0,0.2,0))); break;
	}
}

// Functia de idle a GLUT - folosita pentru animatia de rotatie a cuburilor
void spinDisplay()
{
	spin=spin+0.2;
	if(spin>360.0)
		spin= spin -360.0;
		
	
}

// Callback pentru a procesa inputul de mouse
void mouse(int buton, int stare, int x, int y)
{
	switch(buton)
	{
	// Am apasat click stanga : porneste animatia si realizeaza picking
	case GLUT_LEFT_BUTTON:
		if(stare == GLUT_DOWN)
		{
			// in aceasta variabila se va intoarce obiectul la care s-a executat pick
			obiect = 0;

			pick(x,y);

			glutIdleFunc(spinDisplay);
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if(stare== GLUT_DOWN)
			glutIdleFunc(NULL);
		break;
	}
}


// functie de proiectie
void reshape(int w, int h)
{
	// Main Window
	glViewport(0,0, (GLsizei) w, (GLsizei) h);
	// calculare aspect ratio ( Width/ Height )
	GLfloat aspect = (GLfloat) w / (GLfloat) h;

	// intram in modul proiectie
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// incarcam matrice de perspectiva 
	gluPerspective(45, aspect, 1.0, 60);

	// Initializeaza contextul OpenGL asociat ferestrei
	init();

}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
	int w = 800, h= 600;
	glutInitWindowSize(w,h);
	glutInitWindowPosition(100,100);
	
	// Main window
	mainWindow = glutCreateWindow("Lab7");

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboardPressed);
	glutKeyboardUpFunc(keyReleased);
	glutReshapeFunc(reshape);
	glutSpecialFunc(keyboard);
	glutMouseFunc(mouse);

	// Initializeaza scena 3D
	initScene();

	glutMainLoop();
	return 0;
}