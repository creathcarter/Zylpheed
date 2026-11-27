#!/bin/sh
# Generate the icon files the build uses from the large working images in
# src/icons_new/.
#
#   sh src/icons/resize-icons.sh [--build] [--preview]
#
#   --build     afterwards run "make" and "make install" in build-win64/
#   --preview   also write src/icons_new/_preview.png, a contact sheet of
#               the generated icons on a light and a dark background
#
# Run it in the MSYS2 UCRT64 shell (needs ImageMagick 7: pacman -S
# mingw-w64-ucrt-x86_64-imagemagick).
#
# src/icons_new/ is the working directory for the artwork. Edit or replace
# a PNG there (any size, 256x256 recommended), run this script, rebuild.
#
#   src/icons_new/toolbar/NAME.png  ->  src/icons/stock_NAME.png
#   src/icons_new/folders/NAME.png  ->  src/icons/NAME.png
#   src/icons_new/status/NAME.png   ->  src/icons/NAME.png
#   src/icons_new/app/NAME.png      ->  src/icons/NAME.png
#
# toolbar/, folders/ and status/ hold single-colour line art. Only its
# shape (the alpha channel) matters: the program paints these icons in the
# theme's text colour when it loads them (see stock_pixmap.c), so they are
# visible on light and dark themes. They are written here in white.
#
# app/ holds full-colour images (program icon, About logo, tray "new mail"
# icon); they are used as they are.
#
# A source that is missing is skipped and the existing file in src/icons/
# is kept.

set -e

die() { echo "resize-icons.sh: $*" >&2; exit 1; }

BUILD=
PREVIEW=
for a in "$@"; do
	case "$a" in
	--build)   BUILD=1 ;;
	--preview) PREVIEW=1 ;;
	*)         die "unknown option $a" ;;
	esac
done

command -v magick >/dev/null || die "magick (ImageMagick 7) not found in PATH"

DIR=$(cd "$(dirname "$0")" && pwd)
TOP=$(cd "$DIR/../.." && pwd)
UTOP=$TOP
# magick is a native Windows program: give it Windows-style paths.
if command -v cygpath >/dev/null; then
	DIR=$(cygpath -m "$DIR")
	TOP=$(cygpath -m "$TOP")
fi
SRC=$TOP/src/icons_new
[ -d "$SRC" ] || die "$SRC not found"

# source (below src/icons_new, without .png), then output:WIDTHxHEIGHT ...
# Sizes are the ones the program expects; several outputs can come from
# one source.
TABLE='
toolbar/addressbook      stock_addressbook:24x24
toolbar/attach           stock_attach:24x24
toolbar/book             stock_book:16x16
toolbar/delete           stock_delete:24x24 stock_delete_16:16x16
toolbar/hand-signed      stock_hand-signed:24x24
toolbar/inbox            stock_inbox:16x16
toolbar/insert-file      stock_insert-file:24x24
toolbar/mail             stock_mail:24x24
toolbar/mail-compose     stock_mail-compose:24x24 stock_mail-compose_16:16x16
toolbar/mail-forward     stock_mail-forward:24x24
toolbar/mail-receive     stock_mail-receive:24x24
toolbar/mail-reply       stock_mail-reply:24x24
toolbar/mail-reply-to-all stock_mail-reply-to-all:24x24
toolbar/mail-send        stock_mail-send:24x24
toolbar/mail_receive_all stock_mail_receive_all:24x24
toolbar/mail_send_queue  stock_mail_send_queue:24x24
toolbar/notspam          stock_notspam:24x24
toolbar/outbox           stock_outbox:16x16
toolbar/person           stock_person:16x16
toolbar/spam             stock_spam:24x24 stock_spam_16:16x16
toolbar/remote-mailbox   stock_remote-mailbox:24x24
toolbar/next             stock_next:24x24
toolbar/prev             stock_prev:24x24
toolbar/search           stock_search:24x24
toolbar/print            stock_print:24x24
toolbar/stop             stock_stop:24x24
toolbar/execute          stock_execute:24x24
toolbar/common-prefs     stock_common-prefs:24x24
toolbar/account-prefs    stock_account-prefs:24x24
toolbar/save             stock_save:24x24
toolbar/editor           stock_editor:24x24
folders/folder-close     folder-close:16x16
folders/folder-noselect  folder-noselect:16x16
folders/folder-open      folder-open:16x16
folders/folder-search    folder-search:16x16
folders/group            group:16x16
folders/html             html:16x16
status/address           address:16x16
status/category          category:16x16
status/clip              clip:16x16
status/complete          complete:16x16
status/continue          continue:16x16
status/deleted           deleted:16x16
status/error             error:16x16
status/forwarded         forwarded:16x16
status/interface         interface:16x16
status/jpilot            jpilot:16x16
status/ldap              ldap:16x16
status/linewrap          linewrap:24x24
status/mail              mail:16x16
status/mark              mark:16x16
status/new               new:16x16
status/offline           offline:16x16
status/online            online:16x16
status/replied           replied:16x16
status/unread            unread:16x16
status/vcard             vcard:16x16
app/stock_sylpheed       stock_sylpheed:48x48 stock_sylpheed_32:32x32 stock_sylpheed_16:16x16
app/stock_sylpheed_newmail stock_sylpheed_newmail:48x48 stock_sylpheed_newmail_16:16x16
app/sylpheed-logo        sylpheed-logo:x46
'

# Line art: strokes thinner than about one pixel after scaling turn into a
# faint grey, so the shape is thickened first when the target is small.
# The radius is in pixels of a 256-pixel source.
dilate_for() {
	case "$1" in
	16x16)	echo 3 ;;
	*)	echo 0 ;;
	esac
}

# line_art SRC OUT WxH
line_art() {
	r=$(dilate_for "$3")
	if [ "$r" != 0 ]; then
		# scale the radius if the source is not 256 pixels wide
		w=$(magick identify -format '%w' "$1[0]")
		r=$(( (r * w + 128) / 256 ))
	fi
	if [ "$r" != 0 ]; then
		magick "$1" -alpha extract -morphology Dilate "Disk:$r" \
			-resize "$3" -background black -gravity center \
			-extent "$3" "$TMP"
	else
		magick "$1" -alpha extract \
			-resize "$3" -background black -gravity center \
			-extent "$3" "$TMP"
	fi
	magick -size "$3" xc:white "$TMP" -alpha off \
		-compose CopyOpacity -composite -strip "PNG32:$2"
}

# colour SRC OUT WxH   (or xH: fixed height, proportional width)
colour() {
	case "$3" in
	x*)	magick "$1" -resize "$3" -strip "PNG32:$2" ;;
	*)	magick "$1" -resize "$3" -background none -gravity center \
			-extent "$3" -strip "PNG32:$2" ;;
	esac
}

TMP=$SRC/_tmp.png
trap 'rm -f "$TMP" "$SRC/_square.png"' EXIT

LINE_OUT=
n=0
missing=
while read -r src outs; do
	[ -n "$src" ] || continue
	f=$SRC/$src.png
	if [ ! -f "$f" ]; then
		missing="$missing $src"
		continue
	fi
	for spec in $outs; do
		out=${spec%%:*}
		size=${spec##*:}
		case "$src" in
		app/*)	colour "$f" "$DIR/$out.png" "$size" ;;
		*)	line_art "$f" "$DIR/$out.png" "$size"
			LINE_OUT="$LINE_OUT $out" ;;
		esac
		n=$((n + 1))
	done
	if [ "$src" = app/stock_sylpheed ]; then
		# program icon: Windows .ico and the top-level PNGs
		sq=$SRC/_square.png
		magick "$f" -background none -gravity center \
			-resize 256x256 -extent 256x256 "PNG32:$sq"
		magick "$sq" -define icon:auto-resize=256,128,64,48,32,24,16 \
			"$DIR/zylpheed.ico"
		colour "$sq" "$TOP/zylpheed.png" 48x48
		colour "$sq" "$TOP/zylpheed-64x64.png" 64x64
		colour "$sq" "$TOP/zylpheed-128x128.png" 128x128
		rm -f "$sq"
	fi
done <<EOF
$TABLE
EOF

echo "generated $n icon files in src/icons/"
[ -z "$missing" ] || echo "no source (existing icon kept):$missing"

# sources that the table does not know
for f in "$SRC"/*/*.png; do
	rel=${f#"$SRC"/}
	rel=${rel%.png}
	echo "$TABLE" | grep -q "^$rel " || echo "not used by the program: $rel.png"
done

if [ -n "$PREVIEW" ]; then
	# the program tints line art with the theme text colour: imitate it
	set --
	for o in $LINE_OUT; do set -- "$@" "$DIR/$o.png"; done
	# 12 icons per row, each in a 32x32 cell
	magick "$@" -background none -gravity center -extent 32x32 \
		+append +repage -crop 384x32 +repage -append "$SRC/_grid.png"
	magick \( "$SRC/_grid.png" -fill '#2e3436' -colorize 100 \
		   -background '#f6f5f4' -flatten \) \
		\( "$SRC/_grid.png" -background '#353535' -flatten \) \
		-append -filter point -resize 300% "$SRC/_preview.png"
	rm -f "$SRC/_grid.png"
	echo "preview: src/icons_new/_preview.png"
fi

if [ -n "$BUILD" ]; then
	[ -f "$UTOP/build-win64/Makefile" ] || die "build-win64 is not configured"
	make -C "$UTOP/build-win64" -j8
	make -C "$UTOP/build-win64" install >/dev/null
	echo "built and installed into build-win64/dist"
fi
