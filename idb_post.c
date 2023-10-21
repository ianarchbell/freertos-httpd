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

#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#endif

#define TOKEN_VALUE_BUFSIZE 16
#define URI_BUFSIZE 256
#define MAX_TOKEN_SIZE 64

static void *current_connection;
static NameFunction* current_nameFunction;
static void *valid_connection;
static char* POST_uri;

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
  printf("POST received with uri %s\n", uri);
  current_nameFunction = isRoute(uri, HTTP_POST);
  if (current_nameFunction) {
    printf("after memcmp\n");
    if (current_connection != connection) {
      current_connection = connection;
      valid_connection = NULL;
      /* default page is "login failed" */
      snprintf(response_uri, response_uri_len, "/failure");
      /* e.g. for large uploads to slow flash over a fast connection, you should
         manually update the rx window. That way, a sender can only send a full
         tcp window at a time. If this is required, set 'post_aut_wnd' to 0.
         We do not need to throttle upload speed here, so: */
      *post_auto_wnd = 1;
      return ERR_OK;
    }
  }
  return ERR_VAL;
}

char*
getTokenValue(struct pbuf *p, char* token)
{
  char tokenBuf[MAX_TOKEN_SIZE];

  strcpy(tokenBuf, token);
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
      return token_value;
    }
  }
  return NULL;
}

err_t
httpd_post_receive_data(void *connection, struct pbuf *p)
{  
  char namedRoute[URI_BUFSIZE] = "";
  char* prefix_ptr;
  char* token_ptr;
  char* token_ptr1; 
  char* token_value;

  if (current_connection == connection) {

      POST_uri = pvPortMalloc(URI_BUFSIZE);

    // construct router route from parms
    strcpy(namedRoute, current_nameFunction->routeName);

    // first token - either the complete route or prefix to ':'
    prefix_ptr = strtok(namedRoute, ":");
    if(!prefix_ptr){
      strcpy(POST_uri, current_nameFunction->routeName);
      return ERR_OK;
    }
    else{
      // Copy the prefix to the uri
      strcpy(POST_uri, prefix_ptr);
      POST_uri[strlen(prefix_ptr)-1] = 0x00;
       // take 1 off the length for the '/' - we add one for every variable found
      token_ptr = prefix_ptr;
    }
    while(token_ptr){
      token_ptr1 =  strtok(NULL, "/");
      printf("token_ptr1: %s\n", token_ptr1); 
      if(token_ptr1)
        token_ptr = token_ptr1;
      printf("token_ptr: %s\n", token_ptr);  
      token_value = getTokenValue(p, token_ptr);
      printf("token_value: %s\n", token_value);  
      strcat(POST_uri, "/");
      strcat(POST_uri, token_value);
      token_ptr = strtok(NULL, ":");  
    }
  }
  if(p)
    pbuf_free(p);

  return ERR_OK;
} 

void
httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{
  /* default page is "JSON failure" */
  snprintf(response_uri, response_uri_len, "/failure");
  if (current_connection == connection) {
    if (POST_uri) {
      printf("POSTing route uri: %s\n", POST_uri);
      NameFunction* routeFunction = isRoute(POST_uri, HTTP_POST);
      if(routeFunction){
        route(current_nameFunction, POST_uri, strlen(POST_uri));
        snprintf(response_uri, response_uri_len, "/success");
      }
      vPortFree(POST_uri);
      POST_uri = NULL;
    }
  }
  current_connection = NULL;
  valid_connection = NULL;
}

