#include "ospray/fb/FrameBuffer.ih"

export void setTile(void *uniform _fb, void *uniform _tile)
{
  uniform Tile *uniform        tile = (uniform Tile *uniform)_tile;
  uniform FrameBuffer *uniform fb   = (uniform FrameBuffer *uniform)_fb;
  fb->setTile(fb,*tile);
}

export void accumTile(void *uniform _fb, void *uniform _tile)
{
  uniform Tile *uniform        tile = (uniform Tile *uniform)_tile;
  uniform FrameBuffer *uniform fb   = (uniform FrameBuffer *uniform)_fb;
  fb->accumTile(fb,*tile);
}

