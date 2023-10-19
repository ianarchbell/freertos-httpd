/**
 * 
 * Handles HTTP POST
 * 
*/

#include "lwip/opt.h"

#include "lwip/apps/httpd.h"
#include "lwip/def.h"
#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

#include "router.h"

#if !LWIP_HTTPD_SUPPORT_POST
#error This needs LWIP_HTTPD_SUPPORT_POST
#endif

#define TOKEN_VALUE_BUFSIZE 16
#define URI_BUFSIZE 256
#define MAX_TOKEN_SIZE 64

static void *current_connection;
static NameFunction* current_nameFunction;
static void *valid_connection;
static void *uri;


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

  printf("in getTokenValue for %s\n", token);
  strcpy(tokenBuf, token);
  strcat(tokenBuf, "=");
  u16_t token_index = pbuf_memfind(p, tokenBuf, strlen(tokenBuf), 0);
  if (token_index != 0xFFFF) {
    printf("we have token\n");
    u16_t token_value_index = token_index + strlen(tokenBuf);
    u16_t len_token_value = 0;

    /* may not be the only value */
    u16_t end_token_value_index = pbuf_memfind(p, "&", 1, token_value_index);
    if (end_token_value_index != 0xFFFF) {
      printf("& found\n");
      len_token_value = end_token_value_index - token_value_index;
    } else {
       printf("& not found\n");
       printf("ptot = %d, token_value_index = %d\n",p->tot_len, token_value_index);
       len_token_value = p->tot_len - token_value_index; 
    }
  
    if ((len_token_value > 0) && (len_token_value < TOKEN_VALUE_BUFSIZE))  {
      /* provide contiguous storage if p is a chained pbuf */
      char buf_token_value[TOKEN_VALUE_BUFSIZE];
      char *token_value = (char *)pbuf_get_contiguous(p, buf_token_value, len_token_value, len_token_value, token_value_index);
      token_value[len_token_value] = 0;
      printf("Token: %s, value: %s\n", token, token_value);
      return token_value;
    }
  }
  return NULL;
}



err_t
httpd_post_receive_data(void *connection, struct pbuf *p)
{  
  char namedRoute[URI_BUFSIZE] = "";
  uri = malloc(URI_BUFSIZE);
  char* token_ptr, token_ptr1, token_value;

  printf("in httpd_post_receive\n");
  printf("current_nameFunction->routeName : %s\n", current_nameFunction->routeName);
  if (current_connection == connection) {

    // construct router route from parms
    strcpy(namedRoute, current_nameFunction->routeName);

    // first token - either the complete route or prefix to ':'
    char* token_ptr = strtok(namedRoute, ":");
    if(!token_ptr){
      printf("no parms on POST route\n");
      strcpy(uri, current_nameFunction->routeName);
      return ERR_OK;
    }
    else{
      // get the token after the ':' prior to either '/' or end of route
      strcpy(uri, token_ptr);
      printf("token_ptr: %s\n", token_ptr);
    }
      char* token_ptr =  strtok(NULL, "/");
      printf("token_ptr: %s\n", token_ptr);
      while(!token_ptr){
        if(!token_ptr){
          token_ptr = token_ptr1;
          char* tokenValue = getTokenValue(p, token_ptr);
          printf("tokenValue: %s\n", tokenValue);
          strcat(uri, tokenValue);  
        }
        else{
          printf("uri_now: %s\n", uri);
          char* tokenValue = getTokenValue(p, token_ptr);
          strcat(uri,tokenValue);
        } 
      }
    }
    printf("uri: %s, token_ptr: %s, token_ptr1: %s, tokenValue: %s\n", uri, token_ptr, token_ptr1, token_value);


    printf("In current connection: %.*s\n",p->tot_len, p->payload); 
    char* value = getTokenValue(p, "value");
    if (value)
      printf("Token name: %s, value %s\n", "value", value);
    value = getTokenValue(p, "gpio");
    if (value)
      printf("Token name: %s, value %s\n", "gpio", value);  

          // if (!strcmp(key, "lwip") && !strcmp(value, "post")) {
          //   /* key and valueword are correct, create a "session" */
          //   valid_connection = connection;
          //   printf("key: %s, value: %s\n", key, value);
          // }
        //}
      //}
    /* not returning ERR_OK aborts the connection, so return ERR_OK unless the
       conenction is unknown */
    }
    return ERR_OK;
} 

void
httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{
  /* default page is "login failed" */
  printf("in httpd_post_finished\n");
  snprintf(response_uri, response_uri_len, "/failure");
  if (current_connection == connection) {
    if (uri) {
      printf("uri: %s\n", uri);
      NameFunction* routeFunction = isRoute(uri, HTTP_POST);
      if(routeFunction){
        route(current_nameFunction, uri, strlen(uri));
        snprintf(response_uri, response_uri_len, "/success");

        free(uri);
      }
    }
  }
  current_connection = NULL;
  valid_connection = NULL;
}

