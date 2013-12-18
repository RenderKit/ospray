struct vec3fa {
  float x;
  float y;
  float z;
  float w;
};

task void renderLine(uniform vec3fa *uniform fb,
                     uniform int size_x,
                     uniform int size_y,
                     uniform int frameID)
{
  uniform int y = taskIndex;
  foreach (x = 0...size_x) {
    int ix = (x+frameID)%size_x;
    int iy = (y+frameID)%size_y;
    int iz = (x+y);
    float fx = ((float)ix)/size_x;
    float fy = ((float)iy)/size_y;
    float fz = ((float)iz)/(size_x+size_y);
    int idx = x + y * size_x;
    fb[idx].x = fx;
    fb[idx].y = fy;
    fb[idx].w = fz;
    fb[idx].z = 0.f;
  }
}


export void ispc__TestRenderer_renderFrame(uniform vec3fa *uniform fb,
                                            uniform int size_x, 
                                            uniform int size_y,
                                            uniform int frameID)
{
  launch[size_y] renderLine(fb,size_x,size_y,frameID);
}

