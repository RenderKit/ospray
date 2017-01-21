#include "command.h"

namespace ospray {
const char* commandToString(CommandTag tag) {
  switch (tag) {
    case CMD_INVALID: return "CMD_INVALID";
    case CMD_NEW_RENDERER: return "CMD_NEW_RENDERER";
    case CMD_FRAMEBUFFER_CREATE: return "CMD_FRAMEBUFFER_CREATE";
    case CMD_RENDER_FRAME: return "CMD_RENDER_FRAME";
    case CMD_FRAMEBUFFER_CLEAR: return "CMD_FRAMEBUFFER_CLEAR";
    case CMD_FRAMEBUFFER_MAP: return "CMD_FRAMEBUFFER_MAP";
    case CMD_FRAMEBUFFER_UNMAP: return "CMD_FRAMEBUFFER_UNMAP";
    case CMD_NEW_DATA: return "CMD_NEW_DATA";
    case CMD_NEW_MODEL: return "CMD_NEW_MODEL";
    case CMD_NEW_GEOMETRY: return "CMD_NEW_GEOMETRY";
    case CMD_NEW_MATERIAL: return "CMD_NEW_MATERIAL";
    case CMD_NEW_LIGHT: return "CMD_NEW_LIGHT";
    case CMD_NEW_CAMERA: return "CMD_NEW_CAMERA";
    case CMD_NEW_VOLUME: return "CMD_NEW_VOLUME";
    case CMD_NEW_TRANSFERFUNCTION: return "CMD_NEW_TRANSFERFUNCTION";
    case CMD_NEW_TEXTURE2D: return "CMD_NEW_TEXTURE2D";
    case CMD_ADD_GEOMETRY: return "CMD_ADD_GEOMETRY";
    case CMD_REMOVE_GEOMETRY: return "CMD_REMOVE_GEOMETRY";
    case CMD_ADD_VOLUME: return "CMD_ADD_VOLUME";
    case CMD_COMMIT: return "CMD_COMMIT";
    case CMD_LOAD_MODULE: return "CMD_LOAD_MODULE";
    case CMD_RELEASE: return "CMD_RELEASE";
    case CMD_REMOVE_VOLUME: return "CMD_REMOVE_VOLUME";
    case CMD_SET_MATERIAL: return "CMD_SET_MATERIAL";
    case CMD_SET_REGION: return "CMD_SET_REGION";
    case CMD_SET_REGION_DATA: return "CMD_SET_REGION_DATA";
    case CMD_SET_OBJECT: return "CMD_SET_OBJECT";
    case CMD_SET_STRING: return "CMD_SET_STRING";
    case CMD_SET_INT: return "CMD_SET_INT";
    case CMD_SET_FLOAT: return "CMD_SET_FLOAT";
    case CMD_SET_VEC2F: return "CMD_SET_VEC2F";
    case CMD_SET_VEC2I: return "CMD_SET_VEC2I";
    case CMD_SET_VEC3F: return "CMD_SET_VEC3F";
    case CMD_SET_VEC3I: return "CMD_SET_VEC3I";
    case CMD_SET_VEC4F: return "CMD_SET_VEC4F";
    case CMD_SET_PIXELOP: return "CMD_SET_PIXELOP";
    case CMD_REMOVE_PARAM: return "CMD_REMOVE_PARAM";
    case CMD_NEW_PIXELOP: return "CMD_NEW_PIXELOP";
    case CMD_API_MODE: return "CMD_API_MODE";
    case CMD_FINALIZE: return "CMD_FINALIZE";
    default: return "Unrecognized CommandTag";
  }
}
}

