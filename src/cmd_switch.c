/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika
 * Copyright (C) 2001-2019, TABATA Keiichi. All rights reserved.
 */

/*
 * @choose, @select, @switchコマンド
 *
 * [Changes]
 *  - 2017/08/14 作成
 *  - 2017/10/31 効果音に対応
 *  - 2019/09/17 NEWSに対応
 *  - 2022/06/17 @chooseに対応、@newsの設定項目を@switchに統合
 */

#include "suika.h"

#define ASSERT_INVALID_BTN_INDEX (0)

/* 親ボタンの最大数 */
#define PARENT_COUNT		(8)

/* 親ボタン1つあたりの子ボタンの最大数 */
#define CHILD_COUNT		(8)

/* @selectの選択肢の数 */
#define SELECT_OPTION_COUNT	(3)

/* コマンドの引数 */
#define PARENT_MESSAGE(n)	(SWITCH_PARAM_PARENT_M1 + n)
#define CHILD_LABEL(p,c)	(SWITCH_PARAM_CHILD1_L1 + 16 * p + 2 * c)
#define CHILD_MESSAGE(p,c)	(SWITCH_PARAM_CHILD1_M1 + 16 * p + 2 * c)
#define CHOOSE_LABEL(n)		(CHOOSE_PARAM_LABEL1 + n * 2)
#define CHOOSE_MESSAGE(n)	(CHOOSE_PARAM_LABEL1 + n * 2 + 1)
#define SELECT_LABEL(n)		(SELECT_PARAM_LABEL1 + n)
#define SELECT_MESSAGE(n)	(SELECT_PARAM_TEXT1 + n)

/* NEWSコマンド時のswitchボックス開始オフセット */
#define SWITCH_BASE	(4)

/* 指定した親選択肢が無効であるか */
#define IS_PARENT_DISABLED(n)	(parent_button[n].msg == NULL)

/* システムメニューのボタンのインデックス */
#define SYSMENU_NONE	(-1)
#define SYSMENU_QSAVE	(0)
#define SYSMENU_QLOAD	(1)
#define SYSMENU_SAVE	(2)
#define SYSMENU_LOAD	(3)
#define SYSMENU_AUTO	(4)
#define SYSMENU_SKIP	(5)
#define SYSMENU_HISTORY	(6)
#define SYSMENU_CONFIG	(7)

/* 親選択肢のボタン */
static struct parent_button {
	const char *msg;
	const char *label;
	bool has_child;
	int child_count;
	int x;
	int y;
	int w;
	int h;
} parent_button[PARENT_COUNT];

/* 子選択肢のボタン */
static struct child_button {
	const char *msg;
	const char *label;
	int x;
	int y;
	int w;
	int h;
} child_button[PARENT_COUNT][CHILD_COUNT];

/* 最初の描画であるか */
static bool is_first_frame;

/* 子選択肢の最初の描画であるか */
static bool is_child_first_frame;

/* ポイントされている親項目のインデックス */
static int pointed_parent_index;

/* 選択されているされている親項目のインデックス */
static int selected_parent_index;

/* ポイントされている子項目のインデックス */
static int pointed_child_index;

/* クイックロードを行ったか */
static bool did_quick_load;

/* セーブモードに遷移するか */
static bool need_save_mode;

/* ロードモードに遷移するか */
static bool need_load_mode;

/* ヒストリモードに遷移するか */
static bool need_history_mode;

/* コフィグモードに遷移するか */
static bool need_config_mode;

/* システムメニューを表示中か */
static bool is_sysmenu;

/* システムメニューの最初のフレームか */
static bool is_sysmenu_first_frame;

/* システムメニューのどのボタンがポイントされているか */
static int sysmenu_pointed_index;

/* システムメニューのどのボタンがポイントされていたか */
static int old_sysmenu_pointed_index;

/* システムメニューが終了した直後か */
static bool is_sysmenu_finished;

/* 折りたたみシステムメニューが前のフレームでポイントされていたか */
static bool is_collapsed_sysmenu_pointed_prev;

/* 前方参照 */
static bool init(void);
static bool get_choose_info(void);
static bool get_select_info(void);
static bool get_parents_info(void);
static bool get_children_info(void);
static void draw_frame(int *x, int *y, int *w, int *h);
static void draw_frame_parent(int *x, int *y, int *w, int *h);
static void draw_frame_child(int *x, int *y, int *w, int *h);
static int get_pointed_parent_index(void);
static int get_pointed_child_index(void);
static int get_sysmenu_pointed_button(void);
static void get_sysmenu_button_rect(int btn, int *x, int *y, int *w, int *h);
static void draw_fo_fi_parent(void);
static void draw_switch_parent_images(void);
static void update_switch_parent(int *x, int *y, int *w, int *h);
static void draw_fo_fi_child(void);
static void draw_switch_child_images(void);
static void update_switch_child(int *x, int *y, int *w, int *h);
static void draw_text(int x, int y, int w, const char *t, bool is_news);
static void draw_keep(void);
static void process_sysmenu_click(void);
static void process_main_click(void);
static void draw_sysmenu(int *x, int *y, int *w, int *h);
static void draw_collapsed_sysmenu(int *x, int *y, int *w, int *h);
static bool is_collapsed_sysmenu_pointed(void);
static void play_se(const char *file);
static bool cleanup(void);

/*
 * switchコマンド
 */
bool switch_command(int *x, int *y, int *w, int *h)
{
	/* 初期化処理を行う */
	if (!is_in_command_repetition())
		if (!init())
			return false;

	/* モードが変更されるクリックを処理する */
	if (!is_sysmenu)
		process_main_click();
	else
		process_sysmenu_click();

	/* 描画を行う */
	if (!did_quick_load) {
		draw_frame(x, y, w, h);
		is_sysmenu_finished = false;
	}

	/* クイックロード・セーブ・ロード・ヒストリが選択された場合 */
	if (did_quick_load || need_save_mode || need_load_mode ||
	    need_history_mode || need_config_mode)
		stop_command_repetition();

	/* 終了処理を行う */
	if (!is_in_command_repetition())
		if (!cleanup())
			return false;

	/* セーブ・ロード・ヒストリ画面に移行する */
	if (need_save_mode) {
		draw_stage_fo_thumb();
		if (!prepare_gui_mode(SAVE_GUI_FILE, true, true))
			return false;
		start_gui_mode();
	}
	if (need_load_mode) {
		if (!prepare_gui_mode(LOAD_GUI_FILE, true, true))
			return false;
		start_gui_mode();
	}
	if (need_history_mode) {
		if (!prepare_gui_mode(HISTORY_GUI_FILE, true, true))
			return false;
		start_gui_mode();
	}
	if (need_config_mode) {
		if (!prepare_gui_mode(CONFIG_GUI_FILE, true, true))
			return false;
		start_gui_mode();
	}

	return true;
}

/* コマンドの初期化処理を行う */
bool init(void)
{
	int type;

	start_command_repetition();

	is_first_frame = true;
	pointed_parent_index = -1;
	selected_parent_index = -1;
	pointed_child_index = -1;
	is_child_first_frame = false;
	is_sysmenu = false;
	did_quick_load = false;
	need_save_mode = false;
	need_load_mode = false;
	need_history_mode = false;
	need_config_mode = false;

	type = get_command_type();
	if (type == COMMAND_CHOOSE) {
		/* @chooseコマンドの引数情報を取得する */
		if (!get_choose_info())
			return false;
	} else if (type == COMMAND_SELECT) {
		/* @selectコマンドの引数情報を取得する */
		if (!get_select_info())
			return false;
	} else {
		/* 親選択肢の情報を取得する */
		if (!get_parents_info())
			return false;

		/* 子選択肢の情報を取得する */
		if (!get_children_info())
			return false;
	}

	/* 名前ボックス、メッセージボックスを非表示にする */
	show_namebox(false);
	show_msgbox(false);

	/* オートモードを終了する */
	if (is_auto_mode()) {
		stop_auto_mode();
		show_automode_banner(false);
	}

	/* スキップモードを終了する */
	if (is_skip_mode()) {
		stop_skip_mode();
		show_skipmode_banner(false);
	}

	return true;
}

/* @chooseコマンドの引数情報を取得する */
static bool get_choose_info(void)
{
	const char *label, *msg;
	int i;

	memset(parent_button, 0, sizeof(parent_button));
	memset(child_button, 0, sizeof(child_button));

	/* 選択肢の情報を取得する */
	for (i = 0; i < PARENT_COUNT; i++) {
		/* ラベルを取得する */
		label = get_string_param(CHOOSE_LABEL(i));
		if (strcmp(label, "") == 0)
			break;

		/* メッセージを取得する */
		msg = get_string_param(CHOOSE_MESSAGE(i));
		if (strcmp(msg, "") == 0) {
			log_script_choose_no_message();
			log_script_exec_footer();
			return false;
		}

		/* ボタンの情報を保存する */
		parent_button[i].msg = msg;
		parent_button[i].label = label;
		parent_button[i].has_child = false;
		parent_button[i].child_count = 0;

		/* 座標を計算する */
		get_switch_rect(i, &parent_button[i].x,
				&parent_button[i].y,
				&parent_button[i].w,
				&parent_button[i].h);
	}

	/* 最後のメッセージとして保存する */
	if (!set_last_message(parent_button[0].msg))
		return false;

	return true;
}

/* @selectコマンドの引数情報を取得する */
static bool get_select_info(void)
{
	const char *label, *msg;
	int i;

	memset(parent_button, 0, sizeof(parent_button));
	memset(child_button, 0, sizeof(child_button));

	/* 選択肢の情報を取得する */
	for (i = 0; i < SELECT_OPTION_COUNT; i++) {
		/* ラベルを取得する */
		label = get_string_param(SELECT_LABEL(i));

		/* メッセージを取得する */
		msg = get_string_param(SELECT_MESSAGE(i));

		/* ボタンの情報を保存する */
		parent_button[i].msg = msg;
		parent_button[i].label = label;
		parent_button[i].has_child = false;
		parent_button[i].child_count = 0;

		/* 座標を計算する */
		get_switch_rect(i, &parent_button[i].x,
				&parent_button[i].y,
				&parent_button[i].w,
				&parent_button[i].h);
	}

	/* 最後のメッセージとして保存する */
	if (!set_last_message(parent_button[0].msg))
		return false;

	return true;
}

/* 親選択肢の情報を取得する */
static bool get_parents_info(void)
{
	const char *p;
	int i, parent_button_count = 0;
	bool is_first;

	memset(parent_button, 0, sizeof(parent_button));

	/* 親選択肢の情報を取得する */
	is_first = true;
	for (i = 0; i < PARENT_COUNT; i++) {
		/* 親選択肢のメッセージを取得する */
		p = get_string_param(PARENT_MESSAGE(i));
		assert(strcmp(p, "") != 0);

		/* @switchの場合、"*"が現れたら親選択肢の読み込みを停止する */
		if (get_command_type() == COMMAND_SWITCH) {
			if (strcmp(p, "*") == 0)
				break;
		} else {
			/* @newsの場合、"*"が現れたら選択肢をスキップする */
			if (strcmp(p, "*") == 0)
				continue;
		}

		/* メッセージを保存する */
		parent_button[i].msg = p;
		if (is_first) {
			/* 最後のメッセージとして保存する */
			if (!set_last_message(p))
				return false;
			is_first = false;
		}

		/* ラベルがなければならない */
		p = get_string_param(CHILD_LABEL(i, 0));
		if (strcmp(p, "*") == 0 || strcmp(p, "") == 0) {
			log_script_switch_no_label();
			log_script_exec_footer();
			return false;
		}

		/* 子の最初のメッセージが"*"か省略なら、一階層のメニューと
		   判断してラベルを取得する */
		p = get_string_param(CHILD_MESSAGE(i, 0));
		if (strcmp(p, "*") == 0 || strcmp(p, "") == 0) {
			p = get_string_param(CHILD_LABEL(i, 0));
			parent_button[i].label = p;
			parent_button[i].has_child = false;
			parent_button[i].child_count = 0;
		} else {
			parent_button[i].label = NULL;
			parent_button[i].has_child = true;
			parent_button[i].child_count = 0;
		}

		/* 座標を計算する */
		if (get_command_type() == COMMAND_SWITCH) {
			get_switch_rect(i, &parent_button[i].x,
					&parent_button[i].y,
					&parent_button[i].w,
					&parent_button[i].h);
		} else {
			get_news_rect(i, &parent_button[i].x,
				      &parent_button[i].y,
				      &parent_button[i].w,
				      &parent_button[i].h);
		}

		parent_button_count++;
	}
	if (parent_button_count == 0) {
		log_script_switch_no_item();
		log_script_exec_footer();
		return false;
	}

	return true;
}

/* 子選択肢の情報を取得する */
static bool get_children_info(void)
{
	const char *p;
	int i, j;

	memset(child_button, 0, sizeof(child_button));

	/* 子選択肢の情報を取得する */
	for (i = 0; i < PARENT_COUNT; i++) {
		/* 親選択肢が無効の場合、スキップする */
		if (IS_PARENT_DISABLED(i))
			continue;

		/* 親選択肢が子選択肢を持たない場合、スキップする */
		if (!parent_button[i].has_child)
			continue;

		/* 子選択肢の情報を取得する */
		for (j = 0; j < CHILD_COUNT; j++) {
			/* ラベルを取得し、"*"か省略が現れたらスキップする */
			p = get_string_param(CHILD_LABEL(i, j));
			if (strcmp(p, "*") == 0 || strcmp(p, "") == 0)
				break;
			child_button[i][j].label = p;

			/* メッセージを取得する */
			p = get_string_param(CHILD_MESSAGE(i, j));
			if (strcmp(p, "*") == 0 || strcmp(p, "") == 0) {
				log_script_switch_no_item();
				log_script_exec_footer();
				return false;
			}
			child_button[i][j].msg = p;

			/* 座標を計算する */
			get_switch_rect(j, &child_button[i][j].x,
					&child_button[i][j].y,
					&child_button[i][j].w,
					&child_button[i][j].h);
		}
		assert(j > 0);
		parent_button[i].child_count = j;
	}

	return true;
}

/* フレームを描画する */
static void draw_frame(int *x, int *y, int *w, int *h)
{
	*x = 0;
	*y = 0;
	*w = 0;
	*h = 0;

	/* セーブ画面かヒストリ画面から復帰した場合のフラグをクリアする */
	check_gui_flag();

	/* 初回描画の場合 */
	if (is_first_frame) {
		pointed_parent_index = get_pointed_parent_index();

		/* 親選択肢の描画を行う */
		draw_fo_fi_parent();
		update_switch_parent(x, y, w, h);

		/* 名前ボックス、メッセージボックスを消すため再描画する */
		*x = 0;
		*y = 0;
		*w = conf_window_width;
		*h = conf_window_height;

		is_first_frame = false;
		return;
	}

	/* システムメニューを表示中でない場合 */
	if (!is_sysmenu) {
		if (selected_parent_index == -1) {
			/* 親選択肢を選んでいる最中の場合 */
			draw_frame_parent(x, y, w, h);
		} else {
			/* 子選択肢を選んでいる最中の場合 */
			draw_frame_child(x, y, w, h);
		}

		/* 折りたたみシステムメニューを表示する */
		draw_collapsed_sysmenu(x, y, w, h);
	}

	/* システムメニューを表示中の場合 */
	if (is_sysmenu)
		draw_sysmenu(x, y, w, h);

	/* システムメニューを終了した直後の場合 */
	if (is_sysmenu_finished) {
		update_switch_parent(x, y, w, h);
		draw_collapsed_sysmenu(x, y, w, h);
	}
}

/* 親選択肢の描画を行う */
static void draw_frame_parent(int *x, int *y, int *w, int *h)
{
	int new_pointed_index;

	new_pointed_index = get_pointed_parent_index();

	if (new_pointed_index == -1 && pointed_parent_index == -1) {
		draw_keep();
	} else if (new_pointed_index == pointed_parent_index) {
		draw_keep();
	} else {
		/* ボタンを描画する */
		pointed_parent_index = new_pointed_index;
		update_switch_parent(x, y, w, h);

		/* SEを再生する */
		if (new_pointed_index != -1 && !is_left_clicked &&
		    !is_sysmenu_finished) {
			play_se(get_command_type() == COMMAND_NEWS ?
				conf_news_change_se : conf_switch_change_se);
		}
	}

	/* マウスの左ボタンでクリックされた場合 */
	if (new_pointed_index != -1 && is_left_clicked &&
	    !is_sysmenu_finished) {
		selected_parent_index = new_pointed_index;
		pointed_child_index = -1;
		is_child_first_frame = true;

		if (parent_button[new_pointed_index].has_child) {
			/* 子選択肢の描画を行う */
			draw_fo_fi_child();
			update_switch_child(x, y, w, h);

			/* SEを鳴らす */
			play_se(conf_switch_parent_click_se_file);
		} else {
			/* ステージをボタンなしで描画しなおす */
			draw_stage();

			/* SEを鳴らす */
			play_se(conf_switch_child_click_se_file);

			/* 繰り返し動作を終了する */
			stop_command_repetition();
		}

		/* ステージ全体を再描画する */
		*x = 0;
		*y = 0;
		*w = conf_window_width;
		*h = conf_window_height;
	}
}

/* 子選択肢の描画を行う */
static void draw_frame_child(int *x, int *y, int *w, int *h)
{
	int new_pointed_index;

	if (is_right_button_pressed) {
		selected_parent_index = -1;

		/* 親選択肢の描画を行う */
		draw_fo_fi_parent();
		update_switch_parent(x, y, w, h);
		return;
	}

	new_pointed_index = get_pointed_child_index();

	if (is_child_first_frame) {
		is_child_first_frame = false;

		/* ボタンを描画する */
		pointed_child_index = new_pointed_index;
		update_switch_child(x, y, w, h);
	} else if (new_pointed_index == -1 && pointed_child_index == -1) {
		draw_keep();
	} else if (new_pointed_index == pointed_child_index) {
		draw_keep();
	} else {
		/* ボタンを描画する */
		pointed_child_index = new_pointed_index;
		update_switch_child(x, y, w, h);

		/* SEを再生する */
		if (new_pointed_index != -1 && !is_left_clicked)
			play_se(conf_switch_change_se);
	}

	/* マウスの左ボタンでクリックされた場合 */
	if (new_pointed_index != -1 && is_left_clicked) {
		/* SEを鳴らす */
		play_se(conf_switch_child_click_se_file);

		/* ステージをボタンなしで描画しなおす */
		draw_stage();
		*x = 0;
		*y = 0;
		*w = conf_window_width;
		*h = conf_window_height;

		/* 繰り返し動作を終了する */
		stop_command_repetition();
	}
}

/* 親選択肢でポイントされているものを取得する */
static int get_pointed_parent_index(void)
{
	int i;

	/* システムメニュー表示中は選択しない */
	if (is_sysmenu)
		return -1;

	for (i = 0; i < PARENT_COUNT; i++) {
		if (IS_PARENT_DISABLED(i))
			continue;

		if (mouse_pos_x >= parent_button[i].x &&
		    mouse_pos_x < parent_button[i].x + parent_button[i].w &&
		    mouse_pos_y >= parent_button[i].y &&
		    mouse_pos_y < parent_button[i].y + parent_button[i].h)
			return i;
	}

	return -1;
}

/* 子選択肢でポイントされているものを取得する */
static int get_pointed_child_index(void)
{
	int i, n;

	n = selected_parent_index;
	for (i = 0; i < parent_button[n].child_count; i++) {
		if (mouse_pos_x >= child_button[n][i].x &&
		    mouse_pos_x < child_button[n][i].x +
		    child_button[n][i].w &&
		    mouse_pos_y >= child_button[n][i].y &&
		    mouse_pos_y < child_button[n][i].y +
		    child_button[n][i].h)
			return i;
	}

	return -1;
}

/* 選択中のシステムメニューのボタンを取得する */
static int get_sysmenu_pointed_button(void)
{
	int rx, ry, btn_x, btn_y, btn_w, btn_h, i;

	/* システムメニューを表示中でない場合は非選択とする */
	if (!is_sysmenu)
		return SYSMENU_NONE;

	/* マウス座標からシステムメニュー画像内座標に変換する */
	rx = mouse_pos_x - conf_sysmenu_x;
	ry = mouse_pos_y - conf_sysmenu_y;

	/* ボタンを順番に見ていく */
	for (i = SYSMENU_QSAVE; i <= SYSMENU_CONFIG; i++) {
		/* ボタンの座標を取得する */
		get_sysmenu_button_rect(i, &btn_x, &btn_y, &btn_w, &btn_h);

		/* マウスがボタンの中にあればボタンの番号を返す */
		if ((rx >= btn_x && rx < btn_x + btn_w) &&
		    (ry >= btn_y && ry < btn_y + btn_h))
			return i;
	}

	/* ボタンがポイントされていない */
	return SYSMENU_NONE;
}

/* システムメニューのボタンの座標を取得する */
static void get_sysmenu_button_rect(int btn, int *x, int *y, int *w, int *h)
{
	switch (btn) {
	case SYSMENU_QSAVE:
		*x = conf_sysmenu_qsave_x;
		*y = conf_sysmenu_qsave_y;
		*w = conf_sysmenu_qsave_width;
		*h = conf_sysmenu_qsave_height;
		break;
	case SYSMENU_QLOAD:
		*x = conf_sysmenu_qload_x;
		*y = conf_sysmenu_qload_y;
		*w = conf_sysmenu_qload_width;
		*h = conf_sysmenu_qload_height;
		break;
	case SYSMENU_SAVE:
		*x = conf_sysmenu_save_x;
		*y = conf_sysmenu_save_y;
		*w = conf_sysmenu_save_width;
		*h = conf_sysmenu_save_height;
		break;
	case SYSMENU_LOAD:
		*x = conf_sysmenu_load_x;
		*y = conf_sysmenu_load_y;
		*w = conf_sysmenu_load_width;
		*h = conf_sysmenu_load_height;
		break;
	case SYSMENU_AUTO:
		*x = conf_sysmenu_auto_x;
		*y = conf_sysmenu_auto_y;
		*w = conf_sysmenu_auto_width;
		*h = conf_sysmenu_auto_height;
		break;
	case SYSMENU_SKIP:
		*x = conf_sysmenu_skip_x;
		*y = conf_sysmenu_skip_y;
		*w = conf_sysmenu_skip_width;
		*h = conf_sysmenu_skip_height;
		break;
	case SYSMENU_HISTORY:
		*x = conf_sysmenu_history_x;
		*y = conf_sysmenu_history_y;
		*w = conf_sysmenu_history_width;
		*h = conf_sysmenu_history_height;
		break;
	case SYSMENU_CONFIG:
		*x = conf_sysmenu_config_x;
		*y = conf_sysmenu_config_y;
		*w = conf_sysmenu_config_width;
		*h = conf_sysmenu_config_height;
		break;
	default:
		assert(ASSERT_INVALID_BTN_INDEX);
		break;
	}
}

/* 親選択肢のFO/FIレイヤを描画する */
static void draw_fo_fi_parent(void)
{
	lock_draw_char_on_fo_fi();
	draw_stage_fo_fi();
	draw_switch_parent_images();
	unlock_draw_char_on_fo_fi();
}

/* 親選択肢のイメージを描画する */
void draw_switch_parent_images(void)
{
	int i;
	bool is_news;

	assert(selected_parent_index == -1);

	for (i = 0; i < PARENT_COUNT; i++) {
		if (IS_PARENT_DISABLED(i))
			continue;

		/* NEWSの項目であるか調べる */
		is_news = get_command_type() == COMMAND_NEWS &&
			i < SWITCH_BASE;

		/* FO/FIレイヤにスイッチを描画する */
		if (!is_news) {
			draw_switch_fg_image(parent_button[i].x,
					     parent_button[i].y);
			draw_switch_bg_image(parent_button[i].x,
					     parent_button[i].y);
		} else {
			draw_news_fg_image(parent_button[i].x,
					   parent_button[i].y);
			draw_news_bg_image(parent_button[i].x,
					   parent_button[i].y);
		}

		/* テキストを描画する */
		draw_text(parent_button[i].x, parent_button[i].y,
			  parent_button[i].w, parent_button[i].msg, is_news);
	}
}

/* 親選択肢を画面に描画する */
void update_switch_parent(int *x, int *y, int *w, int *h)
{
	int i, bx, by, bw, bh;

	assert(selected_parent_index == -1);

	i = pointed_parent_index;

	/* 描画するFIレイヤの矩形を求める */
	bx = by = bw = bh = 0;
	if (i != -1) {
		bx = parent_button[i].x;
		by = parent_button[i].y;
		bw = parent_button[i].w;
		bh = parent_button[i].h;
	}

	/* FOレイヤ全体とFIレイヤの矩形を画面に描画する */
	draw_stage_with_button(bx, by, bw, bh);

	/* 更新範囲を設定する */
	*x = 0;
	*y = 0;
	*w = conf_window_width;
	*h = conf_window_height;
}

/* 子選択肢のFO/FIレイヤを描画する */
static void draw_fo_fi_child(void)
{
	lock_draw_char_on_fo_fi();
	draw_stage_fo_fi();
	draw_switch_child_images();
	unlock_draw_char_on_fo_fi();
}

/* 子選択肢のイメージを描画する */
void draw_switch_child_images(void)
{
	int i, j;

	assert(selected_parent_index != -1);
	assert(parent_button[selected_parent_index].child_count > 0);

	i = selected_parent_index;
	for (j = 0; j < parent_button[i].child_count; j++) {
		/* FO/FIレイヤにスイッチを描画する */
		draw_switch_fg_image(child_button[i][j].x,
				     child_button[i][j].y);
		draw_switch_bg_image(child_button[i][j].x,
				     child_button[i][j].y);

		/* テキストを描画する */
		draw_text(child_button[i][j].x, child_button[i][j].y,
			  child_button[i][j].w, child_button[i][j].msg, false);
	}
}

/* 親選択肢を画面に描画する */
void update_switch_child(int *x, int *y, int *w, int *h)
{
	int i, j, bx, by, bw, bh;

	assert(selected_parent_index != -1);

	i = selected_parent_index;
	j = pointed_child_index;

	/* 描画するFIレイヤの矩形を求める */
	bx = by = bw = bh = 0;
	if (j != -1) {
		bx = child_button[i][j].x;
		by = child_button[i][j].y;
		bw = child_button[i][j].w;
		bh = child_button[i][j].h;
	}

	/* FO全体とFIの1矩形を描画する(GPU用) */
	draw_stage_with_button(bx, by, bw, bh);

	/* 更新範囲を設定する */
	*x = 0;
	*y = 0;
	*w = conf_window_width;
	*h = conf_window_height;
}

/* 選択肢のテキストを描画する */
static void draw_text(int x, int y, int w, const char *t, bool is_news)
{
	uint32_t c;
	int mblen, xx;
	pixel_t inactive_body_color, inactive_outline_color;
	pixel_t active_body_color, active_outline_color;

	/* 色を決める */
	inactive_body_color =
		make_pixel_slow(0xff,
				(pixel_t)conf_font_color_r,
				(pixel_t)conf_font_color_g,
				(pixel_t)conf_font_color_b);
	inactive_outline_color =
		make_pixel_slow(0xff,
				(pixel_t)conf_font_outline_color_r,
				(pixel_t)conf_font_outline_color_g,
				(pixel_t)conf_font_outline_color_b);
	if (conf_switch_color_active) {
		active_body_color =
			make_pixel_slow(0xff,
					(pixel_t)conf_switch_color_active_body_r,
					(pixel_t)conf_switch_color_active_body_g,
					(pixel_t)conf_switch_color_active_body_b);
		active_outline_color =
			make_pixel_slow(0xff,
					(pixel_t)conf_switch_color_active_outline_r,
					(pixel_t)conf_switch_color_active_outline_g,
					(pixel_t)conf_switch_color_active_outline_b);
	} else {
		active_body_color = inactive_body_color;
		active_outline_color = inactive_outline_color;
	}

	/* 描画位置を決める */
	xx = x + (w - get_utf8_width(t)) / 2;
	y += is_news ? conf_news_text_margin_y : conf_switch_text_margin_y;

	/* 1文字ずつ描画する */
	while (*t != '\0') {
		/* 描画する文字を取得する */
		mblen = utf8_to_utf32(t, &c);
		if (mblen == -1)
			return;

		/* 描画する */
		xx += draw_char_on_fo_fi(xx, y, c,
					 inactive_body_color,
					 inactive_outline_color,
					 active_body_color,
					 active_outline_color);

		/* 次の文字へ移動する */
		t += mblen;
	}
}

/* 描画する(GPU用) */
static void draw_keep(void)
{
	int x, y, w, h, i, j;

	x = y = w = h = 0;
	if (selected_parent_index == -1) {
		i = pointed_parent_index;
		if (i != -1) {
			x = parent_button[i].x;
			y = parent_button[i].y;
			w = parent_button[i].w;
			h = parent_button[i].h;
		}
	} else {
		i = selected_parent_index;
		j = pointed_child_index;
		if (j != -1) {
			x = child_button[i][j].x;
			y = child_button[i][j].y;
			w = child_button[i][j].w;
			h = child_button[i][j].h;
		}
	}

	/* FO全体とFIの1矩形を描画する(GPU用) */
	draw_stage_with_button_keep(x, y, w, h);
}

/* システムメニュー非表示中のクリックを処理する */
static void process_main_click(void)
{
	bool enter_sysmenu;

	/* ヒストリ画面への遷移を確認する */
	if (is_up_pressed && get_history_count() != 0) {
		play_se(conf_msgbox_history_se);
		need_history_mode = true;
		return;
	}

	/* システムメニューへの遷移を確認していく */
	enter_sysmenu = false;

	/* 右クリックされたとき */
	if (selected_parent_index == -1 && is_right_button_pressed)
		enter_sysmenu = true;

	/* エスケープキーが押下されたとき */
	if (is_escape_pressed)
		enter_sysmenu = true;

	/* 折りたたみシステムメニューがクリックされたとき */
	if (is_left_clicked && is_collapsed_sysmenu_pointed())
		enter_sysmenu = true;

	/* システムメニューを開始するとき */
	if (enter_sysmenu) {
		/* SEを再生する */
		play_se(conf_sysmenu_enter_se);

		/* システムメニューを表示する */
		is_sysmenu = true;
		is_sysmenu_first_frame = true;
		sysmenu_pointed_index = get_sysmenu_pointed_button();
		old_sysmenu_pointed_index = sysmenu_pointed_index;
		is_sysmenu_finished = false;
		return;
	}
}

/* システムメニュー表示中のクリックを処理する */
static void process_sysmenu_click(void)
{
	/* 右クリックされた場合と、エスケープキーが押下されたとき */
	if (is_right_button_pressed || is_escape_pressed) {
		/* SEを再生する */
		play_se(conf_sysmenu_leave_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;
		return;
	}

	/* ポイントされているシステムメニューのボタンを求める */
	old_sysmenu_pointed_index = sysmenu_pointed_index;
	sysmenu_pointed_index = get_sysmenu_pointed_button();

	/* ボタンのないところを左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_NONE && is_left_clicked) {
		/* SEを再生する */
		play_se(conf_sysmenu_leave_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;
		return;
	}

	/* セーブロードが無効な場合 */
	if (!is_save_load_enabled() &&
	    (sysmenu_pointed_index == SYSMENU_QSAVE ||
	     sysmenu_pointed_index == SYSMENU_QLOAD ||
	     sysmenu_pointed_index == SYSMENU_SAVE ||
	     sysmenu_pointed_index == SYSMENU_LOAD))
		sysmenu_pointed_index = SYSMENU_NONE;

	/* クイックセーブデータがない場合 */
	if (!have_quick_save_data() &&
	    sysmenu_pointed_index == SYSMENU_QLOAD)
		sysmenu_pointed_index = SYSMENU_NONE;

	/* switchではオートとスキップを無効にする */
	if (sysmenu_pointed_index == SYSMENU_AUTO ||
	    sysmenu_pointed_index == SYSMENU_SKIP)
		sysmenu_pointed_index = SYSMENU_NONE;

	/* 左クリックされていない場合、何もしない */
	if (!is_left_clicked)
		return;

	/* クイックセーブが左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_QSAVE) {
		/* SEを再生する */
		play_se(conf_sysmenu_qsave_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;

		/* クイックセーブを行う */
		quick_save();
		return;
	}

	/* クイックロードが左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_QLOAD) {
		/* クイックロードを行う */
		if (quick_load()) {
			/* SEを再生する */
			play_se(conf_sysmenu_qload_se);

			/* システムメニューを終了する */
			is_sysmenu = false;
			is_sysmenu_finished = true;

			/* 後処理を行う */
			did_quick_load = true;
		}
		return;
	}

	/* セーブが左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_SAVE) {
		/* SEを再生する */
		play_se(conf_sysmenu_save_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;

		/* セーブモードに移行する */
		need_save_mode = true;
		return;
	}

	/* ロードが左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_LOAD) {
		/* SEを再生する */
		play_se(conf_sysmenu_load_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;

		/* ロードモードに移行する */
		need_load_mode = true;
		return;
	}

	/* ヒストリが左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_HISTORY) {
		/* ヒストリがない場合はヒストリモードを開始しない */
		if (get_history_count() == 0)
			return;

		/* SEを再生する */
		play_se(conf_sysmenu_history_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;

		/* ヒストリモードを開始する */
		need_history_mode = true;
		return;
	}

	/* コンフィグが左クリックされた場合 */
	if (sysmenu_pointed_index == SYSMENU_CONFIG) {
		/* SEを再生する */
		play_se(conf_sysmenu_config_se);

		/* システムメニューを終了する */
		is_sysmenu = false;
		is_sysmenu_finished = true;

		/* コンフィグモードを開始する */
		need_config_mode = true;
		return;
	}
}

/* システムメニューを描画する */
static void draw_sysmenu(int *x, int *y, int *w, int *h)
{
	int bx, by, bw, bh;
	bool qsave_sel, qload_sel, save_sel, load_sel, auto_sel, skip_sel;
	bool history_sel, config_sel, redraw;

	/* 描画するかの判定状態を初期化する */
	qsave_sel = false;
	qload_sel = false;
	save_sel = false;
	load_sel = false;
	auto_sel = false;
	skip_sel = false;
	history_sel = false;
	config_sel = false;
	redraw = false;

	/* システムメニューの最初のフレームの場合、描画する */
	if (is_sysmenu_first_frame) {
		redraw = true;
		is_sysmenu_first_frame = false;
	}

	/* クイックセーブボタンがポイントされているかを取得する */
	if (sysmenu_pointed_index == SYSMENU_QSAVE) {
		qsave_sel = true;
		if (old_sysmenu_pointed_index != SYSMENU_QSAVE &&
		    !is_sysmenu_first_frame) {
			play_se(conf_sysmenu_change_se);
			redraw = true;
		}
	}

	/* クイックロードボタンがポイントされているかを取得する */
	if (sysmenu_pointed_index == SYSMENU_QLOAD) {
		qload_sel = true;
		if (old_sysmenu_pointed_index != SYSMENU_QLOAD &&
		    !is_sysmenu_first_frame) {
			play_se(conf_sysmenu_change_se);
			redraw = true;
		}
	}

	/* セーブボタンがポイントされているかを取得する */
	if (sysmenu_pointed_index == SYSMENU_SAVE) {
		save_sel = true;
		if (old_sysmenu_pointed_index != SYSMENU_SAVE &&
		    !is_sysmenu_first_frame) {
			play_se(conf_sysmenu_change_se);
			redraw = true;
		}
	}

	/* ロードボタンがポイントされているかを取得する */
	if (sysmenu_pointed_index == SYSMENU_LOAD) {
		load_sel = true;
		if (old_sysmenu_pointed_index != SYSMENU_LOAD &&
		    !is_sysmenu_first_frame) {
			play_se(conf_sysmenu_change_se);
			redraw = true;
		}
	}

	/* ヒストリがポイントされているかを取得する */
	if (sysmenu_pointed_index == SYSMENU_HISTORY) {
		history_sel = true;
		if (old_sysmenu_pointed_index != SYSMENU_HISTORY &&
		    !is_sysmenu_first_frame) {
			play_se(conf_sysmenu_change_se);
			redraw = true;
		}
	}

	/* コンフィグがポイントされているかを取得する */
	if (sysmenu_pointed_index == SYSMENU_CONFIG) {
		config_sel = true;
		if (old_sysmenu_pointed_index != SYSMENU_CONFIG &&
		    !is_sysmenu_first_frame) {
			play_se(conf_sysmenu_change_se);
			redraw = true;
		}
	}

	/* ポイント項目がなくなった場合 */
	if (sysmenu_pointed_index == SYSMENU_NONE) {
		if (old_sysmenu_pointed_index != SYSMENU_NONE)
			redraw = true;
	}

	/* GPUを利用している場合 */
	if (is_gpu_accelerated())
		redraw = true;
		
	/* 描画する */
	if (redraw) {
		/* 背景を描画する */
		get_sysmenu_rect(&bx, &by, &bw, &bh);
		union_rect(x, y, w, h, *x, *y, *w, *h, bx, by, bw, bh);
		draw_stage_rect_with_buttons(bx, by, bw, bh, 0, 0, 0, 0);

		/* システムメニューを描画する */
		draw_stage_sysmenu(false,
				   false,
				   is_save_load_enabled(),
				   is_save_load_enabled() &&
				   have_quick_save_data(),
				   qsave_sel,
				   qload_sel,
				   save_sel,
				   load_sel,
				   auto_sel,
				   skip_sel,
				   history_sel,
				   config_sel,
				   x, y, w, h);
		is_sysmenu_first_frame = false;
	}
}

/* 折りたたみシステムメニューを描画する */
static void draw_collapsed_sysmenu(int *x, int *y, int *w, int *h)
{
	bool is_pointed;

	/* 折りたたみシステムメニューがポイントされているか調べる */
	is_pointed = is_collapsed_sysmenu_pointed();

	/* 描画する */
	draw_stage_collapsed_sysmenu(is_pointed, x, y, w, h);

	/* SEを再生する */
	if (!is_sysmenu_finished &&
	    (is_collapsed_sysmenu_pointed_prev != is_pointed))
		play_se(conf_sysmenu_collapsed_se);

	/* 折りたたみシステムメニューのポイント状態を保持する */
	is_collapsed_sysmenu_pointed_prev = is_pointed;
}

/* 折りたたみシステムメニューがポイントされているか調べる */
static bool is_collapsed_sysmenu_pointed(void)
{
	int bx, by, bw, bh;

	get_collapsed_sysmenu_rect(&bx, &by, &bw, &bh);
	if (mouse_pos_x >= bx && mouse_pos_x < bx + bw &&
	    mouse_pos_y >= by && mouse_pos_y < by + bh)
		return true;

	return false;
}

/* SEを再生する */
static void play_se(const char *file)
{
	struct wave *w;

	if (file == NULL || strcmp(file, "") == 0)
		return;

	w = create_wave_from_file(SE_DIR, file, false);
	if (w == NULL)
		return;

	set_mixer_input(SE_STREAM, w);
}

/* コマンドを終了する */
static bool cleanup(void)
{
	int n, m;

	/* セーブ・ロードを行う際はコマンドの移動を行わない */
	if (did_quick_load || need_save_mode || need_load_mode ||
	    need_history_mode || need_config_mode)
		return true;

	n = selected_parent_index;

	/* 子選択肢が選択された場合 */
	if (parent_button[n].has_child) {
		m = pointed_child_index;
		return move_to_label(child_button[n][m].label);
	}

	/* 親選択肢が選択された場合 */
	return move_to_label(parent_button[n].label);
}
