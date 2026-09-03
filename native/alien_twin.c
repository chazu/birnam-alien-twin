#include <Tw/Tw.h>
#include <Tw/Twerrno.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    tdisplay display;
    tmsgport message_port;
    tmenu menu;
    tscreen screen;
} AlienTwinConnection;

typedef struct {
    unsigned long type;
    unsigned long widget;
    unsigned long code;
    unsigned long shift_flags;
    int x;
    int y;
    int width;
    int height;
    char *text;
} AlienTwinEvent;

static char last_error[512] = "success";
static unsigned long connection_serial;

static void clear_error(void) {
    memcpy(last_error, "success", sizeof("success"));
}

static void set_error(const char *message) {
    if (message == NULL || *message == '\0') {
        message = "unknown Twin error";
    }
    (void)snprintf(last_error, sizeof(last_error), "%s", message);
}

static void capture_twin_error(tdisplay display) {
    const tw_errno *error = Tw_ErrnoLocation(display);
    const char *message = Tw_StrError(display, error->E);
    const char *detail = Tw_StrErrorDetail(display, error->E, error->S);
    (void)snprintf(last_error, sizeof(last_error), "%s%s", message, detail);
}

static AlienTwinConnection *valid_connection(void *opaque) {
    AlienTwinConnection *connection = (AlienTwinConnection *)opaque;
    if (connection == NULL || connection->display == NULL) {
        set_error("Twin connection is closed");
        return NULL;
    }
    return connection;
}

static int fits_dat(int value) {
    return value >= SHRT_MIN && value <= SHRT_MAX;
}

static int valid_window_id(unsigned long window) {
    if (window > UINT_MAX || window == TW_NOID) {
        set_error("invalid Twin window id");
        return 0;
    }
    return 1;
}

static int valid_rgb24(unsigned long rgb) {
    if (rgb > 0xFFFFFFul) {
        set_error("Twin RGB color is outside the 24-bit range");
        return 0;
    }
    return 1;
}

static trgb rgb24_to_trgb(unsigned long rgb) {
    return TRGB((byte)((rgb >> 16) & 0xFFu), (byte)((rgb >> 8) & 0xFFu),
                (byte)(rgb & 0xFFu));
}

static char *copy_text(const char *text, size_t length) {
    char *copy = (char *)malloc(length + 1);
    if (copy == NULL) {
        return NULL;
    }
    if (length != 0) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

const char *alien_twin_protocol_version_string(void) {
    return TW_PROTOCOL_VERSION_STR;
}

unsigned long alien_twin_protocol_version(void) {
    return (unsigned long)TW_PROTOCOL_VERSION;
}

const char *alien_twin_last_error(void) {
    return last_error;
}

void *alien_twin_open_display(const char *display_name) {
    AlienTwinConnection *connection;
    char message_port_name[64];
    tcolor normal = TCOL(tblack, twhite);
    tcolor selected = TCOL(tblack, tgreen);
    tcolor disabled = TCOL(tBLACK, twhite);
    tcolor selected_disabled = TCOL(tBLACK, tblack);
    tcolor shortcut = TCOL(tred, twhite);
    tcolor selected_shortcut = TCOL(tred, tgreen);

    if (display_name != NULL && *display_name == '\0') {
        display_name = NULL;
    }

    connection = (AlienTwinConnection *)calloc(1, sizeof(*connection));
    if (connection == NULL) {
        set_error("could not allocate a Twin connection");
        return NULL;
    }

    connection->display = Tw_Open(display_name);
    if (connection->display == NULL) {
        capture_twin_error(NULL);
        free(connection);
        return NULL;
    }

    ++connection_serial;
    (void)snprintf(message_port_name, sizeof(message_port_name), "birnam-%ld-%lu",
                   (long)getpid(), connection_serial);
    connection->message_port =
        Tw_CreateMsgPort(connection->display, (byte)strlen(message_port_name), message_port_name);
    connection->screen = Tw_FirstScreen(connection->display);
    connection->menu = Tw_CreateMenu(connection->display, normal, selected, disabled,
                                     selected_disabled, shortcut, selected_shortcut, (byte)0);

    if (connection->message_port == TW_NOID || connection->screen == TW_NOID ||
        connection->menu == TW_NOID) {
        capture_twin_error(connection->display);
        Tw_Close(connection->display);
        free(connection);
        return NULL;
    }

    (void)Tw_Create4MenuCommonMenuItem(connection->display, connection->menu);
    clear_error();
    return connection;
}

void *alien_twin_open_default(void) {
    return alien_twin_open_display(NULL);
}

int alien_twin_close(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL) {
        return 0;
    }
    Tw_Close(connection->display);
    connection->display = NULL;
    free(connection);
    clear_error();
    return 1;
}

int alien_twin_connection_fd(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    return connection == NULL ? -1 : Tw_ConnectionFd(connection->display);
}

unsigned long alien_twin_library_version(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    return connection == NULL ? 0 : (unsigned long)Tw_LibraryVersion(connection->display);
}

unsigned long alien_twin_server_version(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    return connection == NULL ? 0 : (unsigned long)Tw_ServerVersion(connection->display);
}

int alien_twin_flush(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL) {
        return 0;
    }
    if (!Tw_Flush(connection->display)) {
        capture_twin_error(connection->display);
        return 0;
    }
    clear_error();
    return 1;
}

int alien_twin_sync(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL) {
        return 0;
    }
    if (!Tw_Sync(connection->display)) {
        capture_twin_error(connection->display);
        return 0;
    }
    clear_error();
    return 1;
}

int alien_twin_in_panic(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    return connection == NULL ? 1 : (int)Tw_InPanic(connection->display);
}

int alien_twin_display_width(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    return connection == NULL ? 0 : (int)Tw_GetDisplayWidth(connection->display);
}

int alien_twin_display_height(void *opaque) {
    AlienTwinConnection *connection = valid_connection(opaque);
    return connection == NULL ? 0 : (int)Tw_GetDisplayHeight(connection->display);
}

unsigned long alien_twin_create_window_configured(void *opaque, const char *title, int width,
                                                  int height, unsigned long cursor_type,
                                                  unsigned long attributes, unsigned long flags) {
    AlienTwinConnection *connection = valid_connection(opaque);
    size_t title_length;
    twindow window;
    tcolor text_color = TCOL(tblack, twhite);

    if (connection == NULL) {
        return 0;
    }
    if (title == NULL) {
        title = "";
    }
    title_length = strlen(title);
    if (title_length > SHRT_MAX || !fits_dat(width) || !fits_dat(height) || width <= 0 ||
        height <= 0) {
        set_error("window title or dimensions are outside Twin's range");
        return 0;
    }

    window = Tw_CreateWindow(connection->display, (dat)title_length, title, NULL, connection->menu,
                             text_color, (uldat)cursor_type, (uldat)attributes, (uldat)flags,
                             (dat)width, (dat)height, (dat)0);
    if (window == TW_NOID) {
        capture_twin_error(connection->display);
        return 0;
    }
    Tw_MapWindow(connection->display, window, connection->screen);
    if (!Tw_Flush(connection->display)) {
        capture_twin_error(connection->display);
        return 0;
    }
    clear_error();
    return (unsigned long)window;
}

unsigned long alien_twin_create_window(void *opaque, const char *title, int width, int height) {
    return alien_twin_create_window_configured(
        opaque, title, width, height, TW_LINECURSOR,
        TW_WINDOW_WANT_KEYS | TW_WINDOW_WANT_MOUSE | TW_WINDOW_WANT_CHANGES | TW_WINDOW_DRAG |
            TW_WINDOW_RESIZE | TW_WINDOW_CLOSE,
        TW_WINDOWFL_ROWS_DEFCOL);
}

int alien_twin_delete_window(void *opaque, unsigned long window) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_RecursiveDeleteWindow(connection->display, (twindow)window);
    clear_error();
    return 1;
}

int alien_twin_write_utf8(void *opaque, unsigned long window, const char *text) {
    AlienTwinConnection *connection = valid_connection(opaque);
    size_t length;
    if (connection == NULL || !valid_window_id(window) || text == NULL) {
        set_error("invalid Twin window id or UTF-8 text");
        return 0;
    }
    length = strlen(text);
    if (length > UINT_MAX) {
        set_error("UTF-8 text is too long for Twin");
        return 0;
    }
    Tw_WriteUtf8Window(connection->display, (twindow)window, (uldat)length, text);
    clear_error();
    return 1;
}

int alien_twin_set_title(void *opaque, unsigned long window, const char *title) {
    AlienTwinConnection *connection = valid_connection(opaque);
    size_t length;
    if (connection == NULL || !valid_window_id(window) || title == NULL) {
        set_error("invalid Twin window id or title");
        return 0;
    }
    length = strlen(title);
    if (length > SHRT_MAX) {
        set_error("Twin window title is too long");
        return 0;
    }
    Tw_SetTitleWindow(connection->display, (twindow)window, (dat)length, title);
    clear_error();
    return 1;
}

int alien_twin_goto(void *opaque, unsigned long window, int x, int y) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_GotoXYWindow(connection->display, (twindow)window, (ldat)x, (ldat)y);
    clear_error();
    return 1;
}

int alien_twin_resize(void *opaque, unsigned long window, int width, int height) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window) || !fits_dat(width) || !fits_dat(height) ||
        width <= 0 || height <= 0) {
        set_error("invalid Twin window id or dimensions");
        return 0;
    }
    Tw_ResizeWindow(connection->display, (twindow)window, (dat)width, (dat)height);
    clear_error();
    return 1;
}

int alien_twin_move(void *opaque, unsigned long window, int x, int y) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window) || !fits_dat(x) || !fits_dat(y)) {
        set_error("invalid Twin window id or position");
        return 0;
    }
    Tw_SetXYWindow(connection->display, (twindow)window, (dat)x, (dat)y);
    clear_error();
    return 1;
}

int alien_twin_scroll(void *opaque, unsigned long window, int dx, int dy) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_ScrollWindow(connection->display, (twindow)window, (ldat)dx, (ldat)dy);
    clear_error();
    return 1;
}

int alien_twin_set_visible(void *opaque, unsigned long window, int visible) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_SetVisibleWidget(connection->display, (twindow)window,
                        visible ? (byte)ttrue : (byte)tfalse);
    clear_error();
    return 1;
}

int alien_twin_focus(void *opaque, unsigned long window) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_FocusSubWidget(connection->display, (twindow)window);
    clear_error();
    return 1;
}

int alien_twin_raise(void *opaque, unsigned long window) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_RaiseWindow(connection->display, (twindow)window);
    clear_error();
    return 1;
}

int alien_twin_lower(void *opaque, unsigned long window) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window)) {
        return 0;
    }
    Tw_LowerWindow(connection->display, (twindow)window);
    clear_error();
    return 1;
}

int alien_twin_set_text_color(void *opaque, unsigned long window, unsigned long foreground,
                              unsigned long background) {
    AlienTwinConnection *connection = valid_connection(opaque);
    if (connection == NULL || !valid_window_id(window) || !valid_rgb24(foreground) ||
        !valid_rgb24(background)) {
        return 0;
    }
    Tw_SetColTextWindow(connection->display, (twindow)window,
                        TCOL(rgb24_to_trgb(foreground), rgb24_to_trgb(background)));
    clear_error();
    return 1;
}

int alien_twin_fill_rect(void *opaque, unsigned long window, int x, int y, int width, int height,
                         unsigned long codepoint, unsigned long foreground,
                         unsigned long background) {
    AlienTwinConnection *connection = valid_connection(opaque);
    tcell *row;
    tcell cell;
    int line;

    if (connection == NULL || !valid_window_id(window) || !fits_dat(x) || !fits_dat(y) ||
        !fits_dat(width) || !fits_dat(height) || width <= 0 || height <= 0 ||
        (long)y + (long)height - 1 > SHRT_MAX || codepoint > 0x10FFFFul ||
        !valid_rgb24(foreground) || !valid_rgb24(background)) {
        set_error("invalid Twin fill rectangle, codepoint, or color");
        return 0;
    }

    row = (tcell *)malloc((size_t)width * sizeof(*row));
    if (row == NULL) {
        set_error("could not allocate a Twin fill row");
        return 0;
    }
    cell = TCELL(TCOL(rgb24_to_trgb(foreground), rgb24_to_trgb(background)),
                 (trune)codepoint);
    for (int column = 0; column < width; ++column) {
        row[column] = cell;
    }
    for (line = 0; line < height; ++line) {
        Tw_DrawTCellWindow(connection->display, (twindow)window, (dat)width, (dat)1, (dat)x,
                           (dat)(y + line), (dat)width, row);
    }
    free(row);
    clear_error();
    return 1;
}

void *alien_twin_next_event(void *opaque, int wait) {
    AlienTwinConnection *connection = valid_connection(opaque);
    AlienTwinEvent *event;
    tmsg message;

    if (connection == NULL) {
        return NULL;
    }
    message = Tw_ReadMsg(connection->display, wait ? (byte)ttrue : (byte)tfalse);
    if (message == NULL) {
        if (Tw_InPanic(connection->display)) {
            capture_twin_error(connection->display);
        }
        return NULL;
    }

    event = (AlienTwinEvent *)calloc(1, sizeof(*event));
    if (event == NULL) {
        set_error("could not allocate a Twin event");
        return NULL;
    }
    event->type = (unsigned long)message->Type;
    event->widget = (unsigned long)message->Event.EventCommon.W;
    event->code = (unsigned long)message->Event.EventCommon.Code;

    switch ((udat)message->Type) {
    case TW_MSG_WIDGET_KEY:
        event->shift_flags = (unsigned long)message->Event.EventKeyboard.ShiftFlags;
        event->text = copy_text(message->Event.EventKeyboard.AsciiSeq,
                                (size_t)message->Event.EventKeyboard.SeqLen);
        break;
    case TW_MSG_WIDGET_MOUSE:
        event->shift_flags = (unsigned long)message->Event.EventMouse.ShiftFlags;
        event->x = (int)message->Event.EventMouse.X;
        event->y = (int)message->Event.EventMouse.Y;
        break;
    case TW_MSG_WIDGET_CHANGE:
        event->shift_flags = (unsigned long)message->Event.EventWidget.Flags;
        event->x = (int)message->Event.EventWidget.X;
        event->y = (int)message->Event.EventWidget.Y;
        event->width = (int)message->Event.EventWidget.XWidth;
        event->height = (int)message->Event.EventWidget.YWidth;
        break;
    case TW_MSG_WIDGET_GADGET:
        event->shift_flags = (unsigned long)message->Event.EventGadget.Flags;
        break;
    case TW_MSG_USER_CONTROL:
    case TW_MSG_USER_CONTROL_REPLY:
        event->x = (int)message->Event.EventControl.X;
        event->y = (int)message->Event.EventControl.Y;
        event->text = copy_text(message->Event.EventControl.Data,
                                (size_t)message->Event.EventControl.Len);
        break;
    default:
        break;
    }

    if ((message->Type == TW_MSG_WIDGET_KEY || message->Type == TW_MSG_USER_CONTROL ||
         message->Type == TW_MSG_USER_CONTROL_REPLY) &&
        event->text == NULL) {
        free(event);
        set_error("could not copy Twin event text");
        return NULL;
    }
    clear_error();
    return event;
}

unsigned long alien_twin_event_type(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->type;
}

unsigned long alien_twin_event_widget(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->widget;
}

unsigned long alien_twin_event_code(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->code;
}

unsigned long alien_twin_event_shift_flags(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->shift_flags;
}

int alien_twin_event_x(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->x;
}

int alien_twin_event_y(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->y;
}

int alien_twin_event_width(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->width;
}

int alien_twin_event_height(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? 0 : event->height;
}

const char *alien_twin_event_text(void *opaque) {
    const AlienTwinEvent *event = (const AlienTwinEvent *)opaque;
    return event == NULL ? NULL : event->text;
}

int alien_twin_event_free(void *opaque) {
    AlienTwinEvent *event = (AlienTwinEvent *)opaque;
    if (event == NULL) {
        return 0;
    }
    free(event->text);
    free(event);
    return 1;
}
