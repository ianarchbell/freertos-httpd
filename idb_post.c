/**
 * 
 * Handles HTTP POST
 * 
*/
#include <FreeRTOS.h>
#include "lwip/opt.h"

#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

#include "idb_router.h"
#include "idb_config.h"

#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#endif

#define TOKEN_VALUE_BUFSIZE 128
#define URI_BUFSIZE 128
#define MAX_TOKEN_SIZE 128

//static void *current_connection;
static NameFunction* current_nameFunction;
//static void *valid_connection;
static char* POST_uri = NULL;

/**
 * 
 * Called by HTTPD on receipt of POST request
 * Check this is the current connection and point to default 
 * /failure route to return "success: false" JSON if failure
 * 
*/

err_t
httpd_post_begin(void *connection, const char *uri, const char *http_request,
                 u16_t http_request_len, int content_len, char *response_uri,
                 u16_t response_uri_len, u8_t *post_auto_wnd)
{
  LWIP_UNUSED_ARG(connection);
  LWIP_UNUSED_ARG(http_request);
  LWIP_UNUSED_ARG(http_request_len);
  LWIP_UNUSED_ARG(content_len);
  LWIP_UNUSED_ARG(post_auto_wnd);

  TRACE_PRINTF("POST received with uri %s\n", uri);
  current_nameFunction = isRoute(uri, HTTP_POST);
  if(current_nameFunction){
    POST_uri = pvPortMalloc(URI_BUFSIZE); // allocate a URI buffer (freed on finish)
    if (!POST_uri)
      panic("Unable to allocate POST_uri\n");
  }
  else {   
      /* default page is "success: false" json */
      snprintf(response_uri, response_uri_len, "/failure");
      /* e.g. for large uploads to slow flash over a fast connection, you should
         manually update the rx window. That way, a sender can only send a full
         tcp window at a time. If this is required, set 'post_aut_wnd' to 0.
         We do not need to throttle upload speed here, so: */
      *post_auto_wnd = 1;
      return ERR_VAL;
  }
  return ERR_OK;
}

int url_decode(char* out, const char* in)
{
    static const char tbl[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
         0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
    };
    char c, v1, v2, *beg=out;
    if(in != NULL) {
        while((c=*in++) != '\0') {
            if(c == '%') {
                if((v1=tbl[(unsigned char)*in++])<0 || 
                   (v2=tbl[(unsigned char)*in++])<0) {
                    *beg = '\0';
                    return -1;
                }
                c = (v1<<4)|v2;
            }
            *out++ = c;
        }
    }
    *out = '\0';
    return 0;
}

int
getTokenValue(char* decoded_value, struct pbuf *p, const char* token)
{
  char tokenBuf[MAX_TOKEN_SIZE];

  strncpy(tokenBuf, token, MAX_TOKEN_SIZE);
  tokenBuf[MAX_TOKEN_SIZE-1] = '\0';
  strcat(tokenBuf, "=");
  u16_t token_index = pbuf_memfind(p, tokenBuf, strlen(tokenBuf), 0);
  if (token_index != 0xFFFF) {
    u16_t token_value_index = token_index + strlen(tokenBuf);
    u16_t len_token_value = 0;

    /* may not be the only value */
    u16_t end_token_value_index = pbuf_memfind(p, "&", 1, token_value_index);
    if (end_token_value_index != 0xFFFF) {
      len_token_value = end_token_value_index - token_value_index;
    } else {
       len_token_value = p->tot_len - token_value_index; 
    }
  
    if ((len_token_value > 0) && (len_token_value < TOKEN_VALUE_BUFSIZE))  {
      /* provide contiguous storage if p is a chained pbuf */
      char buf_token_value[TOKEN_VALUE_BUFSIZE];
      char *token_value = (char *)pbuf_get_contiguous(p, buf_token_value, len_token_value, len_token_value, token_value_index);
      token_value[len_token_value] = 0;
      url_decode(decoded_value, token_value);
      TRACE_PRINTF("Decoded POST token: %s, original: %s\n", decoded_value, token_value);
      return 1;
    }
  }
  return 0;
}

err_t
httpd_post_receive_data(void *connection, struct pbuf *p)
{  
  char namedRoute[URI_BUFSIZE] = "";
  char* prefix_ptr;
  char* token_ptr;
  char* token_ptr1; 

  //TRACE_PRINTF("Handling POST receieve\n");
  if (POST_uri != NULL) {

    // construct router route from parms
    strncpy(namedRoute, current_nameFunction->routeName, URI_BUFSIZE);
    namedRoute[URI_BUFSIZE-1] = '\0';

    // first token - either the complete route or prefix to ':'
    prefix_ptr = strtok(namedRoute, ":");
    if(!prefix_ptr){
      strncpy(POST_uri, current_nameFunction->routeName, URI_BUFSIZE);
      POST_uri[URI_BUFSIZE-1] = '\0';
      return ERR_OK;
    }
    else{
      // Copy the prefix to the uri
      strncpy(POST_uri, prefix_ptr, URI_BUFSIZE);
      POST_uri[URI_BUFSIZE-1] = '\0';
      POST_uri[strlen(prefix_ptr)-1] = 0x00;
       // take 1 off the length for the '/' - we add one for every variable found
      token_ptr = prefix_ptr;
    }
    while(token_ptr){      
      token_ptr1 =  strtok(NULL, "/");
      //TRACE_PRINTF("token_ptr1: %s\n", token_ptr1); 
      if(token_ptr1)
        token_ptr = token_ptr1;
      //TRACE_PRINTF("token_ptr: %s\n", token_ptr);  
      char* token_value = pvPortMalloc(URI_BUFSIZE);
      if(getTokenValue(token_value, p, token_ptr)){
        //TRACE_PRINTF("token_value: %s\n", token_value);  
        strcat(POST_uri, "/");
        if(strlen(token_value) + strlen(POST_uri) > sizeof POST_uri)
          strncat(POST_uri, token_value, sizeof POST_uri - strlen(POST_uri));
        else
          panic("Token too large for the buffer in POST\n");
        token_ptr = strtok(NULL, ":");   
      }
      vPortFree(token_value);
     }
  }
  else{
    TRACE_PRINTF("POST: no uri\n");
  }
  if(p)
    pbuf_free(p);

  return ERR_OK;
} 

/**
 * 
 * Free up the uri and return success
 * 
*/
void
httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{

  /* default page is "JSON failure" */
 snprintf(response_uri, response_uri_len, "/failure");
  if (POST_uri) {
    TRACE_PRINTF("POSTing route uri: %s\n", POST_uri);
    NameFunction* routeFunction = isRoute(POST_uri, HTTP_POST);
    if(routeFunction){
      char buffer[128];
      route(routeFunction, buffer, sizeof buffer, POST_uri);
      snprintf(response_uri, response_uri_len,"/success");
    }
    vPortFree(POST_uri);
    POST_uri = NULL;
  }
}



