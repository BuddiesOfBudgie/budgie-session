desktops = [
  'budgie-labwc'
]

budgie_gsd_session_components = [
    'org.buddiesofbudgie.SettingsDaemon.A11ySettings',
    'org.buddiesofbudgie.SettingsDaemon.Datetime',
    'org.buddiesofbudgie.SettingsDaemon.Housekeeping',
    'org.buddiesofbudgie.SettingsDaemon.Power',
    'org.buddiesofbudgie.SettingsDaemon.PrintNotifications',
    'org.buddiesofbudgie.SettingsDaemon.Rfkill',
    'org.buddiesofbudgie.SettingsDaemon.Sharing',
    'org.buddiesofbudgie.SettingsDaemon.Smartcard',
    'org.buddiesofbudgie.SettingsDaemon.Sound',
    'org.buddiesofbudgie.SettingsDaemon.Wacom',
]

# Always in Budgie
budgie_components = [
    'org.buddiesofbudgie.BudgieDaemon',
    'org.buddiesofbudgie.BudgiePanel',
    'org.buddiesofbudgie.BudgiePowerDialog',
    'org.buddiesofbudgie.BudgieScreenshotDialog'
]

if with_polkit == true
    budgie_components += 'org.buddiesofbudgie.BudgiePolkit'
endif

#dep_gsd = dependency('gnome-settings-daemon', version: gnome_minimum_version)
session_components = budgie_components + budgie_gsd_session_components

if enable_session_selector
  desktops += 'budgie-custom-session'
endif

desktop_conf = configuration_data()
desktop_conf.set('bindir', session_bindir)
desktop_conf.set('libexecdir', join_paths(session_libexecdir, 'budgie-desktop'))
desktop_conf.set('SESSION_COMPONENTS', ';'.join(session_components))
desktop_conf.set('PACKAGE_VERSION', session_version)

# Set folder location for gnome-settings-daemon
# Unfortunately this info is not provided by its pkgconfig file
gsd_libexecdir = get_option('gsd-libexecdir')
if gsd_libexecdir == ''
    gsd_libexecdir = join_paths(get_option('prefix'), get_option('libexecdir'))
endif
desktop_conf.set('gsd_libexecdir', gsd_libexecdir)


foreach name: desktops
  desktop = name + '.desktop'

  desktop_in = configure_file(
    input: desktop + '.in.in',
    output: desktop + '.in',
    configuration: desktop_conf
  )

  install_dir = session_datadir / 'wayland-sessions'
  
  desktop_target = i18n.merge_file(
    type: 'desktop',
    input: desktop_in,
    output: desktop,
    po_dir: po_dir,
    install: true,
    install_dir: install_dir
  )
endforeach

sessions = [
  'budgie-labwc'
]

foreach session: sessions
  session_file = session + '.session'
  desktop = session_file + '.desktop'

  desktop_in = configure_file(
    input: desktop + '.in.in',
    output: desktop + '.in',
    configuration: desktop_conf
  )

  i18n.merge_file(
    type: 'desktop',
    input: desktop_in,
    output: session_file,
    po_dir: po_dir,
    install: true,
    install_dir: join_paths(session_pkgdatadir, 'sessions')
  )
endforeach

schema_conf = configuration_data()

schema = 'org.buddiesofbudgie.SessionManager.gschema.xml'

configure_file(
  input: schema + '.in',
  output: schema,
  install: true,
  install_dir: join_paths(session_datadir, 'glib-2.0', 'schemas'),
  configuration: schema_conf
)

data = files('hardware-compatibility')

if enable_session_selector
  data += files('session-selector.ui')
endif

install_data(
  data,
  install_dir: session_pkgdatadir
)

# install various desktop files to ensure they autostart
autostart_components = budgie_gsd_session_components + [
    'org.buddiesofbudgie.SettingsDaemon.DiskUtilityNotify',
    'org.buddiesofbudgie.SettingsDaemon.UsbProtection',
    'org.buddiesofbudgie.SettingsDaemon.Wwan'
]

foreach component: autostart_components
    configure_file(
        input: join_paths('settingsdaemon', component + '.desktop.in'),
        output: '@BASENAME@',
        configuration: desktop_conf,
        install_dir: xdg_appdir,
    )
endforeach

# complete the translations for various desktop files and ensure they autostart

autostart_components = [
  'budgie-desktop-powerdialog',
  'budgie-desktop-screenshotdialog',
  'budgie-desktop-nm-applet'
]

foreach component: autostart_components
    desktop_in = configure_file(
        input: join_paths(component + '.desktop.in.in'),
        output: join_paths(component + '.desktop.in'),
        configuration: desktop_conf,
    )

    # Now merge the translations of the .desktop.in to a .desktop
    i18n.merge_file(
      type: 'desktop',
      input: desktop_in,
      output: join_paths(component + '.desktop'),
      po_dir: po_dir,
      install: true,
      install_dir: xdg_appdir,
    )
endforeach

# Write the session /usr/bin/startbudgiedesktop script
configure_file(
    input: 'startbudgiedesktop.in',
    output: 'startbudgiedesktop',
    configuration: desktop_conf,
    install_dir: join_paths(get_option('prefix'), get_option('bindir')),
)
