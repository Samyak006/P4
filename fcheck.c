#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include "types.h"
#include "fs.h"

#define BLOCK_SIZE (BSIZE)
#define T_DIR 1 //DIRECTORY
#define T_FILE 2 //FILE
#define T_DEV 3 // DEVICE
char bitarr[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };
#define BITSET(bitmapblocks, blockaddr) ((*(bitmapblocks + blockaddr / 8)) & (bitarr[blockaddr % 8]))


char* maddr;

int
check_valid_inode(struct dinode* dip, struct dirent* de, int n)
{
  int i;
  for(i = 0;i<n; i++,de++)
  {
    if(dip[de->inum].type!= T_DIR && dip[de->inum].type != T_FILE && dip[de->inum].type != T_DEV && dip[de->inum].type!=0)
    {
      return 1;
    }
  }
 return 0;
}

int
check_valid_addr(struct dinode* dip, struct dirent* de, struct superblock* sb,int n)
{
 int i,j;
 uint addr;
 for (i = 0; i<n;i++,de++)
 {
   for(j = 0;j<NDIRECT;j++)
   {
    addr = dip[i].addrs[j];
    if ( addr == 0)
      continue;
     if (addr < 0 || addr> sb->size)
       return 1;
   }
  }
 return 0;
}

int check_valid_indir( struct dinode* dip, struct dirent* de, struct superblock* sb,int n)
{
   int i,j;
   uint* inaddr;
   uint blockaddr;
   for(i = 0; i<n; i++,de++)
   {
     blockaddr = dip[de->inum].addrs[NDIRECT];
     if( blockaddr == 0)
       continue;
     if ( blockaddr < 0 || blockaddr >= sb->size)
         return 1;
     inaddr = (uint *) (maddr + (dip[de->inum].addrs[NDIRECT])*BLOCK_SIZE);
     for(j = 0;j<NINDIRECT;j++,inaddr++)
     { 
       blockaddr = *(inaddr);
       if (blockaddr == 0)
         continue;
       if (blockaddr < 0 || blockaddr >= sb->size)
	   return 1;
     }
   }
   return 0;
}

int
check_root_dir(struct dinode* dip, struct dirent *de,int n)
{
 int f_curr = 0;
 int f_par = 0;
  if(!strcmp(de->name,".") &&(de->inum==ROOTINO))
       f_curr = 1;
  printf("%d",f_curr);
  de++;
  
  if(!strcmp(de->name,"..") && (de->inum == ROOTINO))
     f_par = 1;
  printf("%d",f_par);
  if (f_curr==1 && f_par==1)
    return 0;
 return 1;
}

int
check_formatting(struct dinode* dip, struct dirent *de, int n)
{
  int i,j; 
  for(i = 0; i<n;i++,de++)
  {
    if(dip[de->inum].type == T_DIR)
    {
       for(j = 0;j<NDIRECT;j++)
       {
         // yet to be implemented;
       }
    }
  }
  return 0;
}
int
check_addr_inode(struct dinode* dip, struct dirent* de, struct superblock* sb, int n)
{
  int i,j;
  uint blockaddr;
  uint* inaddr;
  // bitmapblock address is incorrect
  char* bitmapblock = (char*) (maddr + (sb->ninodes)*BLOCK_SIZE);
  for(i = 0;i<n;i++,de++)
  {
    for (j = 0;j<NDIRECT;j++)
    {
      blockaddr = dip[i].addrs[j];
      if ( blockaddr == 0)
        continue;
      if(!BITSET(bitmapblock,blockaddr))
        return 1;
    }
    inaddr = (uint*)(maddr + (dip[i].addrs[NDIRECT])*BLOCK_SIZE);
    for (j = 0;j<NINDIRECT;j++,inaddr++)
    {
      blockaddr = *(inaddr);
      if ( blockaddr == 0)
        continue;
      if(!BITSET(bitmapblock,blockaddr))
        return 1;
    }
  }
  return 0;
}
int
main(int argc, char *argv[])
{
  //int r,i;
  int n,fsfd;
  char *addr;
  struct dinode *dip;
  struct superblock *sb;
  struct dirent *de;
  struct stat st;
  //char* bitmapblock;
  if(argc < 2){
    fprintf(stderr, "Usage: sample fs.img ...\n");
    exit(1);
  }


  fsfd = open(argv[1], O_RDONLY);
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }
  // arg 2 = 524248
  if (fstat(fsfd,&st)!=0)                         // to get the size of the file and storing it to a stat object. this is done in order to get the size as it might change wrt testcases.
  {
    fprintf(stderr,"failed to fstat file %s \n", argv[1]);
    return 1;
  }
  addr = mmap(NULL,st.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
  if (addr == MAP_FAILED){
	perror("mmap failed");
	exit(1);
  }
  maddr = addr;
  /* read the super block */
  sb = (struct superblock *) (addr + 1 * BLOCK_SIZE);
  //printf("fs size %d, no. of blocks %d, no. of inodes %d \n", sb->size, sb->nblocks, sb->ninodes);

  /* read the inodes */
  dip = (struct dinode *) (addr + IBLOCK((uint)0)*BLOCK_SIZE); 
  //printf("begin addr %p, begin inode %p , offset %d \n", addr, dip, (char *) dip - addr);
  // bitmapblocks
  //bitmapblocks = (char*) (dip+ sb->ninodes*BLOCK_SIZE);
  // read root inode
  // printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);

  // get the address of root dir 
  de = (struct dirent *) (addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);
  // get the address of bitmap start
  //bitmapblock = (char*)(addr + (sb->ninodes)*BLOCK_SIZE);
 // print the entries in the first block of root dir 
  //printf("%u\n",bitmapblock);
  n = dip[ROOTINO].size/sizeof(struct dirent);
  //checking for valid direct address.
  int valid_inode;
  int valid_diraddr;
  int valid_indir;
  //int check_format;
  int root_dir;
  //int addUseFB;
  //int bitmarkNU;
  
  // checking for bad inode.
  // TESTCASE: 1
  if((valid_inode = check_valid_inode(dip,de,n))==1)
  {
    fprintf(stderr,"ERROR: bad inode.\n"); 
    exit(1);
  } 
  
  // checking for bad direct address in inode 
  // TESTCASE: 2 ( badindir 1 not working)  
  if ((valid_diraddr = check_valid_addr(dip,de,sb,n))==1)
   {
     fprintf(stderr, "ERROR: bad direct address in inode.\n");
     exit(1);
   }
  if ((valid_indir = check_valid_indir(dip,de,sb,n))==1)
  {
    fprintf(stderr, "ERROR: bad indirect address in inode.\n");
    exit(1);
  }
  
  // checking for directory formatting.
  // TESTCASE: 3
  if ((root_dir = check_root_dir(dip,de,n))==1)
  {
    fprintf(stderr,"ERROR: root directory does not exist.\n");
    exit(1);
  }
  
  //checking if the directoty is formatted properly.
  /* TESTCASE: 4 unsure about implementation.
  if ((check_format = check_formatting(dip,de,sb,n))==1)
  {
    fprintf(stderr,"ERROR: directory not properly formatted");
    exit(1);
  }*/
  
  // checking for in-use inodes, each block address in use is also marked in use in the bitmap.
  /* TESTCASE: 5 
  if ((addUseFB = check_addr_inode(dip,de,sb,n))==1)
  {
    fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
    exit(1);
  }*/
  
  //for (i = 0; i < n; i++,de++){
  // for blocks marked in-use in bitmap, the block shoud actually be in-use in an inode or indirect block somewhere.
  /* TESTCASE: 6
  if(bitmarkNU)
  {
    fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
    exit(1);
  }*/
 // printf(" inum %d, name %s ", de->inum, de->name);
  //printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);
 // }
  
  exit(0);

}

