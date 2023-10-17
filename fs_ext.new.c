/*
*    Enables SD Carding using elm FATfs support - I used CarlK's SPI/SDIO
*    implementation for Pi Pico. Uses lwip HTTPD by turning on these switches:
*
*       #define LWIP_HTTPD_CUSTOM_FILES 1
*       #define LWIP_HTTPD_DYNAMIC_FILE_READ 1
*       #define LWIP_HTTPD_FILE_EXTENSION 1
*       #define LWIP_HTTPD_DYNAMIC_HEADERS 1
*
*    We keep a pointer to the sdFile in the pextension of the httpd file struct
*
*    It's allocated on the open and freed on the close
*    
*    On the open we set the file length according to sd file and the index to zero
*    We also set LWIP_HTTPD_CUSTOM_FILES so that our fs_read_custom is called
*
*    In the read we must set the file index because fs.c will not do it if fs_read_custom is called
*
*    In the close we close the sd file and free the allocated struct
*    
*/
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_utils.h"

#include "fs.h"



#include "hw_config.h"
#include "router.h"

#include <stdbool.h>
#include <string.h>

#include "lwip/apps/httpd.h"

// only mount once
static bool init = false;

#define FS_FILE_FLAGS_ROUTE             0x80
#define FS_FILE_FLAGS_HEADER_OUT        0x40

#define MAX_ROUTE_LEN 1460

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

int fs_open_custom(struct fs_file *file, const char *name){

    void (*fun_ptr)(char*);

    // check if it is a route first, if not it must be a custom file
    fun_ptr = (void (*)(char *))isRoute(name);

    if(!fun_ptr)
    {
        if(!init){
            // mount sd card

            FF_Disk_t *pxDisk = NULL;

            if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)){
                panic("failed to mount card\n");
                return 0;
            };
            printf("mounted %s: successfully\n", DEVICENAME);
            init = true;   
        }
        char fullpath[256];
        sprintf(fullpath, "%s%s",MOUNTPOINT, name);
        //printf("opening custom: %s\n", fullpath);
        FF_FILE *pxFile = ff_fopen(fullpath, "r");
        if (pxFile && ff_filelength( pxFile )) {
        
            size_t fsize = ff_filelength( pxFile );
            
            if (fsize > 0){
                // file exists we'll read it
                file->data = NULL;
                file->len = fsize;
                file->index = 0;
                file->flags |= FS_FILE_FLAGS_HEADER_INCLUDED; // we are now providing our own headers
                file->flags |= FS_FILE_FLAGS_CUSTOM;
                file->pextension = pxFile;
            }
        }
        else{
            printf("failed to open file %s\n", name);
            return 0;
        }
    }
    else{ // is a route
        // flag as custom and a route
        file->data = NULL;
        file->index = 0;
        file->flags |= FS_FILE_FLAGS_HEADER_INCLUDED; // we are now providing our own headers
        file->flags |= FS_FILE_FLAGS_CUSTOM;
        file->flags |= FS_FILE_FLAGS_ROUTE; // not used in fs.c
        file->pextension = fun_ptr; // store the handler for read
        file->len = MAX_ROUTE_LEN; // max len
        //printf("open: %s is a route\n", name);
    }
    return 1;
}

int printHTMLHeaders(char* buffer, int count){
    strcpy(buffer, "HTTP/1.1 200 OK\n");
    strcat(buffer, "Content-Type: text/html; charset=utf-8\n");
    sprintf(buffer+strlen(buffer), "Content-Length: %d\n\n", count);
    return strlen(buffer);
}


int fs_read_custom(struct fs_file *file, char *buffer, int count){

    printf("fs_read_custom count: %d\n", count);
 
    uint32_t br = 0;

    if (!((file->flags & FS_FILE_FLAGS_ROUTE) != 0)) {

        //int offset = 0;
        // if (!((file->flags & FS_FILE_FLAGS_HEADER_OUT) != 0)){
        //     offset = printHTMLHeaders(buffer, file->len);
        //     file->flags |= FS_FILE_FLAGS_HEADER_OUT;
        //     count -= offset; // allow for headers
        //     file->len += offset; // make sure we get to eof after we have inserted headers
        // } 

        //printf("reading custom\n");
        if (file->index < file->len){
            //printf("file-len in read: %d\n", file->len);

            FF_FILE *pxFile = file->pextension;
            if (pxFile == NULL)
                panic("custom read error no SD file pointer\n");
            br = ff_fread( buffer, 1, count, pxFile); 
            //printf("Bytes read %d\n", br);
            //printf("%.*s", count, buffer);
            //br += offset;
            file->index += br;               
        }
        
    }

    else{
        //printf("Handling route in fs_ext.c\n");
        NameFunction* fun_ptr = file->pextension;
        if (fun_ptr){
            //printf("calling route handler\n");
            route(fun_ptr, buffer, count);
        }
        //printf("custom read route, buffer %s\n", buffer);
        br = strlen(buffer);
        //printf("bytes read: %i\n", br);
        file->len = br; // only reads one record at the moment *** IAN
        file->index += br;
    }
    printf("Bytes returned %d\n", br);
    return br;
};

void fs_close_custom(struct fs_file *file){
    printf("in close\n");
    if (!((file->flags & FS_FILE_FLAGS_ROUTE) != 0)) {
        ff_fclose(file->pextension);
    }
    else{
        file->pextension = 0;
    }
    printf("closed custom file\n");
}    

