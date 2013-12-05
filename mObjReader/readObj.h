//
// Copyright 2012-2013, Syoyo Fujita.
//
// Licensed under 2-clause BSD liecense.
//
#ifndef _TINY_OBJ_LOADER_H
#define _TINY_OBJ_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <GL/gl.h>
#include <glm/glm.hpp>

using namespace std;
class Mesh
{
	public:
			std::string name;

			float ambient[3];
			float diffuse[3];
			float specular[3];
			float transmittance[3];
			float emission[3];
			float shininess;
			float ior; // index of refraction

			string ambient_texname;
			string diffuse_texname;
			string specular_texname;
			string normal_texname;
			map<string, string> unknown_parameter;

			vector<GLfloat> vertices;
			vector<GLfloat> normals;
			vector<GLfloat> uvs;
			vector<GLuint>  elements;

//			std::string  name;
//			material_t   material;
//			mesh_t       mesh;

			Mesh() {}
			~Mesh() {}
}; 

	// Loads .obj from a file.
	// 'shapes' will be filled with parsed shape data
	// The function returns error string.
	// Returns empty string when loading .obj success.
	// 'mtl_basepath' is optional, and used for base path for .mtl file.
	
	// output = vector<shape_t>& shapes;
	std::string load_obj(std::vector<shape_t>& shapes, const char* filename, const char* mtl_basepath = NULL);

#endif  // _TINY_OBJ_LOADER_H
