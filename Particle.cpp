/*      Particle.cpp
 *      
 *      Floppyright 2010 Tuna <tuna@supertunaman.com>
 *      
 *      A particle system for the Arduino and any given graphic LCD.
 *      
 */


#include "Particle.h"
#include <stdlib.h>
#include <math.h>

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
void Particles::moveParticles()
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

        if (particles[i].velX != 0) {
            particles[i].velX *= 0.85f;        // subtract from positive numbers
        } 
        
        /*if (particles[i].velY > 0) {
            particles[i].velY--;        // for both coordinates.
        } else {
            particles[i].velY++;
        }*/
        if (particles[i].velY < 21)
            particles[i].velY = particles[i].velY + 3;
        else {
            if (particles[i].velY != 0) {
                particles[i].velY *= 0.85f;        // subtract from positive numbers
            }
        }

        if (particles[i].life == 0) {
             delParticle(i);
            
            // this particle is being replaced with the one at the end 
            // of the list, so we have to take a step back 
            i--; 
        }
        else {
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
            else if ((particles[i].x < 0) || (particles[i].x > SCREEN_WIDTH) || (particles[i].y > SCREEN_HEIGHT))
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
void Particles::createExplosion(int x, int y, int num_parts, uint16_t color, int life)
{
    int i;
    Particle particle;
    
    for (i = 0; i < num_parts; i++)
    {
        particle.x = x;
        particle.y = y;
        particle.life = life;
        particle.velX = (rand() % 60) - 30;
        particle.velY = (rand() % 60) - 30;
        particle.color = color;
        
        addParticle(&particle);
    }
}

int Particles::getActiveParticles()
{
    return activeParticles;
}
