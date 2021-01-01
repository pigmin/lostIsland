/*      Particle.cpp
 *      
 *      Floppyright 2010 Tuna <tuna@supertunaman.com>
 *      
 *      A particle system for the Arduino and any given graphic LCD.
 *      
 */


#include "Particle.h"
#include <stdlib.h>
#include <WMath.h>
#include "gfx_utils.h"

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

        if (particles[i].weight > 0)
        {
            if (particles[i].velX != 0) {
                particles[i].velX *= 0.85f;        // subtract from positive numbers
            }             
            if (particles[i].velY < 21)
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
        particle.w = random(1,3);
        particle.h = particle.w;
        particle.life = life;
        particle.weight = 1;
        particle.velX = (rand() % 70) - 35;
        particle.velY = (rand() % 60) - 40;
        particle.color = color;
        
        addParticle(&particle);
    }
}


// creates num_parts particles at x,y with random velocities
void Particles::createBodyExplosion(int x, int y, int num_parts, uint16_t color, int life)
{
    int i;
    Particle particle;
    
    for (i = 0; i < num_parts; i++)
    {
        particle.x = x + 8;
        particle.y = y + 8;
        particle.w = random(2,4);
        particle.h = random(2,8);
        particle.life = life;
        particle.weight = 1;
        particle.velX = (rand() % 70) - 35;
        particle.velY = (rand() % 60) - 40;
        particle.color = color;
        
        addParticle(&particle);
    }
}


// creates num_parts particles at x,y with random velocities
void Particles::createDust(int x, int y, int num_parts, int xspeed, int yspeed, int life)
{
    int i;
    Particle particle;
    
    for (i = 0; i < num_parts; i++)
    {
        particle.w = random(2,5);
        particle.x = x + random(-4,4);
        particle.y = y - random(particle.w, particle.w + 2);
        particle.h = particle.w;
        particle.life = life;
        particle.weight = 0;
        particle.velX = xspeed;
        particle.velY = yspeed;
        uint8_t bright = random(200,255);
        particle.color = RGBConvert(bright, bright, bright);
        
        addParticle(&particle);
    }
}

int Particles::getActiveParticles()
{
    return activeParticles;
}
