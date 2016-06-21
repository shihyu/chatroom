/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2016 satan <satan@geeker>
 * 
 * im-client is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * im-client is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#define MAXSIZE 65536
typedef struct _Private Private;
struct _Private
{
	/* ANJUTA: Widgets declaration for im_client.ui - DO NOT REMOVE */
};

static Private* priv = NULL;
//登陆窗口
GtkWidget *windowLogin = NULL;
//聊天窗口
GtkWidget *windowChat = NULL;

//昵称输入
GtkEntry *inputName;
//服务器地址
GtkEntry *inputServer;
//服务器端口
GtkEntry *inputPort;
//提示消息
GtkLabel *labelInfo;
//消息滚动窗口
GtkScrolledWindow *scrolledWindow;
//消息接收框
GtkTextView *textRecv;
//消息发送框
GtkTextView *textSend;
//消息框文本长度
int length;
//TCP连接
int tcp_link = -1;
//昵称
char nickname[20];

//接收消息线程
pthread_t pid;
void* recv_thread(void *);
/* For testing purpose, define TEST to use the local (not installed) ui file */
//#define TEST
#ifdef TEST
#define UI_FILE_LOGIN "src/im_login.ui"
#define UI_FILE_CHAT "src/im_chat.ui"
#else
#define UI_FILE_LOGIN PACKAGE_DATA_DIR"/ui/im_login.ui"
#define UI_FILE_CHAT PACKAGE_DATA_DIR"/ui/im_chat.ui"
#endif
#define WINDOW_LOGIN "windowLogin"
#define WINDOW_CHAT "windowChat"

/* Signal handlers */
/* Note: These may not be declared static because signal autoconnection
 * only works with non-static methods
 */

/* Called when the window is closed */
void
destroy (GtkWidget *widget, gpointer data)
{
	if (widget == windowChat)
	{
		gtk_main_quit ();
		pthread_cancel(pid);
		close(tcp_link);
	}
	else if (windowChat == NULL)
	{
		gtk_main_quit();
	}
}

static GtkWidget*
create_window_login (void)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	/* Load UI from file */
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE_LOGIN, &error))
	{
		g_critical ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	/* Auto-connect signal handlers */
	gtk_builder_connect_signals (builder, NULL);

	/* Get the window object from the ui file */
	window = GTK_WIDGET (gtk_builder_get_object (builder, WINDOW_LOGIN));
        if (!window)
        {
                g_critical ("Widget \"%s\" is missing in file %s.",
				WINDOW_LOGIN,
				UI_FILE_LOGIN);
        }

	priv = g_malloc (sizeof (struct _Private));
	/* ANJUTA: Widgets initialization for im_client.ui - DO NOT REMOVE */

	inputName = (GtkEntry *)gtk_builder_get_object (builder, "inputName");
	inputServer = (GtkEntry *)gtk_builder_get_object (builder, "inputServer");
	inputPort = (GtkEntry *)gtk_builder_get_object (builder, "inputPort");
	labelInfo = (GtkLabel *)gtk_builder_get_object (builder, "labelInfo");
	g_object_unref (builder);

	
	return window;
}

static GtkWidget*
create_window_chat (void)
{
	GtkWidget *window;
	GtkBuilder *builder;
	GError* error = NULL;

	/* Load UI from file */
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE_CHAT, &error))
	{
		g_critical ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

	/* Auto-connect signal handlers */
	gtk_builder_connect_signals (builder, NULL);

	/* Get the window object from the ui file */
	window = GTK_WIDGET (gtk_builder_get_object (builder, WINDOW_CHAT));
        if (!window)
        {
                g_critical ("Widget \"%s\" is missing in file %s.",
				WINDOW_CHAT,
				UI_FILE_CHAT);
        }

	priv = g_malloc (sizeof (struct _Private));
	/* ANJUTA: Widgets initialization for im_client.ui - DO NOT REMOVE */

	textRecv = (GtkTextView *)gtk_builder_get_object (builder, "textRecv");
	textSend = (GtkTextView *)gtk_builder_get_object (builder, "textSend");
	scrolledWindow = (GtkScrolledWindow *)gtk_builder_get_object (builder, "scrolledWindow");
	GtkLabel *tmp = (GtkLabel *)gtk_builder_get_object (builder, "labelName");
	gtk_label_set_text(tmp, nickname);
	g_object_unref (builder);

	
	return window;
}
//清空数组，以指定字符填充
void clean_array(char * arr, int len, char ch)
{
	int i;
	for(i = 0; i < len; i++)
	{
		arr[i] = ch;
	}
}
//创建连接
int create_link(char *name, char *server, int port)
{
	struct sockaddr_in remote;
	if (strlen(name) <= 0 || strlen(server) <= 0 || port <= 0)
	{
		return -1;
	}
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = inet_addr(server);
	remote.sin_port = htons(port);
	tcp_link = socket(AF_INET, SOCK_STREAM, 0);
	return connect(tcp_link, (struct sockaddr *)&remote, sizeof(remote));
}

//发送数据
void send_message(char * type, char *name, int length, char *message)
{
	char data[1024];
	sprintf(data,"%s:|%s:|%d:|%d:|%s", type, name, time((time_t*)NULL), length, message);
	send(tcp_link, (void *)data, strlen(data), 0);
}
//登陆判断
void btn_login(GtkWidget *widget, gpointer data)
{
	gtk_label_set_text(labelInfo, "网络连接中...");
	char * name = (char *)gtk_entry_get_text (inputName);
	char * server = (char *)gtk_entry_get_text (inputServer);
	int port = atoi((char *)gtk_entry_get_text (inputPort));
	//创建TCP连接失败
	if (tcp_link <= 0 && create_link (name, server, port) == -1)
	{
		tcp_link = -1;
		gtk_label_set_text(labelInfo, "网络连接失败");
		return;
	}

	//验证昵称
	char res[100];
	send_message ("login", name, 0, "");
	recv(tcp_link, res, 100, 0);

	//登陆成功
	if (strcmp(res, "success") == 0)
	{
		strcpy(nickname, name);
		windowChat = create_window_chat ();
		gtk_widget_show (windowChat);
		gtk_widget_destroy (windowLogin);
		pthread_create(&pid, 0, recv_thread, 0);
	}
	else
	{
		gtk_label_set_text(labelInfo, "昵称已存在");
	}
	
}
//处理接收到的数据，加入接收框内
void deal_message(char *data)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(textRecv));
	char msg[1024];
	char *p;
	char *delim = ":|";
	time_t t;
	p = strtok(data, delim);
	if (strcmp(p, "message") != 0)
	{
		return;
	}
	int i = 0;
	while((p = strtok(NULL, delim)))
	{
		switch (i)
		{
			case 0:
				strcpy(msg, p);
				strcat(msg, "\t");
				break;
			case 1:
				t = atoi(p);
				strcat(msg, ctime(&t));
				break;
			case 3:
				strcat(msg, p);
				strcat(msg, "\n\n");
				break;
			default:
				break;
		}
		i++;
	}
	int msg_len = strlen(msg);
	GtkTextIter start;
	GtkTextIter end;
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	//消息框文本达到最大长度
	if (length + msg_len >= MAXSIZE)
	{
		gtk_text_buffer_delete (buffer, &start, &end);
		gtk_text_buffer_insert(buffer, &end, (gchar*)msg, strlen(msg));
		length = msg_len;
	}
	else
	{
		gtk_text_buffer_insert (buffer, &end,(gchar *)msg, strlen(msg));
		length += msg_len;
	}

	GtkTextMark *mk = gtk_text_buffer_get_insert (buffer);
	gtk_text_view_scroll_to_mark (textRecv, mk, 0.0, FALSE, 0,0);
}
//发送消息
void btn_send(GtkWidget *widget, gpointer data)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW(textSend));
	GtkTextIter start;
	GtkTextIter end;
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	gchar * message = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
	if (strlen(message) <= 0)
	{
		return;
	}
	send_message ("message", nickname, strlen(message), message);
	gtk_text_buffer_delete (buffer, &start, &end);
}

void* recv_thread (void *p)
{
	char buf[1024];
	int cursize = 0;
	while (1)
	{
		cursize = recv(tcp_link, buf, sizeof(buf), 0);
		if (cursize <= 0)
		{
			g_print("recv_thread\n");
			return;
		}
		g_print("%s\n", buf);
		deal_message (buf);
		clean_array (buf, cursize, '\0');
	}
	
}
int
main (int argc, char *argv[])
{
#ifdef ENABLE_NLS

	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
	gtk_init (&argc, &argv);

	windowLogin = create_window_login ();
	gtk_widget_show (windowLogin);

	gtk_main ();

	g_free (priv);


	return 0;
}

