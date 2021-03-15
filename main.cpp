#include <iostream>
#include <gtk-3.0/gtk/gtk.h>
#include <curl/curl.h>
#include <gdk/gdk.h>
#include <string>
#include <fstream>
#include <vector>

typedef struct
{
    GtkWidget *entry;
    GtkWidget *textview;
    GtkWidget *textviewresp;
} Widgets;

using namespace std;


size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size*nmemb;
    try
    {
        s->append((char*)contents, newLength);
    }
    catch(std::bad_alloc &e)
    {
        //handle memory problem
        return 0;
    }
    return newLength;
}

void findAndReplaceAll(std::string & data, std::string toSearch, std::string replaceStr)
{
    // Get the first occurrence
    size_t pos = data.find(toSearch);
    // Repeat till end is reached
    while( pos != std::string::npos)
    {
        // Replace this occurrence of Sub String
        data.replace(pos, toSearch.size(), replaceStr);
        // Get the next occurrence from the current position
        pos =data.find(toSearch, pos + replaceStr.size());
    }
}

void sendRequest1 ( GtkButton *button, Widgets *widg )
{
    ( void ) button;
    gchar *txt;
    GtkTextIter start, end;
    GtkTextBuffer *buff,*respBuffer;

    /* disable text view and get contents of buffer */
    gtk_widget_set_sensitive (widg->textview, FALSE);
    buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widg->textview));
    gtk_text_buffer_get_start_iter (buff, &start);
    gtk_text_buffer_get_end_iter (buff, &end);
    txt = gtk_text_buffer_get_text (buff, &start, &end, FALSE);
    gtk_text_buffer_set_modified (buff, FALSE);
    gtk_widget_set_sensitive (widg->textview, TRUE);


    const gchar *url = gtk_entry_get_text ( GTK_ENTRY ( widg->entry ) );

    long http_code;

    char curlErrorBuffer[CURL_ERROR_SIZE];
    CURL *curl = curl_easy_init();
    CURLcode res;
    string responseBuffer;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "X-WH-APIKEY: F3C74230818DA487BB2017CE5D0290F4DABCAFD7"); // API Key. If API Key is wrong API will return 401 status code.
    headers = curl_slist_append(headers, "X-WH-START: 1391165350"); // Start time for report. Unixtime
    headers = curl_slist_append(headers, "X-WH-END: 1391186951"); // End time for report. Unixtime
    headers = curl_slist_append(headers, "Accept: application/json");  // Currenlty we support JSON ONLY, required header
    headers = curl_slist_append(headers, "Content-Type: application/json"); // Currenlty we support JSON ONLY, required header

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl,CURLOPT_POST,1);
        curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,false);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, txt);
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);
        curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L); //remove this to disable verbose output
        curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, 5);
        curl_easy_setopt (curl, CURLOPT_TIMEOUT, 5);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        http_code = 0;
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            responseBuffer =  fprintf(stderr, "curl_easy_perform() failed: %s\n",
                                      curl_easy_strerror(res));
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        responseBuffer.replace(0,1,"{\n");
        findAndReplaceAll(responseBuffer, ",", ",\n");

        cout << responseBuffer << endl;

        gtk_text_view_set_editable(GTK_TEXT_VIEW(widg->textviewresp), true);
        buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widg->textviewresp));
        const gchar *rsp = responseBuffer.data();
        gtk_text_buffer_set_text (buff, rsp, -1);
    }
    curl_easy_cleanup(curl); // always cleanup https://www.programmersought.com/article/47241477861/
}

/**
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *url_label, *request_label;
    GtkWidget *request_entry;
    GtkWidget *ok_button;
    GtkWidget *vbox;
    GtkTextBuffer *buff;

    GtkWidget *horizontalBoxForTextInput;
    GtkWidget *horizontalBoxForTextarea;
    GtkWidget *verticalBoxForRequest;
    GtkWidget *verticalBoxForResponse;

    // menu
    GtkWidget *menubar;
    GtkWidget *fileMenu;
    GtkWidget *fileMi;
    GtkWidget *quitMi;
    GtkWidget *aboutMi;

    // scroll
    GtkWidget *scrolled_req, *scrolled_res;
    double from_bottom = 0.0;

    Widgets *widg = g_slice_new ( Widgets ); // набор виджетов для окна: textarea, text input

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Request Maker v 0.2 by Khtulhu");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 400);
    //gtk_window_set_resizable (GTK_WINDOW(window), FALSE);// это просто убирает кнопку изменения размера
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    g_signal_connect(window, "destroy",G_CALLBACK(gtk_main_quit), NULL);

    url_label = gtk_label_new(NULL); // "url"
    request_label = gtk_label_new("JSON request: ");

/////// создание объекта "виджет" с полями ввода
    widg->textview = gtk_text_view_new(); // request field
    widg->textviewresp = gtk_text_view_new(); // response field
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widg->textview), true);
    // default text
    buff = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widg->textview));
    gtk_text_buffer_set_text (buff, "{\"id\":\"\1234\"}", -1);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(widg->textviewresp), true);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(widg->textview), true);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(widg->textviewresp), true);
    gtk_widget_set_size_request(widg->textview,400,300); // ширина и высота текстового поля
    gtk_widget_set_size_request(widg->textviewresp,400,300); // ширина и высота текстового поля
    widg->entry = gtk_entry_new(); // url field


    gtk_label_set_width_chars(GTK_LABEL(url_label), 12);
    gtk_label_set_width_chars(GTK_LABEL(request_label), 12);
    request_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(request_entry), true);

    ok_button = gtk_button_new_with_label("Отправить запрос");

    /// ***
    g_signal_connect ( G_OBJECT ( ok_button      ), "clicked",  G_CALLBACK ( sendRequest1 ), ( gpointer ) widg );
    g_signal_connect ( G_OBJECT ( widg->entry ), "activate", G_CALLBACK ( sendRequest1 ), ( gpointer ) widg );
    /// ***

    // попытка приделать прокрутку текстового поля
    scrolled_req = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request (scrolled_req, 400, -1 );
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_req), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled_req), widg->textview);

    scrolled_res = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request (scrolled_req, 400, -1 );
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_req), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (scrolled_res), widg->textviewresp);


    // суб-контейнеры для размещения внутри основного контейнера
    horizontalBoxForTextInput = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);     // для однострочного ввода
    horizontalBoxForTextarea = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); // контейнер для поля ввода запроса
    verticalBoxForRequest =  gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    verticalBoxForResponse =  gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);   // общий контейнер

    // menu
    menubar = gtk_menu_bar_new();
    fileMenu = gtk_menu_new();

    fileMi = gtk_menu_item_new_with_label("File");
    quitMi = gtk_menu_item_new_with_label("Quit");
    aboutMi = gtk_menu_item_new_with_label("About");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileMi), fileMenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), aboutMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), quitMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMi);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileMenu);
    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(quitMi), "activate",
                     G_CALLBACK(gtk_main_quit), NULL);

    // URL input
    gtk_box_pack_start(GTK_BOX(horizontalBoxForTextInput), widg->entry, TRUE, TRUE, 5); //

    // request input
    gtk_box_pack_start(GTK_BOX(verticalBoxForRequest), scrolled_req, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(verticalBoxForResponse), scrolled_res, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(verticalBoxForRequest), widg->textview, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(horizontalBoxForTextarea), verticalBoxForRequest, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(verticalBoxForResponse), widg->textviewresp, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(horizontalBoxForTextarea), verticalBoxForResponse, TRUE, TRUE, 5);

    gtk_box_pack_start(GTK_BOX(vbox), horizontalBoxForTextInput, FALSE, TRUE, 5); // контейнер для однострочного текстового ввода true/false регулирует ширину и высоту
    gtk_box_pack_start(GTK_BOX(vbox), horizontalBoxForTextarea, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), ok_button, FALSE, FALSE, 5);

    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
