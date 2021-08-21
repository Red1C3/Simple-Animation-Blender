/*MIT License

Copyright (c) 2021 Mohammad Issawi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#version 450
layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in ivec3 mBones;
layout(location=3) in vec3 weights;
layout(binding=0) uniform Block{
  mat4 bones[50];
  mat4 MVP;
  vec3 color;
};
void main(){
  vec4 mPos=vec4(pos,1);
  if(mBones!=ivec3(-1,-1,-1)){
    mPos=weights.x*bones[mBones.x]*mPos+
    weights.y*bones[mBones.y]*mPos+
    weights.z*bones[mBones.z]*mPos;
    }
    gl_Position=MVP*mPos;
}