# Project Notes: GTK Application Fork

## Goal
Fork an existing GTK2-based desktop application and modernize it. Possible work includes migrating the UI, adding dark mode, and keeping multi-platform support.

## Target platforms (from the existing app)
- Windows 7 and later
- Linux
- macOS (OS X)
- BSD and other Unix systems

## Key findings so far

### GTK version constraints
- **GTK 2:** Works on Linux, BSD, Unix, and macOS (macOS only via XQuartz or an experimental Quartz backend, so it doesn't feel native). Last maintained as a maintenance-only branch.
- **GTK 3.24:** Last GTK 3 series. Works on Windows 7, Linux, BSD, and macOS (native Quartz backend, distributed via Homebrew/MacPorts). This is the likely upgrade target if Windows 7 support must be kept.
- **GTK 4:** Requires Windows 10 or later, so it cannot be used if Windows 7 is supported.
- The real limit on macOS, BSD, and Unix is often the minimum OS version, not GTK itself. Verify a GTK 3.24 build on the oldest supported macOS and BSD releases.

### Migration (if moving away from GTK entirely)
- Effort depends on how tightly UI code is coupled to logic. A small app may take days to weeks. A large app with custom widgets may take months.
- Main work: signal handlers, layout (GTK packing does not map 1:1), Cairo drawing, GLib main loop and threading, theming, and resource/i18n handling.
- Framework options for Windows 7/8/10/11 support:
  - WPF on .NET Framework 4.8 (C#/XAML): supports Windows 7 SP1 through 11.
  - Qt 5.15 LTS (C++): supports Windows 7 through 11. Qt 6 requires Windows 10 or later.
  - wxWidgets 3.2 (C++): supports Windows 7 through 11, native Win32 controls.
  - WinUI 3 and modern .NET (8+): do not support Windows 7 or 8.

### Dark mode
- GTK 3: set `gtk-application-prefer-dark-theme`, or use a `GtkCssProvider` with a custom palette (recommended for control).
- GTK 4: `AdwStyleManager` (libadwaita) or `gtk_settings` properties.
- GTK 2: limited, mainly via gtkrc files.
- Watch for custom-drawn widgets, hard-coded colors in `.ui` or code, and icons that don't suit dark backgrounds.

### Git and forking
- GitHub Fork button: keeps the "forked from" link and PR path to the source. Public forks cannot be made private through the normal flow.
- Manual clone and push: creates an independent repo with no fork link.
- Either way, add the original as an `upstream` remote to fetch and merge its changes:
  `git remote add upstream <original-url>`, then `git fetch upstream`.

## Open questions
- Why the original project stayed on GTK2 (check README, changelog, and issues for GTK 3 plans).
- Exact minimum macOS and BSD versions the project promises.
- Whether the fork should keep the GTK2 UI or migrate to GTK 3.24 or another framework.
- Whether the fork should be a GitHub fork or an independent repo.
- Language of the codebase (C, C++, Python, etc.) and GTK version in use.

## Decisions made
- None final yet. Leaning toward GTK 3.24 as the upgrade path if Windows 7 support is kept.

## Next steps
1. Confirm the project's GTK version, language, and minimum OS targets.
2. Decide between GitHub fork and independent repo, and add the `upstream` remote.
3. Build the existing app on Linux to confirm a working baseline.
4. Test a GTK 3.24 build on Windows 7, Windows 10/11, macOS (oldest supported), and one BSD or Linux system.
5. Plan dark-mode palette and migration order (screen by screen).
