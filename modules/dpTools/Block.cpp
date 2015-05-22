#include "Block.h"
#include "minilzo/minilzo.h"

namespace ospray {
  
  embree::AtomicCounter BlockRef::nextBlockID = 0;

  std::string BlockRef::fileNameBase = ".";
  
  bool Block::push(const Triangle &tri) { 
    if (triangle.size() == MAX_TRIS_PER_BLOCK) 
      return false;
    this->triangle.push_back(tri);
    return true;
  }
  
  // std::string BlockRef::getFileName() 
  // {
  //   std::stringstream name;
  //   name << fileNameBase << "/" << std::hex << (int*)blockID << ".block";  
  //   return name.str();
  // };

  // Mutex pMutex;

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]



  void Block::writeTo(const std::string &fn)
  {

    std::stringstream cmd;
#if 0
static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);
    FILE *file = fopen(fn.c_str(),"wb");
    if (!file) throw std::runtime_error("could not open file");
    assert(file);
    size_t numTriangles = triangle.size();
    fwrite(&numTriangles,sizeof(numTriangles),1,file);

    lzo_uint szCompressed = numTriangles*sizeof(Triangle);
    char *compressed = (char*)embree::alignedMalloc(szCompressed);
    // PRINT((int*)compressed);
    
    // char __aligned(64) wrkMem[LZO1X_MEM_COMPRESS];
    
    // PRINT(triangle.size());
    // char *wrkMem = (char*)malloc(LZO1X_1_MEM_COMPRESS+100);
    lzo1x_1_compress((lzo_bytep)&triangle[0],triangle.size()*sizeof(Triangle),(lzo_bytep)compressed,&szCompressed,wrkmem);
    fwrite(&szCompressed,sizeof(szCompressed),1,file);
    size_t numWritten = fwrite(compressed,1,szCompressed,file);
    assert(numWritten == szCompressed);
    fclose(file);
    embree::alignedFree(compressed);
#else
    FILE *file = fopen(fn.c_str(),"w+");
    if (!file) throw std::runtime_error("could not popen bzip2");
    assert(file);
    size_t numTriangles = triangle.size();
    fwrite(&numTriangles,sizeof(numTriangles),1,file);
    size_t numWritten = fwrite(&triangle[0],sizeof(triangle[0]),numTriangles,file);
    assert(numWritten == numTriangles);
    fclose(file);
#endif
  }

  void Block::readFrom(const std::string &fn)
  {
#if 0
static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);
    FILE *file = fopen(fn.c_str(),"rb");
    if (!file) throw std::runtime_error("could not fopen file");
    assert(file);
    size_t numTriangles;
    fread(&numTriangles,sizeof(numTriangles),1,file);
    triangle.resize(numTriangles);

    lzo_uint szCompressed;
    fread(&szCompressed,sizeof(szCompressed),1,file);
    char *compressed = (char*)embree::alignedMalloc(szCompressed);
    size_t numRead = fread(compressed,1,szCompressed,file);
    assert(numRead == szCompressed);
    
    lzo_uint szDecompressed = 0;
    lzo1x_decompress_safe((lzo_bytep)compressed,szCompressed,(lzo_bytep)&triangle[0],&szDecompressed,wrkmem);
    // printf("lzo read %li -> %li\n",szCompressed,szDecompressed);

    fclose(file);
    embree::alignedFree(compressed);
#else
    assert(block == NULL);
    FILE *file = fopen(fn.c_str(),"rb");
    if (!file) throw std::runtime_error("could not fopen ");
    assert(file);
    size_t numTriangles;
    fread(&numTriangles,sizeof(numTriangles),1,file);
    triangle.resize(numTriangles);
    size_t numRead = fread(&triangle[0],sizeof(triangle[0]),numTriangles,file);
    assert(numRead == numTriangles);
    fclose(file);
#endif
  }

  std::string BlockRef::getFileName()
  {
    int len = strlen(fileNameBase.c_str())+100;
    char fn[len];
    sprintf(fn,"%s/block_%012li.block",fileNameBase.c_str(),blockID);
    return fn;
  }

  void BlockRef::saveBlock(Block &block)
  {
    block.writeTo(getFileName());
  }

  void BlockRef::loadBlock(Block &block)
  {
    block.readFrom(getFileName());
  }


  // void BlockDB::eraseBlock(size_t blockID)
  // {
  //   std::stringstream fileName;
  //   fileName << fileNameBase << "/" << std::hex << (int*)blockID << ".block";
  //   unlink(fileName.str().c_str());
  // }

}
