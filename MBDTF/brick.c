#pragma warning(disable : 4819)

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "games.h"

// ��� ����
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 800
#define PADDLE_WIDTH 100
#define PADDLE_HEIGHT 10
#define BALL_SIZE 15
#define BRICK_ROWS 7
#define BRICK_COLS 10
#define BRICK_WIDTH 80
#define BRICK_HEIGHT 30
#define BRICK_PADDING 5
#define BALL_SPEED 7.0
#define ITEM_FALL_SPEED 3.5
#define TOTAL_BRICK_WIDTH ((BRICK_WIDTH + BRICK_PADDING) * BRICK_COLS + BRICK_PADDING)
#define TOTAL_BRICK_HEIGHT ((BRICK_HEIGHT + BRICK_PADDING) * BRICK_ROWS + BRICK_PADDING)

// ������ Ÿ�� ����
typedef enum {
    ITEM_PADDLE_EXTEND,
    ITEM_MULTI_BALL,
    ITEM_EXTRA_LIFE,
    ITEM_TYPES_COUNT
} ItemType;

// ������ ����ü
typedef struct {
    double x, y;
    ItemType type;
    bool active;
} Item;

// �� ����ü
typedef struct {
    double x, y;
    double dx, dy;
    bool active;
} Ball;

// ���� ���� ����ü
typedef struct {
    Ball balls[10];
    int active_balls;
    GtkWidget* drawing_area;
    double paddle_x;
    int score;
    bool game_running;
    int bricks[BRICK_ROWS][BRICK_COLS];
    int lives;
    Item items[20];
    int active_items;
    double paddle_width;
    int paddle_extend_time;
    int level;
} GameState;

// ���� ���� ���� ����
static GameState game_state;

// �Լ� ����
static void init_game(void);
static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data);
void switch_to_main_menu(GtkWidget* widget, gpointer data);
static void spawn_item(double x, double y);
static void add_new_ball(void);
static gboolean update_game(gpointer data);
static void restart_game(GtkWidget* widget, gpointer data);

// ������ ȿ�� ���� �Լ�
static void apply_item_effect(ItemType type) {
    switch (type) {
    case ITEM_PADDLE_EXTEND:
        game_state.paddle_width = PADDLE_WIDTH * 1.5;
        game_state.paddle_extend_time = 600;
        break;

    case ITEM_MULTI_BALL:
        add_new_ball();
        break;

    case ITEM_EXTRA_LIFE:
        // �ִ� ���� ���� 5���� ����
        if (game_state.lives < 5) {
            game_state.lives++;
        }
        break;
    default:
        break;
    }
}

// update_game �Լ� ����
static gboolean update_game(gpointer data) {
    if (!game_state.game_running) return TRUE;

    // ��� �� ������Ʈ
    for (int ball_idx = 0; ball_idx < 10; ball_idx++) {
        if (!game_state.balls[ball_idx].active) continue;

        Ball* ball = &game_state.balls[ball_idx];

        // �� �̵�
        ball->x += ball->dx;
        ball->y += ball->dy;

        // �� ��
        if (ball->x <= 0 || ball->x >= WINDOW_WIDTH) ball->dx = -ball->dx;
        if (ball->y <= 0) ball->dy = -ball->dy;

        // �е� �浹 ����
        if (ball->y >= WINDOW_HEIGHT - 20 &&
            ball->x >= game_state.paddle_x &&
            ball->x <= game_state.paddle_x + game_state.paddle_width) {
            // �е��� �߾��� �������� �浹 ��ġ�� ����� ��ġ ��� (-1.0 ~ 1.0)
            double relative_intersect_x = (ball->x - (game_state.paddle_x + game_state.paddle_width / 2)) / (game_state.paddle_width / 2);

            // �ݻ簢 ��� (�ִ� 75��)
            double max_angle = 75.0 * M_PI / 180.0;
            double bounce_angle = relative_intersect_x * max_angle;

            // ���� �ӵ� ���
            double current_speed = sqrt(ball->dx * ball->dx + ball->dy * ball->dy);

            // ���ο� ���� ����
            ball->dx = current_speed * sin(bounce_angle);
            ball->dy = -current_speed * cos(bounce_angle);
        }

        // �ٴڿ� ����� ��
        if (ball->y >= WINDOW_HEIGHT) {
            ball->active = false;
            game_state.active_balls--;

            // ��� ���� �������� ���� ���� ����
            if (game_state.active_balls == 0) {
                game_state.lives--;

                if (game_state.lives <= 0) {
                    game_state.game_running = FALSE;
                    send_game_score(username, "bp", game_state.score);
                }
                else {
                    // ���ο� �� ���� - ���� ������ �´� �ӵ���
                    game_state.balls[0].x = WINDOW_WIDTH / 2;
                    game_state.balls[0].y = WINDOW_HEIGHT - 50;

                    double speed_multiplier = 1.0 + (game_state.level - 1) * 0.2;
                    if (speed_multiplier > 2.0) speed_multiplier = 2.0;

                    double angle = M_PI / 2 + (rand() % 40 - 20) * M_PI / 180.0;
                    game_state.balls[0].dx = BALL_SPEED * speed_multiplier * cos(angle);
                    game_state.balls[0].dy = -BALL_SPEED * speed_multiplier * sin(angle);
                    game_state.balls[0].active = true;
                    game_state.active_balls = 1;
                }
            }
        }

        // ���� �浹 �˻�
        double brick_start_x = (WINDOW_WIDTH - ((BRICK_WIDTH + BRICK_PADDING) * BRICK_COLS + BRICK_PADDING)) / 2;
        double brick_start_y = 50;

        bool collision_detected = false;

        for (int i = 0; i < BRICK_ROWS && !collision_detected; i++) {
            for (int j = 0; j < BRICK_COLS && !collision_detected; j++) {
                if (game_state.bricks[i][j] > 0) {
                    double brick_x = brick_start_x + j * (BRICK_WIDTH + BRICK_PADDING) + BRICK_PADDING;
                    double brick_y = brick_start_y + i * (BRICK_HEIGHT + BRICK_PADDING) + BRICK_PADDING;

                    // ���� �߽ɰ� ���� �� �𼭸� ������ �Ÿ� ���
                    double closest_x = fmax(brick_x, fmin(ball->x, brick_x + BRICK_WIDTH));
                    double closest_y = fmax(brick_y, fmin(ball->y, brick_y + BRICK_HEIGHT));

                    // ���� �߽ɰ� ���� ����� ���� �� ������ �Ÿ�
                    double distance_x = ball->x - closest_x;
                    double distance_y = ball->y - closest_y;
                    double distance = sqrt(distance_x * distance_x + distance_y * distance_y);

                    // ���� ������(BALL_SIZE/2)���� �Ÿ��� ������ �浹
                    if (distance < BALL_SIZE / 2) {
                        int original_durability = game_state.bricks[i][j];
                        game_state.bricks[i][j]--;

                        // ������ ������ �μ����� ��
                        if (game_state.bricks[i][j] == 0) {
                            game_state.score += original_durability * 10;
                        }
                        else {
                            game_state.score += 5;
                        }

                        // ��� ���� Ÿ�� �� 30% Ȯ���� ������ ���� (�ı� �� Ÿ�� ����)
                        if (rand() % 100 < 30) {
                            spawn_item(brick_x + BRICK_WIDTH / 2, brick_y + BRICK_HEIGHT / 2);
                        }

                        // �浹 ���⿡ ���� �ݻ� ó��
                        if (fabs(distance_x) > fabs(distance_y)) {
                            ball->dx = -ball->dx;  // �¿� �浹
                        }
                        else {
                            ball->dy = -ball->dy;  // ���� �浹
                        }

                        collision_detected = true;
                        break;
                    }
                }
            }
        }
    }

    // ������ �̵� �� �浹 �˻�
    for (int i = 0; i < 20; i++) {
        if (game_state.items[i].active) {
            game_state.items[i].y += ITEM_FALL_SPEED;

            // �е�� �浹 �˻�
            if (game_state.items[i].y >= WINDOW_HEIGHT - 20 &&
                game_state.items[i].x >= game_state.paddle_x &&
                game_state.items[i].x <= game_state.paddle_x + game_state.paddle_width) {
                apply_item_effect(game_state.items[i].type);
                game_state.items[i].active = false;
                game_state.active_items--;
            }

            // ȭ�� ������ �������� �˻�
            if (game_state.items[i].y >= WINDOW_HEIGHT) {
                game_state.items[i].active = false;
                game_state.active_items--;
            }
        }
    }

    // ��� ������ �ı��Ǿ����� Ȯ��
    bool all_bricks_destroyed = true;
    for (int i = 0; i < BRICK_ROWS && all_bricks_destroyed; i++) {
        for (int j = 0; j < BRICK_COLS; j++) {
            if (game_state.bricks[i][j] > 0) {
                all_bricks_destroyed = false;
                break;
            }
        }
    }

    // ��� ������ �ı��Ǹ� ���� ������
    if (all_bricks_destroyed) {
        game_state.level++;
        init_game();  // ���� ���� �ʱ�ȭ
    }

    // ���� ǥ�� ������Ʈ
    GtkWidget* vbox = gtk_widget_get_parent(game_state.drawing_area);
    vbox = gtk_widget_get_parent(vbox);
    GList* children = gtk_container_get_children(GTK_CONTAINER(vbox));
    GtkWidget* score_label = GTK_WIDGET(children->data);

    char score_text[50];
    sprintf(score_text, "Score: %d    Lives: (%d/5)    Level: %d",
        game_state.score, game_state.lives, game_state.level);
    gtk_label_set_text(GTK_LABEL(score_label), score_text);

    g_list_free(children);

    gtk_widget_queue_draw(game_state.drawing_area);
    return TRUE;
}

// ���콺 �̵�
static gboolean on_motion_notify(GtkWidget* widget, GdkEventMotion* event,
    gpointer data) {
    game_state.paddle_x = event->x - PADDLE_WIDTH / 2;

    // �е��� ȭ�� ������ ������ �ʵ��� ����
    if (game_state.paddle_x < 0) game_state.paddle_x = 0;
    if (game_state.paddle_x > WINDOW_WIDTH - PADDLE_WIDTH)
        game_state.paddle_x = WINDOW_WIDTH - PADDLE_WIDTH;

    return TRUE;
}

// Ű �̺�Ʈ �ڵ鷯 �Լ� ����
static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    if (event->keyval == GDK_KEY_k || event->keyval == GDK_KEY_K) {  // ��ҹ��� ��� ó��
        if (game_state.game_running) {                               // ������ ���� ���� ���� ó��
            game_state.level++;
            init_game();
            return TRUE;
        }
    }
    return FALSE;
}

// restart_game �Լ� ����
static void restart_game(GtkWidget* widget, gpointer data) {
    game_state.level = 0;            // ������ 0���� ����
    game_state.game_running = true;  // ���� ���¸� ���� ������ ����
    init_game();

    // drawing_area�� �ٽ� ��Ŀ�� ����
    gtk_widget_grab_focus(game_state.drawing_area);
}

// create_breakout_screen �Լ� (������ ����)
GtkWidget* create_breakout_screen(GtkStack* stack) {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // ������ ���� ǥ�ø� ���� ���̺�
    char initial_text[50];
    sprintf(initial_text, "Score: 0    Lives: 3    Level: 1");
    GtkWidget* score_label = gtk_label_new(initial_text);
    gtk_widget_set_name(score_label, "score_label");
    gtk_widget_set_halign(score_label, GTK_ALIGN_CENTER);  // �߾� ���� �߰�
    gtk_box_pack_start(GTK_BOX(vbox), score_label, FALSE, FALSE, 10);

    // ���� ������ �߾ӿ� ��ġ�ϱ� ���� �����̳�
    GtkWidget* center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(center_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), center_box, TRUE, TRUE, 0);

    // ���� ����
    game_state.drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(game_state.drawing_area, WINDOW_WIDTH, WINDOW_HEIGHT);
    gtk_box_pack_start(GTK_BOX(center_box), game_state.drawing_area, FALSE, FALSE, 0);

    // ��Ʈ�� ��ư�� ���� ���� �ڽ�
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 15);

    // �޴��� ���ư��� ��ư
    GtkWidget* back_button = gtk_button_new_with_label("Back to Menu");
    g_signal_connect(back_button, "clicked", G_CALLBACK(switch_to_main_menu), stack);  // stack�� ���� ����
    gtk_box_pack_start(GTK_BOX(button_box), back_button, FALSE, FALSE, 5);

    // ����� ��ư
    GtkWidget* restart_button = gtk_button_new_with_label("Restart Game");
    g_signal_connect(restart_button, "clicked", G_CALLBACK(restart_game), NULL);
    gtk_box_pack_start(GTK_BOX(button_box), restart_button, FALSE, FALSE, 5);

    // �̺�Ʈ ����
    g_signal_connect(game_state.drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    gtk_widget_add_events(game_state.drawing_area,
        GDK_POINTER_MOTION_MASK |
        GDK_KEY_PRESS_MASK |
        GDK_FOCUS_CHANGE_MASK);  // ��Ŀ�� ���� �̺�Ʈ �߰�
    g_signal_connect(game_state.drawing_area, "motion-notify-event",
        G_CALLBACK(on_motion_notify), NULL);
    g_signal_connect(game_state.drawing_area, "key-press-event",
        G_CALLBACK(on_key_press), NULL);

    // drawing_area�� Ű �̺�Ʈ�� ���� �� �ֵ��� ����
    gtk_widget_set_can_focus(game_state.drawing_area, TRUE);
    gtk_widget_grab_focus(game_state.drawing_area);

    return vbox;
}

// ���� ���� �Լ�
void start_breakout_game(GtkWidget* widget, gpointer data) {
    GtkStack* stack = GTK_STACK(data);
    gtk_stack_set_visible_child_name(stack, "breakout_screen");

    // ���� ó�� ������ ���� ������ 0���� �����Ͽ� init_game���� 1�� �ʱ�ȭ�ǵ��� ��
    game_state.level = 0;
    init_game();

    // ���� Ÿ�̸Ӱ� �ִٸ� ����
    static guint timer_id = 0;
    if (timer_id > 0) {
        g_source_remove(timer_id);
    }

    // ���ο� Ÿ�̸� ����
    timer_id = g_timeout_add(16, update_game, NULL);  // �� 60 FPS
}

// CSS ��Ÿ�� ������ ���� �Լ� �߰� (main.c�� �߰�)
void apply_css(void) {
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "#score_label {"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "    color: #ffffff;"
        "    margin: 10px;"
        "}",
        -1, NULL);

    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_object_unref(provider);
}

// spawn_item �Լ� ���� �߰�
static void spawn_item(double x, double y) {
    if (game_state.active_items >= 20) return;
    for (int i = 0; i < 20; i++) {
        if (!game_state.items[i].active) {
            game_state.items[i].active = true;
            game_state.items[i].x = x;
            game_state.items[i].y = y;
            game_state.items[i].type = rand() % ITEM_TYPES_COUNT;
            game_state.active_items++;
            break;
        }
    }
}

// switch_to_main_menu �Լ� ����
static void switch_to_main_menu(GtkWidget* widget, gpointer data) {
    GtkStack* stack = GTK_STACK(data);
    game_state.game_running = false;                       // ���� ����
    gtk_stack_set_visible_child_name(stack, "main_menu");  // "main_menu"�� ����
}

// init_game �Լ� ����
static void init_game(void) {
    // ù ��° �� �ʱ�ȭ
    game_state.balls[0].x = WINDOW_WIDTH / 2;
    game_state.balls[0].y = WINDOW_HEIGHT - 50;

    game_state.paddle_x = (WINDOW_WIDTH - PADDLE_WIDTH) / 2;

    // ������ ó�� ���۵� ���� ������ ������ �ʱ�ȭ
    if (game_state.level <= 0) {
        game_state.score = 0;
        game_state.level = 1;
    }

    game_state.game_running = true;
    game_state.lives = 3;
    game_state.paddle_width = PADDLE_WIDTH;
    game_state.active_items = 0;
    game_state.paddle_extend_time = 0;

    // ���� �ʱ�ȭ ���� - ������ ���� ������ ����
    for (int i = 0; i < BRICK_ROWS; i++) {
        for (int j = 0; j < BRICK_COLS; j++) {
            if (game_state.level == 1) {
                // ���� 1������ 95%�� Ȯ���� ������ 1, 5%�� Ȯ���� ������ 2
                game_state.bricks[i][j] = (rand() % 100 < 95) ? 1 : 2;
            }
            else {
                // ���� 2���ʹ� ���� ���� ����
                int max_durability = game_state.level + 1;  // �ִ� ������ ����
                if (max_durability > 5) max_durability = 5;

                int min_durability = (game_state.level) / 2;  // �ּ� ������ ����
                if (min_durability < 1) min_durability = 1;
                if (min_durability > 3) min_durability = 3;

                game_state.bricks[i][j] = min_durability + (rand() % (max_durability - min_durability + 1));
            }
        }
    }

    // �� �ӵ� ���� - ������ ���� ����
    double speed_multiplier = 1.0 + (game_state.level - 1) * 0.05;  // ������ 5% ����
    if (speed_multiplier > 2.0) speed_multiplier = 2.0;            // �ִ� 2��� ����

    double angle = M_PI / 2 + (rand() % 40 - 20) * M_PI / 180.0;
    game_state.balls[0].dx = BALL_SPEED * speed_multiplier * cos(angle);
    game_state.balls[0].dy = -BALL_SPEED * speed_multiplier * sin(angle);
    game_state.balls[0].active = true;
    game_state.active_balls = 1;

    // ������ ���� ��Ȱ��ȭ
    for (int i = 1; i < 10; i++) {
        game_state.balls[i].active = false;
    }

    // ������ �ʱ�ȭ
    for (int i = 0; i < 20; i++) {
        game_state.items[i].active = false;
    }
}

// ���ο� �� �߰� �Լ�
static void add_new_ball(void) {
    if (game_state.active_balls >= 10) return;

    for (int i = 0; i < 10; i++) {
        if (!game_state.balls[i].active) {
            // ���� Ȱ��ȭ�� ù ��° ���� ��ġ���� �� �� ����
            Ball* first_ball = NULL;
            for (int j = 0; j < 10; j++) {
                if (game_state.balls[j].active) {
                    first_ball = &game_state.balls[j];
                    break;
                }
            }

            if (first_ball) {
                game_state.balls[i].x = first_ball->x;
                game_state.balls[i].y = first_ball->y;

                // ���� ������ �´� �ӵ� ���
                double speed_multiplier = 1.0 + (game_state.level - 1) * 0.05;
                if (speed_multiplier > 2.0) speed_multiplier = 2.0;

                // �ణ �ٸ� ������ �߻�
                double angle = M_PI / 2 + (rand() % 40 - 20) * M_PI / 180.0;
                game_state.balls[i].dx = BALL_SPEED * speed_multiplier * cos(angle);
                game_state.balls[i].dy = -BALL_SPEED * speed_multiplier * sin(angle);
                game_state.balls[i].active = true;
                game_state.active_balls++;
                break;
            }
        }
    }
}

// on_draw �Լ� ����
static gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    // ��� �׸���
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    // �е� �׸���
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, game_state.paddle_x, WINDOW_HEIGHT - 20,
        game_state.paddle_width, PADDLE_HEIGHT);
    cairo_fill(cr);

    // ��� Ȱ��ȭ�� �� �׸���
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    for (int i = 0; i < 10; i++) {
        if (game_state.balls[i].active) {
            cairo_arc(cr, game_state.balls[i].x, game_state.balls[i].y,
                BALL_SIZE / 2, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }

    // ���� �׸��� ����
    double brick_start_x = (WINDOW_WIDTH - TOTAL_BRICK_WIDTH) / 2;
    double brick_start_y = 50;

    for (int i = 0; i < BRICK_ROWS; i++) {
        for (int j = 0; j < BRICK_COLS; j++) {
            if (game_state.bricks[i][j] > 0) {
                // ���� ��ġ ���
                double brick_x = brick_start_x + j * (BRICK_WIDTH + BRICK_PADDING) + BRICK_PADDING;
                double brick_y = brick_start_y + i * (BRICK_HEIGHT + BRICK_PADDING) + BRICK_PADDING;

                // �������� ���� ���� ����
                switch (game_state.bricks[i][j]) {
                case 1:
                    cairo_set_source_rgb(cr, 0.4, 0.6, 1.0);  // �Ľ��� ����
                    break;
                case 2:
                    cairo_set_source_rgb(cr, 0.4, 1.0, 0.4);  // �Ľ��� �׸�
                    break;
                case 3:
                    cairo_set_source_rgb(cr, 1.0, 1.0, 0.4);  // �Ľ��� ���ο�
                    break;
                case 4:
                    cairo_set_source_rgb(cr, 1.0, 0.6, 0.4);  // �Ľ��� ������
                    break;
                case 5:
                    cairo_set_source_rgb(cr, 1.0, 0.4, 0.4);  // �Ľ��� ����
                    break;
                }

                // ���� �׸���
                cairo_rectangle(cr, brick_x, brick_y, BRICK_WIDTH, BRICK_HEIGHT);
                cairo_fill(cr);

                // ������ �ؽ�Ʈ ǥ��
                cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);  // ������
                cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
                cairo_set_font_size(cr, 20.0);

                // �ؽ�Ʈ �߾� ������ ���� ���
                char durability_text[2];
                sprintf(durability_text, "%d", game_state.bricks[i][j]);

                cairo_text_extents_t extents;
                cairo_text_extents(cr, durability_text, &extents);

                double text_x = brick_x + (BRICK_WIDTH - extents.width) / 2;
                double text_y = brick_y + (BRICK_HEIGHT + extents.height) / 2;

                cairo_move_to(cr, text_x, text_y);
                cairo_show_text(cr, durability_text);
            }
        }
    }

    // ������ �׸���
    for (int i = 0; i < 20; i++) {
        if (game_state.items[i].active) {
            switch (game_state.items[i].type) {
            case ITEM_PADDLE_EXTEND:
                cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);  // �����
                break;
            case ITEM_MULTI_BALL:
                cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);  // �ʷϻ�
                break;
            case ITEM_EXTRA_LIFE:
                cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);  // ������
                break;
            }
            cairo_rectangle(cr,
                game_state.items[i].x - 7.5, game_state.items[i].y - 7.5,
                15, 15);  // 15x15 ũ���� ������
            cairo_fill(cr);
        }
    }

    return FALSE;
}