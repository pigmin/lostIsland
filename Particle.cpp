/*      Particle.cpp
 *      
 *      Floppyright 2010 Tuna <tuna@supertunaman.com>
 *      
 *      A particle system for the Arduino and any given graphic LCD.
 *      
 */

#include "defines.h"
#include "Particle.h"
#include <stdlib.h>
#include <WMath.h>
#include "gfx_utils.h"
#include "WaterSim.h"


Particles::Particles(){}
Particles::~Particles(){}

// Adds a particle to particles[]
void Particles::addParticle(Particle *particle)
{
    // don't exceed the max! 
    if (activeParticles >= MAX_PARTICLES)
        return;
    
    particles[activeParticles] = *particle;
    activeParticles++;
}

// replaces particles[index] with last particle in the list
void Particles::delParticle(int index)
{
    particles[index] = particles[activeParticles - 1];
    activeParticles--;
}

void Particles::clearParticles() {
    activeParticles = 0;
}

// updates the posisitions of all the particles with their velocities
void Particles::moveParticles(int cameraX, int cameraY)
{
    int i;
    
    // This is what deccelerates the particles.
    for (i = 0; i < activeParticles; i++)
    {
        particles[i].x += particles[i].velX / 10;    // calculate true velocity
        particles[i].y += particles[i].velY / 10;

        if (particles[i].life > 0) {
            particles[i].life = particles[i].life - 1;
        }

        if (particles[i].weight > 0)
        {
            if (particles[i].velX != 0) {
                particles[i].velX *= 0.85f;        // subtract from positive numbers
            }             
            if (particles[i].velY < FPS)
                particles[i].velY = particles[i].velY + 3 * particles[i].weight;
            else {
                if (particles[i].velY != 0) {
                    particles[i].velY *= 0.85f;        // subtract from positive numbers
                }
            }
        }
        else if (particles[i].weight < 0)
        {
            if (particles[i].velX != 0) {
                particles[i].velX *= 0.85f;        // subtract from positive numbers
            }             
            if (particles[i].velY > FPS)
                particles[i].velY = particles[i].velY + 3*particles[i].weight;
            else {
                if (particles[i].velY != 0) {
                    particles[i].velY *= 0.85f;        // subtract from positive numbers
                }
            }
        }

        if (particles[i].life == 0) {
             delParticle(i);
            
            // this particle is being replaced with the one at the end 
            // of the list, so we have to take a step back 
            i--; 
        }
        else {
            int px = particles[i].x - cameraX;
            int py = particles[i].y - cameraY;
            // and delete stopped particles from the list.
            // perhaps I should move this to up about 15 lines, so that stopped
            //  pixels can be deleted from the sketch.
            if ((abs(particles[i].velX)  < 1 ))
            {
                delParticle(i);
                
                // this particle is being replaced with the one at the end 
                // of the list, so we have to take a step back 
                i--; 
            }
            else if ( px < 0 || px > SCREEN_WIDTH || py > SCREEN_HEIGHT )
            {
                delParticle(i);
                
                // this particle is being replaced with the one at the end 
                // of the list, so we have to take a step back 
                i--; 
            }
        }

    }
}

// creates num_parts particles at x,y with random velocities
void Particles::createExplosion(int x, int y, int num_parts, int xspeed, int yspeed, uint16_t color, uint16_t color2, int life)
{
    int i;
    Particle particle;
    
    for (i = 0; i < num_parts; i++)
    {
        particle.x = x;
        particle.y = y;
        particle.w = 2*random(1,2);
        particle.h = particle.w;
        particle.life = life;
        particle.weight = 1;
        particle.velX = xspeed*(rand() % 70) - 35;
        particle.velY = yspeed*(rand() % 60) - 40;
        particle.color = color;
        particle.color2 = color2;
        
        addParticle(&particle);
    }
}


// creates num_parts particles at x,y with random velocities
void Particles::createDirectionalExplosion(int x, int y, int num_parts, uint8_t size, uint8_t direction, uint16_t color, uint16_t color2, int life)
{
    int i;
    Particle particle;
    int rangeX1 = 0;
    int rangeX2 = 0;

    int rangeY1 = 0;
    int rangeY2 = 0;


    if (direction & Direction_Bottom)
        rangeY2 = 60;
    if (direction & Direction_Up)
        rangeY1 = -60;
    if (direction & Direction_Left)
        rangeX1 = -60;
    if (direction & Direction_Right)
        rangeX2 = 60;

    for (i = 0; i < num_parts; i++)
    {
        particle.x = x;
        particle.y = y;
        particle.w = random(2, size);
        particle.h = particle.w;
        particle.life = life;
        particle.weight = 1;
        particle.velX = random(rangeX1, rangeX2);
        particle.velY = random(rangeY1, rangeY2);
        particle.color = color;
        particle.color2 = color2;
        
        addParticle(&particle);
    }
}

// creates num_parts particles at x,y with random velocities
void Particles::createBodyExplosion(int x, int y, int num_parts, uint16_t color, uint16_t color2, int life)
{
    int i;
    Particle particle;
    
    for (i = 0; i < num_parts; i++)
    {
        particle.x = x + 8;
        particle.y = y + 8;
        particle.w = 2*random(1,2);
        particle.h = particle.w;
        particle.life = life;
        particle.weight = 1;
        particle.velX = (rand() % 70) - 35;
        particle.velY = (rand() % 60) - 40;
        particle.color = color;
        particle.color2 = color2;
        
        addParticle(&particle);
    }
}


// creates num_parts particles at x,y with random velocities
void Particles::createLandingDust(int x, int y, int num_parts, int xspeed, int yspeed, int life)
{
    int i;
    Particle particle;
    
    for (i = 0; i < num_parts; i++)
    {
        particle.w = 2;
        particle.x = x + random(-6,6);
        particle.y = y - random(particle.w, particle.w + 4);
        particle.h = particle.w;
        particle.life = random(FPS, FPS>>2);
        particle.weight = 0;
        particle.velX = xspeed * ( (rand() % 6) - 3 );
        particle.velY = yspeed*random(0,2);
        uint8_t bright = random(220,255);
        particle.color = RGBConvert(bright, bright, bright);
        particle.color2 = 0xFFFF;
        
        addParticle(&particle);
    }
}

int Particles::getActiveParticles()
{
    return activeParticles;
}
