/**
 * From the OpenGL Programming wikibook: http://en.wikibooks.org/wiki/OpenGL_Programming
 * This file is in the public domain.
 * Contributors: Sylvain Beucler
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "readObj.h"

using namespace std;

void load_obj(const char* filename, Mesh* mesh) {
  ifstream in(filename, ios::in);
	std::ofstream t;
	t.open("test.txt");
	GLuint maxv = 0, maxuv  = 0, maxn = 0;


	if (!in) { cerr << "Cannot open " << filename << endl; exit(1); }

	vector<glm::vec4> temp_vertices;
	vector<glm::vec2> temp_uv;
	vector<glm::vec3> temp_normals;

  string line;
  while(getline(in, line)) {
    if (line.substr(0,2) == "v ") {
      istringstream s(line.substr(2));
      glm::vec4 v; 
			s >> v.x; 
			s >> v.y; 
			s >> v.z; 
			v.w = 1.0;
  
			mesh->vertices.push_back(v);
    }//if  

		else if(line.substr(0,3) == "vt "){
			istringstream s(line.substr(3));
			glm::vec2 v;
			s >> v.x;
			s >> v.y;

			t << v.x << " " << v.y << "\n";

			temp_uv.push_back(v);
		}//else if

		else if(line.substr(0, 3) == "vn "){
			istringstream s(line.substr(3));
			glm::vec3 v;

			s >> v.x;
			s >> v.y;
			s >> v.z;

			temp_normals.push_back(v);
		}//else if

		else if (line.substr(0,2) == "f ") {
			// a = vertices index, b = tex coord index, c = normal index
			GLuint a[3], b[3], c[3];
      
			sscanf(line.c_str(), "%*s %d/%d/%d %d/%d/%d %d/%d/%d",
														&a[0], &b[0], &c[0],
														&a[1], &b[1], &c[1],
														&a[2], &b[2], &c[2]);

			for(int i = 0; i < 3; i ++){
				maxv = max(maxv, a[i]);
				maxuv = max(maxuv, b[i]);
				maxn = max(maxn, c[i]);
			}	
			
			for(int i = 0; i < 3; i++){
				mesh->elements.push_back( a[i]);			// Vertex
				mesh->uvIndices.push_back(b[i]);			// Texture Coords
				mesh->normalIndices.push_back(c[i]);	// Normals
			}//for
    }//else if

    else if (line[0] == '#') { }
    
		else { }
  }//while

	t.close();

	// Rearrange all the indices to piece everything together nicely
	for(unsigned int i = 0; i < mesh->elements.size(); i++){
//		t << i << "\n";	
//		
		unsigned int currVertexIndex = mesh->elements[i],
								 currUvIndex 		 = mesh->uvIndices[i],
								 currNormIndex   = mesh->normalIndices[i];
//			
//		glm::vec4 vertex = temp_vertices[currVertexIndex-1];
		glm::vec2 uv 		 = temp_uv[currUvIndex];
//	
//		t << currUvIndex-1 << "\t" << uv.x << " " << uv.y;
//		t << " ...OK\n";
//		glm::vec3 normal = temp_normals[currNormIndex-1];
//
//		mesh->vertices.push_back(vertex);
		mesh->UV.push_back(uv);
//		mesh->normals.push_back(normal);
//
//
//		mesh->elements[i] --;
	}//for
}
