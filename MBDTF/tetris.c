#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "games.h"

#define GRID_WIDTH 10
#define GRID_HEIGHT 20

GtkLabel* tetris_score_label;  // ���� ������ score_label ����

int grid[GRID_HEIGHT][GRID_WIDTH] = { 0 }; // ������ ������
int current_tetromino[4][4];             // ���� ����
int current_x = GRID_WIDTH / 2 - 2;      // ������ X ��ġ
int current_y = 0;                       // ������ Y ��ġ
int game_speed = 500;                    // ���� �ӵ� (ms)
int score_Tetris = 0;                    // ����
bool game_over_Tetris = false;           // ���� ���� ����
int level_Tetris = 1;                    // ����

static guint game_loop_id = 0;
GtkWidget* next_block_area = NULL;


int next_tetromino[4][4]; // ���� ��� ����
int tetrominos[7][4][4] = {
    // I ���
    {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // O ���
    {{0, 1, 1, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // T ���
    {{0, 1, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // S ���
    {{0, 1, 1, 0}, {1, 1, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // Z ���
    {{1, 1, 0, 0}, {0, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // L ���
    {{1, 0, 0, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
    // J ���
    {{0, 0, 1, 0}, {1, 1, 1, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}} };


void clear_lines();                      // �� �� �� ����
void update_tetris_score(int score);     // ���� ����
void spawn_new_tetromino();              // ���ο� ���� ����
bool check_collision(int new_x, int new_y, int new_tetromino[4][4]);  // �浹 �˻�
void update_level_and_speed();           // ���� �� �ӵ� ������Ʈ
gboolean game_loop(GtkWidget* widget, gpointer data);  // ���� ����



// Realize �̺�Ʈ �ڵ鷯
void on_next_block_realize(GtkWidget* widget, gpointer data) {
    g_message("next_block_area is now realized.");
}

// �浹 �˻�
bool check_collision(int new_x, int new_y, int new_tetromino[4][4]) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (new_tetromino[y][x] == 1) {
                int grid_x = new_x + x;
                int grid_y = new_y + y;

                // �迭 ��� �ʰ� �˻�
                if (grid_x < 0 || grid_x >= GRID_WIDTH || grid_y < 0 || grid_y >= GRID_HEIGHT) {
                    return true; // �浹 �߻�
                }

                if (grid[grid_y][grid_x] != 0) {
                    return true; // �浹 �߻�
                }
            }
        }
    }
    return false;
}

// ���� ȸ��
void rotate_tetromino()
{
    int rotated[4][4] = { 0 };

    // �ð� ���� ȸ��
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            rotated[x][3 - y] = current_tetromino[y][x];
        }
    }

    if (!check_collision(current_x, current_y, rotated))
    {
        // ȸ�� �ݿ�
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                current_tetromino[y][x] = rotated[y][x];
            }
        }
    }
}

// ���ο� ���� ����
void spawn_new_tetromino() {
    memcpy(current_tetromino, next_tetromino, sizeof(current_tetromino));

    int random_index = rand() % 7;
    memcpy(next_tetromino, tetrominos[random_index], sizeof(next_tetromino));

    current_x = GRID_WIDTH / 2 - 2;
    current_y = 0;

    if (check_collision(current_x, current_y, current_tetromino)) {
        game_over_Tetris = true;
        return;
    }

    // Realize ���θ� Ȯ���� �� ȣ��
    if (GTK_IS_WIDGET(next_block_area) && gtk_widget_get_realized(next_block_area)) {
        gtk_widget_queue_draw(next_block_area); // ���� ��� ����
    }
    else {
        g_warning("spawn_new_tetromino: next_block_area not realized yet.");
    }
}

gboolean draw_game_board(GtkWidget* widget, cairo_t* cr, gpointer data)
{
    double cell_size = 30;

    // ���
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    // ������ ���
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x] != 0) {
                if (game_over_Tetris) {
                    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0); // ������
                }
                else {
                    cairo_set_source_rgb(cr, 0.0, 0.0, 0.5); // £�� �Ķ���
                }
                cairo_rectangle(cr, x * cell_size, y * cell_size, cell_size - 1, cell_size - 1);
                cairo_fill(cr);
            }
        }
    }

    // ���� ����
    if (!game_over_Tetris) {
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (current_tetromino[y][x] == 1) {
                    cairo_set_source_rgb(cr, 0.5, 0.8, 1.0); // �ϴû�
                    cairo_rectangle(cr, (current_x + x) * cell_size, (current_y + y) * cell_size, cell_size - 1, cell_size - 1);
                    cairo_fill(cr);
                }
            }
        }
    }

    // ���� ���� �޽���
    if (game_over_Tetris) {
        cairo_set_source_rgb(cr, 1.0, 0.0, 1.0); // ������
        cairo_set_font_size(cr, 40);
        cairo_move_to(cr, 50, 250);
        cairo_show_text(cr, "Game Over");
    }

    return FALSE;
}

gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    switch (event->keyval) {
    case GDK_KEY_Left: // ���� �̵�
        if (!check_collision(current_x - 1, current_y, current_tetromino)) {
            current_x--;
        }
        break;

    case GDK_KEY_Right: // ������ �̵�
        if (!check_collision(current_x + 1, current_y, current_tetromino)) {
            current_x++;
        }
        break;

    case GDK_KEY_Down: // ����Ʈ ��� (�� ĭ�� �Ʒ��� �̵�)
        if (!check_collision(current_x, current_y + 1, current_tetromino)) {
            current_y++;
        }
        else {
            // �浹 �� ���� ����
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    if (current_tetromino[y][x] == 1) {
                        grid[current_y + y][current_x + x] = 1;
                    }
                }
            }
            clear_lines(); // �� ����
            if (!game_over_Tetris) {
                spawn_new_tetromino(); // �� ���� ����
            }
        }
        break;

    case GDK_KEY_space: // �ϵ� ���
        while (!check_collision(current_x, current_y + 1, current_tetromino)) {
            current_y++; // �浹 ������ ������ �Ʒ��� �̵�
        }

        // ���� ����
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (current_tetromino[y][x] == 1) {
                    grid[current_y + y][current_x + x] = 1;
                }
            }
        }

        // ���� �߰� (�ϵ� ��� ���ʽ�)
        score_Tetris += 5; // �ϵ� ��� ���ʽ� ����
        update_tetris_score(score_Tetris);

        clear_lines(); // �� ����
        if (!game_over_Tetris) {
            spawn_new_tetromino(); // �� ���� ����
        }
        break;

    case GDK_KEY_Up: // ���� ȸ��
        rotate_tetromino();
        break;
    }

    gtk_widget_queue_draw(widget); // ȭ�� ���� ��û
    return TRUE;
}

void update_tetris_score(int score) {
    // tetris_score_label ��ȿ�� �˻�
    if (!GTK_IS_LABEL(tetris_score_label)) {
        g_warning("update_tetris_score: tetris_score_label�� ��ȿ���� �ʽ��ϴ�!");
        return;
    }

    // ��Ʈ���� ���ھ� �� ������Ʈ
    char score_text[16];
    snprintf(score_text, sizeof(score_text), "Score: %d", score);
    gtk_label_set_text(GTK_LABEL(tetris_score_label), score_text);

    // ���� �� �ӵ� ������Ʈ
    update_level_and_speed();

    // ���� �޴��� ���ھ� �� ����ȭ
    GtkWidget* main_menu_score_label = gtk_widget_get_parent(GTK_WIDGET(tetris_score_label));
    if (GTK_IS_LABEL(main_menu_score_label)) {
        gtk_label_set_text(GTK_LABEL(main_menu_score_label), score_text);
    }
    else {
        g_warning("update_tetris_score: main_menu_score_label�� ��ȿ���� �ʽ��ϴ�!");
    }

    // ������ ���� ����ȭ
    if (username[0] != '\0' && !is_guest_mode) {
        g_message("���� ����ȭ: %d", score);
        send_game_score(username, "tetris", score);
    }
    else {
        g_message("���� ����ȭ ���� (�Խ�Ʈ ��� Ȱ��ȭ)");
    }
}

void clear_lines() {
    int cleared_lines = 0;

    for (int y = GRID_HEIGHT - 1; y >= 0; y--) {
        bool full_line = true;

        for (int x = 0; x < GRID_WIDTH; x++) {
            if (grid[y][x] == 0) {
                full_line = false;
                break;
            }
        }

        if (full_line) {
            cleared_lines++;
            for (int ny = y; ny > 0; ny--) {
                for (int nx = 0; nx < GRID_WIDTH; nx++) {
                    grid[ny][nx] = grid[ny - 1][nx];
                }
            }
            for (int nx = 0; nx < GRID_WIDTH; nx++) {
                grid[0][nx] = 0;
            }

            y++; // �� �� ������ ��ŭ �ٽ� �˻�
        }
    }

    if (cleared_lines > 0) {
        score_Tetris += cleared_lines * 100;  // 1�� ������ 100��
        update_tetris_score(score_Tetris);  // ���� ����
    }
}

gboolean draw_next_tetromino(GtkWidget* widget, cairo_t* cr, gpointer data) {
    double cell_size = 20;

    // ����
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    // ���� ��� �׸���
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (next_tetromino[y][x] == 1) {
                cairo_set_source_rgb(cr, 0.5, 0.8, 1.0); // �ϴû�
                cairo_rectangle(cr, x * cell_size, y * cell_size, cell_size - 1, cell_size - 1);
                cairo_fill(cr);
            }
        }
    }

    return FALSE;
}

void initialize_tetris_ui(GtkWidget* score_label_widget, GtkWidget* next_block_widget) {
    if (!GTK_IS_LABEL(score_label_widget)) {
        g_warning("initialize_tetris_ui: score_label_widget�� ��ȿ���� �ʽ��ϴ�!");
        return;
    }
    gtk_label_set_text(GTK_LABEL(score_label_widget), "Score: 0");

    if (!GTK_IS_WIDGET(next_block_widget)) {
        g_warning("initialize_tetris_ui: next_block_widget�� ��ȿ���� �ʽ��ϴ�!");
        return;
    }
    gtk_widget_queue_draw(next_block_widget);
}

void update_level_and_speed() {
    int new_level = score_Tetris / 500 + 1;
    if (new_level > level_Tetris) {
        level_Tetris = new_level;
        game_speed = 500 - (level_Tetris - 1) * 50;
        if (game_speed < 100) {
            game_speed = 100;
        }

        if (game_loop_id > 0) {
            g_source_remove(game_loop_id);
        }
        game_loop_id = g_timeout_add(game_speed, (GSourceFunc)game_loop, NULL);

        g_message("���� ���! ���� ����: %d, �ӵ�: %dms", level_Tetris, game_speed);
    }
}








gboolean game_loop(GtkWidget* widget, gpointer data) {
    if (game_over_Tetris) {
        return FALSE; // ���� ���� ���¸� ���� ����
    }

    static gboolean is_soft_drop = FALSE;

    // ����Ʈ ����� Ȱ��ȭ�� ��� �� ������ �̵�
    if (is_soft_drop || !check_collision(current_x, current_y + 1, current_tetromino)) {
        current_y++;
    }
    else {
        // �浹 �߻� �� ���� ����
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                if (current_tetromino[y][x] == 1) {
                    grid[current_y + y][current_x + x] = 1;
                }
            }
        }
        clear_lines(); // �� ����
        spawn_new_tetromino(); // �� ���� ����

        if (game_over_Tetris) {
            return FALSE; // ���� ����
        }
    }

    gtk_widget_queue_draw(widget); // ���� ���� �ٽ� �׸���
    return TRUE; // ���� ����
}

void on_start_button_clicked(GtkWidget* widget, gpointer data) {
    if (game_loop_id > 0) {
        g_source_remove(game_loop_id);  // �ߺ� Ÿ�̸� ����
        game_loop_id = 0;
    }

    if (!GTK_IS_LABEL(tetris_score_label)) {
        return; // ���� ��ȿ���� ������ ����
    }

    // UI �ʱ�ȭ
    initialize_tetris_ui(GTK_WIDGET(tetris_score_label), next_block_area);

    // ������ �ʱ�ȭ
    memset(grid, 0, sizeof(grid));
    score_Tetris = 0;
    game_over_Tetris = false;

    // ù ��° ������ ���� ���� �ʱ�ȭ
    int random_index = rand() % 7;
    memcpy(next_tetromino, tetrominos[random_index], sizeof(next_tetromino));
    spawn_new_tetromino(); // ù ���� ����

    // ���� ���� ����
    game_loop_id = g_timeout_add(game_speed, (GSourceFunc)game_loop, data);

    // drawing_area�� ��Ŀ�� ����
    if (GTK_IS_WIDGET(data)) {
        gtk_widget_grab_focus(GTK_WIDGET(data));
    }
}

void update_ui(GtkWidget* drawing_area, GtkWidget* score_label) {
    if (GTK_IS_WIDGET(drawing_area)) {
        gtk_widget_queue_draw(drawing_area);
    }

    if (GTK_IS_LABEL(score_label)) {
        char score_text[16];
        snprintf(score_text, sizeof(score_text), "Score: %d", score_Tetris);
        gtk_label_set_text(GTK_LABEL(score_label), score_text);
    }
}

GtkWidget* create_tetris_screen(GtkStack* stack) {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);

    // ���� ǥ�� ��
    tetris_score_label = GTK_LABEL(gtk_label_new("Score: 0"));
    GtkWidget* score_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(score_container, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(score_container), GTK_WIDGET(tetris_score_label), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), score_container, FALSE, FALSE, 0);

    // ���� ����
    GtkWidget* drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, 300, 600);
    gtk_widget_set_can_focus(drawing_area, TRUE); // ��Ŀ�� �����ϵ��� ����
    g_signal_connect(drawing_area, "realize", G_CALLBACK(gtk_widget_grab_focus), NULL); // ��Ŀ�� ���� ����
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 5);

    // ���� ��� ǥ�� ����
    next_block_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(next_block_area, 100, 100);
    gtk_box_pack_start(GTK_BOX(vbox), next_block_area, FALSE, FALSE, 5);

    // Realize �̺�Ʈ �ڵ鷯 �߰�
    g_signal_connect(next_block_area, "realize", G_CALLBACK(on_next_block_realize), NULL);
    g_signal_connect(next_block_area, "draw", G_CALLBACK(draw_next_tetromino), NULL);

    // Start ��ư
    GtkWidget* start_button = gtk_button_new_with_label("Start");
    gtk_widget_set_can_focus(start_button, FALSE); // ��ư ��Ŀ�� ����
    gtk_box_pack_start(GTK_BOX(vbox), start_button, FALSE, FALSE, 5);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_button_clicked), drawing_area);

    // Back ��ư
    GtkWidget* back_button = gtk_button_new_with_label("Back to Main Menu");
    gtk_widget_set_can_focus(back_button, FALSE); // ��ư ��Ŀ�� ����
    gtk_box_pack_start(GTK_BOX(vbox), back_button, FALSE, FALSE, 5);
    g_signal_connect(back_button, "clicked", G_CALLBACK(switch_to_main_menu), stack);

    g_signal_connect(drawing_area, "draw", G_CALLBACK(draw_game_board), NULL);
    g_signal_connect(drawing_area, "key-press-event", G_CALLBACK(on_key_press), stack);

    return vbox;
}


void start_tetris_game(GtkWidget* widget, gpointer data) {
    GtkStack* stack = GTK_STACK(data);
    gtk_stack_set_visible_child_name(stack, "tetris_screen");
}

