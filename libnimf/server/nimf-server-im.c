/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * nimf-server-im.c
 * This file is part of Nimf.
 *
 * Copyright (C) 2015-2018 Hodong Kim <cogniti@gmail.com>
 *
 * # 법적 고지
 *
 * Nimf 소프트웨어는 대한민국 저작권법과 국제 조약의 보호를 받습니다.
 * Nimf 개발자는 대한민국 법률의 보호를 받습니다.
 * 커뮤니티의 위력을 이용하여 개발자의 시간과 노동력을 약탈하려는 행위를 금하시기 바랍니다.
 *
 * * 커뮤니티 게시판에 개발자를 욕(비난)하거나
 * * 욕보이는(음해하는) 글을 작성하거나
 * * 허위 사실을 공표하거나
 * * 명예를 훼손하는
 *
 * 등의 행위는 정보통신망 이용촉진 및 정보보호 등에 관한 법률의 제재를 받습니다.
 *
 * # 면책 조항
 *
 * Nimf 는 무료로 배포되는 오픈소스 소프트웨어입니다.
 * Nimf 개발자는 개발 및 유지보수에 대해 어떠한 의무도 없고 어떠한 책임도 없습니다.
 * 어떠한 경우에도 보증하지 않습니다. 도덕적 보증 책임도 없고, 도의적 보증 책임도 없습니다.
 * Nimf 개발자는 리브레오피스, 이클립스 등 귀하가 사용하시는 소프트웨어의 버그를 해결해야 할 의무가 없습니다.
 * Nimf 개발자는 귀하가 사용하시는 배포판에 대해 기술 지원을 해드려야 할 의무가 없습니다.
 *
 * Nimf is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nimf is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program;  If not, see <http://www.gnu.org/licenses/>.
 */

#include "nimf-server-im.h"
#include <string.h>

G_DEFINE_TYPE (NimfServerIM, nimf_server_im, NIMF_TYPE_SERVICE_IM);

void
nimf_server_im_emit_commit (NimfServiceIM *im,
                            const gchar   *text)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  NimfServerIM *server_im = NIMF_SERVER_IM (im);

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_COMMIT,
                     (gchar *) text, strlen (text) + 1, NULL);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_COMMIT_REPLY);
}

void nimf_server_im_emit_preedit_start (NimfServiceIM *im)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  if (G_UNLIKELY (im->use_preedit == FALSE &&
                  im->preedit_state == NIMF_PREEDIT_STATE_END))
    return;

  NimfServerIM *server_im = NIMF_SERVER_IM (im);

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_PREEDIT_START, NULL, 0, NULL);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_PREEDIT_START_REPLY);
  im->preedit_state = NIMF_PREEDIT_STATE_START;
}

void
nimf_server_im_emit_preedit_changed (NimfServiceIM    *im,
                                     const gchar      *preedit_string,
                                     NimfPreeditAttr **attrs,
                                     gint              cursor_pos)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  if (G_UNLIKELY (im->use_preedit == FALSE &&
                  im->preedit_state == NIMF_PREEDIT_STATE_END))
    return;

  NimfServerIM *server_im = NIMF_SERVER_IM (im);
  gchar *data;
  gsize  data_len;
  gint   str_len = strlen (preedit_string);
  gint   n_attr = 0;
  gint   i;

  while (attrs[n_attr] != NULL)
    n_attr++;

  data_len = str_len + 1 + n_attr * sizeof (NimfPreeditAttr) + sizeof (gint);
  data = g_strndup (preedit_string, data_len - 1);

  for (i = 0; attrs[i] != NULL; i++)
    *(NimfPreeditAttr *)
      (data + str_len + 1 + i * sizeof (NimfPreeditAttr)) = *attrs[i];

  *(gint *) (data + data_len - sizeof (gint)) = cursor_pos;

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_PREEDIT_CHANGED,
                     data, data_len, g_free);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_PREEDIT_CHANGED_REPLY);
}

void nimf_server_im_emit_preedit_end (NimfServiceIM *im)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  if (G_UNLIKELY (im->use_preedit == FALSE &&
                  im->preedit_state == NIMF_PREEDIT_STATE_END))
    return;

  NimfServerIM *server_im = NIMF_SERVER_IM (im);

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_PREEDIT_END, NULL, 0, NULL);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_PREEDIT_END_REPLY);
  im->preedit_state = NIMF_PREEDIT_STATE_END;
}

gboolean
nimf_server_im_emit_retrieve_surrounding (NimfServiceIM *im)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  NimfServerIM *server_im = NIMF_SERVER_IM (im);

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_RETRIEVE_SURROUNDING, NULL, 0, NULL);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_RETRIEVE_SURROUNDING_REPLY);

  if (server_im->connection->result->reply == NULL)
    return FALSE;

  return *(gboolean *) (server_im->connection->result->reply->data);
}

gboolean
nimf_server_im_emit_delete_surrounding (NimfServiceIM *im,
                                        gint           offset,
                                        gint           n_chars)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  NimfServerIM *server_im = NIMF_SERVER_IM (im);

  gint *data = g_malloc (2 * sizeof (gint));
  data[0] = offset;
  data[1] = n_chars;

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_DELETE_SURROUNDING,
                     data, 2 * sizeof (gint), g_free);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_DELETE_SURROUNDING_REPLY);

  if (server_im->connection->result->reply == NULL)
    return FALSE;

  return *(gboolean *) (server_im->connection->result->reply->data);
}

void nimf_server_im_emit_beep (NimfServiceIM *im)
{
  g_debug (G_STRLOC ": %s", G_STRFUNC);

  NimfServerIM *server_im = NIMF_SERVER_IM (im);

  nimf_send_message (server_im->connection->socket, im->icid,
                     NIMF_MESSAGE_BEEP, NULL, 0, NULL);
  nimf_result_iteration_until (server_im->connection->result, NULL, im->icid,
                               NIMF_MESSAGE_BEEP_REPLY);
}

NimfServerIM *nimf_server_im_new (NimfConnection *connection,
                                  NimfServer     *server)
{
  NimfServerIM *server_im;

  server_im = g_object_new (NIMF_TYPE_SERVER_IM, "server", server, NULL);
  server_im->connection = connection;

  return server_im;
}

static void
nimf_server_im_init (NimfServerIM *nimf_server_im)
{
}

static void
nimf_server_im_finalize (GObject *object)
{
  G_OBJECT_CLASS (nimf_server_im_parent_class)->finalize (object);
}

static void
nimf_server_im_class_init (NimfServerIMClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  NimfServiceIMClass *service_im_class = NIMF_SERVICE_IM_CLASS (class);

  object_class->finalize = nimf_server_im_finalize;

  service_im_class->emit_commit          = nimf_server_im_emit_commit;
  service_im_class->emit_preedit_start   = nimf_server_im_emit_preedit_start;
  service_im_class->emit_preedit_changed = nimf_server_im_emit_preedit_changed;
  service_im_class->emit_preedit_end     = nimf_server_im_emit_preedit_end;
  service_im_class->emit_retrieve_surrounding = nimf_server_im_emit_retrieve_surrounding;
  service_im_class->emit_delete_surrounding = nimf_server_im_emit_delete_surrounding;
  service_im_class->emit_beep            = nimf_server_im_emit_beep;
}
