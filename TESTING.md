# Manual regression checklist

Run this checklist on every build that is meant to replace the previous one (end of each
phase in `PORTING_PLAN.md`, and before any release). It expands section 7 of the plan into
steps a tester can follow and tick off.

Record for each run: date, tester, build (commit, GTK version, configure summary), Windows
version, display scaling. Mark each item **pass**, **fail** (with a note or issue number) or
**n/a** (feature not built). Compare against the previous build when behaviour is unclear.

## 0. Preparation

- [ ] Use a **throwaway profile**, never a real one. Start with
      `zylpheed.exe --configdir <scratch dir> --ipcport 50299 --instance-id ZylpheedTest`
      (see `BUILD-WIN64.md`, section 6). Keep a second scratch profile that is a copy of a
      realistic old profile (several accounts, a few thousand messages, filters, address
      book, templates) for the upgrade checks in section 1.
- [ ] Test mail accounts available: one POP3, one IMAP4 server (ideally one Dovecot-style and
      one Gmail or Microsoft 365 account for OAuth2), one SMTP with authentication, one NNTP
      server. Use dedicated test mailboxes; some steps delete mail.
- [ ] A GnuPG installation (Gpg4win) with a test key pair and a second test public key.
- [ ] Sample messages: plain text, HTML-only, multipart/alternative, nested multiparts,
      attachments (image, PDF, zip, .eml), non-ASCII headers (RFC 2047, ISO-2022-JP,
      UTF-8, Windows-1252), long lines, an X-Face header, a signed and an encrypted message,
      a message with a malformed MIME structure.
- [ ] Sample import files: an mbox file, an LDIF file, a CSV address file, a vCard file.
- [ ] Note the configure summary of the build; mark items for disabled features n/a.

## 1. Start-up, profile and upgrade

- [ ] Fresh profile: main window appears with the "Mailbox setting" dialog; accepting the
      default creates `Mailboxes\Mail` inside the profile directory.
- [ ] "New account setup" follows; completing it for POP3 and IMAP4 creates a working
      account.
- [ ] Copy of an old profile: starts without errors, all folders, message counts, marks,
      colour labels, filters, address book, templates, actions and key bindings are as
      before. Summary caches (`.sylpheed_cache`, `.sylpheed_mark`) are read, not rebuilt
      (check: opening a large folder is fast the first time).
- [ ] The same old profile still opens in the previous (32-bit) build afterwards
      (summary/mark caches written by this build are accepted).
- [ ] Window size, position, pane sizes and column widths are remembered across restarts.
- [ ] `--safe-mode` starts without plug-ins.
- [ ] `zylpheed.ini` (section `[Zylpheed]`) next to the executable with `configdir=` is honoured.
- [ ] Exit via File > Exit, via window close button and via `--exit` from a second process;
      configuration is saved in each case (change a setting, exit, restart, check).
- [ ] Log window (Tools > Log window) shows protocol logs; `zylpheed.log` is written.

## 2. Accounts and connections

For each of POP3, IMAP4, NNTP and SMTP:

- [ ] Plain connection.
- [ ] SSL/TLS on connect (e.g. 995/993/563/465).
- [ ] STARTTLS.
- [ ] Certificate checks: a valid certificate is accepted silently; a self-signed or
      wrong-host certificate brings up the certificate dialog; "accept permanently" stores
      it in `trust.crt` and is not asked again; "reject" aborts the connection.
- [ ] Authentication methods offered in account preferences work (PLAIN, LOGIN, CRAM-MD5,
      APOP for POP3).
- [ ] Wrong password: clear error, password prompt, no hang.
- [ ] Password not stored: prompted once per session.
- [ ] OAuth2 (Gmail / Microsoft): `zyl-auth-helper.exe` opens the browser, the token is
      obtained, refreshed after expiry, and IMAP/SMTP log in with XOAUTH2.
- [ ] SOCKS4/SOCKS5 proxy (with and without authentication) for receive and send.
- [ ] IPv6 server address (if available).
- [ ] Network drop during a transfer: error is reported, Zylpheed stays responsive,
      the next check works.
- [ ] Multiple accounts; default account; account selector in compose; per-account
      signature and SMTP server.

## 3. Receiving

- [ ] Get mail (current account) and Get from all accounts.
- [ ] POP3: "remove messages on server", "remove after N days", "receive size limit",
      UIDL state survives restart (no duplicate download).
- [ ] POP3: filtering on receive applies.
- [ ] IMAP4: folder list is fetched; new mail appears; flags (read, flagged, deleted)
      sync both ways with another client.
- [ ] IMAP4: subscribe/unsubscribe, create/rename/delete/move folder on server.
- [ ] IMAP4: large mailbox (10 000+ messages) opens; cancelling a long operation works.
- [ ] Auto-check interval; check on start-up.
- [ ] Remote POP3 (Tools > POP3 mailbox): lists messages, deletes, downloads selected.
- [ ] New-mail notification window and tray icon change (see section 17).
- [ ] Receiving while composing or while a modal dialog is open does not break the dialog.

## 4. Sending, queue and drafts

- [ ] Send a plain message; check headers at the receiving side (Date, Message-ID,
      From, encoding of non-ASCII subject and names).
- [ ] Send with attachments (binary, non-ASCII file name).
- [ ] Send with "send later": message goes to Queue; Send queued messages sends it.
- [ ] SMTP error (bad recipient, auth failure): message stays in queue / compose window
      stays open, error shown.
- [ ] Save to Drafts, reopen from Drafts, send; draft is removed.
- [ ] Sent copy stored in the configured Sent folder (local and IMAP).
- [ ] Message with Bcc: Bcc not in sent headers at recipient.

## 5. Compose

- [ ] New message, reply, reply to all, reply to sender, reply to list, forward inline,
      forward as attachment, redirect.
- [ ] Quoting: quote mark, quote format template, reply with selected text only.
- [ ] Templates: insert, apply on reply/forward, template variables.
- [ ] Signature: automatic insert, insert manually, signature separator, per-account.
- [ ] Attachments: add via dialog, drag and drop from Explorer, remove, change properties,
      MIME type detection.
- [ ] Insert file; Save to file.
- [ ] External editor: opens, changes come back, closing compose while editor runs asks to
      terminate it.
- [ ] Spell checking (when built with GtkSpell): misspelled words underlined, suggestions
      in context menu, language switch.
- [ ] Undo/redo; cut, copy, paste, paste as quotation; select all.
- [ ] Line wrapping: automatic wrap, wrap current paragraph, wrap all long lines; the
      ruler shows the column and follows the cursor.
- [ ] Character encoding selection; automatic encoding choice for non-ASCII text.
- [ ] Address completion in To/Cc (Tab), address book button.
- [ ] Header fields: add/remove Cc, Bcc, Reply-To, Followup-To, custom headers.
- [ ] Auto-save of drafts; recovery after kill.
- [ ] Input method editor (Japanese/Chinese IME): candidate window positioned at the cursor
      in the body and in header entries; committing text works.

## 6. Folders

- [ ] MH folders: create, rename, move (drag and drop), delete, folder properties.
- [ ] Mark all read; empty Trash; update summary; rebuild folder tree.
- [ ] IMAP folders: same operations against the server.
- [ ] News: subscribe to a group, read articles, mark read, unsubscribe; post an article.
- [ ] Search folders (virtual folders): create with rules; contents update; the
      search cache is reused on the next open.
- [ ] Folder colours / unread counts / bold for new mail in the folder tree.
- [ ] Folder tree context menus.

## 7. Summary (message list)

- [ ] Threading on/off; expand/collapse threads; thread sorting.
- [ ] Sort by each column, ascending/descending; sort persists per folder.
- [ ] Column set and order (Configure columns); column widths remembered.
- [ ] Colour labels: set, clear, rename labels in preferences, filter by label.
- [ ] Marks: read/unread, flag, mark for delete/move, lock; mark all read.
- [ ] Delete, move and copy (menu, keys, drag and drop to folder tree).
- [ ] Quick search bar: each filter type (all, unread, flagged, labelled, recent...), text
      search on subject/from/body, clearing the search.
- [ ] Query search (Edit > Search messages): runs, can be cancelled, results open the
      message.
- [ ] Very large folder: scrolling, selection and status bar counts stay correct.
- [ ] Dates before 1970 and far-future dates display and sort sensibly.

## 8. Message view

- [ ] Header pane: From, To, Cc, Subject, Date in configured format; X-Face image shows
      for a message with an X-Face header.
- [ ] Displayed headers configuration (Preferences > Display > Displayed headers).
- [ ] Plain-text body with each sample encoding; line breaks and tabs correct.
- [ ] HTML-only message is converted to text; links clickable.
- [ ] URLs and mail addresses are clickable; opening a URL starts the browser.
- [ ] Inline images displayed (and resized if configured).
- [ ] MIME tree: shown for multipart messages; select parts; open, save, save all,
      open with, print a part.
- [ ] View source; view all headers.
- [ ] Malformed MIME message does not crash and shows what it can.
- [ ] Message search within the shown message (Edit > Find in current message).
- [ ] Separate message window (double-click).

## 9. Filters and junk mail

- [ ] Create, edit, reorder, enable/disable filter rules; all condition types
      (header, body, size, age, account, command test, ...) and actions (move, copy,
      delete, mark, label, forward, execute, stop).
- [ ] Regular expression conditions (Oniguruma build).
- [ ] Filter on receive; filter selected messages manually; filter log.
- [ ] Junk mail: configure bsfilter (or another classifier), learn junk / not junk,
      automatic classification on receive, junk folder.

## 10. Actions

- [ ] Create actions of each kind (pipe message, insert output into compose, run on
      selection, asynchronous); they appear in the Tools > Actions menu and run.
- [ ] Action input dialog for actions that ask for input.

## 11. Address book

- [ ] Add, edit, delete persons and e-mail addresses; groups; folders.
- [ ] Add sender to address book from the message view and summary.
- [ ] Import LDIF, CSV, vCard; export CSV; results match the source files.
- [ ] Address completion uses the address book (compose and filter dialogs).
- [ ] Select-address dialog from compose.
- [ ] LDAP and JPilot address books (only when built with them).

## 12. PGP/MIME (GPGME)

- [ ] Sign a message; recipient verifies it.
- [ ] Encrypt to a recipient; recipient decrypts it.
- [ ] Sign and encrypt; inline (non-MIME) signing if offered.
- [ ] Incoming signed message: good, bad and unknown-key signatures are shown correctly.
- [ ] Incoming encrypted message: passphrase prompt (pinentry or Zylpheed dialog),
      decrypted display, wrong passphrase handling.
- [ ] Key selection dialog when several keys match.

## 13. Import and export

- [ ] Import an mbox file into a new folder; message count and content correct.
- [ ] Import from Outlook Express `.dbx` (Windows) if a sample exists.
- [ ] Export a folder to mbox; re-import gives the same messages.
- [ ] Export with "export as separate files" (eml).

## 14. Printing

- [ ] Print a message (GTK print dialog); page setup; print preview.
- [ ] Non-ASCII text and long lines print correctly.
- [ ] Print a MIME part via the external application.

## 15. Preferences

- [ ] Open every page of Common preferences and every tab of Account preferences; change
      a value on each page, OK, reopen: the value stuck.
- [ ] Key binding presets (Default, Mew/Wanderlust, Mutt, Old Sylpheed): applying a preset
      changes the menu accelerators; custom accelerators set by pressing keys on a menu
      item are saved to `menurc` and survive restart.
- [ ] Toolbar customisation (main window and compose window): add, remove, reorder
      buttons; text/icon modes.
- [ ] Fonts: message view, compose, list fonts; effect is immediate.
- [ ] Colours: quote levels, URI colour, colour labels.
- [ ] Folder properties dialog (per-folder settings).
- [ ] Icon theme selection.

## 16. Plug-ins

- [ ] Plug-in manager lists loaded plug-ins (Tools > Plug-in manager).
- [ ] `attachment_tool` plug-in: placed in `plugins\` next to `zylpheed.exe` (or in
      `<configdir>\plugins`), it loads (`--debug` prints "attachment_tool plug-in loading
      done") and its menu item works on a message with attachments.
- [ ] `test` plug-in: loads; its menu items, toolbar button and callbacks (message view,
      compose, folder) log as expected.
- [ ] A plug-in built for an older plug-in interface version is refused with a message,
      not a crash.

## 17. Tray icon and notifications

- [ ] Tray icon appears; tooltip shows unread counts; icon changes on new mail.
- [ ] Left click shows/hides the main window; right-click menu works.
- [ ] Minimise to tray / close to tray options.
- [ ] New-mail notification popup appears, shows sender/subject, closes by itself and on
      click.
- [ ] Notification sound (if configured).

## 18. Update check

- [ ] Help > Update check: reports the current state (needs `curl.exe` on `PATH`).
- [ ] Automatic update check on start-up when enabled; can be disabled.
- [ ] Plug-in update check.

## 19. Command-line remote operations

With an instance running (same `--configdir`, `--ipcport` and `--instance-id`):

- [ ] `--receive`, `--receive-all`
- [ ] `--compose`, `--compose mailto:addr?subject=x`, `--attach file`
- [ ] `--send`
- [ ] `--status`, `--status-full [folder]` print counts
- [ ] `--open folderid/msgnum` and `--open file.eml`
- [ ] `--exit`
- [ ] Running a second instance without arguments brings the first one to the front.
- [ ] `mailto:` handler: after registering with `zylpheed-mailto-protocol_user.reg`,
      clicking a mailto link in a browser opens a compose window.

## 20. Windows integration

- [ ] Drag and drop: files from Explorer onto compose (attach) and onto the message view;
      messages from the summary to Explorer (saved as .eml) if supported.
- [ ] Native file chooser dialogs (open, save, select folder) work with non-ASCII paths.
- [ ] Profile and mailbox paths containing non-ASCII characters and spaces.
- [ ] Suspend/resume (sleep the machine during an IMAP connection): reconnects cleanly.
- [ ] Log off / shut down with Zylpheed running: configuration is saved.
- [ ] High-DPI: 100 %, 150 %, 200 % scaling; text and icons readable, dialogs fit.
- [ ] Multi-monitor: windows reopen on the right monitor.
- [ ] Dark mode (from phase 4): readable in light and dark Windows themes.

## 21. Localisation

- [ ] Run with at least English, German, Japanese and one right-to-left-neutral non-Latin
      locale (e.g. Russian): menus and dialogs translated, no truncated labels.
- [ ] Translated strings with formatted values (e.g. "%d new messages") show correct
      numbers.
- [ ] Japanese: ISO-2022-JP sending and receiving, IME input (see section 5).

## 22. Stability and resources

- [ ] Leave running for a day with auto-check: no growth in memory/handles beyond normal
      (Task Manager: memory, handles, GDI objects).
- [ ] Rapid folder switching and message selection during receive: no crash, no hang.
- [ ] No GTK/GLib critical warnings on the console (start with `--debug` from a console
      to see them).
