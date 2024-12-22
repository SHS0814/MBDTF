#pragma warning(disable : 4819)

#define _CRT_SECURE_NO_WARNINGS
#include <gtk/gtk.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "games.h"

#define TILE_SIZE 100
#define TILE_MARGIN 10

GtkLabel* score_label;  // ���� ǥ�� ��
int score = 0;          // ���� ������ score ����

int swap_count = 2;    // ���� ��ư ��� ���� Ƚ��
int remove_count = 1;  // ���� ��ư ��� ���� Ƚ��

bool swap_mode = false; // ��ȯ ��� Ȱ��ȭ ����
int selected_tiles[2][2] = { {-1, -1}, {-1, -1} }; // ���õ� �� Ÿ���� ��ǥ
int selected_count = 0; // ���õ� Ÿ�� ����

bool remove_mode = false; // ���� ��� Ȱ��ȭ ����
GtkWidget* button_container; // ��ư �����̳ʸ� ���� ������ ����

// �Լ� ������Ÿ�� �߰�
void on_swap_button_clicked(GtkWidget* widget, gpointer data);
gboolean on_tile_click(GtkWidget* widget, GdkEventButton* event, gpointer data);
void on_remove_button_clicked(GtkWidget* widget, gpointer data);

void update_score() {
    if (GTK_IS_LABEL(score_label)) {
        char score_text[50];
        sprintf(score_text, "Score: %d", score);
        gtk_label_set_text(score_label, score_text);
        while (g_main_context_iteration(NULL, FALSE));  // Force the update to be drawn immediately
    }
}

// ���� and �����ͱ�������
GtkWidget* drawing_area;
int** grid;
int grid_size = 4;

// �׸��� �ʱ�ȭ �Լ�
void initialize_grid(int size) {
    grid = (int**)malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++) {
        grid[i] = (int*)malloc(size * sizeof(int));
        for (int j = 0; j < size; j++) {
            grid[i][j] = 0;  // �ʱ�ȭ
        }
    }
}

// ���� �׸��� �޸� ����
void free_grid(int size) {
    for (int i = 0; i < size; i++) {
        free(grid[i]);
    }
    free(grid);
}

// ���ο� Ÿ�� ����
void add_random_tile() {
    int** empty_tiles = (int**)malloc(grid_size * grid_size * sizeof(int*));
    int empty_count = 0;

    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (grid[i][j] == 0) {
                empty_tiles[empty_count] = malloc(2 * sizeof(int));
                empty_tiles[empty_count][0] = i;
                empty_tiles[empty_count][1] = j;
                empty_count++;
            }
        }
    }

    if (empty_count > 0) {
        int* tile = empty_tiles[rand() % empty_count];
        int random_value = (rand() % 10 < 9) ? 2 : 4;
        grid[tile[0]][tile[1]] = random_value;
    }
    // ���� �޸� ����
    for (int i = 0; i < empty_count; i++) {
        free(empty_tiles[i]);
    }
    free(empty_tiles);
}

// Ÿ�� �� ����
void set_tile_color(cairo_t* cr, int value) {
    switch (value) {
    case 2:
        cairo_set_source_rgb(cr, 0.93, 0.89, 0.85);
        break;
    case 4:
        cairo_set_source_rgb(cr, 0.93, 0.87, 0.78);
        break;
    case 8:
        cairo_set_source_rgb(cr, 0.96, 0.64, 0.38);
        break;
    case 16:
        cairo_set_source_rgb(cr, 0.96, 0.58, 0.38);
        break;
    case 32:
        cairo_set_source_rgb(cr, 0.96, 0.48, 0.38);
        break;
    case 64:
        cairo_set_source_rgb(cr, 0.96, 0.38, 0.28);
        break;
    case 128:
        cairo_set_source_rgb(cr, 0.93, 0.81, 0.45);
        break;
    case 256:
        cairo_set_source_rgb(cr, 0.93, 0.80, 0.38);
        break;
    case 512:
        cairo_set_source_rgb(cr, 0.93, 0.78, 0.28);
        break;
    case 1024:
        cairo_set_source_rgb(cr, 0.93, 0.76, 0.18);
        break;
    case 2048:
        cairo_set_source_rgb(cr, 0.93, 0.75, 0.08);
        break;
    default:
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
        break;
    }
}
// Ÿ�� �׸���
gboolean on_draw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);  // ����
    cairo_paint(cr);

    // �ܰ��� �׸���
    cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);  // �ܰ��� �� (������)
    cairo_set_line_width(cr, 5);
    cairo_rectangle(cr, TILE_MARGIN, TILE_MARGIN,
        grid_size * TILE_SIZE + (grid_size - 1) * TILE_MARGIN,
        grid_size * TILE_SIZE + (grid_size - 1) * TILE_MARGIN);
    cairo_stroke(cr);

    // Ÿ�� �׸���
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            int value = grid[i][j];
            set_tile_color(cr, value);

            cairo_rectangle(cr, TILE_MARGIN + j * (TILE_SIZE + TILE_MARGIN),
                TILE_MARGIN + i * (TILE_SIZE + TILE_MARGIN),
                TILE_SIZE, TILE_SIZE);
            cairo_fill(cr);

            if (value != 0) {
                cairo_set_source_rgb(cr, 0, 0, 0);  // ���� ���� (������)
                cairo_set_font_size(cr, 24);
                cairo_move_to(cr, TILE_MARGIN + j * (TILE_SIZE + TILE_MARGIN) + TILE_SIZE / 3,
                    TILE_MARGIN + i * (TILE_SIZE + TILE_MARGIN) + TILE_SIZE / 1.5);
                cairo_show_text(cr, g_strdup_printf("%d", value));
            }
        }
    }
    return FALSE;
}

// Ÿ�� �̵� �� ��ġ�� �Լ�
int move_tiles(int dx, int dy) {
    int moved = 0;
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            int x = (dx == 1) ? (grid_size - 1 - i) : i;
            int y = (dy == 1) ? (grid_size - 1 - j) : j;

            if (grid[x][y] != 0) {
                int nx = x, ny = y;
                while (nx + dx >= 0 && nx + dx < grid_size && ny + dy >= 0 && ny + dy < grid_size && grid[nx + dx][ny + dy] == 0) {
                    nx += dx;
                    ny += dy;
                }

                if (nx + dx >= 0 && nx + dx < grid_size && ny + dy >= 0 && ny + dy < grid_size && grid[nx + dx][ny + dy] == grid[x][y]) {
                    grid[nx + dx][ny + dy] *= 2;
                    score += grid[nx + dx][ny + dy];
                    grid[x][y] = 0;
                    moved = 1;
                }
                else if (nx != x || ny != y) {
                    grid[nx][ny] = grid[x][y];
                    grid[x][y] = 0;
                    moved = 1;
                }
            }
        }
    }

    if (moved) {
        update_score();
    }
    return moved;
}

bool is_game_over() {
    // �� ĭ Ȯ��
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            if (grid[i][j] == 0) {
                return false;  // �� ĭ ����
            }

            // ���� Ÿ�� ��ĥ �� �ִ��� Ȯ��
            if (i > 0 && grid[i][j] == grid[i - 1][j]) return false;              // ��
            if (i < grid_size - 1 && grid[i][j] == grid[i + 1][j]) return false;  // �Ʒ�
            if (j > 0 && grid[i][j] == grid[i][j - 1]) return false;              // ����
            if (j < grid_size - 1 && grid[i][j] == grid[i][j + 1]) return false;  // ������
        }
    }
    return true;  // �� �̻� ������ �� ����
}

gboolean on_key_press1(GtkWidget* widget, GdkEventKey* event, gpointer data) {
    int moved = 0;

    switch (event->keyval) {
    case GDK_KEY_Left:
        moved = move_tiles(0, -1);
        break;
    case GDK_KEY_Right:
        moved = move_tiles(0, 1);
        break;
    case GDK_KEY_Up:
        moved = move_tiles(-1, 0);
        break;
    case GDK_KEY_Down:
        moved = move_tiles(1, 0);
        break;
    }

    if (moved) {
        add_random_tile();
        if (GTK_IS_WIDGET(drawing_area) && gtk_widget_get_visible(drawing_area)) {
            gtk_widget_queue_draw(drawing_area);  // Queue a redraw of the drawing area
        }
        if (GTK_IS_LABEL(score_label)) {
            update_score();  // Update the score label
        }

        // ���� ���� üũ �� ���� ����
        if (is_game_over()) {
            printf("Game Over! Final Score: %d\n", score);

            // ������ ���� ����
            send_game_score(username, "2048", score);

            // �޽��� �ڽ� ǥ��
            GtkWidget* dialog = gtk_message_dialog_new(
                NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "Game Over!\nFinal Score: %d", score);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);

            // ���� �޴��� �̵�
            GtkStack* stack = GTK_STACK(data);
            switch_to_main_menu(NULL, stack);
        }
    }

    return TRUE;
}

// ���� ���� ���� üũ �Լ�

GtkWidget* create_2048_screen(GtkStack* stack) {
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(vbox, GTK_ALIGN_CENTER);  // VBox�� ��� ����
    gtk_widget_set_valign(vbox, GTK_ALIGN_CENTER);

    // ���� ǥ�� ��
    score_label = GTK_LABEL(gtk_label_new("Score: 0"));
    GtkWidget* score_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(score_container, GTK_ALIGN_CENTER);  // ���� ���� ��� ����
    gtk_box_pack_start(GTK_BOX(score_container), GTK_WIDGET(score_label), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), score_container, FALSE, FALSE, 0);

    // ��ư �����̳�
    button_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_halign(button_container, GTK_ALIGN_CENTER);

    // Swap ��ư
    GtkWidget* swap_button = gtk_button_new_with_label("Swap Tiles");
    gtk_box_pack_start(GTK_BOX(button_container), swap_button, TRUE, TRUE, 0);
    g_signal_connect(swap_button, "clicked", G_CALLBACK(on_swap_button_clicked), NULL);

    // Remove ��ư
    GtkWidget* remove_button = gtk_button_new_with_label("Remove Tile");
    gtk_box_pack_start(GTK_BOX(button_container), remove_button, TRUE, TRUE, 0);
    g_signal_connect(remove_button, "clicked", G_CALLBACK(on_remove_button_clicked), NULL);

    // 2048 ���� ���� (DrawingArea)
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area, grid_size * TILE_SIZE + (grid_size + 1) * TILE_MARGIN,
        grid_size * TILE_SIZE + (grid_size + 1) * TILE_MARGIN);
    gtk_box_pack_start(GTK_BOX(vbox), drawing_area, TRUE, TRUE, 0);

    // ��Ŀ���� ���� �� �ֵ��� �����ϰ� �̺�Ʈ ����ũ �߰�
    gtk_widget_set_can_focus(drawing_area, TRUE);
    gtk_widget_add_events(drawing_area, GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK);

    // DrawingArea�� �̺�Ʈ ����
    g_signal_connect(drawing_area, "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(drawing_area, "key-press-event", G_CALLBACK(on_key_press1), stack);
    g_signal_connect(drawing_area, "realize", G_CALLBACK(gtk_widget_grab_focus), NULL);
    g_signal_connect(drawing_area, "button-press-event", G_CALLBACK(on_tile_click), NULL);

    // Drawing Area�� ��Ŀ�� ����
    if (GTK_IS_WIDGET(drawing_area)) {
        gtk_widget_grab_focus(drawing_area);
    }

    // ȭ�� ������Ʈ ��û
    if (GTK_IS_WIDGET(drawing_area)) {
        gtk_widget_queue_draw(drawing_area);
    }

    // �ڷΰ��� ��ư
    GtkWidget* back_button = gtk_button_new_with_label("Back to Main Menu");
    g_signal_connect(back_button, "clicked", G_CALLBACK(switch_to_main_menu), stack);
    gtk_box_pack_start(GTK_BOX(button_container), back_button, TRUE, TRUE, 0);

    // ��ư �����̳� �߰�
    gtk_box_pack_start(GTK_BOX(vbox), button_container, FALSE, FALSE, 0);


    // 2048 ���� �ʱ�ȭ
    initialize_grid(grid_size);
    add_random_tile();
    add_random_tile();

    return vbox;
}

void start_2048_game(GtkWidget* widget, gpointer data) {
    GtkStack* stack = GTK_STACK(data);

    // 2048 ���� �ʱ�ȭ
    score = 0;
    initialize_grid(grid_size);
    add_random_tile();
    add_random_tile();

    // ���� ��� ��� Ƚ�� �ʱ�ȭ
    swap_count = 2;
    remove_count = 1;

    // 2048 ȭ������ ��ȯ
    gtk_stack_set_visible_child_name(stack, "2048_screen");

    // Drawing Area�� ��Ŀ�� ����
    if (GTK_IS_WIDGET(drawing_area)) {
        gtk_widget_grab_focus(drawing_area);
    }

    // ȭ�� ������Ʈ ��û
    if (GTK_IS_WIDGET(drawing_area)) {
        gtk_widget_queue_draw(drawing_area);
    }
}
gboolean on_tile_click(GtkWidget* widget, GdkEventButton* event, gpointer data) {
    // Ŭ���� ��ġ�� ���
    int x = (event->x - TILE_MARGIN) / (TILE_SIZE + TILE_MARGIN);
    int y = (event->y - TILE_MARGIN) / (TILE_SIZE + TILE_MARGIN);

    // ���� ���� Ȯ��
    if (x >= 0 && x < grid_size && y >= 0 && y < grid_size) {
        if (remove_mode) {
            // ���� ����� ��
            if (grid[y][x] != 0) {
                grid[y][x] = 0; // Ÿ�� ����
                printf("Tile at (%d, %d) removed.\n", y, x);

                // ���� ����
                gtk_widget_queue_draw(drawing_area);

                // ���� ��� ����
                remove_mode = false;
            }
            else {
                printf("Tile at (%d, %d) is already empty.\n", y, x);
            }
        }
        else if (swap_mode) {
            // ���� ��ȯ ��� ���� (�״�� ����)
            selected_tiles[selected_count][0] = y;
            selected_tiles[selected_count][1] = x;
            selected_count++;

            if (selected_count == 2) {
                // Ÿ�� �� ��ȯ
                int temp = grid[selected_tiles[0][0]][selected_tiles[0][1]];
                grid[selected_tiles[0][0]][selected_tiles[0][1]] = grid[selected_tiles[1][0]][selected_tiles[1][1]];
                grid[selected_tiles[1][0]][selected_tiles[1][1]] = temp;

                // ���� ����
                gtk_widget_queue_draw(drawing_area);

                // �ʱ�ȭ �� ��� ����
                swap_mode = false;
                selected_count = 0;
                printf("Tiles swapped: (%d, %d) <-> (%d, %d)\n",
                    selected_tiles[0][0], selected_tiles[0][1],
                    selected_tiles[1][0], selected_tiles[1][1]);
            }
        }
    }
    return FALSE;
}

void on_remove_button_clicked(GtkWidget* widget, gpointer data) {
    if (remove_count > 0) {  // ���� Ƚ�� Ȯ��
        if (!remove_mode) {
            remove_mode = true;
            printf("Remove mode activated! Click a tile to remove.\n");
        }
        else {
            remove_mode = false;
            printf("Remove mode deactivated.\n");
        }
        remove_count--;  // ��� Ƚ�� ����
        printf("Remove uses left: %d\n", remove_count);
    }
    else {
        printf("No more removals available!\n");
    }
}

void on_swap_button_clicked(GtkWidget* widget, gpointer data) {
    if (swap_count > 0) {  // ���� Ƚ�� Ȯ��
        if (!swap_mode) {
            swap_mode = true;
            selected_count = 0;
            for (int i = 0; i < 2; i++) {
                selected_tiles[i][0] = -1;
                selected_tiles[i][1] = -1;
            }
            printf("Swap mode activated! Select two tiles.\n");
        }
        else {
            swap_mode = false;
            printf("Swap mode deactivated.\n");
        }
        swap_count--;  // ��� Ƚ�� ����
        printf("Swap uses left: %d\n", swap_count);
    }
    else {
        printf("No more swaps available!\n");
    }
}