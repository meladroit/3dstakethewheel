// 3dstakethewheel: playing with 3DS gamecard saves
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
// and of course, this being a 3DS program ...
#include <3ds.h>
// plus let's add smea's secret ingredient ...
#include "filesystem.h"

Result FSUSER_ControlArchive(Handle handle, FS_archive archive)
{
	u32* cmdbuf=getThreadCommandBuffer();

	u32 b1 = 0, b2 = 0;

	cmdbuf[0]=0x080d0144;
	cmdbuf[1]=archive.handleLow;
	cmdbuf[2]=archive.handleHigh;
	cmdbuf[3]=0x0;
	cmdbuf[4]=0x1; //buffer1 size
	cmdbuf[5]=0x1; //buffer1 size
	cmdbuf[6]=0x1a;
	cmdbuf[7]=(u32)&b1;
	cmdbuf[8]=0x1c;
	cmdbuf[9]=(u32)&b2;
 
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;
 
	return cmdbuf[1];
}

Result check_for_file()
{
	Handle outFileHandle;
	Result ret = 0;
	int fail = 0;
 
    ret = FSUSER_OpenFile(&saveGameFsHandle, &outFileHandle, saveGameArchive, FS_makePath(PATH_CHAR, "/save00.bin"), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
    if(ret){fail = -8; goto writeFail;}

	ret = FSFILE_Close(outFileHandle);
	if(ret){fail = -10; goto writeFail;}
    
	writeFail:
	if(fail)printf("save00.bin doesn't exist\n");
	else printf("save00.bin exists\n");

	return ret;
}

Result write_savedata(char* path, u8* data, u32 size)
{
	if(!path || !data || !size)return -1;

	Handle outFileHandle;
	u32 bytesWritten;
	Result ret = 0;
	int fail = 0;

	ret = FSUSER_OpenFile(&saveGameFsHandle, &outFileHandle, saveGameArchive, FS_makePath(PATH_CHAR, path), FS_OPEN_CREATE | FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
	if(ret){fail = -8; goto writeFail;}

	ret = FSFILE_Write(outFileHandle, &bytesWritten, 0x0, data, size, 0x10001);
	if(ret){fail = -9; goto writeFail;}

	ret = FSFILE_Close(outFileHandle);
	if(ret){fail = -10; goto writeFail;}

	ret = FSUSER_ControlArchive(saveGameFsHandle, saveGameArchive);

	writeFail:
	if(fail)printf("failed to write to file : %d\n     %08X %08X\n", fail, (unsigned int)ret, (unsigned int)bytesWritten);
	else printf("successfully wrote to file !\n     %08X               \n", (unsigned int)bytesWritten);

	return ret;
}


// I'm so lazy that I'm just going to copy stuff from the ctrulib sdmc example
//this will contain the data read from SDMC
u8* buffer = NULL;
off_t buffer_size;

int read_payload(char* path, u8* data)
{
    FILE *file = fopen(path,"rb");
	if (file == NULL)
        return 1;

	// seek to end of file
	fseek(file,0,SEEK_END);

	// file pointer tells us the size
	buffer_size = ftell(file);

	// seek back to start
	fseek(file,0,SEEK_SET);

	//allocate a buffer
	buffer=malloc(buffer_size);
	if(!buffer)
        return 1;

	//read contents !
	off_t bytesRead = fread(buffer,1,buffer_size,file);

	//close the file because we like being nice and tidy
	fclose(file);

	if(buffer_size!=bytesRead)
        return 1;
    return 0;
}

int main()
{
	gfxInitDefault();
	gfxSet3D(false);

	Result ret = filesystemInit();
    if(ret)
        printf("Something happened\n");

	PrintConsole topConsole;
	consoleInit(GFX_TOP, &topConsole);
	
	consoleSelect(&topConsole);
	consoleClear();
    
    int savefile_read = read_payload("save02.bin",buffer);
    if(savefile_read)
    {
        printf("savefile read unsuccessful\n");
        goto abortabortabort;
    }
    else
    {
        printf("savefile read successful\n");
        printf("savefile size is %lu\n",(u32)buffer_size);
    }
    
    ret = write_savedata("/save02.bin",buffer,(u32)buffer_size);
    if(ret)
    {
        printf("savefile write unsuccessful\n");
    }
    else
    {
        printf("partway there ...\n");
    }
    
    int payload_read = read_payload("payload.bin",buffer);
    if(payload_read)
    {
        printf("payload read unsuccessful\n");
        goto abortabortabort;
    }
    else
    {
        printf("payload read successful\n");
        printf("payload size is %lu\n",(u32)buffer_size);
    }
    
    ret = check_for_file();
    if(ret)
    {
        printf("can't seem to read from card\n");
    }
    
    ret = write_savedata("/payload.bin",buffer,(u32)buffer_size);
    if(ret)
    {
        printf("payload write unsuccessful\n");
    }
    else
    {
        printf("hey, it worked!\n");
    }
    
	while (aptMainLoop())
	{
		hidScanInput();
		if(hidKeysDown() & KEY_START)break;

		gspWaitForVBlank();
	}
    
    abortabortabort:
	filesystemExit();

	gfxExit();
	return 0;
}
