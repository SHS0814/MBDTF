#include <gtk/gtk.h>
#include <libwebsockets.h>
#include <json-c/json.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 구조체 정의
typedef struct {
    GtkWidget* window;
    GtkWidget* entry;
    GtkWidget* send_button;
    GtkWidget* chat_box;
} MessageScreen;

static MessageScreen* message_screen;
static struct lws_context* context;
static struct lws* wsi;

// 콜백 함수
static int callback_message(struct lws* wsi, enum lws_callback_reasons reason,
    void* user, void* in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("WebSocket 연결 성공\n");
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
        // 메시지 수신 처리
        printf("수신: %s\n", (char*)in);
        break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
        // 메시지 전송 처리
        if (message_screen && message_screen->entry) {
            const char* msg = gtk_entry_get_text(GTK_ENTRY(message_screen->entry));
            unsigned char buf[LWS_PRE + 512];
            size_t msg_len = strlen(msg);
            memcpy(&buf[LWS_PRE], msg, msg_len);
            lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
        }
        else {
            printf("Entry 위젯이 초기화되지 않았습니다.\n");
        }
        break;

    case LWS_CALLBACK_CLIENT_CLOSED:
        printf("연결 종료\n");
        break;

    default:
        break;
    }
    return 0;
}

// 프로토콜 정의
static struct lws_protocols protocols[] = {
    {"message-protocol", callback_message, 0, 512},
    {NULL, NULL, 0, 0}
};

// 메시지 화면 생성 및 스택 저장
GtkWidget* create_message_screen(GtkStack* stack) {
    message_screen = g_malloc(sizeof(MessageScreen));

    // 메시지 창 생성
    message_screen->window = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(message_screen->window, 400, 600);

    // 채팅 박스 추가
    message_screen->chat_box = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_screen->chat_box), FALSE);
    gtk_box_pack_start(GTK_BOX(message_screen->window), message_screen->chat_box, TRUE, TRUE, 0);

    // 입력 필드와 버튼 추가
    message_screen->entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(message_screen->window), message_screen->entry, FALSE, TRUE, 0);

    message_screen->send_button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(message_screen->window), message_screen->send_button, FALSE, TRUE, 0);

    // 스택에 화면 추가
    gtk_stack_add_named(stack, message_screen->window, "message_screen");
}

// 메시지 창 전환 및 소켓 연결 시작
void start_message(GtkStack* stack) {
    // 창 전환
    gtk_stack_set_visible_child_name(stack, "ttt_screen");

    // 소켓 연결
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;

    context = lws_create_context(&info);

    struct lws_client_connect_info ccinfo = { 0 };
    ccinfo.context = context;
    ccinfo.address = "localhost";
    ccinfo.port = 8080;
    ccinfo.path = "/";
    ccinfo.protocol = "message-protocol";

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        fprintf(stderr, "WebSocket 연결 실패\n");
    }

    // GTK 이벤트 루프 내에서 WebSocket 처리
    g_timeout_add(100, (GSourceFunc)lws_service, context);
}
