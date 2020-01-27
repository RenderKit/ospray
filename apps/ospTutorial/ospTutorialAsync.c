// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

/* This is a small example tutorial of how to use OSPRay's async API in an
 * application. We setup up two scenes which are rendered asynchronously
 * in parallel to each other.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "ospray/ospray_util.h"

typedef struct
{
  int x, y;
} vec2i;

// helper function to write the rendered image as PPM file
void writePPM(const char *fileName, const vec2i *size, const uint32_t *pixel)
{
  FILE *file = fopen(fileName, "wb");
  if (!file) {
    fprintf(stderr, "fopen('%s', 'wb') failed: %d", fileName, errno);
    return;
  }
  fprintf(file, "P6\n%i %i\n255\n", size->x, size->y);
  unsigned char *out = (unsigned char *)alloca(3 * size->x);
  for (int y = 0; y < size->y; y++) {
    const unsigned char *in =
        (const unsigned char *)&pixel[(size->y - 1 - y) * size->x];
    for (int x = 0; x < size->x; x++) {
      out[3 * x + 0] = in[4 * x + 0];
      out[3 * x + 1] = in[4 * x + 1];
      out[3 * x + 2] = in[4 * x + 2];
    }
    fwrite(out, 3 * size->x, sizeof(char), file);
  }
  fprintf(file, "\n");
  fclose(file);
}

void buildScene1(OSPCamera *camera,
    OSPWorld *world,
    OSPRenderer *renderer,
    OSPFrameBuffer *framebuffer,
    vec2i imgSize);

void buildScene2(OSPCamera *camera,
    OSPWorld *world,
    OSPRenderer *renderer,
    OSPFrameBuffer *framebuffer,
    vec2i imgSize);

int main(int argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError init_error = ospInit(&argc, argv);
  if (init_error != OSP_NO_ERROR)
    return init_error;

  vec2i imgSizes[2] = {0};
  imgSizes[0].x = 1024; // width
  imgSizes[0].y = 768; // height

  imgSizes[1].x = 800;
  imgSizes[1].y = 600;

  OSPCamera cameras[2] = {0};
  OSPWorld worlds[2] = {0};
  OSPRenderer renderers[2] = {0};
  OSPFrameBuffer framebuffers[2] = {0};
  buildScene1(
      &cameras[0], &worlds[0], &renderers[0], &framebuffers[0], imgSizes[0]);
  buildScene2(
      &cameras[1], &worlds[1], &renderers[1], &framebuffers[1], imgSizes[1]);

  printf("starting renders\n");
  OSPFuture futures[2] = {0};
  // render one frame for each scene
  for (int i = 0; i < 2; ++i) {
    futures[i] =
        ospRenderFrame(framebuffers[i], renderers[i], cameras[i], worlds[i]);
  }

  for (int i = 0; i < 2; ++i) {
    int isFinished = ospIsReady(futures[i], OSP_TASK_FINISHED);
    printf("status of 'futures[%i]' is %i\n", i, isFinished);
  }

  // We don't need to wait for them in the order they were started
  for (int i = 1; i >= 0; --i) {
    ospWait(futures[i], OSP_FRAME_FINISHED);
    printf("...done!\n");
    printf(
        "variance of render %i was %f\n", i, ospGetVariance(framebuffers[i]));
    ospRelease(futures[i]);
  }

  // access framebuffer and write its content as PPM file
  const uint32_t *fb =
      (uint32_t *)ospMapFrameBuffer(framebuffers[0], OSP_FB_COLOR);
  writePPM("firstFrame-scene1.ppm", &imgSizes[0], fb);
  ospUnmapFrameBuffer(fb, framebuffers[0]);

  fb = (uint32_t *)ospMapFrameBuffer(framebuffers[1], OSP_FB_COLOR);
  writePPM("firstFrame-scene2.ppm", &imgSizes[1], fb);
  ospUnmapFrameBuffer(fb, framebuffers[1]);

  // render 10 more frames, which are accumulated to result in a better
  // converged image
  for (int frames = 0; frames < 10; frames++) {
    for (int i = 0; i < 2; ++i) {
      futures[i] =
          ospRenderFrame(framebuffers[i], renderers[i], cameras[i], worlds[i]);
    }
    for (int i = 0; i < 2; ++i) {
      ospWait(futures[i], OSP_FRAME_FINISHED);
      ospRelease(futures[i]);
    }
  }

  fb = (uint32_t *)ospMapFrameBuffer(framebuffers[0], OSP_FB_COLOR);
  writePPM("accumulatedFrame-scene1.ppm", &imgSizes[0], fb);
  ospUnmapFrameBuffer(fb, framebuffers[0]);

  fb = (uint32_t *)ospMapFrameBuffer(framebuffers[1], OSP_FB_COLOR);
  writePPM("accumulatedFrame-scene2.ppm", &imgSizes[1], fb);
  ospUnmapFrameBuffer(fb, framebuffers[1]);

  // final cleanups
  for (int i = 0; i < 2; ++i) {
    ospRelease(cameras[i]);
    ospRelease(worlds[i]);
    ospRelease(renderers[i]);
    ospRelease(framebuffers[i]);
  }

  printf("Shutting down\n");
  fflush(0);
  ospShutdown();

  return 0;
}

void buildScene1(OSPCamera *camera,
    OSPWorld *world,
    OSPRenderer *renderer,
    OSPFrameBuffer *framebuffer,
    vec2i imgSize)
{
  // camera
  float cam_pos[] = {0.f, 0.f, 0.f};
  float cam_up[] = {0.f, 1.f, 0.f};
  float cam_view[] = {0.1f, 0.f, 1.f};

  // triangle mesh data
  static float vertex[] = {-1.0f,
      -1.0f,
      3.0f,
      -1.0f,
      1.0f,
      3.0f,
      1.0f,
      -1.0f,
      3.0f,
      0.1f,
      0.1f,
      0.3f};
  static float color[] = {0.9f,
      0.5f,
      0.5f,
      1.0f,
      0.8f,
      0.8f,
      0.8f,
      1.0f,
      0.8f,
      0.8f,
      0.8f,
      1.0f,
      0.5f,
      0.9f,
      0.5f,
      1.0f};
  static int32_t index[] = {0, 1, 2, 1, 2, 3};

  // create and setup camera
  *camera = ospNewCamera("perspective");
  ospSetFloat(*camera, "aspect", imgSize.x / (float)imgSize.y);
  ospSetParam(*camera, "pos", OSP_VEC3F, cam_pos);
  ospSetParam(*camera, "dir", OSP_VEC3F, cam_view);
  ospSetParam(*camera, "up", OSP_VEC3F, cam_up);
  ospCommit(*camera); // commit each object to indicate modifications are done

  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("mesh");
  OSPData data = ospNewSharedData1D(vertex, OSP_VEC3F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.position", data);
  ospRelease(data); // we are done using this handle

  data = ospNewSharedData1D(color, OSP_VEC4F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.color", data);
  ospRelease(data);

  data = ospNewSharedData1D(index, OSP_VEC3UI, 2);
  ospCommit(data);
  ospSetObject(mesh, "index", data);
  ospRelease(data);

  ospCommit(mesh);

  // put the mesh into a model
  static OSPGeometricModel model;
  model = ospNewGeometricModel(mesh);
  ospCommit(model);
  ospRelease(mesh);

  // put the model into a group (collection of models)
  OSPGroup group = ospNewGroup();
  OSPData models = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
  ospSetObject(group, "geometry", models);
  ospCommit(group);
  ospRelease(model);
  ospRelease(models);

  // put the group into an instance (give the group a world transform)
  static OSPInstance instance;
  instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // put the instance in the world
  *world = ospNewWorld();
  OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
  ospSetObject(*world, "instance", instances);

  // create and setup light for Ambient Occlusion
  static OSPLight light;
  light = ospNewLight("ambient");
  ospCommit(light);
  OSPData lights = ospNewSharedData1D(&light, OSP_LIGHT, 1);
  ospCommit(lights);
  ospSetObject(*world, "light", lights);

  ospCommit(*world);
  ospRelease(instance);
  ospRelease(instances);

  // create renderer
  *renderer =
      ospNewRenderer("scivis"); // choose Scientific Visualization renderer

  // complete setup of renderer
  ospSetInt(*renderer, "aoSamples", 1);
  ospSetFloat(*renderer, "backgroundColor", 1.0f); // white, transparent
  ospCommit(*renderer);

  ospRelease(light);
  ospRelease(lights);

  // create and setup framebuffer
  *framebuffer = ospNewFrameBuffer(imgSize.x,
      imgSize.y,
      OSP_FB_SRGBA,
      OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
}

void buildScene2(OSPCamera *camera,
    OSPWorld *world,
    OSPRenderer *renderer,
    OSPFrameBuffer *framebuffer,
    vec2i imgSize)
{
  // camera
  float cam_pos[] = {2.0f, -1.f, -4.f};
  float cam_up[] = {0.f, 1.f, 0.f};
  float cam_view[] = {-0.2f, 0.25f, 1.f};

  // triangle mesh data
  static float vertex[] = {-2.0f,
      -2.0f,
      2.0f,
      -2.0f,
      3.0f,
      2.0f,
      2.0f,
      -2.0f,
      2.0f,
      0.1f,
      -0.1f,
      1.f};
  static float color[] = {0.0f,
      0.1f,
      0.8f,
      1.0f,
      0.8f,
      0.8f,
      0.0f,
      1.0f,
      0.8f,
      0.8f,
      0.0f,
      1.0f,
      0.9f,
      0.1f,
      0.0f,
      1.0f};
  static int32_t index[] = {0, 1, 2, 1, 2, 3};

  // create and setup camera
  *camera = ospNewCamera("perspective");
  ospSetFloat(*camera, "aspect", imgSize.x / (float)imgSize.y);
  ospSetParam(*camera, "pos", OSP_VEC3F, cam_pos);
  ospSetParam(*camera, "dir", OSP_VEC3F, cam_view);
  ospSetParam(*camera, "up", OSP_VEC3F, cam_up);
  ospCommit(*camera); // commit each object to indicate modifications are done

  // create and setup model and mesh
  OSPGeometry mesh = ospNewGeometry("mesh");
  OSPData data = ospNewSharedData1D(vertex, OSP_VEC3F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.position", data);
  ospRelease(data);

  data = ospNewSharedData1D(color, OSP_VEC4F, 4);
  ospCommit(data);
  ospSetObject(mesh, "vertex.color", data);
  ospRelease(data);

  data = ospNewSharedData1D(index, OSP_VEC3UI, 2);
  ospCommit(data);
  ospSetObject(mesh, "index", data);
  ospRelease(data);

  ospCommit(mesh);

  // put the mesh into a model
  static OSPGeometricModel model;
  model = ospNewGeometricModel(mesh);
  ospCommit(model);
  ospRelease(mesh);

  // put the model into a group (collection of models)
  OSPGroup group = ospNewGroup();
  OSPData models = ospNewSharedData1D(&model, OSP_GEOMETRIC_MODEL, 1);
  ospSetObject(group, "geometry", models);
  ospCommit(group);
  ospRelease(model);
  ospRelease(models);

  // put the group into an instance (give the group a world transform)
  static OSPInstance instance;
  instance = ospNewInstance(group);
  ospCommit(instance);
  ospRelease(group);

  // put the instance in the world
  *world = ospNewWorld();
  OSPData instances = ospNewSharedData1D(&instance, OSP_INSTANCE, 1);
  ospSetObject(*world, "instance", instances);

  // create and setup light for Ambient Occlusion
  static OSPLight light;
  light = ospNewLight("ambient");
  ospCommit(light);
  OSPData lights = ospNewSharedData1D(&light, OSP_LIGHT, 1);
  ospCommit(lights);
  ospSetObject(*world, "light", lights);

  ospCommit(*world);
  ospRelease(instances);
  ospRelease(instance);

  // create renderer
  *renderer =
      ospNewRenderer("scivis"); // choose Scientific Visualization renderer

  // complete setup of renderer
  ospSetInt(*renderer, "aoSamples", 4);
  ospSetFloat(*renderer, "backgroundColor", 0.2f); // gray, transparent
  ospCommit(*renderer);

  ospRelease(light);
  ospRelease(lights);

  // create and setup framebuffer
  *framebuffer = ospNewFrameBuffer(imgSize.x,
      imgSize.y,
      OSP_FB_SRGBA,
      OSP_FB_COLOR | /*OSP_FB_DEPTH |*/ OSP_FB_ACCUM);
}
