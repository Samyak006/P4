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

int
check_valid_addr(struct dinode* dip)
{
 int i;
 uint* addr = dip->addrs;
 for(i = 0;i<NDIRECT;i++)
 {
   if ( addr[i] == 0)
     continue;
   if ( addr[i] < 0 || addr[i] >= dip->size)
     fprintf(stderr,"bad direct address in inode.");
     return 1;
 }
 return 0;
}

int
format_checker(struct dinode* dip, struct dirent *de,int n)
{
 int i,f_curr,f_par;
 for(i = 0;i<n;i++,de++)
 {
   if(!strcmp(de->name,"."))
     f_curr = 1;
   if(!strcmp(de->name,".."))
     f_par = 1;
 }
 if (f_curr && f_par)
   return 0;
 return 1;
}

int
main(int argc, char *argv[])
{
  int r,i,n,fsfd;
  char *addr;
  struct dinode *dip;
  struct superblock *sb;
  struct dirent *de;
  struct stat st;

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
  /* read the super block */
  sb = (struct superblock *) (addr + 1 * BLOCK_SIZE);
  printf("fs size %d, no. of blocks %d, no. of inodes %d \n", sb->size, sb->nblocks, sb->ninodes);

  /* read the inodes */
  dip = (struct dinode *) (addr + IBLOCK((uint)0)*BLOCK_SIZE); 
  printf("begin addr %p, begin inode %p , offset %d \n", addr, dip, (char *) dip - addr);

  // read root inode
  printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);

  // get the address of root dir 
  de = (struct dirent *) (addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);

  // print the entries in the first block of root dir 

  n = dip[ROOTINO].size/sizeof(struct dirent);
  //checking for valid direct address.
  int valid_diraddr;
  if ((valid_diraddr = check_valid_addr(dip))==1)
  {
    fprintf(stderr, "ERROR: bad direct address in inode.");
    exit(1);
  }
  int check_format;
  if ((check_format = format_checker(dip,de,n))==1)
  {
    fprintf(stderr,"ERROR: directory not properly formatted.");
    exit(1);
  }
  for (i = 0; i < n; i++,de++){
 	// checking for bad inode.
	if(dip[de->inum].type >3)
	{
	  fprintf(stderr,"ERROR: bad inode.\n"); 
	  exit(1);
	}
 	printf(" inum %d, name %s ", de->inum, de->name);
	printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);
  }
  exit(0);

}

