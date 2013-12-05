//
// Copyright 2012-2013, Syoyo Fujita.
// 
// Licensed under 2-clause BSD liecense.
//

//
// version 0.9.6: Support Ni(index of refraction) mtl parameter.
//                Parse transmittance material parameter correctly.
// version 0.9.5: Parse multiple group name.
//                Add support of specifying the base path to load material file.
// version 0.9.4: Initial suupport of group tag(g)
// version 0.9.3: Fix parsing triple 'x/y/z'
// version 0.9.2: Add more .mtl load support
// version 0.9.1: Add initial .mtl load support
// version 0.9.0: Initial
//


#include <cstdlib>
#include <cstring>
#include <cassert>

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

#include "readObj.h"

using namespace std;
//namespace tinyobj {

struct VertexIndex {
  int v_idx, vt_idx, vn_idx;
  VertexIndex() {};
  VertexIndex(int idx) : v_idx(idx), vt_idx(idx), vn_idx(idx) {};
  VertexIndex(int vidx, int vtidx, int vnidx) : v_idx(vidx), vt_idx(vtidx), vn_idx(vnidx) {};
};

// for std::map
static inline bool operator<(const VertexIndex& a, const VertexIndex& b)
{
  if (a.v_idx != b.v_idx) 
		return (a.v_idx < b.v_idx);

	if (a.vn_idx != b.vn_idx) 
		return (a.vn_idx < b.vn_idx);

	if (a.vt_idx != b.vt_idx) 
		return (a.vt_idx < b.vt_idx);

  return false;
}

struct obj_shape {
  vector<float> v;
  vector<float> vn;
  vector<float> vt;
};

//static inline bool isSpace(const char c) {
//  return (c == ' ') || (c == '\t');
//}
//
//static inline bool isNewLine(const char c) {
//  return (c == '\r') || (c == '\n') || (c == '\0');
//}
//
//// Make index zero-base, and also support relative index. 
//static inline int fixIndex(int idx, int n)
//{
//  int i;
//
//  if (idx > 0) {
//    i = idx - 1;
//  } else if (idx == 0) {
//    i = 0;
//  } else { // negative value = relative
//    i = n + idx;
//  }
//  return i;
//}
//
//static inline string parseString(const char*& token)
//{
//  string s;
//  int b = strspn(token, " \t");
//  int e = strcspn(token, " \t\r");
//  s = string(&token[b], &token[e]);
//
//  token += (e - b);
//  return s;
//}
//
//static inline float parseFloat(const char*& token)
//{
//  token += strspn(token, " \t");
//  float f = (float)atof(token);
//  token += strcspn(token, " \t\r");
//  return f;
//}
//
//static inline void parseFloat2(float& x, float& y, const char*& token)
//{
//  x = parseFloat(token);
//  y = parseFloat(token);
//}
//
//static inline void parseFloat3(float& x, float& y, float& z, const char*& token)
//{
//  x = parseFloat(token);
//  y = parseFloat(token);
//  z = parseFloat(token);
//}
//
//
//// Parse triples: i, i/j/k, i//k, i/j
//static VertexIndex parseTriple(const char* &token, int vsize, int vnsize, int vtsize)
//{
//    VertexIndex vi(-1);
//
//    vi.v_idx = fixIndex(atoi(token), vsize);
//    token += strcspn(token, "/ \t\r");
//    if (token[0] != '/') {
//      return vi;
//    }
//    token++;
//
//    // i//k
//    if (token[0] == '/') {
//      token++;
//      vi.vn_idx = fixIndex(atoi(token), vnsize);
//      token += strcspn(token, "/ \t\r");
//      return vi;
//    }
//    
//    // i/j/k or i/j
//    vi.vt_idx = fixIndex(atoi(token), vtsize);
//    token += strcspn(token, "/ \t\r");
//    if (token[0] != '/') {
//      return vi;
//    }
//
//    // i/j/k
//    token++;  // skip '/'
//    vi.vn_idx = fixIndex(atoi(token), vnsize);
//    token += strcspn(token, "/ \t\r");
//    return vi; 
//}

static unsigned int updateVertex(map<VertexIndex, unsigned int>& vertexCache,
  															 vector<float>& positions,
																 vector<float>& normals,
																 vector<float>& texcoords,
																 const vector<float>& in_positions,
																 const vector<float>& in_normals,
																 const vector<float>& in_texcoords,
																 const VertexIndex& i)
{
  const map<VertexIndex, unsigned int>::iterator it = vertexCache.find(i);

  if (it != vertexCache.end()) {
    // found cache
    return it->second;
  }

  assert(in_positions.size() > (3*i.v_idx+2));

  positions.push_back(in_positions[3*i.v_idx+0]);
  positions.push_back(in_positions[3*i.v_idx+1]);
  positions.push_back(in_positions[3*i.v_idx+2]);

  if (i.vn_idx >= 0) {
    normals.push_back(in_normals[3*i.vn_idx+0]);
    normals.push_back(in_normals[3*i.vn_idx+1]);
    normals.push_back(in_normals[3*i.vn_idx+2]);
  }

  if (i.vt_idx >= 0) {
    texcoords.push_back(in_texcoords[2*i.vt_idx+0]);
    texcoords.push_back(in_texcoords[2*i.vt_idx+1]);
  }

  unsigned int idx = positions.size() / 3 - 1;
  vertexCache[i] = idx;

  return idx;
}

static bool exportFaceGroupToShape(Mesh *shape,
																	 const vector<float> &in_positions,
																	 const vector<float> &in_normals,
																	 const vector<float> &in_texcoords,
																	 const vector<vector<VertexIndex> >& faceGroup,
																	 const material_t &material,
																	 const string &name)
{
  if (faceGroup.empty()) {
    return false;
  }//if

  // Flattened version of vertex data
  vector<float> positions;
  vector<float> normals;
  vector<float> texcoords;
  map<VertexIndex, unsigned int> vertexCache;
  vector<unsigned int> indices;

  // Flatten vertices and indices
  for (size_t i = 0; i < faceGroup.size(); i++) {
    const vector<VertexIndex>& face = faceGroup[i];

    VertexIndex i0 = face[0];
    VertexIndex i1(-1);
    VertexIndex i2 = face[1];

    size_t npolys = face.size();

    // Polygon -> triangle fan conversion
    for (size_t k = 2; k < npolys; k++) {
      i1 = i2;
      i2 = face[k];

      unsigned int v0 = updateVertex(vertexCache, positions, normals, texcoords, 
																		 in_positions, in_normals, in_texcoords, i0);

			unsigned int v1 = updateVertex(vertexCache, positions, normals, texcoords, 
																		 in_positions, in_normals, in_texcoords, i1);

			unsigned int v2 = updateVertex(vertexCache, positions, normals, texcoords, 
																		 in_positions, in_normals, in_texcoords, i2);

      indices.push_back(v0);
      indices.push_back(v1);
      indices.push_back(v2);
    }//for2
  }//for1

  //
  // Construct shape.
  //
  shape = name;
  shape.vertices.swap(positions);
  shape.normals .swap(normals);
  shape.uvs		  .swap(texcoords);
  shape.elements.swap(indices);
  shape.material = material;

  return true;
}

  
void InitMaterial(Mesh *material) 
{
  material.name = "";
  material.ambient_texname = "";
  material.diffuse_texname = "";
  material.specular_texname = "";
  material.normal_texname = "";

	for (int i = 0; i < 3; i ++) {
    material.ambient[i] = 0.f;
    material.diffuse[i] = 0.f;
    material.specular[i] = 0.f;
    material.transmittance[i] = 0.f;
    material.emission[i] = 0.f;
  }//for

  material.shininess = 1.f;
  material.unknown_parameter.clear();
}

//string LoadMtl(map<string, material_t>& material_map,
//  									const char* filename, const char* mtl_basepath)
//{
//  material_map.clear();
//  stringstream err;
//
//  string filepath;
//
//  if (mtl_basepath) {
//    filepath = string(mtl_basepath) + string(filename);
//  }//if
//	
//	else {
//    filepath = string(filename);
//  }//else
//
//  ifstream ifs(filepath.c_str());
//  if (!ifs) {
//    err << "Cannot open file [" << filepath << "]" << endl;
//    return err.str();
//  }//if
//
//  material_t material;
//  
//  int maxchars = 8192;  // Alloc enough size.
//  vector<char> buf(maxchars);  // Alloc enough size.
//
//	while (ifs.peek() != -1) {
//    ifs.getline(&buf[0], maxchars);
//
//    string linebuf(&buf[0]);
//
//    // Trim newline '\r\n' or '\r'
//    if (linebuf.size() > 0) {
//      if (linebuf[linebuf.size()-1] == '\n') linebuf.erase(linebuf.size()-1);
//    }//if
//
//    if (linebuf.size() > 0) {
//      if (linebuf[linebuf.size()-1] == '\n') linebuf.erase(linebuf.size()-1);
//    }//if
//
//    // Skip if empty line.
//    if (linebuf.empty()) {
//      continue;
//    }//if
//
//    // Skip leading space.
//    const char* token = linebuf.c_str();
//    token += strspn(token, " \t");
//
//    assert(token);
//    if (token[0] == '\0') continue; // empty line
//    
//    if (token[0] == '#') continue;  // comment line
//    
//    // new mtl
//    if ((0 == strncmp(token, "newmtl", 6)) && isSpace((token[6]))) {
//      // flush previous material.
//      material_map.insert(pair<string, material_t>(material.name, material));
//
//      // initial temporary material
//      InitMaterial(material);
//
//      // set new mtl name
//      char namebuf[4096];
//      token += 7;
//      sscanf(token, "%s", namebuf);
//      material.name = namebuf;
//      continue;
//    }//if
//    
//    // ambient
//    if (token[0] == 'K' && token[1] == 'a' && isSpace((token[2]))) {
//      token += 2;
//      
//			float r, g, b;
//      parseFloat3(r, g, b, token);
//      	material.ambient[0] = r;
//      	material.ambient[1] = g;
//      	material.ambient[2] = b;
//     
//		 	continue;
//    }//if
//    
//    // diffuse
//    if (token[0] == 'K' && token[1] == 'd' && isSpace((token[2]))) {
//      token += 2;
//      
//			float r, g, b; 
//			parseFloat3(r, g, b, token);
//      	material.diffuse[0] = r;
//      	material.diffuse[1] = g;
//      	material.diffuse[2] = b;
//      
//			continue;
//    }//if
//    
//    // specular
//    if (token[0] == 'K' && token[1] == 's' && isSpace((token[2]))) {
//      token += 2;
//      float r, g, b;
//      parseFloat3(r, g, b, token);
//      	material.specular[0] = r;
//      	material.specular[1] = g;
//      	material.specular[2] = b;
//      
//			continue;
//    }//if
//    
//    // transmittance
//    if (token[0] == 'K' && token[1] == 't' && isSpace((token[2]))) {
//      token += 2;
//      
//			float r, g, b;
//			parseFloat3(r, g, b, token);
//      	material.transmittance[0] = r;
//      	material.transmittance[1] = g;
//      	material.transmittance[2] = b;
//      
//			continue;
//    }
//
//    // ior(index of refraction)
//    if (token[0] == 'N' && token[1] == 'i' && isSpace((token[2]))) {
//      token += 2;
//      material.ior = parseFloat(token);
//      
//			continue;
//    }//if
//
//    // emission
//    if(token[0] == 'K' && token[1] == 'e' && isSpace(token[2])) {
//      token += 2;
//      
//			float r, g, b;
//      parseFloat3(r, g, b, token);
//      	material.emission[0] = r;
//      	material.emission[1] = g;
//      	material.emission[2] = b;
//      
//			continue;
//    }//if
//
//    // shininess
//    if(token[0] == 'N' && token[1] == 's' && isSpace(token[2])) {
//      token += 2;
//      material.shininess = parseFloat(token);
//      
//			continue;
//    }//if
//
//    // ambient texture
//    if ((0 == strncmp(token, "map_Ka", 6)) && isSpace(token[6])) {
//      token += 7;
//      material.ambient_texname = token;
//      
//			continue;
//    }//if
//
//    // diffuse texture
//    if ((0 == strncmp(token, "map_Kd", 6)) && isSpace(token[6])) {
//      token += 7;
//      material.diffuse_texname = token;
//      
//			continue;
//    }//if
//
//    // specular texture
//    if ((0 == strncmp(token, "map_Ks", 6)) && isSpace(token[6])) {
//      token += 7;
//      material.specular_texname = token;
//      
//			continue;
//    }//if
//
//    // normal texture
//    if ((0 == strncmp(token, "map_Ns", 6)) && isSpace(token[6])) {
//      token += 7;
//      material.normal_texname = token;
//      
//			continue;
//    }//if
//
//    // unknown parameter
//    const char* _space = strchr(token, ' ');
//    
//		if(!_space) {
//      _space = strchr(token, '\t');
//    }//if
//    
//		if(_space) {
//      int len = _space - token;
//      string key(token, len);
//      string value = _space + 1;
//      material.unknown_parameter.insert(pair<string, string>(key, value));
//    }//if
//  }//while
//
//  // flush last material.
//  material_map.insert(pair<string, material_t>(material.name, material));
//
//  return err.str();
//}

string load_obj(vector<shape_t>& shapes, const char* filename, const char* mtl_basepath)
{
  shapes.clear();

  stringstream err;

  ifstream ifs(filename);
  if (!ifs) {
    err << "Cannot open file [" << filename << "]" << endl;
    return err.str();
  }//if

  vector<float> v;
  vector<float> vn;
  vector<float> vt;
  vector<vector<VertexIndex> > faceGroup;
  string name;

  // material
  map<string, material_t> material_map;
  material_t material;

  int maxchars = 8192;  // Alloc enough size.
  vector<char> buf(maxchars);  // Alloc enough size.
  
	while (ifs.peek() != -1) {
    ifs.getline(&buf[0], maxchars);

    string linebuf(&buf[0]);

    // Trim newline '\r\n' or '\r'
    if (linebuf.size() > 0) {
      if (linebuf[linebuf.size()-1] == '\n') linebuf.erase(linebuf.size()-1);
    }

    if (linebuf.size() > 0) {
      if (linebuf[linebuf.size()-1] == '\n') linebuf.erase(linebuf.size()-1);
    }

    // Skip if empty line.
    if (linebuf.empty()) {
      continue;
    }

    // Skip leading space.
    const char* token = linebuf.c_str();
    token += strspn(token, " \t");

    assert(token);
    if (token[0] == '\0') continue; // empty line
    
    if (token[0] == '#') continue;  // comment line

    // vertex
    if (token[0] == 'v' && isSpace((token[1]))) {
      token += 2;
      float x, y, z;
      parseFloat3(x, y, z, token);
      v.push_back(x);
      v.push_back(y);
      v.push_back(z);
      continue;
    }//if

    // normal
    if (token[0] == 'v' && token[1] == 'n' && isSpace((token[2]))) {
      token += 3;
      float x, y, z;
      parseFloat3(x, y, z, token);
      vn.push_back(x);
      vn.push_back(y);
      vn.push_back(z);
      continue;
    }//if

    // texcoord
    if (token[0] == 'v' && token[1] == 't' && isSpace((token[2]))) {
      token += 3;
      float x, y;
      parseFloat2(x, y, token);
      vt.push_back(x);
      vt.push_back(y);
      continue;
    }//if

    // face
    if (token[0] == 'f' && isSpace((token[1]))) {
      token += 2;
      token += strspn(token, " \t");

      vector<VertexIndex> face;
      while (!isNewLine(token[0])) {
        VertexIndex vi = parseTriple(token, v.size() / 3, vn.size() / 3, vt.size() / 2);
        face.push_back(vi);
        int n = strspn(token, " \t\r");
        token += n;
      }

      faceGroup.push_back(face);
      
      continue;
    }//if

    // use mtl
    if ((0 == strncmp(token, "usemtl", 6)) && isSpace((token[6]))) {

      char namebuf[4096];
      token += 7;
      sscanf(token, "%s", namebuf);

      if (material_map.find(namebuf) != material_map.end()) {
        material = material_map[namebuf];
      } else {
        // { error!! material not found }
        InitMaterial(material);
      }
      continue;

    }//if

    // load mtl
    if ((0 == strncmp(token, "mtllib", 6)) && isSpace((token[6]))) {
      char namebuf[4096];
      token += 7;
      sscanf(token, "%s", namebuf);

      string err_mtl = LoadMtl(material_map, namebuf, mtl_basepath);
      if (!err_mtl.empty()) {
        faceGroup.clear();  // for safety
        return err_mtl;
      }
      continue;
    }//if

    // group name
    if (token[0] == 'g' && isSpace((token[1]))) {

      // flush previous face group.
      shape_t shape;
      bool ret = exportFaceGroupToShape(shape, v, vn, vt, faceGroup, material, name);
      if (ret) {
        shapes.push_back(shape);
      }

      faceGroup.clear();

      vector<string> names;
      while (!isNewLine(token[0])) {
        string str = parseString(token);
        names.push_back(str);
        token += strspn(token, " \t\r"); // skip tag
      }//while

      assert(names.size() > 0);

      // names[0] must be 'g', so skipt 0th element.
      if (names.size() > 1) {
        name = names[1];
      } else {
        name = "";
      }

      continue;
    }//if

    // object name
    if (token[0] == 'o' && isSpace((token[1]))) {

      // flush previous face group.
      shape_t shape;
      bool ret = exportFaceGroupToShape(shape, v, vn, vt, faceGroup, material, name);
      if (ret) {
        shapes.push_back(shape);
      }

      faceGroup.clear();

      // @todo { multiple object name? }
      char namebuf[4096];
      token += 2;
      sscanf(token, "%s", namebuf);
      name = string(namebuf);


      continue;
    }//if

    // Ignore unknown command.
  }//while

  shape_t shape;
  bool ret = exportFaceGroupToShape(shape, v, vn, vt, faceGroup, material, name);
  if (ret) {
    shapes.push_back(shape);
  }

  faceGroup.clear();  // for safety

  return err.str();
}

//}; //namespace
