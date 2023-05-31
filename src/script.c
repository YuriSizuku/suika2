/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika 2
 * Copyright (C) 2001-2023, TABATA Keiichi. All rights reserved.
 */

/*
 * [Changes]
 *  - 2016/06/01 作成
 *  - 2017/08/13 スイッチに対応
 *  - 2018/07/21 @gosubに対応
 *  - 2019/09/17 @newsに対応
 *  - 2021/06/05 @bgにエフェクトを追加
 *  - 2021/06/05 @volにマスターボリュームを追加
 *  - 2021/06/05 @menuのボタン数を増やした
 *  - 2021/06/06 @seにボイスストリーム出力を追加
 *  - 2021/06/10 @bgと@chにマスク描画を追加
 *  - 2021/06/10 @chにオフセットとアルファを追加
 *  - 2021/06/10 @chaに対応
 *  - 2021/06/12 @shakeに対応
 *  - 2021/06/15 @setsaveに対応
 *  - 2021/07/07 @goto $SAVEに対応
 *  - 2021/07/19 @chsに対応
 *  - 2022/05/11 @videoに対応
 *  - 2022/06/05 @skipに対応
 *  - 2022/06/06 デバッガに対応
 *  - 2022/06/17 @chooseに対応
 *  - 2022/07/29 @guiに対応
 *  - 2022/10/19 ローケルに対応
 *  - 2023/01/06 日本語コマンド名、パラメータ名指定、カギカッコに対応
 *  - 2023/01/14 スタートアップファイル/ラインに対応
 */

#include "suika.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

/* 1行の読み込みサイズ */
#define LINE_BUF_SIZE	(65536)

/*
 * コマンド配列
 */

/* コマンドの引数の最大数(コマンド名も含める) */
#define PARAM_SIZE	(137)

/* コマンド配列 */
static struct command {
	int type;
	int line;
	char *text;
	char *param[PARAM_SIZE];
	char locale[3];
} cmd[SCRIPT_CMD_SIZE];

/* コマンドの数 */
static int cmd_size;

/* 行数 */
int script_lines;

#ifdef USE_DEBUGGER
/* コメント行のテキスト */
#define SCRIPT_LINE_SIZE	(65536)
static char *comment_text[SCRIPT_LINE_SIZE];

/* エラーの数 */
static int error_count;
#endif

/*
 * 命令の種類
 */

struct insn_item {
	const char *str;	/* 命令の文字列 */
	int type;		/* コマンドのタイプ */
	int min;		/* 最小のパラメータ数 */
	int max;		/* 最大のパラメータ数 */
} insn_tbl[] = {
	/* 背景変更 */
	{"@bg", COMMAND_BG, 1, 3},
	{U8("@背景"), COMMAND_BG, 1, 3},

	/* BGM再生 */
	{"@bgm", COMMAND_BGM, 1, 2},
	{U8("@音楽"), COMMAND_BGM, 1, 2},

	/* キャラ変更 */
	{"@ch", COMMAND_CH, 1, 7},
	{U8("@キャラ"), COMMAND_CH, 1, 7},

	/* クリック待ち */
	{"@click", COMMAND_CLICK, 0, 1},
	{U8("@クリック"), COMMAND_CLICK, 0, 1},

	/* 時間指定待ち */
	{"@wait", COMMAND_WAIT, 1, 1},
	{U8("@時間待ち"), COMMAND_WAIT, 1, 1},

	/* ラベルへジャンプ */
	{"@goto", COMMAND_GOTO, 1, 1},
	{U8("@ジャンプ"), COMMAND_GOTO, 1, 1},

	/* シナリオファイルのロード */
	{"@load", COMMAND_LOAD, 1, 1},
	{U8("@シナリオ"), COMMAND_LOAD, 1, 1},

	/* ボリューム設定 */
	{"@vol", COMMAND_VOL, 2, 3},
	{U8("@音量"), COMMAND_VOL, 2, 3},

	/* 変数設定 */
	{"@set", COMMAND_SET, 3, 3},
	{U8("@フラグをセット"), COMMAND_SET, 3, 3},

	/* 変数分岐 */
	{"@if", COMMAND_IF, 4, 4},
	{U8("@フラグでジャンプ"), COMMAND_IF, 4, 4},

	/* 効果音 */
	{"@se", COMMAND_SE, 1, 2},
	{U8("@効果音"), COMMAND_SE, 1, 2},

	/* キャラアニメ */
	{"@cha", COMMAND_CHA, 6, 6},
	{U8("@キャラ移動"), COMMAND_CHA, 6, 6},

	/* 画面を揺らす */
	{"@shake", COMMAND_SHAKE, 4, 4},
	{U8("@振動"), COMMAND_SHAKE, 4, 4},

	/* ステージの一括変更 */
	{"@chs", COMMAND_CHS, 4, 7},
	{U8("@場面転換"), COMMAND_CHS, 4, 7},

	/* ビデオ再生 */
	{"@video", COMMAND_VIDEO, 1, 1},
	{U8("@動画"), COMMAND_VIDEO, 1, 1},

	/* 選択肢 */
	{"@choose", COMMAND_CHOOSE, 2, 16},
	{U8("@選択肢"), COMMAND_CHOOSE, 2, 16},

	/* 章タイトル */
	{"@chapter", COMMAND_CHAPTER, 1, 1},
	{U8("@章"), COMMAND_CHAPTER, 1, 1},

	/* GUI */
	{"@gui", COMMAND_GUI, 1, 2},
	{U8("@メニュー"), COMMAND_GUI, 1, 2},

	/* WMS */
	{"@wms", COMMAND_WMS, 1, 1},
	{U8("@スクリプト"), COMMAND_WMS, 1, 1},

	/* スキップ設定 */
	{"@skip", COMMAND_SKIP, 1, 1},
	{"@スキップ", COMMAND_SKIP, 1, 1},

	/* その他 */
	{"@setsave", COMMAND_SETSAVE, 1, 1},
	{"@gosub", COMMAND_GOSUB, 1, 1},
	{"@return", COMMAND_RETURN, 0, 0},

	/* deprecated */
	{"@select", COMMAND_SELECT, 6, 6},
	{"@menu", COMMAND_MENU, 7, 83},
	{"@news", COMMAND_NEWS, 9, 136},
	{"@retrospect", COMMAND_RETROSPECT, 11, 55},
	{"@switch", COMMAND_SWITCH, 9, 136},
};

#define INSN_TBL_SIZE	(sizeof(insn_tbl) / sizeof(struct insn_item))

/*
 * パラメータの名前
 */

struct param_item {
	int type;
	int param_index;
	const char *name;
} param_tbl[] = {
	/* @bg */
	{COMMAND_BG, BG_PARAM_FILE, "file="},
	{COMMAND_BG, BG_PARAM_FILE, U8("ファイル=")},
	{COMMAND_BG, BG_PARAM_SPAN, "duration="},
	{COMMAND_BG, BG_PARAM_SPAN, U8("秒=")},
	{COMMAND_BG, BG_PARAM_METHOD, "effect="},
	{COMMAND_BG, BG_PARAM_METHOD, U8("エフェクト=")},

	/* @bgm */
	{COMMAND_BGM, BG_PARAM_FILE, "file="},
	{COMMAND_BGM, BG_PARAM_FILE, U8("ファイル=")},

	/* @ch */
	{COMMAND_CH, CH_PARAM_POS, "position="},
	{COMMAND_CH, CH_PARAM_POS, U8("位置=")},
	{COMMAND_CH, CH_PARAM_FILE, "file="},
	{COMMAND_CH, CH_PARAM_FILE, U8("ファイル=")},
	{COMMAND_CH, CH_PARAM_SPAN, "duration="},
	{COMMAND_CH, CH_PARAM_SPAN, U8("秒=")},
	{COMMAND_CH, CH_PARAM_METHOD, "effect="},
	{COMMAND_CH, CH_PARAM_METHOD, U8("エフェクト=")},
	{COMMAND_CH, CH_PARAM_OFFSET_X, "right="},
	{COMMAND_CH, CH_PARAM_OFFSET_X, U8("右=")},
	{COMMAND_CH, CH_PARAM_OFFSET_Y, "down="},
	{COMMAND_CH, CH_PARAM_OFFSET_Y, U8("下=")},
	{COMMAND_CH, CH_PARAM_ALPHA, "alpha="},
	{COMMAND_CH, CH_PARAM_ALPHA, U8("アルファ=")},

	/* @wait */
	{COMMAND_WAIT, WAIT_PARAM_SPAN, "duration="},
	{COMMAND_WAIT, WAIT_PARAM_SPAN, U8("秒=")},

	/* @goto */
	{COMMAND_GOTO, GOTO_PARAM_LABEL, "destination="},
	{COMMAND_GOTO, GOTO_PARAM_LABEL, U8("行き先=")},

	/* @load */
	{COMMAND_LOAD, LOAD_PARAM_FILE, "file="},
	{COMMAND_LOAD, LOAD_PARAM_FILE, U8("ファイル=")},

	/* @vol */
	{COMMAND_VOL, VOL_PARAM_STREAM, "track="},
	{COMMAND_VOL, VOL_PARAM_STREAM, U8("トラック=")},
	{COMMAND_VOL, VOL_PARAM_VOL, "volume="},
	{COMMAND_VOL, VOL_PARAM_VOL, U8("音量=")},
	{COMMAND_VOL, VOL_PARAM_SPAN, "duration="},
	{COMMAND_VOL, VOL_PARAM_SPAN, U8("秒=")},

	/* @se */
	{COMMAND_SE, SE_PARAM_FILE, "file="},
	{COMMAND_SE, SE_PARAM_FILE, U8("ファイル=")},

	/* @choose */
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL1, "destination1="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL1, U8("行き先1=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT1, "option1="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT1, U8("選択肢1=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL2, "destination2="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL2, U8("行き先2=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT2, "option2="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT2, U8("選択肢2=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL3, "destination3="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL3, U8("行き先3=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT3, "option3="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT3, U8("選択肢3=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL4, "destination4="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL4, U8("行き先4=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT4, "option4="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT4, U8("選択肢4=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL5, "destination5="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL5, U8("行き先5=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT5, "option5="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT5, U8("選択肢5=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL6, "destination6="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL6, U8("行き先6=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT6, "option6="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT6, U8("選択肢6=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL7, "destination7="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL7, U8("行き先7=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT7, "option7="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT7, U8("選択肢7=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL8, "destination8="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_LABEL8, U8("行き先8=")},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT8, "option8="},
	{COMMAND_CHOOSE, CHOOSE_PARAM_TEXT8, U8("選択肢8=")},

	/* @cha */
	{COMMAND_CHA, CHA_PARAM_POS, "position="},
	{COMMAND_CHA, CHA_PARAM_POS, U8("位置=")},
	{COMMAND_CHA, CHA_PARAM_SPAN, "duration="},
	{COMMAND_CHA, CHA_PARAM_SPAN, U8("秒=")},
	{COMMAND_CHA, CHA_PARAM_ACCEL, "acceleration="},
	{COMMAND_CHA, CHA_PARAM_ACCEL, U8("加速=")},
	{COMMAND_CHA, CHA_PARAM_OFFSET_X, "x="},
	{COMMAND_CHA, CHA_PARAM_OFFSET_Y, "y="},
	{COMMAND_CHA, CHA_PARAM_ALPHA, "alpha"},
	{COMMAND_CHA, CHA_PARAM_ALPHA, U8("アルファ=")},

	/* @shake */
	{COMMAND_SHAKE, SHAKE_PARAM_MOVE, "direction="},
	{COMMAND_SHAKE, SHAKE_PARAM_MOVE, U8("方向=")},
	{COMMAND_SHAKE, SHAKE_PARAM_SPAN, "duration="},
	{COMMAND_SHAKE, SHAKE_PARAM_SPAN, U8("秒=")},
	{COMMAND_SHAKE, SHAKE_PARAM_TIMES, "times="},
	{COMMAND_SHAKE, SHAKE_PARAM_TIMES, U8("回数=")},
	{COMMAND_SHAKE, SHAKE_PARAM_AMOUNT, "amplitude="},
	{COMMAND_SHAKE, SHAKE_PARAM_AMOUNT, U8("大きさ=")},

	/* @chs */
	{COMMAND_CHS, CHS_PARAM_CENTER, "center="},
	{COMMAND_CHS, CHS_PARAM_CENTER, U8("中央=")},
	{COMMAND_CHS, CHS_PARAM_RIGHT, "right="},
	{COMMAND_CHS, CHS_PARAM_RIGHT, U8("右=")},
	{COMMAND_CHS, CHS_PARAM_LEFT, "left="},
	{COMMAND_CHS, CHS_PARAM_LEFT, U8("左=")},
	{COMMAND_CHS, CHS_PARAM_BACK, "back="},
	{COMMAND_CHS, CHS_PARAM_BACK, U8("背面=")},
	{COMMAND_CHS, CHS_PARAM_SPAN, "duration="},
	{COMMAND_CHS, CHS_PARAM_SPAN, U8("秒=")},
	{COMMAND_CHS, CHS_PARAM_BG, "background="},
	{COMMAND_CHS, CHS_PARAM_BG, U8("背景=")},
	{COMMAND_CHS, CHS_PARAM_METHOD, "effect="},
	{COMMAND_CHS, CHS_PARAM_METHOD, U8("エフェクト=")},

	/* @video */
	{COMMAND_VIDEO, VIDEO_PARAM_FILE, "file="},
	{COMMAND_VIDEO, VIDEO_PARAM_FILE, U8("ファイル=")},

	/* @chapter */
	{COMMAND_CHAPTER, CHAPTER_PARAM_NAME, "title="},
	{COMMAND_CHAPTER, CHAPTER_PARAM_NAME, U8("タイトル=")},

	/* @gui */
	{COMMAND_GUI, GUI_PARAM_FILE, "file="},
	{COMMAND_GUI, GUI_PARAM_FILE, U8("ファイル=")},

	/* @wms */
	{COMMAND_WMS, WMS_PARAM_FILE, "file="},
	{COMMAND_WMS, WMS_PARAM_FILE, U8("ファイル=")},
};

#define PARAM_TBL_SIZE	(sizeof(param_tbl) / sizeof(struct param_item))

/*
 * コマンド実行ポインタ
 */

/* 実行中のスクリプト名 */
static char *cur_script;

/* 実行中のコマンド番号 */
static int cur_index;

/* 最後にgosubが実行されたコマンド番号 */
static int return_point;

#ifdef USE_DEBUGGER
/*
 * その他
 */

/* パースエラー状態 */
static bool is_parse_error;
#endif

#ifdef USE_DEBUGGER
/*
 * スタートアップ情報
 */

char *startup_file;
int startup_line;
#endif

/*
 * 前方参照
 */
static bool read_script_from_file(const char *fname);
static bool parse_insn(int index, const char *fname, int line,
		       const char *buf, int locale_offset);
static char *strtok_escape(char *buf, bool *escaped);
static bool parse_serif(int index, const char *fname, int line,
			const char *buf, int locale_offset);
static bool parse_message(int index, const char *fname, int line,
			  const char *buf, int locale_offset);
static bool parse_label(int index, const char *fname, int line,
			const char *buf, int locale_offset);

/*
 * 初期化
 */

/*
 * 初期スクリプトを読み込む
 */
bool init_script(void)
{
#ifndef USE_DEBUGGER
	/* スクリプトをロードする */
	if (!load_script(INIT_FILE))
		return false;
#else
	int i;

	/*
	 * 読み込むスクリプトが指定されていればそれを使用し、
	 * そうでなければinit.txtを使用する
	 */
	if (!load_script(startup_file == NULL ? INIT_FILE : startup_file))
		return false;

	/* 開始行が指定されていれば移動する */
	if (startup_line > 0) {
		for (i = 0; i < cmd_size; i++) {
			if (cmd[i].line < startup_line)
				continue;
			if (cmd[i].line >= startup_line) {
				cur_index = i;
				break;
			}
		}
	}
#endif

	return true;
}

/*
 * コマンドを破棄する
 */
void cleanup_script(void)
{
	int i, j;

	for (i = 0; i < SCRIPT_CMD_SIZE; i++) {
		/* コマンドタイプをクリアする */
		cmd[i].type = COMMAND_MIN;

		/* 行の内容を解放する */
		if (cmd[i].text != NULL) {
			free(cmd[i].text);
			cmd[i].text = NULL;
		}

		/* 引数の本体を解放する */
		if (cmd[i].param[0] != NULL) {
			free(cmd[i].param[0]);
			cmd[i].param[0] = NULL;
		}

		/* 引数の参照をNULLで上書きする */
		for (j = 1; j < PARAM_SIZE; j++)
			cmd[i].param[j] = NULL;
	}

#ifdef USE_DEBUGGER
	for (i = 0; i < script_lines; i++) {
		if (comment_text[i] != NULL) {
			free(comment_text[i]);
			comment_text[i] = NULL;
		}
	}
#endif

	if (cur_script != NULL) {
		free(cur_script);
		cur_script = NULL;
	}
}

/*
 * スクリプトとコマンドへの公開アクセス
 */

/*
 * スクリプトをロードする
 */
bool load_script(const char *fname)
{
	/* 現在のスクリプトを破棄する */
	cleanup_script();

	/* スクリプト名を保存する */
	cur_index = 0;
	cur_script = strdup(fname);
	if (cur_script == NULL) {
		log_memory();
		return false;
	}

	/* スクリプトファイルを読み込む */
	if (!read_script_from_file(fname)) {
#ifdef USE_DEBUGGER
		/* 最後の行まで読み込まれる */
#else
		return false;
#endif
	}

	/* コマンドが含まれない場合 */
	if (cmd_size == 0) {
		log_script_no_command(fname);
#ifdef USE_DEBUGGER
		return load_debug_script();
#else
		return false;
#endif
	}

	/* リターンポイントを無効にする */
	set_return_point(-1);

#ifdef USE_DEBUGGER
	if (dbg_is_stop_requested())
		dbg_stop();
	update_debug_info(true);
#endif

	return true;
}

/*
 * スクリプトファイル名を取得する
 */
const char *get_script_file_name(void)
{
	return cur_script;
}

/*
 * 実行中のコマンドのインデックスを取得する(セーブ用)
 */
int get_command_index(void)
{
	return cur_index;
}

/*
 * 実行中のコマンドのインデックスを設定する(ロード用)
 */
bool move_to_command_index(int index)
{
	if (index < 0 || index >= cmd_size)
		return false;

	cur_index = index;

#ifdef USE_DEBUGGER
	if (dbg_is_stop_requested())
		dbg_stop();
	update_debug_info(false);
#endif

	return true;
}

/*
 * 次のコマンドに移動する
 */
bool move_to_next_command(void)
{
	assert(cur_index < cmd_size);

	/* スクリプトの末尾に達した場合 */
	if (++cur_index == cmd_size)
		return false;

#ifdef USE_DEBUGGER
	if (dbg_is_stop_requested())
		dbg_stop();
	update_debug_info(false);
#endif

	return true;
}

/*
 * ラベルへ移動する
 */
bool move_to_label(const char *label)
{
	struct command *c;
	int i;

	/* ラベルを探す */
	for (i = 0; i < cmd_size; i++) {
		/* ラベルでないコマンドをスキップする */
		c = &cmd[i];
		if (c->type != COMMAND_LABEL)
			continue;

		/* ラベルがみつかった場合 */
		if (strcmp(c->param[LABEL_PARAM_LABEL], label) == 0) {
			cur_index = i;

#ifdef USE_DEBUGGER
			if (dbg_is_stop_requested())
				dbg_stop();
			update_debug_info(false);
#endif

			return true;
		}
	}

	/* エラーを出力する */
	log_script_label_not_found(label);
	log_script_exec_footer();
	return false;
}

/*
 * gosubによるリターンポイントを記録する(gosub用)
 */
void push_return_point(void)
{
	return_point = cur_index;
}

/*
 * gosubによるリターンポイントを取得する(return用)
 */
int pop_return_point(void)
{
	int rp;
	rp = return_point;
	return_point = -1;
	return rp;
}

/*
 * gosubによるリターンポイントの行番号を設定する(ロード用)
 *  - indexが-1ならリターンポイントは無効
 */
bool set_return_point(int index)
{
	if (index >= cmd_size)
		return false;

	return_point = index;
	return true;
}

/*
 * gosubによるリターンポイントの行番号を取得する(return,セーブ用)
 *  - indexが-1ならリターンポイントは無効
 */
int get_return_point(void)
{
	return return_point;
}

/*
 * 最後のコマンドであるかを取得する(@goto $SAVE用)
 */
bool is_final_command(void)
{
	if (cur_index == cmd_size - 1)
		return true;

	return false;
}

/*
 * コマンドの行番号を取得する(ログ用)
 */
int get_line_num(void)
{
	return cmd[cur_index].line;
}

/*
 * コマンドの行番号を取得する(ログ用)
 */
const char *get_line_string(void)
{
	struct command *c;

	c = &cmd[cur_index];

	return c->text;
}

/*
 * コマンドのタイプを取得する
 */
int get_command_type(void)
{
	struct command *c;

	assert(cur_index < cmd_size);

	c = &cmd[cur_index];
	assert(c->type > COMMAND_MIN && c->type < COMMAND_MAX);

	return c->type;
}

/*
 * コマンドのロケール指定を取得する
 */
const char *get_command_locale(void)
{
	struct command *c;

	assert(cur_index < cmd_size);

	c = &cmd[cur_index];

	return c->locale;
}

/*
 * 文字列のコマンドパラメータを取得する
 */
const char *get_string_param(int index)
{
	struct command *c;

	assert(cur_index < cmd_size);
	assert(index < PARAM_SIZE);

	c = &cmd[cur_index];

	/* パラメータが省略された場合 */
	if (c->param[index] == NULL)
		return "";

	/* 文字列を返す */
	return c->param[index];
}

/*
 * 整数のコマンドパラメータを取得する
 */
int get_int_param(int index)
{
	struct command *c;

	assert(cur_index < cmd_size);
	assert(index < PARAM_SIZE);

	c = &cmd[cur_index];

	/* パラメータが省略された場合 */
	if (c->param[index] == NULL)
		return 0;

	/* 整数に変換して返す */
	return atoi(c->param[index]);
}

/*
 * 浮動小数点数のコマンドパラメータを取得する
 */
float get_float_param(int index)
{
	struct command *c;

	assert(cur_index < cmd_size);
	assert(index < PARAM_SIZE);

	c = &cmd[cur_index];

	/* パラメータが省略された場合 */
	if (c->param[index] == NULL)
		return 0.0f;

	/* 浮動小数点数に変換して返す */
	return (float)atof(c->param[index]);
}

/*
 * 行の数を取得する
 */
int get_line_count(void)
{
	return script_lines;
}

/*
 * コマンドの数を取得する
 */
int get_command_count(void)
{
	return cmd_size;
}

/*
 * スクリプトファイルの読み込み
 */

/* ファイルを読み込む */
static bool read_script_from_file(const char *fname)
{
	const int BUF_OFS = 4;
	char buf[LINE_BUF_SIZE];
	struct rfile *rf;
	int line;
	int top;
	bool result;

#ifdef USE_DEBUGGER
	error_count = 0;
#endif

	/* ファイルをオープンする */
	rf = open_rfile(SCRIPT_DIR, fname, false);
	if (rf == NULL)
		return false;

	/* 行ごとに処理する */
	cmd_size = 0;
	line = 0;
	result = true;
	while (result) {
#ifdef USE_DEBUGGER
		if (line > SCRIPT_LINE_SIZE) {
			log_script_line_size();
			result = false;
			break;
		}
#endif

		/* 行を読み込む */
		if (gets_rfile(rf, buf, sizeof(buf)) == NULL)
			break;

		/* 最大コマンド数をチェックする */
		if (line >= SCRIPT_CMD_SIZE) {
			log_script_size(SCRIPT_CMD_SIZE);
			result = false;
			break;
		}

		/* ロケールを処理する */
		top = 0;
		if (strlen(buf) > 4 && buf[0] == '+' && buf[3] == '+') {
			cmd[cmd_size].locale[0] = buf[1];
			cmd[cmd_size].locale[1] = buf[2];
			cmd[cmd_size].locale[2] = '\0';
			top = BUF_OFS;
		} else {
			cmd[cmd_size].locale[0] = '\0';
		}

		/* 行頭の文字で仕分けする */
		switch (buf[top]) {
		case '\0':
		case '#':
#ifdef USE_DEBUGGER
			/* コメントを保存する */
			comment_text[line] = strdup(buf);
			if (comment_text[line] == NULL) {
				log_memory();
				return false;
			}
#else
			/* 空行とコメント行を読み飛ばす */
#endif
			break;
		case '@':
			/* 命令行をパースする */
			if (!parse_insn(cmd_size, fname, line, buf, top)) {
#ifdef USE_DEBUGGER
				if (is_parse_error) {
					cmd_size++;
					is_parse_error = false;
				} else {
					result = false;
				}
#else
				result = false;
#endif
			} else {
				cmd_size++;
			}
			break;
		case '*':
			/* セリフ行をパースする */
			if (!parse_serif(cmd_size, fname, line, buf, top)) {
#ifdef USE_DEBUGGER
				if (is_parse_error) {
					cmd_size++;
					is_parse_error = false;
				} else {
					result = false;
				}
#else
				result = false;
#endif
			} else {
				cmd_size++;
			}
			break;
		case ':':
			/* ラベル行をパースする */
			if (!parse_label(cmd_size, fname, line, buf, top)) {
#ifdef USE_DEBUGGER
				if (is_parse_error) {
					cmd_size++;
					is_parse_error = false;
				} else {
					result = false;
				}
#else
				result = false;
#endif
			} else {
				cmd_size++;
			}
			break;
		default:
			/* メッセージ行をパースする */
			if (!parse_message(cmd_size, fname, line, buf, top)) {
#ifdef USE_DEBUGGER
				if (is_parse_error) {
					cmd_size++;
					is_parse_error = false;
				} else {
					result = false;
				}
#else
				result = false;
#endif
			} else {
				cmd_size++;
			}
			break;
		}
		line++;
	}

	script_lines = line;

	close_rfile(rf);

	return result;
}

/* 命令行をパースする */
static bool parse_insn(int index, const char *file, int line, const char *buf,
		       int locale_offset)
{
	struct command *c;
	char *tp;
	int i, j, len, min = 0, max = 0;
	bool escaped;

#ifdef USE_DEBUGGER
	UNUSED_PARAMETER(file);
#endif

	c = &cmd[index];

	/* 行番号とオリジナルの行を保存しておく */
	c->line = line;
	c->text = strdup(buf);
	if (c->text == NULL) {
		log_memory();
		return false;
	}

	/* トークン化する文字列を複製する */
	c->param[0] = strdup(buf + locale_offset);
	if (c->param[0] == NULL) {
		log_memory();
		return false;
	}

	/* 最初のトークンを切り出す */
	strtok_escape(c->param[0], &escaped);

	/* コマンドのタイプを取得する */
	for (i = 0; i < (int)INSN_TBL_SIZE; i++) {
		if (strcasecmp(c->param[0], insn_tbl[i].str) == 0) {
			c->type = insn_tbl[i].type;
			min = insn_tbl[i].min;
			max = insn_tbl[i].max;
			break;
		}
	}
	if (i == INSN_TBL_SIZE) {
		log_script_command_not_found(c->param[0]);
#ifdef USE_DEBUGGER
		is_parse_error = true;
		cmd[index].text[0] = '!';
		set_error_command(index, cmd[index].text);
		if(error_count++ == 0)
			log_command_update_error();
#else
		log_script_parse_footer(file, line, buf);
#endif
		return false;
	}

	/* 2番目以降のトークンを取得する */
	i = 1;
	escaped = false;
	while ((tp = strtok_escape(NULL, &escaped)) != NULL &&
	       i < PARAM_SIZE) {
		if (strcmp(tp, "") == 0) {
			log_script_empty_string();
#ifdef USE_DEBUGGER
			is_parse_error = true;
			cmd[index].text[0] = '!';
			set_error_command(index, cmd[index].text);
			if(error_count++ == 0)
				log_command_update_error();
#else
			log_script_parse_footer(file, line, buf);
#endif
			return false;
		}

		/* パラメータ名がない場合 */
		if (escaped ||
		    c->type == COMMAND_SET || c->type == COMMAND_IF ||
		    strstr(tp, "=") == NULL) {
			c->param[i] = tp;
			i++;
			continue;
		}

		/* パラメータ名をチェックする */
		for (j = 0; j < (int)PARAM_TBL_SIZE; j++) {
			if (strncmp(param_tbl[j].name, tp,
				    strlen(param_tbl[j].name)) == 0) {
				/* 引数を保存する */
				c->param[i] = tp + strlen(param_tbl[j].name);

				/* エスケープする */
				len = (int)strlen(c->param[i]);
				if (c->param[i][0] == '\"' &&
				    c->param[i][len - 1] == '\"') {
					c->param[i][len - 1] = '\0';
					c->param[i]++;
				}
				i++;
				break;
			}
		}
		if (j == PARAM_TBL_SIZE) {
			*strstr(tp, "=") = '\0';
			log_script_param_mismatch(tp);
#ifdef USE_DEBUGGER
			is_parse_error = true;
			cmd[index].text[0] = '!';
			set_error_command(index, cmd[index].text);
			if(error_count++ == 0)
				log_command_update_error();
#else
			log_script_parse_footer(file, line, buf);
#endif
			return false;
		}
	}

	/* パラメータの数をチェックする */
	if (i - 1 < min) {
		log_script_too_few_param(min, i - 1);
#ifdef USE_DEBUGGER
		is_parse_error = true;
		cmd[index].text[0] = '!';
		set_error_command(index, cmd[index].text);
		if(error_count++ == 0)
			log_command_update_error();
#else
		log_script_parse_footer(file, line, buf);
#endif
		return false;
	}
	if (i - 1 > max) {
		log_script_too_many_param(max, i - 1);
#ifdef USE_DEBUGGER
		is_parse_error = true;
		cmd[index].text[0] = '!';
		set_error_command(index, cmd[index].text);
		if(error_count++ == 0)
			log_command_update_error();
#else
		log_script_parse_footer(file, line, buf);
#endif
		return false;
	}

	return true;
}

/* シングル/ダブルクォーテーションでエスケープ可能なトークナイズを実行する */
static char *strtok_escape(char *buf, bool *escaped)
{
	static char *top = NULL;
	char *result;

	/* 初回呼び出しの場合バッファを保存する */
	if (buf != NULL)
		top = buf;
	assert(top != NULL);

	/* すでにバッファの終端に達している場合NULLを返す */
	if (*top == '\0') {
		*escaped = false;
		return NULL;
	}

	/* 先頭のスペースをスキップする */
	for (; *top != '\0' && *top == ' '; top++)
		;
	if (*top == '\0') {
		*escaped = false;
		return NULL;
	}

	/* シングルクオーテーションでエスケープされている場合 */
	if (*top == '\'') {
		result = ++top;
		for (; *top != '\0' && *top != '\''; top++)
			;
		if (*top == '\'')
			*top++ = '\0';
		*escaped = true;
		return result;
	}

	/* ダブルクオーテーションでエスケープされている場合 */
	if (*top == '\"') {
		result = ++top;
		for (; *top != '\0' && *top != '\"'; top++)
			;
		if (*top == '\"')
			*top++ = '\0';
		*escaped = true;
		return result;
	}

	/* エスケープされていない場合 */
	result = top;
	for (; *top != '\0' && *top != ' '; top++)
		;
	if (*top == ' ')
		*top++ = '\0';
	*escaped = false;
	return result;
}

/* セリフ行をパースする */
static bool parse_serif(int index, const char *file, int line, const char *buf,
			int locale_offset)
{
	char *first, *second, *third;

	assert(buf[locale_offset] == '*');

#ifdef USE_DEBUGGER
	UNUSED_PARAMETER(file);
#endif

	/* 行番号とオリジナルの行を保存しておく */
	cmd[index].type = COMMAND_SERIF;
	cmd[index].line = line;
	cmd[index].text = strdup(buf);
	if (cmd[index].text == NULL) {
		log_memory();
		return false;
	}

	/* トークン化する文字列を複製する */
	cmd[index].param[0] = strdup(&buf[locale_offset + 1]);
	if (cmd[index].param[0] == NULL) {
		log_memory();
		return false;
	}

	/* トークンを取得する(2つか3つある) */
	first = strtok(cmd[index].param[0], "*");
	second = strtok(NULL, "*");
	third = strtok(NULL, "*");
	if (first == NULL || second == NULL) {
		log_script_empty_serif();
#ifdef USE_DEBUGGER
		is_parse_error = true;
		cmd[index].text[0] = '!';
		set_error_command(index, cmd[index].text);
		if(error_count++ == 0)
			log_command_update_error();
#else
		log_script_parse_footer(file, line, buf);
#endif
		return false;
	}

	/* トークンの数で場合分けする */
	if (third != NULL) {
		cmd[index].param[SERIF_PARAM_NAME] = first;
		cmd[index].param[SERIF_PARAM_VOICE] = second;
		cmd[index].param[SERIF_PARAM_MESSAGE] = third;
	} else {
		cmd[index].param[SERIF_PARAM_NAME] = first;
		cmd[index].param[SERIF_PARAM_VOICE] = NULL;
		cmd[index].param[SERIF_PARAM_MESSAGE] = second;
	}

	/* 成功 */
	return true;
}

/* メッセージ行をパースする */
static bool parse_message(int index, const char *file, int line,
			  const char *buf, int locale_offset)
{
	char *lpar, *p;

	UNUSED_PARAMETER(file);

	/* 行番号とオリジナルの行(メッセージ全体)を保存しておく */
	cmd[index].type = COMMAND_MESSAGE;
	cmd[index].line = line;
	cmd[index].text = strdup(buf);
	if (cmd[index].text == NULL) {
		log_memory();
		return false;
	}

	/* メッセージ(0番目のパラメータ)を複製する */
	cmd[index].param[MESSAGE_PARAM_MESSAGE] = strdup(buf + locale_offset);
	if (cmd[index].text == NULL) {
		log_memory();
		return false;
	}

	/* 名前「メッセージ」の形式の場合はセリフとする */
	p = cmd[index].param[MESSAGE_PARAM_MESSAGE];
	lpar = strstr(p, U8("「"));
	if (lpar != NULL && lpar != buf &&
	    strcmp(p + strlen(p) - 3, U8("」")) == 0) {
		/* セリフに変更する */
		cmd[index].type = COMMAND_SERIF;

		/* トークン化する */
		*(p + strlen(p) - 3) = '\0';
		*lpar = '\0';
		cmd[index].param[SERIF_PARAM_NAME] = p;
		cmd[index].param[SERIF_PARAM_VOICE] = NULL;
		cmd[index].param[SERIF_PARAM_MESSAGE] = lpar + 3;
	}

	/* 成功 */
	return true;
}

/* ラベル行をパースする */
		static bool parse_label(int index, const char *file, int line, const char *buf,
					int locale_offset)
{
	UNUSED_PARAMETER(file);

	/* 行番号とオリジナルの行(メッセージ全体)を保存しておく */
	cmd[index].type = COMMAND_LABEL;
	cmd[index].line = line;
	cmd[index].text = strdup(buf);
	if (cmd[index].text == NULL) {
		log_memory();
		return false;
	}

	/* ラベルを保存する */
	cmd[index].param[LABEL_PARAM_LABEL] = strdup(&buf[locale_offset + 1]);
	if (cmd[index].param[LABEL_PARAM_LABEL] == NULL) {
		log_memory();
		return false;
	}

	/* 成功 */
	return true;
}

#ifdef USE_DEBUGGER
/*
 * スタートアップファイル/ラインを指定する
 */
bool set_startup_file_and_line(const char *file, int line)
{
	startup_file = strdup(file);
	if (startup_file == NULL) {
		log_memory();
		return false;
	}
	startup_line = line;
	return true;
}

/*
 * スタートアップファイルが指定されたか
 */
bool has_startup_file(void)
{
	if (startup_file != NULL)
		return true;

	return false;
}

/*
 * 指定した行番号以降の最初のコマンドインデックスを取得する
 */
int get_command_index_from_line_number(int line)
{
	int i;

	for (i = 0; i < cmd_size; i++)
		if (cmd[i].line >= line)
			return i;

	return -1;
}

/*
 *  指定した行番号の行全体を取得する
 */
const char *get_line_string_at_line_num(int line)
{
	int i;

	/* コメント行の場合 */
	if (comment_text[line] != NULL)
		return comment_text[line];

	/* コマンドを探す */
	for (i = 0; i < cmd_size; i++) {
		if (cmd[i].line == line)
			return cmd[i].text;
		if (cmd[i].line > line)
			break;
	}

	/* 空行の場合 */
	return "";
}

/*
 * デバッグ用に1コマンドだけ書き換える
 */
bool update_command(int index, const char *cmd_str)
{
	int line;
	int top;

	/* メッセージに変換されるメッセージボックスを表示するようにする */
	error_count = 0;

	/* コマンドのメモリを解放する */
	if (cmd[index].text != NULL) {
		free(cmd[index].text);
		cmd[index].text = NULL;
	}
	if (cmd[index].param[0] != NULL) {
		free(cmd[index].param[0]);
		cmd[index].param[0] = NULL;
	}

	/* ロケールを処理する */
	top = 0;
	if (strlen(cmd_str) > 4 && cmd_str[0] == '+' && cmd_str[3] == '+') {
		cmd[index].locale[0] = cmd_str[1];
		cmd[index].locale[1] = cmd_str[2];
		cmd[index].locale[2] = '\0';
		top = 4;
	} else {
		cmd[index].locale[0] = '\0';
	}

	/* 行頭の文字で仕分けする */
	line = cmd[index].line;
	switch (cmd_str[4]) {
	case '@':
		if (!parse_insn(index, cur_script, line, cmd_str, top))
			return false;
		return true;
	case '*':
		if (!parse_serif(index, cur_script, line, cmd_str, top))
			return false;
		return true;
	case ':':
		if (!parse_label(index, cur_script, line, cmd_str, top))
			return false;
		return true;
	case '\0':
		/* 空行は空白1つに変換する */
		cmd_str = " ";
		/* fall-thru */
	case '#':
		/* コメントもメッセージにする */
		/* fall-thru */
	default:
		if (!parse_message(index, cur_script, line, cmd_str, top))
			return false;
		return true;
	}
	return true;
}

/*
 * エラー時のコマンドを設定する
 */
void set_error_command(int index, char *text)
{
	if (cmd[index].text != text)
		if (cmd[index].text != NULL)
			free(cmd[index].text);
	if (cmd[index].param[0] != NULL)
		free(cmd[index].param[0]);

	cmd[index].type = COMMAND_MESSAGE;
	cmd[index].text = text;
	cmd[index].param[0] = NULL;
}

/*
 * デバッグ用の仮のスクリプトをロードする
 */
bool load_debug_script(void)
{
	cleanup_script();

	cur_script = strdup("DEBUG");
	if (cur_script == NULL) {
		log_memory();
		cleanup_script();
		return false;
	}

	cur_index = 0;
	cmd_size = 1;
	script_lines = 1;

	cmd[0].type = COMMAND_MESSAGE;
	cmd[0].line = 0;
	cmd[0].text = strdup(conf_locale == LOCALE_JA ?
			     /* "実行を終了しました" (utf-8) */
			     "\xe5\xae\x9f\xe8\xa1\x8c\xe3\x82\x92\xe7\xb5\x82"
			     "\xe4\xba\x86\xe3\x81\x97\xe3\x81\xbe\xe3\x81\x97"
			     "\xe3\x81\x9f" :
			     "Execution finished.");
	if (cmd[0].text == NULL) {
		log_memory();
		cleanup_script();
		return false;
	}

	update_debug_info(true);
	return true;
}
#endif
