custom_target(
  'viewer-viewer-man',
  output : 'virt-viewer.1',
  input : 'virt-viewer.pod',
  command : [
    pod2man,
    '--section=1',
    '--center=Virtualization Support',
    '--name=Virt-Viewer',
    '--release=Virt-Viewer @0@'.format(meson.project_version()),
    '@INPUT@', '@OUTPUT@'
  ],
  install : true,
  install_dir : join_paths(mandir, 'man1')
)

custom_target(
  'remote-viewer-man',
  output : 'remote-viewer.1',
  input : 'remote-viewer.pod',
  command : [
    pod2man,
    '--section=1',
    '--center=Virtualization Support',
    '--name=Remote-Viewer',
    '--release=Virt-Viewer @0@'.format(meson.project_version()),
    '@INPUT@', '@OUTPUT@'
  ],
  install : true,
  install_dir : join_paths(mandir, 'man1')
)
