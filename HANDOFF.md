# Zylpheed: where the work stands

Written 2026-11-27, at the end of the GTK3 port session, for whoever picks the work up
next (a person or a new Claude session, possibly on another machine). It records the
state, the owner's decisions and what is still open. Details are in the documents it
points to; this file is the index.

| Document | Content |
|---|---|
| `PORTING_PLAN.md` | Analysis of the code, the phased plan, section 8 status and to-do |
| `BUILD-WIN64.md` | Build environment, build, packaging (section 10), icons (section 11) |
| `TESTING.md` | Manual test checklist |
| `README` | User-facing description of the fork |

## 1. State

- Zylpheed 0.1.0 is a fork of Sylpheed 3.8.0beta1 (rev 1187), ported from GTK 2 to
  GTK 3.24 and to 64-bit Windows 10/11. Phases 0 to 5 of `PORTING_PLAN.md` are done.
- It builds with MSYS2 UCRT64 (GCC 16, GTK 3.24.52, GLib 2.90, OpenSSL 4.1) and produces
  three packages: portable folder, installer, single-file portable `.exe`.
- **Human testing has only just started.** Before 2026-11-27 everything was checked by
  builds, scripted start-up checks and reading logs. No real mail server, no mouse and
  keyboard pass, no installer run. The single-file portable `.exe` and the installer
  have never been run.
- **No security review has been done.** No binaries are to be published before it.
- At the time of writing, everything since the rename to Zylpheed was uncommitted. The
  owner does all git operations.

## 2. Build in one step

In the MSYS2 UCRT64 shell, with `build-win64/` already configured (`BUILD-WIN64.md`):

```
bash nsis/build-all-win64.sh          # program + all three packages into ../snapshots
sh src/icons/resize-icons.sh --preview   # only after changing artwork in src/icons_new/
```

- A change to `configure.ac` or a `*.in` file needs `NOCONFIGURE=1 ./autogen.sh` and
  `../configure ...` (or `./config.status <file>`) in `build-win64` first; the build tree
  is in maintainer mode and does not regenerate by itself.
- Most source files have CRLF line endings. MSYS `sed -i` strips them; edit with a tool
  that preserves them.
- The scripts take 10 to 15 minutes. From Claude Code on Windows, run them as background
  commands through `MSYSTEM=UCRT64 /c/msys64/usr/bin/bash.exe -lc "cd <repo> && ..."`.

## 3. Decisions by the owner (do not reopen without asking)

- 64-bit Windows 10/11 only for now. Linux, BSD and macOS are wanted later; nothing has
  been built there yet (Linux via WSL2 is the suggested next platform).
- Autotools stays. The item-factory-compatible menu facade (`MenuFactory` in
  `src/menu.c`) stays. Refactoring comes after the port.
- Name: Zylpheed everywhere, but translated UI strings, `syl_plugin_*` names and the
  file names inside the profile (`sylpheedrc`, ...) are unchanged. Icon files keep
  "sylpheed" in their names.
- Zylpheed never reads or migrates a Sylpheed profile. Own profile folder, mutex name,
  IPC port (50225).
- Profile location: installed version `%APPDATA%\Zylpheed`; portable folder and portable
  `.exe` keep it in `profiles\` beside the executable (`zylpheed.ini`, `--configdir`).
- Dark theme is the default (`etc\gtk-3.0\settings.ini`). No in-app toggle.
- Update check disabled. GnuPG as in Sylpheed: needs a separately installed GnuPG
  (Gpg4win), nothing bundled.
- Credits: the fork line is "Forked from Sylpheed 3.8.0beta1 by Creath and Claude,
  2026". **No copyright claim names "Creath and Claude"** anywhere (About, README, exe
  properties, macOS plist). The original author's copyright lines stay as they were.
- Help > Manual/FAQ open the Sylpheed web site with a note; no manual is shipped.
- Third-party Sylpheed plug-ins need porting (interface version 0x0200). The "test"
  demo plug-in is built but not shipped.
- Icons: the owner draws them (256x256 white line art) in `src/icons_new/`; the program
  tints line art with the theme's text colour. Do not replace missing artwork with GTK
  symbolic icons; ask for artwork instead.
- Fix plain bugs when found; ask first when a fix changes behaviour or needs a
  technical decision.

## 4. Open items

Waiting for the owner:

- Results of manual testing (`TESTING.md`), with test mail accounts. Credentials must
  never be put in the repository or pasted into a chat.
- Installer test (install, upgrade, uninstall) on a clean Windows 10/11 VM.
- Artwork: tray "new mail" icon (`src/icons_new/app/stock_sylpheed_newmail.png`) is
  still Sylpheed's. The envelope-with-badge status icons are hard to tell apart at
  16 pixels. The 16-pixel program icon is muddy.
- Whether to delete the old snapshots (`Sylpheed-3.8.0beta1-x64_setup.exe`,
  `gtk2-final\`, `gtk3-portable\`).

Known gaps in the program:

- The Windows title bar stays light in the dark theme (GTK 3 does not theme it; a small
  DWM call per window would fix it). Some fixed colour tints do not suit a dark theme.
- The icon tint colour is read once at start; a theme change needs a restart.
- Portable and installed Zylpheed share the instance name and IPC port, so only one
  runs at a time.
- 14 translatable strings still say "Sylpheed" (kept so the translations still match).
- About 1,780 GTK 3 deprecation warnings (optional Phase 6).
- Cache files store 32-bit times. When that is changed, bump `CACHE_VERSION` in the
  same change, not before.
- The dist script reports no licence text for atk, ca-certificates, djvulibre, gsfonts,
  libidn2, oniguruma and compface. djvulibre and gsfonts appeared after ImageMagick was
  installed: check why they are bundled.
- `zylpheed.ico` is about 370 KB uncompressed. `macosx/bundle/zylpheed.icns` is not
  regenerated from the new artwork.
- No `SECURITY.md` yet; add one when the repository goes public.

## 5. The security review (next piece of work)

Agreed with the owner: done after the port, on a dedicated always-on machine, by a
fresh Claude session working from this source tree. Nothing has been started.

Why it matters: a mail client parses hostile input from the network all day, the code
base is C from 1999 onwards, upstream development has stopped, and this port changed
pointer sizes (32 to 64 bit) and the whole UI layer.

Suggested scope, most exposed first:

1. Protocol response parsing: POP3, IMAP4, SMTP, NNTP (`libsylph/pop.c`, `imap.c`,
   `smtp.c`, `nntp.c`, `session.c`, `socket.c`, `recv.c`).
2. Message parsing: headers, MIME structure, encoded words, base64/quoted-printable/
   uuencode, character set conversion (`libsylph/procheader.c`, `procmime.c`, `codeconv.c`,
   `base64.c`, `quoted-printable.c`, `uuencode.c`, `unmime.c`, `html.c`).
3. TLS: certificate and host name checks, the "accept this certificate" path, behaviour
   with OpenSSL 4 against old servers (`libsylph/ssl.c`, `ssl_hostname_validation.c`).
4. compface (X-Face decoding), built by hand from 1.5.2 and fed untrusted headers
   (`src/headerview.c`).
5. Attachments and external programs: file names from messages, opening attachments,
   URI and external-command handling, actions (`src/mimeview.c`, `textview.c`,
   `action.c`, `libsylph/utils.c` path and temp-file helpers).
6. OAuth2 helper (`zyl-auth-helper.exe`) and stored passwords (`accountrc`).
7. Local IPC: the remote-command socket on port 50225 and the instance mutex
   (`src/main.c`).
8. Plug-in loading: DLLs from `<configdir>\plugins` and `plugins\` beside the exe.
9. The packaging: DLL search path, what the installer writes and with which rights,
   the portable `.exe` unpacking into `%TEMP%` (`nsis/`).
10. 64-bit leftovers: the Phase 0 audit fixed the casts and format strings the compiler
    reports; lengths and offsets read from the network or from cache files deserve a
    second look.

Method: read the code in that order; build with AddressSanitizer and UBSan (easiest on
Linux, which also gives the first Linux build); fuzz the parsers in 1, 2 and 4 with small
harnesses around the `libsylph` functions; write findings with severity, a reproducer
and a proposed fix; fix with the owner's agreement.

## 6. Rules for working on the owner's machine

- The owner's real Sylpheed 3.7.0 is installed in `C:\_tools\sylpheed` with a live
  profile in `%APPDATA%\Sylpheed`. Never read, write or delete either. Never touch the
  registry. Never run the installer or uninstaller on that machine.
- Start test instances only with a throwaway `--configdir`, a non-default `--ipcport`
  and an own `--instance-id`.
- No screen captures. No interaction with other applications. End only processes the
  session started itself, identified by command line.
- The owner handles git. No commits, pushes or other git write commands.
- Every build also produces all three packages (`nsis/build-all-win64.sh`).
