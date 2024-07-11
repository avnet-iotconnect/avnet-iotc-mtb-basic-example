# OTA on AWS

OTA support for AWS is still work in progress. The steps below can work with ioTConnect AWS OTA, 
but the results are not consistent and may vary depending on your network. 
We are investigating these consistency issues.

* Due to the AWS S3 signed URL size that we get from IoTConnect, we need to increase both
CY_OTA_HTTP_FILENAME_SIZE and CY_OTA_MQTT_FILENAME_BUFF_SIZE 
(MQTT as well due to a bug where the wrong value is used for an HTTP related buffer size)
to 800 at *mtb_shared/ota-update/\<version>/include/cy_ota_api.h*  
* S3 will allow only up to 100 range requests until it forcefully disconnects an existing connection
which means that the CY_OTA_CHUNK_SIZE needs to be qat least 10k to download the 1MB firmware file.
However, the ota-update library is not stable with larger chunk sizes, so the quick and dirty way 
to address this is a crude patch
applying a crude patch the cy_ota_http.c source file to re-try the connection during the download
after the 100th range request.

To add the retry logic, modify the following source at mtb_shared/ota-update/\<version>/source/cy_ota_http.c. 
  This modification seems to work best with the original CY_OTA_CHUNK_SIZE of 4096:

* Modify cy_ota_http_disconnect_callback near line 489 to indicate that the connection dropped:

```C
static void cy_ota_http_disconnect_callback(cy_http_client_t handle, cy_http_client_disconn_type_t type, void *user_data)
{
    /* for compiler warnings */
    (void)handle;
    (void)type;
    (void)user_data;

    cy_ota_context_t *ctx = (cy_ota_context_t *) user_data;
    ctx->http.connection_established = false;

    /* HTTP is now Synchronous.
...
```

* Inside the while loop near line 1023, append the reconnect logic:

```C
    while( ( (ctx->ota_storage_context.total_bytes_written == 0) ||
              (ctx->ota_storage_context.total_bytes_written < ctx->ota_storage_context.total_image_size) ) &&
            (range_end > range_start) )
    {
    	// AWS S3 will disconnect us after the 100th chunk. Try to reconnect here...
    	if (ctx->http.connection_established != true && ctx->http.connection != NULL) 
    	{
    	    result = cy_http_client_connect(ctx->http.connection, CY_OTA_HTTP_TIMEOUT_SEND, CY_OTA_HTTP_TIMEOUT_RECEIVE);
    	    if(result == CY_RSLT_SUCCESS) 
    	    {
                ctx->http.connection_established = true;
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "Got disconnected... Reconnect successful.");
    	    }
    	    else 
    	    {
                cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_WARNING, "Got disconnected... Reconnect failed.");
    	    }
    	    // else we will simply fail below
    	}
        cy_ota_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "while(ctx->ota_storage_context.total_bytes_written (%ld) < (%ld) ctx->total_image_size)\n", ctx->ota_storage_context.total_bytes_written, ctx->ota_storage_context.total_image_size);
...
```


