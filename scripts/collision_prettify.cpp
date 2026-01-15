#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <algorithm>

static uint8_t sCollision[] = {
	...
};

enum TerrainLoadCmd {
	TERRAIN_LOAD_VERTICES = 0x40, // Begins vertices list for collision triangles
	TERRAIN_LOAD_CONTINUE,        // Stop loading vertices but continues to load other collision commands
	TERRAIN_LOAD_END,             // End the collision list
	TERRAIN_LOAD_ENVIRONMENT      // Loads water/HMC gas
};

int main()
{
	for (int i = 0; i < sizeof(sCollision) / 2; i++)
	{
		std::swap(sCollision[2*i], sCollision[2*i+1]);
	}

	int16_t* collisionData = (int16_t*)sCollision;
	{
		int16_t init = *collisionData++;
		assert(init == TERRAIN_LOAD_VERTICES);
		printf("COL_INIT(),\n");
	}

	int16_t numVertices;
	{
		numVertices = *collisionData++;
		printf("COL_VERTEX_INIT(%d),\n", numVertices);
	}
	{
		for (int i = 0; i < numVertices; i++)
		{
			int16_t x = *collisionData++;
			int16_t y = *collisionData++;
			int16_t z = *collisionData++;
			printf("COL_VERTEX(%d, %d, %d),\n", x, y, z);
		}
	}

	while (1)
	{
		int16_t type = *collisionData++;
		if (TERRAIN_LOAD_CONTINUE == type)
		{
			printf("COL_TRI_STOP(),\n");
			break;
		}

		int16_t count = *collisionData++;
		printf("COL_TRI_INIT(%d, %d),\n", type, count);

		for (int i = 0; i < count; i++)
		{
			int16_t v1 = *collisionData++;
			int16_t v2 = *collisionData++;
			int16_t v3 = *collisionData++;
			printf("COL_TRI(%d, %d, %d),\n", v1, v2, v3);
		}
	}

	{
		int16_t end = *collisionData++;
		assert(end == TERRAIN_LOAD_END);
		printf("COL_END(),\n");
	}
}
