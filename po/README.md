to regenerate the .pot file

meson setup build
meson compile -C budgie-session-pot

repeat the compile statement to extract changed translations
Update po/POTFILES.* for updated/changed files that should be translated / or skipped from being translated.
