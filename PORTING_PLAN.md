# GTK3 Porting Plan

Assessment of the Sylpheed 3.8.0beta1 tree (rev 1187) and a plan to move it from GTK+ 2 to GTK+ 3.24.
Based on a read-through and API census of the source; nothing has been built or run yet.

Hard requirement: full functionality as a Windows 10/11 mail client.
Secondary: do not close the door on Linux, BSD and macOS.

## 0. Decisions (settled)

- **Target:** GTK 3.24, 64-bit Windows 10/11 only. Keep the code clean for both pointer sizes so a 32-bit build stays possible.
- **Goal of this plan:** a fully functional GTK3 build. Riskier structural changes come afterwards (section 4.3).
- **Menus:** item-factory-compatible facade in `menu.c`, maintained in-tree. `GMenu`/`GAction` is a later option behind the same interface.
- **Build system:** autotools, under MSYS2/MinGW-w64. Meson is a possible later project.
- **Refactors:** all optional refactors are deferred until after the port. The port changes only what GTK3 forces.
- **Plugins:** third-party binary plugins will break and that is accepted. The plugin framework itself is ported as completely as possible: every exported function in `libsylpheed-plugin-0.def` keeps its name and meaning where GTK3 allows, both in-tree plugins are ported, and `PLUGIN.txt` gains a migration note for plugin authors listing every signature or behaviour change.

## 1. Shape of the codebase

| Part | Size | GTK dependency |
|---|---|---|
| `libsylph/` (protocols, folders, MIME, filters, prefs, sockets, TLS, OAuth2) | 39k lines, 39 files | None. GLib/GObject only. |
| `src/` front end | 100k lines, 97 `.c` files | 85 files use GTK |
| `src/` address book back end (`addrbook`, `addrcache`, `addrindex`, `addritem`, `ldif`, `vcard`, `jpilot`, `mgutils`) plus `template.c`, `manual.c` | ~10k lines | None, but compiled into the GUI binary |
| `src/plugin.c` + `plugin.h` (`libsylpheed-plugin-0`) | 1.5k lines, 114 exports | Exposes `GtkWidget`, `GtkMenu`, `GtkTextView` and item-factory menu paths |
| `plugin/attachment_tool`, `plugin/test` | small | GTK2 |

The important structural fact: the mail engine is already a separate GTK-free library. The port is a front-end port, not a rewrite.

The main message list, folder tree, MIME tree and address book already use `GtkTreeView` (678 calls), and text display uses `GtkTextView`. Both carry over to GTK3 nearly unchanged.

## 2. Where logic and UI are tangled

Share of each file's function bodies that contain no GTK calls (rough, script-measured):

| File | Lines | GTK-free | What is mixed in |
|---|---|---|---|
| `compose.c` | 8612 | low | Message serialisation reads widgets directly: `compose_write_to_file` (370 lines), `compose_write_headers` (259), `compose_write_attach` (218) pull from the `GtkTextBuffer`, header `GtkEntry`s and the attachment `GtkListStore`. Reply/forward/quote logic, send/queue/draft, external editor and PGP armour all live here. 35 `alertpanel` calls. |
| `summaryview.c` | 6683 | 11% | The `GtkTreeStore` is the only copy of the message-list state. Mark, delete, move, filter, thread and sort all walk tree rows. |
| `folderview.c` | 3405 | 8% | Folder operations (create, rename, move, IMAP download) run inside menu callbacks, with 24 `alertpanel` calls. |
| `mainwindow.c` | 4276 | 12% | Mostly UI. Holds the 238-entry main menu. |
| `inc.c` | 1940 | 65% | Receive controller (per-account session queue, POP3 state handling, error mapping) fused to its progress dialog. |
| `send_message.c` | 1038 | 64% | Same pattern for SMTP and the queue. |
| `rpop3.c` | 1418 | 16% | Remote POP3 browser; session logic inside dialog code. |
| `rfc2015.c` | 1992 | 42% | PGP/MIME sign, encrypt, decrypt (pure GPGME) mixed with passphrase and key-selection dialogs. |
| `action.c` | 1373 | 26% | Command parsing and child-process I/O mixed with dialogs; includes X11 headers. |
| `query_search.c` | 1698 | 29% | Search runs on a worker thread and feeds a queue; the driver is inside the dialog. |
| `update_check.c` | 847 | 60% | Version fetch/compare mixed with prompts. |
| `prefs_common_dialog.c`, `prefs_account_dialog.c` | 7607 | 1–3% | Pure UI. Bind to `libsylph` `PrefParam` tables through `prefs_ui.c`. This is already a clean split. |
| `main.c` | 2083 | — | Start-up, single-instance IPC, command-line remote, Win32 window procedure (power/session events), gtkrc loading, 36 `G_OS_WIN32` blocks. |

### Core-to-UI seams that already exist

`libsylph` reaches the UI only through registered function pointers and `syl_app` GObject signals:

- `set_ui_update_func`, `set_event_loop_func`, `set_progress_func`, `set_input_query_password_func`, `set_log_ui_func`, `set_log_show_status_func`
- `ssl_set_verify_func`, `filter_set_addressbook_func`, `procmsg_set_decrypt_message_func`
- `folder_set_ui_func[2]`, `recv_set_ui_func`, per-session notify callbacks
- signals: `add-msg`, `remove-msg`, `remove-all-msg`, `remove-folder`, `move-folder`, `folderlist-updated`, `account-updated`, `init-done`, `app-exit`

These are toolkit-neutral and need no change for GTK3.

### Concurrency model (the main structural risk)

- The core blocks and pumps the UI. `imap.c`, `socket.c` and `utils.c` call `ui_update()` / `event_loop_iterate()`, which the front end maps to `gtk_main_iteration()`. Any long IMAP operation re-enters the main loop from deep inside the call stack.
- 30 direct `gtk_events_pending`/`gtk_main_iteration` pump sites in 18 front-end files, and many dialogs run their own nested `gtk_main()`.
- 91 `gdk_threads_enter/leave` sites in 17 files, with a custom recursive lock installed via `gdk_threads_set_lock_functions`.
- Real worker threads exist only in the IMAP thread pool, async socket connect, async command execution and query search. From what I read they do not touch GTK; they signal the main thread, which waits by pumping. This needs confirming site by site.

`gdk_threads_*` is deprecated in GTK 3.6 and was never reliable on the Win32 backend. The nested pumping still works in GTK3, so it can stay for the port, but it is the source of most re-entrancy bugs and is what blocks any later GTK4 move.

## 3. GTK2-only API census

Removed in GTK3 (must be replaced):

| API | Uses | Files | Replacement |
|---|---|---|---|
| `GtkItemFactory` | 167 calls, ~590 menu entries, ~210 lookups by path | 23 | See section 4.1 |
| `GtkCList` | 418 | 17 (`editaddress`, `importcsv`, `prefs_display_header`, `prefs_display_items`, `prefs_summary_column`, `prefs_template`, `prefs_actions`, `select-keys`, `prefs_customheader`, `importldif`, `addr_compl`, `prefs_filter_edit`, …) | `GtkTreeView` + `GtkListStore` |
| `GtkCTree` | 33 | `addressadd.c` only | `GtkTreeView` + `GtkTreeStore` |
| `GtkOptionMenu` | 152 | 15 | `GtkComboBox` / `GtkComboBoxText` |
| `GtkCombo` | 15 | 6 | `GtkComboBoxText` with entry |
| `GtkTooltips` | 32 | 10 | `gtk_widget_set_tooltip_text` |
| Direct struct access (`widget->window`, `->allocation`, `->style`, `dialog->vbox`, adjustment fields) | ~450 | 30+ (`textview` 72, `mainwindow` 45, `folderview` 31, `compose` 28) | Accessor functions |
| `GTK_WIDGET_*` flag macros, `GtkObject` | ~135 | 36 | `gtk_widget_get_visible` etc., `GObject` |
| `GdkPixmap`/`GdkBitmap`/`GdkGC`/`gdk_draw_*`/`expose_event` | ~50 | 14 | cairo, `draw` signal, `GdkPixbuf` |
| `GdkColormap`, `gdk_rgb_*`, colour allocation | ~20 | 7 | Delete; `GdkRGBA` |
| `GdkColor`, `gtk_widget_modify_*`, `GtkStyle`, gtkrc parsing | ~100 | 20 | `GdkRGBA`, `GtkStyleContext`, CSS provider |
| `GDK_<key>` keysyms | part of 414 | many | `GDK_KEY_<key>` |
| `gtk_widget_set_usize`/`set_uposition`, `size_request`, `set_has_separator`, `gtk_set_locale` | ~60 | 35 | Modern equivalents or delete |
| `eggtrayicon.c` (X11 XEmbed) | 583 lines | 1 | Delete; `GtkStatusIcon` path already exists |
| GtkSpell 2 | 11 | `compose.c` | gtkspell3 (aspell branch dropped, enchant-2) |
| Sub-header includes (`<gtk/gtkvbox.h>`) | widespread | most | `<gtk/gtk.h>` only |

Deprecated in GTK3 but still functional (can ship, pay down later):

| API | Uses | Files |
|---|---|---|
| `gtk_hbox_new`/`gtk_vbox_new`/paned/separator variants | 499 | 59 |
| `GtkTable` | 367 | 20 |
| Stock items (`GTK_STOCK_*`, image-from-stock) | 305 | 56 |
| `GtkMisc` alignment, `GtkAlignment` | 152 | 28 |
| `GtkUIManager`/`GtkAction` | 29 | `rpop3.c`, `query_search.c` |
| `GtkStatusIcon` | ~25 | `trayicon.c` |
| `gdk_threads_*` | 91 | 17 |
| `gtk_menu_popup`, `gdk_screen_width/height`, rules hint | ~60 | many |

Custom drawing is small: `colorlabel.c` (colour swatches), one border painter in `textview.c`, `gtkshruler.c` (compose ruler widget), the X-Face bitmap in `headerview.c`, and `stock_pixmap.c`'s pixmap cache.

The code still carries `GTK_CHECK_VERSION` branches back to GTK 2.8 (~90 sites, 20 files). Those go first.

## 4. Decoupling: what to do, and what not to

Decoupling is not a precondition for GTK3. `GtkTreeView`, `GtkTextView` and the dialog structure all survive. A large model/view refactor before the port would double the risk for little GTK3 benefit. The recommendation is to decouple only where the port forces a rewrite anyway, and record the rest as follow-up work that mainly pays off for a later GTK4 or non-GTK front end.

### 4.1 Required by the port

1. **Menus.** `GtkItemFactory` is gone, and menu paths are load-bearing: sensitivity updates look items up by path, the plugin API registers items by path, and the user's `menurc` accelerator file and the key-binding presets in `prefs_common_dialog.c` use those paths.
   Recommended: reimplement the small subset of the item-factory interface inside `menu.c` (build `GtkMenu`/`GtkMenuItem` from the existing entry tables, keep path lookup, register `GtkAccelMap` paths). The 590 entries, all callers, the plugin API and users' saved shortcuts stay as they are, and it works on GTK2 and GTK3 alike.
   Alternatives: `GtkUIManager` (already deprecated, means rewriting every table) or `GMenu`/`GAction` (cleanest long term, GTK3-only, breaks accelerator paths and the plugin menu API). Either could follow later behind the same `menu.c` facade.
2. **Threading.** Remove `gdk_threads_*` and the custom lock. Confirm no worker thread calls GTK; route any that do through `g_idle_add`. Keep the nested-loop pumping for now.
3. **Theming.** Replace gtkrc loading and `modify_*` calls with one application CSS provider. This is also where dark mode comes from.
4. **Plugin framework.** GTK2 and GTK3 cannot share a process, so every existing binary plugin stops loading. Bump the plugin interface version and soname. Keep the exported API source-compatible wherever possible: the menu facade preserves `syl_plugin_add_menuitem`/`add_factory_item` and their path arguments, and widget getters keep returning the same kinds of widget. The public header must not expose `GtkItemFactory` types (they no longer exist); where one appeared, substitute an opaque type owned by `menu.h`. Port both in-tree plugins and document every change for plugin authors in `PLUGIN.txt`.

### 4.2 Small structural clean-ups (deferred until after the port)

- Move the GTK-free address book back end and `template.c` out of `src/` into a library target so the compiler enforces the boundary.
- Extract a plain `ComposeDraft` structure (headers, body text, attachment list, options) and make the `compose_write_*` functions take it instead of reading widgets. Makes message generation testable without a window.
- Split `main.c`: platform start-up (Win32 window procedure, console, ini file), IPC, and GTK initialisation in separate files.

### 4.3 Larger follow-up work (after a working GTK3 release)

- Replace the menu facade with `GMenu`/`GAction`.
- Controllers for receive/send/remote-POP3 with progress callbacks instead of owning dialogs.
- Separate PGP/MIME engine from its dialogs in `rfc2015.c`.
- Replace `alertpanel` calls in folder/summary operations with returned errors.
- A message-list model independent of `GtkTreeStore`.
- Replace `ui_update()`/`event_loop_iterate()` pumping in `libsylph` with asynchronous operations. This is the real prerequisite for GTK4.

## 5. Phased plan

The app stays buildable and shippable on GTK2 through phases 1–3. Only phase 4 switches toolkit.

**Phase 0 — Baseline.**
Install MSYS2 and a 64-bit MinGW-w64 toolchain (UCRT64 environment preferred, MINGW64 if a needed package is missing there). Reproducible 64-bit Windows build of the tree against GTK 2.24 with every option the official Windows build used (GPGME, OpenSSL, Oniguruma, threads, libcurl, compface, GtkSpell); record the exact package list and commands in `BUILD-WIN64.md`. Fix only what is needed to build and run: modern autoconf/automake macro updates, OpenSSL 3 (`ssl.c` uses pre-1.1 initialisation and the old `ssleay32`/`libeay32` link names), and the Win64 audit (the code predates Win64, where `long` is 32-bit and pointers are 64-bit; for example `(glong)GetStdHandle(...)` in `main.c`). Build with pointer/int cast and format warnings enabled and clear them all in `libsylph` and `src`. Check whether on-disk cache formats depend on type sizes. Write the manual regression checklist (section 7) as `TESTING.md`.
If GTK2 is not available as a 64-bit package, stop and reassess: phases 1–3 depend on it.

**Phase 1 — Hygiene on GTK 2.24.**
Raise the minimum to 2.24 and delete all older `GTK_CHECK_VERSION` branches. Delete `eggtrayicon`. Single-header includes. Then build with `-DGSEAL_ENABLE -DGTK_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED` (`configure --disable-deprecated` already exists) and fix: accessors for struct fields, widget flag macros, `GtkObject`, tooltips, `set_usize`, keysym names. Mechanical, large in volume, low in risk.

**Phase 2 — Widget replacements on GTK 2.24.**
Menu facade (4.1). `GtkCList`/`GtkCTree` to `GtkTreeView`, one dialog at a time. `GtkOptionMenu`/`GtkCombo` to `GtkComboBox`. Drawing code to cairo. `GdkPixmap` to `GdkPixbuf`. In-tree plugins follow.

**Phase 3 — Threading.**
Remove `gdk_threads_*` and the custom lock, after confirming per site that no worker thread calls GTK.

**Phase 4 — Switch to GTK 3.24.**
`configure.ac` to `gtk+-3.0 >= 3.24`, gtkspell3, gtk-mac-integration-gtk3. `expose_event` to `draw`. `GdkColor` to `GdkRGBA`. CSS provider replaces gtkrc. Size negotiation (`get_preferred_*`) in `gtkshruler`. `gdk_win32_drawable_get_handle` to `gdk_win32_window_get_handle`. Fix what the compiler and the regression checklist turn up. No dual GTK2/GTK3 build is kept after this point.

**Phase 5 — Windows product work.**
Installer: the NSIS script bundles a GTK2 runtime by DLL name and must be rebuilt for the GTK3 set (GTK, GDK, Pango, cairo, HarfBuzz, epoxy, gdk-pixbuf loaders, GSettings schemas, Adwaita and hicolor icon themes). Fonts and HiDPI (`fix_font_setting`, `gtkut_get_dpi` become mostly unnecessary). Tray icon, new-mail notification window, IME input, drag-and-drop with Explorer, printing, native file dialog (`filesel.c` has Win32 COM code), `mailto:` registration, single-instance IPC, suspend/resume handling. Dark mode via the CSS provider and an icon check.

**Phase 6 — Deprecation paydown (optional).**
`GtkTable` to `GtkGrid`, H/V boxes to `GtkBox`, stock items to icon names, `GtkUIManager` users, `GtkStatusIcon` to a native `Shell_NotifyIcon` on Windows.

Relative size: phases 1 and 2 are roughly two thirds of the total effort. Phase 4 is short if 1–3 are complete. Phase 5 is where most Windows-visible bugs will surface.

## 6. Other platforms with GTK3

Viable on all of them; none needs work now beyond not adding Windows-only assumptions.

- **Linux / BSD (X11):** GTK 3.24 is standard. The existing Unix code paths (Unix-domain IPC socket, async connect, `fork`-based actions) are untouched by the port. The X11-specific pointer/keyboard grabs in `passphrase.c` and the X11 include in `action.c` need `GDK_WINDOWING_X11` runtime checks rather than compile-time ones.
- **Wayland:** runs, with known gaps to plan for: `GtkStatusIcon` does not work (needs StatusNotifier/AppIndicator), windows cannot position themselves (affects the notification popup and saved window positions), and no global grabs.
- **macOS:** GTK3 has a native Quartz backend, and the existing `gtkosx_application` integration has a GTK3 build. The bundle scripts in `macosx/` need redoing. Lowest confidence of the three because it cannot be tested here.

Keeping this open costs three habits: keep platform code behind the existing `G_OS_WIN32`/`__APPLE__` guards, keep autotools working (or move to Meson for all platforms at once), and prefer GTK facilities over Win32 calls where both exist.

## 7. Regression checklist areas

Accounts (POP3, IMAP4, NNTP, SMTP; TLS/STARTTLS; OAuth2 via `syl-auth-helper`; SOCKS proxy) · receive (manual, auto-check, remote POP3) · send, queue, drafts · compose (reply/forward/redirect, templates, signatures, attachments, external editor, spell check, undo, line wrap, ruler) · folders (MH, IMAP, news, virtual/search folders) · summary (threading, sorting, columns, colour labels, marks, quick search, query search) · message view (headers, X-Face, images, MIME tree, HTML-to-text, links, source view) · filters and junk (bsfilter) · actions · address book (edit, groups, LDIF/CSV/vCard import, CSV export, completion; LDAP and JPilot where built) · PGP sign/encrypt/verify · import/export mbox · printing · preferences (all pages, key-binding presets, toolbar customisation) · plugins and plugin manager · tray icon and notifications · update check · command-line remote operations · localisation and IME.

## 8. Notes

`PROJECT_NOTES.md` still lists Windows 7 as a target. With a Windows 10 floor GTK4 becomes technically possible, but it removes `GtkMenu`, `GtkStatusIcon`, `gtk_main` nesting and more, and would be a much larger rewrite. GTK 3.24 remains the right target; the follow-up work in 4.3 is what would make GTK4 reachable later.
