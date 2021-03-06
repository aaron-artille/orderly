/*
 * Copyright 2010, Greg Olszewski and Lloyd Hilaiel.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the name of Greg Olszewski and Lloyd Hilaiel nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* a lightweight C representation of json data */

#include "orderly_json.h"

#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

#include <stdlib.h>
#include <string.h>

void
orderly_free_json(const orderly_alloc_funcs * alloc, orderly_json ** node)
{
    if (node && *node) {
        if ((*node)->k) OR_FREE(alloc, (void *) (*node)->k);

        if ((*node)->t == orderly_json_string) {
            OR_FREE(alloc, (void *) (*node)->v.s);
        } else if ((*node)->t == orderly_json_array ||
                   (*node)->t == orderly_json_object)
        {
            orderly_free_json(alloc, &((*node)->v.children.first));
        }
        if ((*node)->next) orderly_free_json(alloc, &((*node)->next));
        OR_FREE(alloc, (void *) (*node));        
    }
}

orderly_json *
orderly_clone_json(const orderly_alloc_funcs * alloc, orderly_json * j)
{
    orderly_json * copy = NULL;
    if (!j) return copy;
    copy = orderly_alloc_json(alloc, j->t);
    if (j->k) BUF_STRDUP(copy->k, alloc, j->k, strlen(j->k));
    
    switch(j->t) {
        case orderly_json_none:
        case orderly_json_null:
            break;
        case orderly_json_string:
            if (j->v.s) BUF_STRDUP(copy->v.s, alloc, j->v.s, strlen(j->v.s));
            break;
        case orderly_json_boolean:
            copy->v.b = j->v.b;
            break;
        case orderly_json_integer:
            copy->v.i = j->v.i;
            break;
        case orderly_json_number:
            copy->v.n = j->v.n;
            break;
        case orderly_json_object:
        case orderly_json_array: {
            /* here's the deep copy part */
            orderly_json * p;

            for (p = j->v.children.first; p != NULL; p = p->next) {
                orderly_json * nk = orderly_clone_json(alloc, p);
                if (copy->v.children.last) {
                    copy->v.children.last->next = nk;
                    copy->v.children.last = nk;
                } else {
                    copy->v.children.first = copy->v.children.last = nk;
                }
            }
            break;
        }
    }
    return copy;
}

orderly_json *
orderly_alloc_json(const orderly_alloc_funcs * alloc, orderly_json_type t)
{
    orderly_json * n = (orderly_json *) OR_MALLOC(alloc, sizeof(orderly_json));
    memset((void *) n, 0, sizeof(orderly_json));
    n->t = t;
    return n;
}

// push an element to the correct place
#define PUSH_NODE(pc, __n)                                                      \
  if (orderly_ps_length((pc)->nodeStack) == 0) {                                \
      orderly_ps_push((pc)->alloc, (pc)->nodeStack, __n);                       \
  } else {                                                                      \
      orderly_json * top = (orderly_json *) orderly_ps_current((pc)->nodeStack);\
      if (top->t == orderly_json_array) {                                       \
          if (top->v.children.last) top->v.children.last->next = (__n);         \
          top->v.children.last = (__n);                                         \
          if (!top->v.children.first) top->v.children.first = (__n);            \
      } else if (top->t == orderly_json_object) {                               \
          if (top->v.children.last) top->v.children.last->next = (__n);         \
          top->v.children.last = (__n);                                         \
          if (!top->v.children.first) top->v.children.first = (__n);            \
          (__n)->k = orderly_ps_current((pc)->keyStack);                        \
          orderly_ps_pop((pc)->keyStack);                                       \
      }                                                                         \
  }

int o_json_parse_start_array(void * ctx)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_array);
    orderly_ps_push(pc->alloc, pc->nodeStack, n);
    return 1;
}

int o_json_parse_end_array(void * ctx)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_ps_current(pc->nodeStack);
    orderly_ps_pop(pc->nodeStack);
    PUSH_NODE(pc, n);
    return 1;
}

int o_json_parse_start_map(void * ctx)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_object);
    orderly_ps_push(pc->alloc, pc->nodeStack, n);
    return 1;
}

int o_json_parse_map_key(void * ctx, const unsigned char * v,
                         unsigned int l)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    char * k = NULL;
    BUF_STRDUP(k, pc->alloc, v, l);
    orderly_ps_push(pc->alloc, pc->keyStack, k);
    return 1;
}

int o_json_parse_end_map(void * ctx)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_ps_current(pc->nodeStack);
    orderly_ps_pop(pc->nodeStack);
    PUSH_NODE(pc, n);
    return 1;
}

int o_json_parse_string(void * ctx, const unsigned char * v, unsigned int l)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_string);
    BUF_STRDUP(n->v.s, pc->alloc, v, l);
    PUSH_NODE(pc, n);
    return 1;
}

int o_json_parse_integer(void * ctx, long l)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_integer);
    n->v.i = l;
    PUSH_NODE(pc, n);
    return 1;
}

int o_json_parse_double(void * ctx, double d)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_number);
    n->v.n = d;
    PUSH_NODE(pc, n);
    return 1;
}

int o_json_parse_null(void * ctx)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_null);
    PUSH_NODE(pc, n);
    return 1;
}


int o_json_parse_boolean(void * ctx, int val)
{
    o_json_parse_context * pc = (o_json_parse_context *) ctx;
    orderly_json * n = orderly_alloc_json(pc->alloc, orderly_json_boolean);
    n->v.b = val;
    PUSH_NODE(pc, n);
    return 1;
}


#include <stdio.h>


orderly_json *
orderly_read_json(orderly_alloc_funcs * alloc,
                  const char * jsonText,
                  unsigned int * len)
{
    static yajl_callbacks callbacks = {
        o_json_parse_null,
        o_json_parse_boolean,
        o_json_parse_integer,
        o_json_parse_double,
        NULL,
        o_json_parse_string,
        o_json_parse_start_map,
        o_json_parse_map_key,
        o_json_parse_end_map,
        o_json_parse_start_array,
        o_json_parse_end_array
    };

    yajl_handle hand;
    yajl_status stat;
    /* allow comments! */
    yajl_parser_config cfg = { 1, 1 };
    o_json_parse_context pc;
    orderly_json * j = NULL;

    memset((void *) &pc, 0, sizeof(pc));
    pc.alloc = alloc;

    /* allocate a parser */
    hand = yajl_alloc(&callbacks, &cfg,
                      (const yajl_alloc_funcs *) alloc,
                      (void *) &pc);

    /* read file data, pass to parser */
    stat = yajl_parse(hand, (const unsigned char *) jsonText, *len);
    *len = yajl_get_bytes_consumed(hand);
    if (stat == yajl_status_insufficient_data) {
        stat = yajl_parse_complete(hand);
    }

    if (stat != yajl_status_ok)
    {
        /* unsigned char * str = yajl_get_error(hand, 1, (const unsigned char *) jsonText, *len); */
        /* fprintf(stderr, (const char *) str); */
        /* yajl_free_error(hand, str); */
    }
    else if (!orderly_ps_length(pc.nodeStack))
    {
        /* XXX: ERROR! */
    }
    else 
    {
        /* we're ok! */
        j = orderly_ps_current(pc.nodeStack);
    }

    yajl_free(hand);
    orderly_ps_free(alloc, pc.nodeStack);
    orderly_ps_free(alloc, pc.keyStack);

    return j;
}


int orderly_write_json2(yajl_gen g, const orderly_json * j)
{
    yajl_gen_status s;
    int rv = 1;

    if (j) {
        if (j->k) yajl_gen_string(g, (const unsigned char *) j->k, strlen(j->k));

        switch (j->t) {
            case orderly_json_none:
                return 0;
            case orderly_json_null:
                s = yajl_gen_null(g);
                break;
            case orderly_json_string:
                s = yajl_gen_string(g, (const unsigned char *) j->v.s, strlen(j->v.s));
                break;
            case orderly_json_boolean:
                s = yajl_gen_bool(g, j->v.b);
                break;
            case orderly_json_integer:
                s = yajl_gen_integer(g, j->v.i);
                break;
            case orderly_json_number:
                s = yajl_gen_double(g, j->v.n);
                break;
            case orderly_json_object:
                s = yajl_gen_map_open(g);
                rv = orderly_write_json2(g, j->v.children.first);
                s = yajl_gen_map_close(g);
                break;
            case orderly_json_array:
                s = yajl_gen_array_open(g);
                rv = orderly_write_json2(g, j->v.children.first);
                s = yajl_gen_array_close(g);
                break;
        }

        if (rv && j->next) rv = orderly_write_json2(g, j->next);
    }

    return rv;
}

static void
bufAppendCallback(void * ctx, const char * str, unsigned int len)
{
    orderly_buf_append((orderly_buf) ctx, str, len);
}


void
orderly_write_json(const orderly_alloc_funcs * alloc,
                   const orderly_json * json,
                   orderly_buf b,
                   int pretty)
{
    yajl_gen_config cfg = { pretty, NULL };
    yajl_gen g = yajl_gen_alloc2(bufAppendCallback, &cfg,
                                 (const yajl_alloc_funcs *) alloc,
                                 (void *) b);
    int rv = orderly_write_json2(g, json);
    yajl_gen_free(g);
}

/*
 *
 */
int orderly_synthesize_callbacks (const yajl_callbacks *cb, 
                                         void *cbctx,
                                         orderly_json *value) {
  int ret = 1; 
    
  switch (value->t) {

    /*
     * NULL
     */
  case orderly_json_null:
    if (cb->yajl_null) {
      ret = cb->yajl_null(cbctx);
    }
    break;

    /*
     * boolean
     */
  case orderly_json_boolean:
    if (cb->yajl_boolean) {
      ret = cb->yajl_boolean(cbctx,value->v.b);
    }
    break;
    
    /*
     * string
     */
  case orderly_json_string:

    if (cb->yajl_string) {
      ret = cb->yajl_string(cbctx,
                                   (unsigned char *)value->v.s,
                                   strlen(value->v.s));
    }
    break;

    /*
     * object / map
     */
  case orderly_json_object:
    /* call the start callback */
    if (cb->yajl_start_map) {
      ret = cb->yajl_start_map(cbctx);
      if (ret == 0) {
        return 0;
      }
    }
    /* recurse - XXX: stack depth limit */
    {
      orderly_json *kiditr;
      for (kiditr = value->v.children.first; kiditr; kiditr = kiditr->next) {
        ret = orderly_synthesize_callbacks(cb,cbctx, kiditr);
        if (ret == 0) {
          return 0;
        }
      }
    }
    /* call end callback */
    if (cb->yajl_end_map) {
      ret = cb->yajl_end_map(cbctx);
      if (ret == 0) {
        return 0;
      }
    }
    break;


    /*
     * array
     */
  case orderly_json_array:
    /* call start callback */
    if (cb->yajl_start_array) {
      ret = cb->yajl_start_array(cbctx);
      if (ret == 0) break;
    }
    /* recurse */
    {
      orderly_json *kiditr;
      for (kiditr = value->v.children.first; kiditr; kiditr = kiditr->next) {
        ret = orderly_synthesize_callbacks(cb,cbctx, kiditr);
        if (ret == 0) break;
      }
    }
    /* call end callback */
    if (cb->yajl_end_array) {
      ret = cb->yajl_end_array(cbctx);
    }

    break;

    /* 
     * integer
     */
  case orderly_json_integer:
    if (cb->yajl_number) {
      assert("unimplemented" == 0);
    } else if (cb->yajl_integer) {
      ret = cb->yajl_integer(cbctx, value->v.i);
    }

    break;

    /* 
     * "number" -- float
     */
  case orderly_json_number:
    if (cb->yajl_number) {
      assert("unimplemented" == 0);
    } else if (cb->yajl_double) {
      ret = cb->yajl_double(cbctx, value->v.n);
    }

    break;



    /* XXX: orderly_json_none appears to be for internal accounting
       only, should never find a default of this type */
  case orderly_json_none:
  default:
    assert("unreachable"==0);
  }


  return ret;
}

