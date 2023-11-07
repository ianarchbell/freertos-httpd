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

//#include "hw_config.h"

#include <stdbool.h>
#include <string.h>

#include "lwip/apps/httpd.h"

#include "idb_router.h"
#include "idb_config.h"

// only mount once
static bool init = false;

#define FS_FILE_FLAGS_ROUTE             0x80
#define FS_FILE_FLAGS_HEADERS_OUT       0x40

#define MAX_ROUTE_LEN 1460

#define DEVICENAME "sd0"
#define MOUNTPOINT "/sd0"

/**
 * 
 * fs_open_custom is called by fs_open if LWIP_HTTPD_CUSTOM_FILES 1
 * LWIP_HTTPD_FILE_EXTENSION 1 also needs to be set as we use pextension 
 * to store either an SDCARD file pointer or a route pointer 
 * if it is a purely a route without a file reference
 * 
*/
int fs_open_custom(struct fs_file *file, const char *name){
    // check if it is a route first, if not it must be a custom file
    NameFunction* fun_ptr = isRoute(name, HTTP_GET);

    if(!fun_ptr)
    {
        if(!init){
            // mount sd card - this is global

            FF_Disk_t *pxDisk = NULL;

            if (!mount(&pxDisk, DEVICENAME, MOUNTPOINT)){
                panic("failed to mount card\n");
                return 0;
            };
            TRACE_PRINTF("Mounted %s: successfully\n", DEVICENAME);
            init = true;   
        }
        char fullpath[256];
        sprintf(fullpath, "%s%s",MOUNTPOINT, name);
        TRACE_PRINTF("Opening custom: %s\n", fullpath);
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
            TRACE_PRINTF("Failed to open file %s\n", name);
            return 0;
        }
    }
    else{ // is a route
        //TRACE_PRINTF("%s is a route\n", name);
        // flag as custom and a route
        file->data = NULL;
        file->index = 0;
        file->flags |= FS_FILE_FLAGS_HEADER_INCLUDED; // we are now providing our own headers
        file->flags |= FS_FILE_FLAGS_CUSTOM;
        file->flags |= FS_FILE_FLAGS_ROUTE; // not used in fs.c
        file->pextension = fun_ptr; // store the handler for read
        if(fun_ptr->uri != NULL){
            panic("fun_ptr should be null: %s\n", fun_ptr->uri);
        }
        fun_ptr->uri = pvPortMalloc(strlen(name)+1);
        if(fun_ptr->uri)
            strcpy(fun_ptr->uri, name);
        else
            panic("Failed to allocate uri\n");    
        file->len = MAX_ROUTE_LEN; // max len
        //TRACE_PRINTF("open: %s is a route\n", name);
    }
    return 1;
}

/**
 * 
 * Helper to print headers
 * 
*/
int printHTTPHeaders(char* buffer, int count){
    int len = snprintf(buffer, count, 
"HTTP/1.1 200 OK\n \
Content-Type: text/html; charset=utf-8\n \
Content-Length: %d\n\n", count);    
    if (len > count)
        panic("HTTP header buffer not big enough");
    // strcat(buffer, "Content-Type: text/html; charset=utf-8\n");
    //sprintf(buffer+strlen(buffer), "Content-Length: %d\n\n", count);
    //return strlen(buffer);
    return len;
}

/**
 * 
 * LWIP_HTTPD_DYNAMIC_FILE_READ 1 needs to be set for fs_read to call fs_custom_read
 * Two main cases through here - either it is a real file, in which case it is read
 * from the SD Card. If not, the route is handled and the response is returned
 * Note that at present the response from a route must fit in one buffer
 * 
*/
int fs_read_custom(struct fs_file *file, char *buffer, int count){

    
    uint32_t br = 0;

    if (!((file->flags & FS_FILE_FLAGS_ROUTE) != 0)) {
        TRACE_PRINTF("fs_read_custom count: %d\n", count);
        TRACE_PRINTF("file->len = %d, file->index = %d\n", file->len, file->index);
        int offset = 0;
        if (!((file->flags & FS_FILE_FLAGS_HEADERS_OUT) != 0)) {
            TRACE_PRINTF("Printing http headers\n");
            offset = printHTTPHeaders(buffer,file->len);
            file->flags |= FS_FILE_FLAGS_HEADERS_OUT;
            file->len += offset; // allow for the headers
        }

        TRACE_PRINTF("Reading custom\n");
        
        if (file->index < file->len){
            count -= offset;
            FF_FILE *pxFile = file->pextension;
            if (pxFile == NULL)
                panic("custom read error no SD file pointer\n");
            br = ff_fread( buffer + offset, 1, count, pxFile);
            //TRACE_PRINTF("buffer: '%.*s'\n", count, buffer);
            br += offset;
            file->index += br;
        }
        else{
             TRACE_PRINTF("File index reached file end, file->len = %d, file->index = %d\n", file->len, file->index);
        }
    }
    else{
        NameFunction* fun_ptr = file->pextension;
        if (fun_ptr){
            TRACE_PRINTF("fs_read_custom : executing route %s, uri: %s\n", fun_ptr->routeName, fun_ptr->uri);
            route(fun_ptr, buffer, count, fun_ptr->uri);
            //TRACE_PRINTF("fs_read_custom : route response:\n%s\n", buffer);
            br = strlen(buffer);
            //TRACE_PRINTF("fs_read_custom : route response length: %u\n", br);
            file->len = br; // only reads one record at the moment *** IAN
            file->index += br;
        }
        else{
            panic("No NameFunction pointer in pextension\n");
        }
    }
    return br;
};

/**
 * 
 * Closes file or in the case of a route simply frees the allocated uri
 * TODO need to look at how this allocation is done because it may be possible
 * for a routing table to be modified before this is freed
 * 
*/
void fs_close_custom(struct fs_file *file){
    //TRACE_PRINTF("Closed custom file\n");
    if (!((file->flags & FS_FILE_FLAGS_ROUTE) != 0)) {
        ff_fclose(file->pextension);
    }
    else{
        NameFunction* fun_ptr = file->pextension;
        if (fun_ptr->uri){
            vPortFree(fun_ptr->uri);
            fun_ptr->uri = NULL;
        }
        file->pextension = 0;
    }
    //TRACE_PRINTF("closed custom file\n");
}    

