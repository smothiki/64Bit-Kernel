#include <sys/writeUtil.h>
#include <defs.h>
#include <sys/errno.h>
#include <sys/memoryProcessor.h>
#include<sys/pageProcessor.h>
#include <sys/errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/tarfsReader.h>
#include <sys/tarfs.h>
#include<sys/sysCallManager.h>

#define INVALIDDESCRIPTER -1

file_descriptor_t fd_list[OPEN_MAX];
struct File* getFile(const char *fileName);
int cur_fd=3;
extern struct registers_t* g_regVals;

int close (int fd){
	if(fd_list[fd].number!=0){
		fd_list[fd].number = INVALIDDESCRIPTER;
		FreePage((uint64_t)fd_list[fd].fp);
		return 0;
	}else{
		errno = EBADF;
		return -1;
	}
}


int open(const char *pathname, int flags){
	struct File* file = getFile(pathname);
	int temp = 3;
	if(file != NULL){
		while(temp < cur_fd && fd_list[temp].number != INVALIDDESCRIPTER ){
			temp++;
		}
		fd_list[temp].number = temp+1;
		fd_list[temp].fp = file;
		fd_list[temp].seek = 0;
		fd_list[temp].flags = flags;
		cur_fd++;
		//printf("fd_list - %d", fd_list[temp].number);
	}else{
		return -1;
	}
    return fd_list[temp].number;
}


struct File* getFile(const char *fileName)
{
    struct posix_header_ustar * entry = (struct posix_header_ustar*)(&_binary_tarfs_start);
	int padding =0;
	struct File* file = (struct File *)AllocateVirtualMemory();
	int exitflag = 0;
    while((uint64_t)entry < (uint64_t)&_binary_tarfs_end)
        {
			int size = getSize(entry->size);
			//printf("entry->name - %s",entry->name);
			if(strcmp(entry->typeflag,"5")  == 0 && strncmp(fileName,entry->name,lastIndexOf (fileName, "/")+1) == 0){
				strncpy(file->parent.d_name,entry->name,NAMEMAX);
				file->parent.offset = (char*)entry - (char*)&_binary_tarfs_start;
				if(++exitflag >= 2)
					break;
			}
            if(strcmp(fileName,entry->name) == 0 && strcmp(entry->typeflag,"0")  == 0){
				strncpy(file->path, fileName, NAMEMAX);
				file->offset = (char*)entry - (char*)&_binary_tarfs_start;
				if(++exitflag >= 2)
					break;
			}
			entry = (struct posix_header_ustar *)((char*)entry + sizeof(struct posix_header_ustar) + size );
			if(size > 0){
				padding = BLOCKSIZE - size%BLOCKSIZE;
				if((char*)&_binary_tarfs_end - (char*)entry >=BLOCKSIZE)
				{
					entry = (struct posix_header_ustar *)((char*)entry + padding);
				}
			}	
		}
		if(strcmp(fileName,entry->name)!=0)
		{
			FreePage((uint64_t)file);
			return NULL;
		}
		return file;
}



int t_read(int fildes, void *buf, int nbyte){	
	int readBytes=0,index=-1,size=0;
	errno = 0;
	while(fd_list[++index].number != fildes && index < OPEN_MAX);
	if(fildes >= 0 && fd_list[index].number != INVALIDDESCRIPTER){
		struct posix_header_ustar * entry = (struct posix_header_ustar *)((char*)(&_binary_tarfs_start)+(fd_list[index].fp->offset));
		size = getSize(entry->size);
		char* curser = (char*)(&_binary_tarfs_start)+(fd_list[index].fp->offset) + BLOCKSIZE + fd_list[index].seek; //Bypassing the header of file.
		while(readBytes < nbyte && fd_list[index].seek < size){
			*((char*)(buf+readBytes++)) = *(curser++);
			fd_list[index].seek++;
		}
		if(readBytes == 0){
			*((char*)(buf+readBytes++)) = 0;
			return 0;
		}
		*((char*)(buf+readBytes++)) = 0;
		if(fd_list[index].seek >= size && readBytes < nbyte){
			*((char*)(buf+readBytes++)) = 26;
		}
	}else{
		printf("\nfile does not exist");
		errno = EBADF;
	}
	return readBytes;
}

void initializeFileDescriptors(){
	extern file_descriptor_t fd_list[OPEN_MAX];
	int i = 0;
	for(i = 0;i < OPEN_MAX; i++){
		fd_list[i].number = INVALIDDESCRIPTER;
		fd_list[i].seek = 0;
		fd_list[i].fp = NULL;
		fd_list[i].flags = 0;
	}
}


 void opendirSysCall()
 {
  DIR * directory1,* directory2;
  directory1 = (DIR *)g_regVals->rcx;
  directory2 = opendir((char*)g_regVals->rbx);
  if(directory2 != NULL){
	  strcpy(directory1->dirname, directory2->dirname);
	  strcpy(directory1->current.d_name, directory2->current.d_name);
	  directory1->current.offset = directory2->current.offset;
	  directory1->dirent_filled = directory2->dirent_filled;
	  g_regVals->rax = SUCCESS;
  }else{
	  g_regVals->rax = FAILURE;
  }
 }

 void readdirSysCall()
 {
  dirent * dirent1, * dirent2;
  dirent1 = (dirent *)g_regVals->rcx;
  dirent2 = readdir((DIR *)g_regVals->rbx);
  if(dirent2 != NULL){
	  strcpy(dirent1->d_name, dirent2->d_name);
	  dirent1->offset = dirent2->offset;
	  g_regVals->rax = SUCCESS;
  }else{
	  g_regVals->rax = FAILURE;
  }
 }
 
  void closedirSysCall()
 {
  int ret;
  ret = closedir((DIR *)g_regVals->rbx);
  if(ret != -1){
	  g_regVals->rax = SUCCESS;
  }else{
	  g_regVals->rax = FAILURE;
  }
 }

void openSysCall()
 {
  uint64_t fd;
  fd = (uint64_t) open((char*)g_regVals->rbx,(int)g_regVals->rcx);
  g_regVals->rax = fd;
 }
 
  void closeSysCall()
 {
  uint64_t ret;
  ret = close((int)g_regVals->rbx);
  if(ret != -1){
	  g_regVals->rax = SUCCESS;
  }else{
	  g_regVals->rax = FAILURE;
  }
 }


