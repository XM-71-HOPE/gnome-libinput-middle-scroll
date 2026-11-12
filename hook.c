#define _GNU_SOURCE
#include <dlfcn.h>
#include <libinput.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>

#define SCROLL_BUTTON 274  /* BTN_MIDDLE */

#include <stdarg.h>
static int logmsg(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    return write(2, buf, n > 0 ? n : 0);
}

static const char *real_name(struct libinput_device *d) {
    static const char *(*f)(struct libinput_device *) = NULL;
    if (!f) f = dlsym(RTLD_NEXT, "libinput_device_get_name");
    return f ? f(d) : "?";
}

static void force(struct libinput_device *device) {
    if (!device) return;
    uint32_t methods = libinput_device_config_scroll_get_methods(device);
    if (!(methods & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN)) return;
    /* 触控板支持双指滚动，跳过它，只处理鼠标 */
    if (methods & LIBINPUT_CONFIG_SCROLL_2FG) return;

    /* 直接走真实函数，避免递归 */
    static uint32_t (*real_set_button)(struct libinput_device *, uint32_t) = NULL;
    static enum libinput_config_status (*real_set_method)(
        struct libinput_device *, enum libinput_config_scroll_method) = NULL;
    if (!real_set_button)
        real_set_button = dlsym(RTLD_NEXT, "libinput_device_config_scroll_set_button");
    if (!real_set_method)
        real_set_method = dlsym(RTLD_NEXT, "libinput_device_config_scroll_set_method");

    if (real_set_button) real_set_button(device, SCROLL_BUTTON);
    if (real_set_method) real_set_method(device, LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN);

    static int logged = 0;
    if (!logged) {
        logged = 1;
        logmsg("[scroll-hook] force dev=%s methods=0x%x now=%d\n",
                 real_name(device), methods,
                 libinput_device_config_scroll_get_method(device), 0, 0);
    }
}

const char *libinput_device_get_name(struct libinput_device *device) {
    force(device);
    return real_name(device);
}

/* Mutter 若调用这两个，强制改回来并记录 */
enum libinput_config_status libinput_device_config_scroll_set_method(
    struct libinput_device *device,
    enum libinput_config_scroll_method method) {
    static enum libinput_config_status (*real)(
        struct libinput_device *, enum libinput_config_scroll_method) = NULL;
    if (!real)
        real = dlsym(RTLD_NEXT, "libinput_device_config_scroll_set_method");

    uint32_t methods = libinput_device_config_scroll_get_methods(device);
    int is_mouse = (methods & LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN) &&
                   !(methods & LIBINPUT_CONFIG_SCROLL_2FG);

    static int logged = 0;
    if (!logged) {
        logged = 1;
        logmsg("[scroll-hook] set_method dev=%s req=%d mouse=%d\n",
               real_name(device), (int)method, is_mouse, 0);
    }

    if (!is_mouse)
        return real(device, method);

    return real(device, LIBINPUT_CONFIG_SCROLL_ON_BUTTON_DOWN);
}

uint32_t libinput_device_config_scroll_set_button(
    struct libinput_device *device, uint32_t button) {
    static uint32_t (*real)(struct libinput_device *, uint32_t) = NULL;
    if (!real)
        real = dlsym(RTLD_NEXT, "libinput_device_config_scroll_set_button");
    (void)button;
    return real(device, SCROLL_BUTTON);
}
