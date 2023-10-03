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

// only mount once
static bool init = false;

int
fs_open_custom(struct fs_file *file, const char *name){

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
    return 1;
}

int fs_read_custom(struct fs_file *file, char *buffer, int count){
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
    else{
        return FS_READ_EOF;
    }
};

void
fs_close_custom(struct fs_file *file){
    if (file->pextension){
        FIL* sdFile = file->pextension;
        f_close(file->pextension);
        free(sdFile);
    }
    printf("closed custom file\n");
}    

