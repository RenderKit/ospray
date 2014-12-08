/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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

