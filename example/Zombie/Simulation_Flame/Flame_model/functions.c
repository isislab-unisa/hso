/*
 * Copyright 2011 University of Sheffield.
 * Author: Dr Paul Richmond
 * Contact: p.richmond@sheffield.ac.uk (http://www.paulrichmond.staff.shef.ac.uk)
 *
 * University of Sheffield retain all intellectual property and
 * proprietary rights in and to this software and related documentation.
 * Any use, reproduction, disclosure, or distribution of this software
 * and related documentation without an express license agreement from
 * University of Sheffield is strictly prohibited.
 *
 * For terms of licence agreement please attached licence or view licence
 * on www.flamegpu.com website.
 *
 */

#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include "header.h"

//Environment Bounds
#define MIN_POSITION -1.0f
#define MAX_POSITION +1.0f

//Interaction radius
#define INTERACTION_RADIUS 0.1f
#define INFECTION_RADIUS 0.1f
#define SEPARATION_RADIUS 0.005f

//Global Scalers
#define TIME_SCALE 0.0005f
#define HUMAN_GLOBAL_SCALE 0.20f
#define ZOMBIE_GLOBAL_SCALE 0.10f

#define INFECTION_PROB 0.8f

//Rule scalers
#define STEER_SCALE 0.65f
#define COLLISION_SCALE 0.75f
#define MATCH_SCALE 1.25f

//Agent states
#define STATE_HUMAN 0
#define STATE_ZOMBIE 1


inline __device__ float dot(glm::vec3 a, glm::vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline __device__ float length(glm::vec3 v)
{
    return sqrtf(dot(v, v));
}

__FLAME_GPU_FUNC__ glm::vec3 boundPosition(glm::vec3 agent_position){
	agent_position.x = (agent_position.x < MIN_POSITION)? MAX_POSITION: agent_position.x;
	agent_position.x = (agent_position.x > MAX_POSITION)? MIN_POSITION: agent_position.x;

	agent_position.y = (agent_position.y < MIN_POSITION)? MAX_POSITION: agent_position.y;
	agent_position.y = (agent_position.y > MAX_POSITION)? MIN_POSITION: agent_position.y;

	agent_position.z = (agent_position.z < MIN_POSITION)? MAX_POSITION: agent_position.z;
	agent_position.z = (agent_position.z > MAX_POSITION)? MIN_POSITION: agent_position.z;

	return agent_position;
}


//Zombie Agent Functions

//The following function arguments have been generated automatically by the FLAMEGPU XParser and are dependant on the function input and outputs. If they are changed manually be sure to match any arguments to the XMML specification.
//Input :
//Output: location
//Agent Output:
__FLAME_GPU_FUNC__ int outputdata(xmachine_memory_Agent* xmemory, xmachine_message_location_list* location_messages)
{
	add_location_message(location_messages, xmemory->id,xmemory->type,xmemory->x, xmemory->y, xmemory->z,xmemory->fx,
    xmemory->fy, xmemory->fz, xmemory->step_size);
	return 0;
}

//The following function arguments have been generated automatically by the FLAMEGPU XParser and are dependant on the function input and outputs. If they are changed manually be sure to match any arguments to the XMML specification.
//Input : location
//Output:
//Agent Output:
__FLAME_GPU_FUNC__ int inputdata(xmachine_memory_Agent* xmemory, xmachine_message_location_list* location_messages,xmachine_message_location_PBM* partition_matrix,RNG_rand48* rand48)
{
	//Agent position vector
	glm::vec3 agent_position = glm::vec3(xmemory->x, xmemory->y, xmemory->z);
	glm::vec3 agent_velocity = glm::vec3(xmemory->fx, xmemory->fy, xmemory->fz);

	//Boids perceived center
	glm::vec3 global_centre = glm::vec3(0.0f, 0.0f, 0.0f);
	int global_centre_count = 0;
	//Boids global velocity matching
	glm::vec3 global_velocity = glm::vec3(0.0f, 0.0f, 0.0f);

	//Boids short range avoidance center
	glm::vec3 collision_centre = glm::vec3(0.0f, 0.0f, 0.0f);
	int collision_count = 0;

	xmachine_message_location* location_message = get_first_location_message(location_messages, partition_matrix, xmemory->x, xmemory->y, xmemory->z);
    int count;
	float random;
    while(location_message)
	{
		count++;

		//Message position vector
		glm::vec3 message_position = glm::vec3(location_message->x, location_message->y, location_message->z);

		if (location_message->id != xmemory->id){
			float separation = length(agent_position - message_position);
            if(separation<(INFECTION_RADIUS)){
                if (location_message->type==STATE_ZOMBIE && xmemory->type==STATE_HUMAN){
                    random = rnd<CONTINUOUS>(rand48);
                    if(random<INFECTION_PROB)
                        xmemory->type=STATE_ZOMBIE;
                        xmemory->step_size = location_message->step_size;
                    }
                }
      separation=length(agent_position-message_position);
			if (separation < (INTERACTION_RADIUS)){

				//Update Percieved global center
				global_centre += message_position;
				global_centre_count += 1;

				//Update global velocity matching
				glm::vec3 message_velocity = glm::vec3(location_message->fx, location_message->fy, location_message->fz);
				global_velocity += message_velocity;

				//Update collision center
				if (separation < (SEPARATION_RADIUS)){ //dependant on model size
					collision_centre += message_position;
					collision_count += 1;
				}

			}
		}
		location_message = get_next_location_message(location_message, location_messages, partition_matrix);
	}

  //Total change in velocity
	glm::vec3 velocity_change = glm::vec3(0.0f, 0.0f, 0.0f);

	//Rule 1) Steer towards perceived center of flock
	glm::vec3 steer_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	if (global_centre_count >0){
		global_centre /= global_centre_count;
		steer_velocity = (global_centre - agent_position)* STEER_SCALE;
	}
	velocity_change += steer_velocity;

	//Rule 2) Match neighbours speeds
	glm::vec3 match_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	if (collision_count > 0){
		global_velocity /= collision_count;
		match_velocity = match_velocity* MATCH_SCALE;
	}
	velocity_change += match_velocity;

	//Rule 3) Avoid close range neighbours
	glm::vec3 avoid_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	if (collision_count > 0){
		collision_centre /= collision_count;
		avoid_velocity = (agent_position - collision_centre)* COLLISION_SCALE;
	}
	velocity_change += avoid_velocity;

	//Global scale of velocity change
  velocity_change *=xmemory->step_size;

	//Update agent velocity
  agent_velocity += (velocity_change);

	//Bound velocity
	float agent_velocity_scale = length(agent_velocity);
	if (agent_velocity_scale > 1){
		agent_velocity /= agent_velocity_scale;
	}

	//Apply the velocity
	agent_position += agent_velocity * TIME_SCALE;

	//Bound position
	agent_position = boundPosition(agent_position);

	//Update agent structure
	xmemory->x = agent_position.x;
	xmemory->y = agent_position.y;
	xmemory->z = agent_position.z;

	xmemory->fx = agent_velocity.x;
	xmemory->fy = agent_velocity.y;
	xmemory->fz = agent_velocity.z;

	return 0;
}



#endif // #ifndef _FUNCTIONS_H_
