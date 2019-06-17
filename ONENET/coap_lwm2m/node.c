/**
 * Copyright (c) 2017 China Mobile IOT.
 * All rights reserved.
**/

#include "internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "m53xx.h"
int nbiot_node_read( nbiot_node_t        *node,
                     nbiot_uri_t         *uri,
				     uint8_t             flag,
					 uint8_t             *buffer,
                     size_t              buffer_len,
                     bool                updated )
{
	if (flag & NBIOT_SET_RESID )
	{
		nbiot_value_t *data = (nbiot_value_t*)node->data;

		if ( !updated || (data->flag&NBIOT_UPDATED) )
		{
			uint8_t *value = NULL;
			uint8_t array[10],temp[8];
			uint16_t len = 0;

			nbiot_itoa(uri->instid,(char *)temp,8);
			strcat((char *)buffer,(char *)temp);
			len = len + strlen((char *)temp);
			strcat((char *)buffer,",");
			len = len + 1;

			if(data->flag & NBIOT_UPDATED)
				data->flag &= ~NBIOT_UPDATED;

			if ( data->type == NBIOT_BOOLEAN )
			{
				uint8_t tmp=data->value.as_bool ? 1 : 0;

				nbiot_itoa(tmp,(char *)array,1);
				len = 1;
				value = array;
			}

			if ( data->type == NBIOT_INTEGER )
			{
				nbiot_itoa(data->value.as_int,(char *)array,10);
				len = strlen((char *)array);
				value = array;
			}

			if ( data->type == NBIOT_FLOAT )
			{
				sprintf((char *)array,"%f",data->value.as_float);
				len = strlen((char *)array);
				value = array;
			}

			if ( data->type == NBIOT_STRING || data->type == NBIOT_BINARY )
			{
				value = (uint8_t*)data->value.as_str.val;
				len = data->value.as_str.len;
			}
			
			if( data->type == NBIOT_HEX_STRING)
			{
				value = (uint8_t*)data->value.as_buf.val;
				len = data->value.as_buf.len;
			}


			nbiot_itoa(node->id,(char *)temp,8);
			strcat((char *)buffer,(char *)temp);
			len=len+strlen((char *)temp);
			strcat((char *)buffer,",");
			len=len+1;

			nbiot_itoa(data->type,(char *)temp,1);
			strcat((char *)buffer,(char *)temp);
			len=len+1;

			strcat((char *)buffer,",");
			len=len+1;

			strcat((char *)buffer,(char *)value);
			strcat((char *)buffer,";");
			len=len+1;

			if(len>=buffer_len)
				return -1;

			return len;
		}
	}
	else if (flag & NBIOT_SET_OBJID )
	{
		int16_t value_len=0,num=0;

		if ( flag & NBIOT_SET_INSTID )
		{
			flag |= NBIOT_SET_RESID;
		}
		else
		{
			flag |= NBIOT_SET_INSTID;
		}

		for ( node = (nbiot_node_t*)node->data; node != NULL; node = node->next )
		{
			if((flag&NBIOT_SET_RESID)==0)
				uri->instid=node->id;

			if(buffer_len>0)
			{
				value_len=nbiot_node_read( node,
											uri,
											flag,
											buffer,
											buffer_len,
											updated );

				if(value_len == -1)
					break;

				num+=value_len;
				buffer_len=buffer_len-num;
			}
		}

		return num;
	}

	return 0;
}

int nbiot_node_write( nbiot_node_t           *node,
                      const nbiot_uri_t      *uri,
                      uint16_t               ackid,
                      uint8_t                *buffer,
                      size_t                 buffer_len,
                      nbiot_write_callback_t write_func )
{
	if ( uri->flag & NBIOT_SET_RESID )
	{
		nbiot_value_t *data = (nbiot_value_t*)node->data;

		if ( !(data->flag&NBIOT_WRITABLE) )
		{
			m53xx_write_rsp(0, ackid);

			return COAP_METHOD_NOT_ALLOWED_405;
		}

		if ( data->type == NBIOT_BOOLEAN )
		{
			data->value.as_bool = nbiot_atoi((char *)buffer,1);
		}

		if (data->type == NBIOT_INTEGER )
		{
			data->value.as_int = nbiot_atoi((char *)buffer,buffer_len);
		}

		if ( data->type == NBIOT_FLOAT )
		{
			data->value.as_float = atof((char *)buffer);
		}

		if ( data->type == NBIOT_STRING || data->type == NBIOT_BINARY )
		{
			nbiot_free(data->value.as_str.val);
			data->value.as_str.val = nbiot_strdup( (char*)buffer, buffer_len );
			data->value.as_str.len = buffer_len;
		}
		
		if( data->type == NBIOT_HEX_STRING )
		{
			nbiot_free(data->value.as_buf.val);
			data->value.as_buf.val = nbiot_bufdup( (unsigned char*)buffer, buffer_len );
			data->value.as_buf.len = buffer_len;
		}

		if ( write_func )
		{
			write_func( uri->objid,
						uri->instid,
						uri->resid,
						data );
		}

		m53xx_write_rsp(1, ackid);

		return COAP_CHANGED_204;
	}

	return COAP_BAD_REQUEST_400;
}



nbiot_node_t* nbiot_node_find( nbiot_device_t    *dev,
                               const nbiot_uri_t *uri )
{
    nbiot_node_t *node = NULL;

    if ( uri->flag & NBIOT_SET_OBJID )
    {
        node = (nbiot_node_t*)NBIOT_LIST_GET( dev->nodes, uri->objid );

        if ( node && (uri->flag&NBIOT_SET_INSTID) )
        {
            node = (nbiot_node_t*)NBIOT_LIST_GET( node->data, uri->instid );

            if ( node && (uri->flag&NBIOT_SET_RESID) )
            {
                node = (nbiot_node_t*)NBIOT_LIST_GET( node->data, uri->resid );
            }
        }
    }

    return node;
}
