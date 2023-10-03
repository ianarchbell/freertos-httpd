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
#include "ff.h"
#include "f_util.h"

#include "fs.h"
#include "hw_config.h"

#include <stdbool.h>
#include <string.h>

// only mount once
static bool init = false;

#define FS_FILE_FLAGS_ROUTE             0x80

int fs_open_custom(struct fs_file *file, const char *name){

    
    void (*fun_ptr)(char*);

    fun_ptr = (void (*)(char *))isRoute(name);

    if(!fun_ptr)
    {

        if(!init){
            sd_card_t *pSD = sd_get_by_num(0);
            FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
            if (FR_OK != fr) 
                panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
            printf("mounted 0: successfully\n");
            init = true;   
        }

        FIL* sdFile = (FIL*) malloc(sizeof(FIL));
        if (sdFile == NULL){
            panic("cannot allocate sdFile\n");
        }
        printf("opening custom: %s\n", name);
        FRESULT fr = f_open(sdFile, name, FA_READ);
        if (FR_OK != fr && FR_EXIST != fr){
            free (sdFile);
            printf("f_open(%s) error: %s (%d)\n", name, FRESULT_str(fr), fr);
            return 0;
        }
    

        file->data = NULL;
        file->len = f_size(sdFile);
        file->index = 0;
        file->flags |= FS_FILE_FLAGS_CUSTOM;
        file->pextension = sdFile;
    }
    else{ // is a route
        // flag as custom and a route
        file->data = NULL;
        file->index = 0;
        file->flags |= FS_FILE_FLAGS_CUSTOM;
        file->flags |= FS_FILE_FLAGS_ROUTE; // not used in fs.c
        file->pextension = fun_ptr; // store the handler for read
        file->len = 45; // hard coded for test
        printf("open: %s is a route\n", name);
    }
    return 1;
}

int fs_read_custom(struct fs_file *file, char *buffer, int count){
    
    if (!((file->flags & FS_FILE_FLAGS_ROUTE) != 0)) {
        printf("reading custom\n");
        if (file->index < file->len){
            FIL* sdFile = file->pextension;
            if (sdFile == NULL)
                panic("custom read error no SD file pointer\n");
            UINT br;
            f_read(sdFile, buffer, count, &br);
            file->index += br;
            return br;
        }
    }
    else{
        void (*fun_ptr)(char*);
        fun_ptr = file->pextension;
        if (fun_ptr){
            printf("calling route handler\n");
            route(fun_ptr, buffer, count);
        }
        printf("custom read route, buffer %s\n", buffer);
        UINT br = strlen(buffer);
        printf("bytes read: %i\n", br);
        file->index += br;
        return br;
    }
};

void fs_close_custom(struct fs_file *file){
    printf("in close\n");
    if (!((file->flags & FS_FILE_FLAGS_ROUTE) != 0)) {
        f_close(file->pextension);
    }
    else{
        file->pextension = 0;
    }
    printf("closed custom file\n");
}    

