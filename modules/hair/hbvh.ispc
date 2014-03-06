
extern "C" 
void ispc_intersectHairlet(void* uniform ptr,       /*!< pointer to user data */
                           varying RTCRay& _ray,     /*!< ray to intersect */
                           uniform size_t item      /*< item to intersect */)
{
  print("intersect %\n",item);
}

